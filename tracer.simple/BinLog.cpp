#include "BinLog.h"

#include <string.h>

BinLogWriter::BinLogWriter(FILE *log) {
	fLog = log;
	lastModule[0] = '\0';
}

bool BinLogWriter::WriteEntry(const char *module, unsigned int offset, unsigned int cost) {
	BinLogEntry ble;

	ble.entryType = ENTRY_TYPE_BASIC_BLOCK;
	ble.modNameLength = 0;
	if (strcmp(lastModule, module)) {
		ble.modNameLength = (unsigned short)strlen(module) + 1;
		strcpy(lastModule, module);
	}
	ble.bbOffset = offset;
	ble.bbCost = cost;

	fwrite(&ble, sizeof(ble), 1, fLog);
	if (ble.modNameLength) {
		fwrite(module, 1, ble.modNameLength, fLog);
	}
	
	return true;
}


