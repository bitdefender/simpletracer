#include "TraceParser.h"
#include "Z3Handler.h"
#include "AddressSanitizer.h"
#include "AbstractSanitizer.h"
#undef FLAG_LEN
#include "BinFormat.h"

#include "revtracer/river.h"
#include "CommonCrossPlatform/Common.h"

#include <string.h>
#include <stdlib.h>

#include "Common.h"

#define MAX_BUFF (1 << 15)

TraceParser::TraceParser()
	: z3Handler(), callStack()
{
	state = NONE;
	lviAddr = lviJcc = -1;
	memset(lastModule, 0, MAX_PATH + 1);
	memset(lastNextModule, 0, MAX_PATH + 1);
	CleanTempStructs();
}

TraceParser::~TraceParser() {}

/*
 * Z3 logs come in the following order:
 * 0. z3 module name (only for z3 address)
 * 1. z3 address or jump
 * 2. z3 ast
 * 3. bare trace entry - metadata
 */
bool TraceParser::Parse(FILE *input) {
	int ret, next = 0, symbolicType;
	// binformat structs
	BinLogEntryHeader *bleh;
	BinLogEntry *ble;
	unsigned char *buff = (unsigned char *) malloc (MAX_BUFF * sizeof(unsigned char));
	if (buff == nullptr)
		DEBUG_BREAK;

	CleanTempStructs();

	while (!feof(input)) {
		ret = ReadFromStream(buff, sizeof(*bleh), input);
		if (ret < 0) {
			DEBUG_BREAK;
		} else if (ret == 0) {
			break;
		}

		PrintState();

		bleh = (BinLogEntryHeader *)buff;
		DebugPrint(bleh->entryType);

		if (bleh->entryLength == 0)
			continue;

		ret = ReadFromStream(buff + ret, bleh->entryLength, input);
		if (ret < 0)
			DEBUG_BREAK;

		ble = (BinLogEntry *)buff;

		switch(bleh->entryType) {
			case ENTRY_TYPE_TEST_NAME:
			case ENTRY_TYPE_INPUT_USAGE:
			case ENTRY_TYPE_TAINTED_INDEX:
				break;
			case ENTRY_TYPE_EXECUTION_REGS:
				HandleRegisters((struct rev::ExecutionRegs *)&ble->data.asExecutionRegisters);
				break;
			case ENTRY_TYPE_BB_MODULE:
				ExchageModule(lastModule,
						(char *)&ble->data,
						bleh->entryLength);
				if (state == NONE)
					break;
				if (state < Z3_AST)
					DEBUG_BREAK;
				state = Z3_OFFSET;
				break;
			case ENTRY_TYPE_BB_OFFSET:
				if (state == Z3_OFFSET) {
					// if `next` not sent for previous tmpBasicBlock
					state = NONE;
				} else {
					state = Z3_OFFSET;
				}

				tmpBasicBlock.assertionData.address.esp = ble->data.asBBOffset.esp;

				tmpBasicBlock.assertionData.jcc.jumpType = ble->data.asBBOffset.jumpType;
				tmpBasicBlock.assertionData.jcc.jumpInstruction = ble->data.asBBOffset.jumpInstruction;

				if (!tmpBasicBlock.current.offset) {
					tmpBasicBlock.current.offset = ble->data.asBBOffset.offset;
				}

				strcpy(tmpBasicBlock.current.module, lastModule);

				DebugPrint(tmpBasicBlock);
				HandleCallInstruction(tmpBasicBlock);
				break;
			case ENTRY_TYPE_BB_NEXT_OFFSET:
				if (state == NONE)
					break;
				if (state != Z3_OFFSET)
					DEBUG_BREAK;
				if (next > 1)
					DEBUG_BREAK;

				tmpBasicBlock.assertionData.jcc.next[next++].offset = ble->data.asBBOffset.offset;
				if (next > 1) {
					// update last elements in assertion arrays
					for (int i = lviAddr + 1; i < addrAssertions.size(); ++i) {
							addrAssertions[i].basicBlock = tmpBasicBlock;
							strcpy(addrAssertions[i].basicBlock.current.module, lastModule);
							DebugPrint(addrAssertions[i]);
					}
					lviAddr = addrAssertions.size() - 1;

					for (int i = lviJcc; i < jccConditions.size(); ++i) {
							jccConditions[i].basicBlock = tmpBasicBlock;
							strcpy(jccConditions[i].basicBlock.current.module, lastModule);

							for (int i = 0; i < 2; ++i) {
								if (jccConditions[i].basicBlock.assertionData.jcc.next[i].module[0] == 0) {
									strcpy(jccConditions[i].basicBlock.assertionData.jcc.next[i].module,
											lastNextModule);
								}
							}
							DebugPrint(jccConditions[i]);
					}
					lviJcc = jccConditions.size() - 1;

					next = 0;
					state = NONE;
					CleanTempStructs();
				}

				break;
			case ENTRY_TYPE_BB_NEXT_MODULE:
				ExchageModule(lastNextModule, (char *)&ble->data, bleh->entryLength);
				if (state == NONE) {
					break;
				}

				if (state != Z3_OFFSET)
					DEBUG_BREAK;

				memset(tmpJccCond.basicBlock.assertionData.jcc.next[next].module, 0, MAX_PATH + 1);
				strncpy(tmpJccCond.basicBlock.assertionData.jcc.next[next].module,
						(char *)&ble->data, bleh->entryLength);
				break;
			case ENTRY_TYPE_Z3_MODULE:
				if (state != NONE)
					DEBUG_BREAK;
				state = Z3_MODULE;

				ExchageModule(lastModule, (char *)&ble->data, bleh->entryLength);
				break;
			case ENTRY_TYPE_Z3_SYMBOLIC:
				if (state == Z3_AST) {
					CleanTempStructs();
				} else if (!(state == NONE || state == Z3_MODULE)) {
					DEBUG_BREAK;
				}
				state = Z3_SYMBOLIC_OBJECT;

				symbolicType = ble->data.asZ3Symbolic.header.entryType;

				switch(symbolicType) {
					case Z3_SYMBOLIC_TYPE_ADDRESS:
						tmpAddrAssertion.offset = tmpBasicBlock.current.offset = ble->data.asZ3Symbolic.source.z3SymbolicAddress.offset;
						tmpAddrAssertion.symbolicBase = ble->data.asZ3Symbolic.source.z3SymbolicAddress.symbolicBase;
						tmpAddrAssertion.scale= ble->data.asZ3Symbolic.source.z3SymbolicAddress.scale;
						tmpAddrAssertion.symbolicIndex= ble->data.asZ3Symbolic.source.z3SymbolicAddress.symbolicIndex;
						tmpAddrAssertion.displacement = ble->data.asZ3Symbolic.source.z3SymbolicAddress.displacement;
						tmpAddrAssertion.composedAddress = ble->data.asZ3Symbolic.source.z3SymbolicAddress.composedAddress;
						tmpAddrAssertion.input = ble->data.asZ3Symbolic.source.z3SymbolicAddress.input;
						tmpAddrAssertion.output = ble->data.asZ3Symbolic.source.z3SymbolicAddress.output;
						break;
					case Z3_SYMBOLIC_TYPE_JCC:
						tmpJccCond.testFlags = ble->data.asZ3Symbolic.source.z3SymbolicJumpCC.testFlags;
						tmpJccCond.condition = ble->data.asZ3Symbolic.source.z3SymbolicJumpCC.symbolicCond;

						for (int i = 0; i < FLAG_LEN; ++i) {
							tmpJccCond.symbolicFlags[i] = ble->data.asZ3Symbolic.source.z3SymbolicJumpCC.symbolicFlags[i];
						}
						break;
					default:
						DEBUG_BREAK;
				}
				break;
			case ENTRY_TYPE_Z3_AST:
				if (state != Z3_SYMBOLIC_OBJECT)
					DEBUG_BREAK;
				state = Z3_AST;

				Z3_ast ast;
				ret = SmtToAst(ast, (char *)&ble->data, bleh->entryLength);

				switch(symbolicType) {
					case Z3_SYMBOLIC_TYPE_ADDRESS:
						tmpAddrAssertion.symbolicAddress = ast;
						addrAssertions.push_back(tmpAddrAssertion);
						break;
					case Z3_SYMBOLIC_TYPE_JCC:
						tmpJccCond.symbolicCondition = ast;
						jccConditions.push_back(tmpJccCond);
						break;
					default:
						DEBUG_BREAK;
				}
				break;
			default:
				DEBUG_BREAK;

		}
	}
	free(buff);
	return true;
}

