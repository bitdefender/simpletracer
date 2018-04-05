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
	: z3Handler()
{
	state = NONE;
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
				if (state == NONE) {
					break;
				} else if (state == Z3_OFFSET) {
					// if `next` not sent for previous tmpBasicBlock
					state = NONE;
					break;
				}
				if (state < Z3_AST)
					DEBUG_BREAK;
				state = Z3_OFFSET;

				switch(symbolicType) {
					case Z3_SYMBOLIC_TYPE_ADDRESS:
						tmpBasicBlock.assertionData.asAddress.esp = ble->data.asBBOffset.esp;
						break;
					case Z3_SYMBOLIC_TYPE_JCC:
						tmpBasicBlock.assertionData.asJcc.jumpType = ble->data.asBBOffset.jumpType;
						tmpBasicBlock.assertionData.asJcc.jumpInstruction = ble->data.asBBOffset.jumpInstruction;
						break;
					default:
						DEBUG_BREAK;
				}

				if (!tmpBasicBlock.current.offset) {
					tmpBasicBlock.current.offset = ble->data.asBBOffset.offset;
				}
				break;
			case ENTRY_TYPE_BB_NEXT_OFFSET:
				if (state == NONE)
					break;
				if (state != Z3_OFFSET)
					DEBUG_BREAK;
				if (next > 1)
					DEBUG_BREAK;

				tmpBasicBlock.assertionData.asJcc.next[next++].offset = ble->data.asBBOffset.offset;
				if (next > 1) {
					switch(symbolicType) {
						case Z3_SYMBOLIC_TYPE_ADDRESS:
							tmpAddrAssertion.basicBlock = tmpBasicBlock;
							strcpy(tmpAddrAssertion.basicBlock.current.module, lastModule);
							DebugPrint(tmpAddrAssertion);
							addrAssertions.push_back(tmpAddrAssertion);
							break;
						case Z3_SYMBOLIC_TYPE_JCC:
							tmpJccCond.basicBlock = tmpBasicBlock;
							strcpy(tmpJccCond.basicBlock.current.module, lastModule);

							for (int i = 0; i < 2; ++i) {
								if (tmpJccCond.basicBlock.assertionData.asJcc.next[i].module[0] == 0) {
									strcpy(tmpJccCond.basicBlock.assertionData.asJcc.next[i].module,
											lastNextModule);
								}
							}

							DebugPrint(tmpJccCond);
							jccConditions.push_back(tmpJccCond);
							break;
						default:
							DEBUG_BREAK;
					}
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

				if (symbolicType == Z3_SYMBOLIC_TYPE_JCC) {
					memset(tmpJccCond.basicBlock.assertionData.asJcc.next[next].module, 0, MAX_PATH + 1);
					strncpy(tmpJccCond.basicBlock.assertionData.asJcc.next[next].module,
							(char *)&ble->data, bleh->entryLength);
				}
				break;
			case ENTRY_TYPE_Z3_MODULE:
				if (state != NONE)
					DEBUG_BREAK;
				state = Z3_MODULE;

				ExchageModule(lastModule, (char *)&ble->data, bleh->entryLength);
				break;
			case ENTRY_TYPE_Z3_SYMBOLIC:
				if (!(state == NONE || state == Z3_MODULE))
					DEBUG_BREAK;
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
						break;
					case Z3_SYMBOLIC_TYPE_JCC:
						tmpJccCond.symbolicCondition = ast;
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
	return false;
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
	PrintJump(jccCondition.basicBlock.assertionData.asJcc.jumpType,
			jccCondition.basicBlock.assertionData.asJcc.jumpInstruction);
	PRINT("\n");
	for (int i = 0; i < 2; ++i) {
		PRINT("JccCondition next[%d]: %s + 0x%08X\n", i,
				jccCondition.basicBlock.assertionData.asJcc.next[i].module,
				jccCondition.basicBlock.assertionData.asJcc.next[i].offset);
	}
	DebugPrint(jccCondition.symbolicCondition);
}

void TraceParser::DebugPrint(const struct AddressAssertion &addrAssertion) {
	PRINT("AddressAssertion %d: + 0x%08X\n", addrAssertions.size(),
			tmpAddrAssertion.offset);
	PRINT("AddressAssertion %p <= %p + %d * %p + %d\n",
			(void *)tmpAddrAssertion.composedAddress,
			(void *)tmpAddrAssertion.symbolicBase,
			tmpAddrAssertion.scale,
			(void *)tmpAddrAssertion.symbolicIndex,
			tmpAddrAssertion.displacement);
	PRINT("AddressAssertion i[%d]/o[%d]\n", tmpAddrAssertion.input,
			tmpAddrAssertion.output);

	PRINT("AddressAssertion tmpBasicBlock: %s + 0x%08X esp: 0x%08X\n",
			tmpAddrAssertion.basicBlock.current.module,
			tmpAddrAssertion.basicBlock.current.offset,
			tmpAddrAssertion.basicBlock.assertionData.asAddress.esp);
	DebugPrint(tmpAddrAssertion.symbolicAddress);
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
