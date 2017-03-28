#ifndef _BINLOG_H_
#define _BINLOG_H_

#include <stdio.h>

#define ENTRY_TYPE_BASIC_BLOCK 0x000000BB

class Logger;

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
	Logger& logger;

	// Variables below are used for buffering mode. 
	// The reason i need buffering is that communication to the tracer.simple process are done by pipes and we can't seek in a pipe.
	// What I do is to write all entries in the buffer at runtime, then when executon ends write data to pipe(stdout) [number of bytes used + buffer]
	bool bufferingEntries;										// True if buffering entries
	char* bufferEntries;										// If this is created with shouldBufferEntries = true => we'll buffer all entries and send them at once
	static const int MAX_ENTRIES_BUFFER_SIZE = 1024*1024*2;   	// Preallocated buffer used when buffering entries. If exceeded an exception occurs. TODO: recreate buffer when exceeded max size ?
	int bufferHeaderPos;


public:
	BinLogWriter(FILE *log, bool shouldBufferEntries, Logger& logger);
	virtual ~BinLogWriter();
	bool WriteEntry(const char *module, unsigned int offset, unsigned int cost);

	void ExecutionEnd();
	void ExecutionBegin();
};

#endif

