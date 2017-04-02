#ifndef _BINFORMAT_H_
#define _BINFORMAT_H_

#include <stdio.h>

#include "AbstractLog.h"

#define ENTRY_TYPE_TEST_NAME 0x0010
#define ENTRY_TYPE_BB_MODULE 0x00B0
#define ENTRY_TYPE_BB_OFFSET 0x00BB

struct BinLogEntryHeader {
	unsigned short entryType;
	unsigned short entryLength;
};

struct BinLogEntry {
	BinLogEntryHeader header;

	union Data {
		struct AsBBOffset {
			// encodes the module name length (including trailing \0)
			// a value of 0 means the current basic block resides in the same module as the previous
			unsigned int offset;
			unsigned short cost;
			unsigned short jumpType;
		} asBBOffset;
	
		/*struct AsBBModule {
			unsigned short modNameLength;
		} asBBModule;

		struct AsTestName {
			unsigned short testNameLength;
		} asTestName;*/
	} data;
};

/*class BinFormat : public AbstractFormat {
private:
	FILE *fLog;
	char lastModule[PATH_LEN];
protected :
	bool _OpenLog();
	bool _CloseLog();
	bool _WriteTestName(const char *testName);
	bool _WriteBasicBlock(const char *module, unsigned int offset, unsigned int cost, unsigned int jumpType);
public :
	virtual void FlushLog();
};*/

class Logger;

class BinFormat : public AbstractFormat {
private :
	char lastModule[PATH_LEN];
	
	// Variables below are used for buffering mode. 
	// The reason i need buffering is that communication to the tracer.simple process are done by pipes and we can't seek in a pipe.
	// What I do is to write all entries in the buffer at runtime, then when executon ends write data to pipe(stdout) [number of bytes used + buffer]
	bool bufferingEntries;										// True if buffering entries
	unsigned char* bufferEntries;								// If this is created with shouldBufferEntries = true => we'll buffer all entries and send them at once
	static const int MAX_ENTRIES_BUFFER_SIZE = 1024*1024*2;   	// Preallocated buffer used when buffering entries. If exceeded an exception occurs. TODO: recreate buffer when exceeded max size ?
	int bufferHeaderPos;
	Logger& logger;
	
	// Writes data either to the internal buffer or to the log file depending on the type
	void WriteData(const unsigned char* data, const unsigned int size, const bool ignoreInBufferedMode = false);
	
	virtual bool WriteTestName(
		const char *testName
	);
	
public :
	BinFormat(AbstractLog *l, bool shouldBufferEntries, Logger& logger);
	virtual ~BinFormat();

	virtual bool WriteBasicBlock(
		const char *module,
		unsigned int offset,
		unsigned int cost,
		unsigned int jumpType
	);
	
	// Callbacks to know about execution status and update internal data structures	
	void OnExecutionEnd() override;
	void OnExecutionBegin(const char* testName) override; // testName optional when running in buffered / flow mode (you can set it as nullptr)
};

#endif

