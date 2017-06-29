#ifndef _BINFORMAT_H_
#define _BINFORMAT_H_

#include <stdio.h>

#include "AbstractLog.h"

#define ENTRY_TYPE_TEST_NAME 0x0010
#define ENTRY_TYPE_BB_MODULE 0x00B0
#define ENTRY_TYPE_BB_OFFSET 0x00BB
#define ENTRY_TYPE_INPUT_USAGE 0x00AA

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

		struct AsInputUsage {
			unsigned int offset;
		} asInputUsage;

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
	char lastModule[4096];
protected :
	bool _OpenLog();
	bool _CloseLog();
	bool _WriteTestName(const char *testName);
	bool _WriteBasicBlock(const char *module, unsigned int offset, unsigned int cost, unsigned int jumpType);
public :
	virtual void FlushLog();
};*/

class BinFormat : public AbstractFormat {
private :
	char lastModule[4096];
public :
	BinFormat(AbstractLog *l) : AbstractFormat(l) {}

	virtual bool WriteTestName(
		const char *testName
	);

	virtual bool WriteBasicBlock(
		const char *module,
		unsigned int offset,
		unsigned int cost,
		unsigned int jumpType
	);


	virtual bool WriteInputUsage(unsigned int offset);
};

#endif

