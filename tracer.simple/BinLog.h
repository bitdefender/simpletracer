#ifndef _BINLOG_H_
#define _BINLOG_H_

#include <stdio.h>

#define ENTRY_TYPE_BASIC_BLOCK 0x000000BB

struct BinLogEntry {
	// used for forward compatibility, must be zero for now
	unsigned int entryType;

	unsigned short bbCost;
	// encodes the module name length (including trailing \0)
	// a value of 0 means the current basic block resides in the same module as the previous
	unsigned short modNameLength;
	unsigned int bbOffset;
};

class BinLogWriter {
private:
	FILE *fLog;
	char lastModule[4096];
public:
	BinLogWriter(FILE *log);
	bool WriteEntry(const char *module, unsigned int offset, unsigned int cost);
};

#endif