int TraceParser::ReadFromStream(unsigned char* buff, size_t size, FILE *input) {
	if (size >= MAX_BUFF)
		DEBUG_BREAK;

	memset(buff, 0, MAX_BUFF);
	return fread(buff, 1, size, input);
}

int TraceParser::SmtToAst(Z3_ast &ast, char *smt, size_t size) {
	ast = z3Handler.toAst(smt, size);
	if (ast  == nullptr)
		return false;

	return true;
}

bool TraceParser::GetAddressAssertion(size_t index, struct AddressAssertion *&addrAssertion) {
	if (index < 0 || index >= addrAssertions.size())
		return false;

	addrAssertion = &addrAssertions[index];
	return true;
}

void TraceParser::GetHandler(Z3Handler *&handler) {
	handler = &z3Handler;
}

void TraceParser::DebugPrint(unsigned type) {
	PRINT("Received data: %04X<", (unsigned short)type);
	switch(type) {
	case ENTRY_TYPE_Z3_MODULE:
		PRINT("Z3 module name");
		break;
	case ENTRY_TYPE_Z3_SYMBOLIC:
		PRINT("Z3 symbolic data");
		break;
	case ENTRY_TYPE_Z3_AST:
		PRINT("Z3 ast");
		break;
	default:
		PRINT("unk");
		break;
	}

	PRINT(">\n");

}

