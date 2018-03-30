#include "TraceParser.h"
#include "Z3Handler.h"
#undef FLAG_LEN
#include "BinFormat.h"

#include "CommonCrossPlatform/Common.h"

#include <string.h>
#include <stdlib.h>

#define MAX_BUFF (1 << 15)

TraceParser::TraceParser()
	: z3Handler()
{}

TraceParser::~TraceParser() {}

/*
 * Z3 logs come in the following order:
 * 1. z3 address or jump
 * 2. z3 ast
 * 3. bare trace entry - metadata
 */
bool TraceParser::Parse(FILE *input) {
	int ret;
	// binformat structs
	BinLogEntryHeader *bleh;
	BinLogEntry *ble;
	unsigned char *buff = (unsigned char *) malloc (MAX_BUFF * sizeof(unsigned char));
	if (buff == nullptr)
		DEBUG_BREAK;

	// traceparser structs
	struct BasicBlock bb;

	while (!feof(input)) {
		ret = ReadFromStream(buff, sizeof(*bleh), input);
		if (ret < 0) {
			DEBUG_BREAK;
		} else if (ret == 0) {
			break;
		}

		bleh = (BinLogEntryHeader *)buff;
		DebugPrint(bleh->entryType);

		ret = ReadFromStream(buff + ret, bleh->entryLength, input);
		if (ret <= 0)
			DEBUG_BREAK;

		ble = (BinLogEntry *)buff;

		int next = 0;
		int symbolicType = 0;
		switch(bleh->entryType) {
			case ENTRY_TYPE_TEST_NAME:
			case ENTRY_TYPE_INPUT_USAGE:
			case ENTRY_TYPE_TAINTED_INDEX:
			case ENTRY_TYPE_BB_MODULE:
				break;
			case ENTRY_TYPE_BB_OFFSET:
				if (symbolicType == 0) {
					continue;
				}
				switch(symbolicType) {
					case Z3_SYMBOLIC_TYPE_ADDRESS:
						bb.assertionData.asAddress.esp = ble->data.asBBOffset.esp;
						symbolicType = 0;
						break;
					case Z3_SYMBOLIC_TYPE_JCC:
						bb.assertionData.asJcc.jumpType = ble->data.asBBOffset.jumpType;
						bb.assertionData.asJcc.jumpInstruction = ble->data.asBBOffset.jumpInstruction;
						// must fill next as well
						break;
				}
				break;
			case ENTRY_TYPE_BB_NEXT_OFFSET:
				if (symbolicType == 0) {
					continue;
				}
				bb.assertionData.asJcc.next[next++].offset = ble->data.asBBNextOffset.offset;
				if (next > 1)
					symbolicType = 0;

				break;
			case ENTRY_TYPE_BB_NEXT_MODULE:
				if (symbolicType == 0) {
					continue;
				}
				//TODO
				break;
			case ENTRY_TYPE_Z3_MODULE:
				// symbolic address coming
				strncpy(bb.current.module,
						(char *)&ble->data, bleh->entryLength);
				bb.current.module[bleh->entryLength + 1] = 0;
				break;
			case ENTRY_TYPE_Z3_SYMBOLIC:
				bb.current.offset = ble->data.asZ3Symbolic.source.z3SymbolicAddress.offset;
				symbolicType = ble->data.asZ3Symbolic.header.entryType;
				break;
			case ENTRY_TYPE_Z3_AST:
				Z3_ast ast;
				ret = SmtToAst(&ast, (char *)&ble->data, bleh->entryLength);
				break;
			default:
				DEBUG_BREAK;

		}
	}
	free(buff);
	fclose(input);
}

int TraceParser::ReadFromStream(unsigned char* buff, size_t size, FILE *input) {
	if (size >= MAX_BUFF)
		DEBUG_BREAK;

	memset(buff, 0, MAX_BUFF);
	return fread(buff, 1, size, input);
}

int TraceParser::SmtToAst(Z3_ast *ast, char *smt, size_t size) {
	*ast = z3Handler.toAst(smt, size);
	if (ast  == nullptr)
		return false;

	return true;
}

void TraceParser::DebugPrint(unsigned type) {
	printf("Received data: %04X<", (unsigned short)type);
	switch(type) {
	case ENTRY_TYPE_Z3_MODULE:
		printf("Z3 module name");
		break;
	case ENTRY_TYPE_Z3_SYMBOLIC:
		printf("Z3 symbolic data");
		break;
	case ENTRY_TYPE_Z3_AST:
		printf("Z3 ast");
		break;
	default:
		printf("unk");
		break;
	}

	printf(">\n");

}
