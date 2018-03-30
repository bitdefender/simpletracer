#include "TraceParser.h"
#undef FLAG_LEN
#include "BinFormat.h"

#include "CommonCrossPlatform/Common.h"

#include <string.h>
#include <stdlib.h>

#define MAX_BUFF (1 << 15)

TraceParser::TraceParser() {}

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
	struct BareBasicBlock bbb;

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

		switch(bleh->entryType) {
			case ENTRY_TYPE_TEST_NAME:
			case ENTRY_TYPE_BB_NEXT_MODULE:
			case ENTRY_TYPE_BB_OFFSET:
			case ENTRY_TYPE_INPUT_USAGE:
			case ENTRY_TYPE_BB_NEXT_OFFSET:
			case ENTRY_TYPE_TAINTED_INDEX:
			case ENTRY_TYPE_BB_MODULE:
				break;
			case ENTRY_TYPE_Z3_MODULE:
				// symbolic address coming
				strncpy(bbb.module, (char *)&ble->data, bleh->entryLength);
				bbb.module[bleh->entryLength + 1] = 0;
				break;
			case ENTRY_TYPE_Z3_SYMBOLIC:
				switch(ble->data.asZ3Symbolic.header.entryType) {
					case Z3_SYMBOLIC_TYPE_ADDRESS:
						break;
					case Z3_SYMBOLIC_TYPE_JCC:
						break;
					default:
						DEBUG_BREAK;
				}
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
	return 0;
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