static const char flagNames[FLAG_LEN][3] = {"CF", "PF", "AF", "ZF", "SF",
"OF", "DF"};

void TraceParser::PrintJump(unsigned short jumpType, unsigned short jumpInstruction) {
	switch(jumpType) {
		case RIVER_JUMP_TYPE_IMM:
			PRINT("type imm;");
			break;
		case RIVER_JUMP_TYPE_MEM:
			PRINT("type mem;");
			break;
		case RIVER_JUMP_TYPE_REG:
			PRINT("type reg;");
			break;
		default:
			break;
			PRINT("0x%02X;", jumpType);
	}
	PRINT(" ");

	switch(jumpInstruction) {
		case RIVER_JUMP_INSTR_RET:
			PRINT("instr ret;");
			break;
		case RIVER_JUMP_INSTR_JMP:
			PRINT("instr jump;");
			break;
		case RIVER_JUMP_INSTR_JXX:
			PRINT("instr jxx;");
			break;
		case RIVER_JUMP_INSTR_CALL:
			PRINT("instr call;");
			break;
		case RIVER_JUMP_INSTR_SYSCALL:
			PRINT("instr syscall;");
			break;
		default:
			PRINT("0x%02X;", jumpInstruction);
	}
}

void TraceParser::DebugPrint(Z3_ast ast) {
	z3Handler.PrintAst(ast);
}

void TraceParser::DebugPrint(const struct JccCondition &jccCondition) {
	PRINT("JccCondition %d: %s + 0x%08X\n", jccConditions.size(),
			jccCondition.basicBlock.current.module,
			jccCondition.basicBlock.current.offset);
	PRINT("JccCondition : %p <=", (void *)jccCondition.condition);
	for (int i = 0; i < FLAG_LEN; ++i) {
		if (jccCondition.testFlags & (1 << i)) {
			PRINT("%s[%p]", flagNames[i],
					(void *)jccCondition.symbolicFlags[i]);
		}
	}
	PRINT("\n");
	PRINT("JccCondition: ");
	PrintJump(jccCondition.basicBlock.assertionData.jcc.jumpType,
			jccCondition.basicBlock.assertionData.jcc.jumpInstruction);
	PRINT("\n");
	for (int i = 0; i < 2; ++i) {
		PRINT("JccCondition next[%d]: %s + 0x%08X\n", i,
				jccCondition.basicBlock.assertionData.jcc.next[i].module,
				jccCondition.basicBlock.assertionData.jcc.next[i].offset);
	}
	DebugPrint(jccCondition.symbolicCondition);
}

