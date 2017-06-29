#include "TextFormat.h"

bool TextFormat::WriteTestName(const char *testName) {
	char line[PATH_LEN + 10];
	int sz = sprintf(line, "## %s\n", testName);

	log->WriteBytes((unsigned char *)line, sz);
	return true;
}

bool TextFormat::WriteBasicBlock(const char *module, unsigned int offset, unsigned int cost, unsigned int jumpType) {
	char line[PATH_LEN + 30];
	int sz = sprintf(line, "%-15s + %08X (%4d) (%4d)\n",
		module,
		offset,
		cost,
		jumpType
	);

	log->WriteBytes((unsigned char *)line, sz);
	return true;
}

bool TextFormat::WriteInputUsage(unsigned int offset) {
	char line[30];
	int sz = sprintf(line, "Input offsets used: s[%d]\n", offset);
	log->WriteBytes((unsigned char *)line, sz);
	return true;
}

/*void TextLog::FlushLog() {
	fflush(fLog);
}*/