void TraceParser::DebugPrint(const struct AddressAssertion &addrAssertion) {
	PRINT("AddressAssertion %d: + 0x%08X\n", addrAssertions.size(),
			addrAssertion.offset);
	PRINT("AddressAssertion %p <= %p + %d * %p + %d\n",
			(void *)addrAssertion.composedAddress,
			(void *)addrAssertion.symbolicBase,
			addrAssertion.scale,
			(void *)addrAssertion.symbolicIndex,
			addrAssertion.displacement);
	PRINT("AddressAssertion i[%d]/o[%d]\n", addrAssertion.input,
			addrAssertion.output);

	PRINT("AddressAssertion basicBlock: %s + 0x%08X esp: 0x%08X\n",
			addrAssertion.basicBlock.current.module,
			addrAssertion.basicBlock.current.offset,
			addrAssertion.basicBlock.assertionData.address.esp);
	DebugPrint(addrAssertion.symbolicAddress);
}

void TraceParser::DebugPrint(const struct BasicBlock &basicBlock) {
	PRINT("BB: %s + 0x%08X; esp: 0x%08X; jt %04X; ji %04X\n",
			basicBlock.current.module,
			basicBlock.current.offset,
			basicBlock.assertionData.address.esp,
			basicBlock.assertionData.jcc.jumpType,
			basicBlock.assertionData.jcc.jumpInstruction);
}

void TraceParser::PrintState() {
	PRINT("state: ");
	switch(state) {
		case NONE:
			PRINT("NONE");
			break;
		case Z3_MODULE:
			PRINT("Z3_MODULE");
			break;
		case Z3_SYMBOLIC_OBJECT:
			PRINT("Z3_SYMBOLIC_OBJECT");
			break;
		case Z3_AST:
			PRINT("Z3_AST");
			break;
		case Z3_OFFSET:
			PRINT("Z3_OFFSET");
			break;
		default:
			DEBUG_BREAK;
	}
	PRINT("\n");
}

void TraceParser::CleanTempStructs() {
	memset(&tmpBasicBlock, 0, sizeof(tmpBasicBlock));
	memset(&tmpAddrAssertion, 0, sizeof(tmpAddrAssertion));
	memset(&tmpJccCond, 0, sizeof(tmpJccCond));
}

void TraceParser::ExchageModule(char *dest, char *moduleName, size_t size) {
	if (size > MAX_PATH)
		DEBUG_BREAK;

	if (size <= 0)
		return;

	if (strcmp(dest, moduleName) != 0) {
		memset(dest, 0, MAX_PATH + 1);
		strncpy(dest, moduleName, size);
		dest[size] = 0;
	}
}

void TraceParser::HandleRegisters(struct rev::ExecutionRegs *regs) {
	if (callStack.Empty()) {
		callStack.Push(regs->esp, 0, nullptr);
	}
}

void TraceParser::HandleCallInstruction(struct BasicBlock &basicBlock) {
	struct CallData cd;
	unsigned espAfterCall, espAfterRet;

	// set return address place (ebp + 4) before modifying call context
	basicBlock.assertionData.address.ebpPlusFour = callStack.GetLastCallFrame();
	printf("WARN: got ebp + 4 %08X\n", basicBlock.assertionData.address.ebpPlusFour);

	switch(basicBlock.assertionData.jcc.jumpInstruction) {
		case RIVER_JUMP_INSTR_CALL:
			callStack.Push(basicBlock.assertionData.address.esp,
					basicBlock.current.offset,
					basicBlock.current.module);
			break;
		case RIVER_JUMP_INSTR_RET:
			// ignore returns that do not correspond to previous call
			espAfterRet = basicBlock.assertionData.address.esp;
			espAfterCall = basicBlock.assertionData.address.ebpPlusFour;
			if (espAfterCall + 4 == espAfterRet) {
				callStack.Pop(cd);
			}
			break;
		default:
			break;
	}
}
