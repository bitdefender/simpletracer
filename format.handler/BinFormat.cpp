#include "BinFormat.h"

#include <string.h>

bool BinFormat::WriteBasicBlock(const char *module,
	unsigned int offset,
	unsigned int cost,
	unsigned int jumpType
) {
	if (strcmp(lastModule, module)) {
		unsigned char buff[PATH_LEN + sizeof(BinLogEntry)];
		BinLogEntry *blem = (BinLogEntry *)buff;
		char *name = (char *)&blem->data;
		blem->header.entryType = ENTRY_TYPE_BB_MODULE;
		blem->header.entryLength = (unsigned short)strlen(module) + 1;
		strcpy(name, module);
		strcpy(lastModule, module);

		log->WriteBytes(buff, sizeof(blem->header) + blem->header.entryLength);
		//fwrite(buff, sizeof(blem->header) + blem->header.entryLength, 1, fLog);
	}

	BinLogEntry bleo;
	bleo.header.entryType = ENTRY_TYPE_BB_OFFSET;
	bleo.header.entryLength = sizeof(bleo.data.asBBOffset);
	bleo.data.asBBOffset.offset = offset;
	bleo.data.asBBOffset.cost = cost;
	bleo.data.asBBOffset.jumpType = jumpType;

	log->WriteBytes((unsigned char *)&bleo, sizeof(bleo));
	//fwrite(&bleo, sizeof(bleo), 1, fLog);
	return true;
}

bool BinFormat::WriteTestName(
	const char *testName
) {
	unsigned char buff[PATH_LEN + sizeof(BinLogEntryHeader)];
	BinLogEntryHeader *bleh = (BinLogEntryHeader *)buff;
	char *name = (char *)&bleh[1];
	bleh->entryType = ENTRY_TYPE_TEST_NAME;
	bleh->entryLength = (unsigned short)strlen(testName) + 1;
	strcpy(name, testName);

	log->WriteBytes(buff, sizeof(*bleh) + bleh->entryLength);
	//fwrite(buff, sizeof(*bleh) + bleh->entryLength, 1, fLog);

	// also reset current module
	lastModule[0] = '\0';
	return true;
}

bool BinFormat::WriteInputUsage(unsigned int offset) {
	BinLogEntry bleo;
	bleo.header.entryType = ENTRY_TYPE_INPUT_USAGE;
	bleo.header.entryLength = sizeof(bleo.data.asInputUsage);
	bleo.data.asInputUsage.offset = offset;

	log->WriteBytes((unsigned char *)&bleo, sizeof(bleo));
	return true;
}

/*bool BinLog::_OpenLog() {
	fLog = fopen(logName, "wb");

	return fLog != nullptr;
}

bool BinLog::_CloseLog() {
	fclose(fLog);
	fLog = nullptr;
	return true;
}

bool BinLog::_WriteTestName(const char *testName) {
	unsigned char buff[PATH_LEN + sizeof(BinLogEntryHeader)];
	BinLogEntryHeader *bleh = (BinLogEntryHeader *)buff;
	char *name = (char *)&bleh[1];
	bleh->entryType = ENTRY_TYPE_TEST_NAME;
	bleh->entryLength = (unsigned short)strlen(testName) + 1;
	strcpy(name, testName);
	
	fwrite(buff, sizeof(*bleh) + bleh->entryLength, 1, fLog);

	// also reset current module
	lastModule[0] = '\0';
	return true;
}

bool BinLog::_WriteBasicBlock(const char *module, unsigned int offset, unsigned int cost, unsigned int jumpType) {
	if (strcmp(lastModule, module)) {
		unsigned char buff[PATH_LEN + sizeof(BinLogEntry)];
		BinLogEntry *blem = (BinLogEntry *)buff;
		char *name = (char *)&blem->data;
		blem->header.entryType = ENTRY_TYPE_BB_MODULE;
		blem->header.entryLength = (unsigned short)strlen(module) + 1;
		strcpy(name, module);
		strcpy(lastModule, module);

		fwrite(buff, sizeof(blem->header) + blem->header.entryLength, 1, fLog);
	}

	BinLogEntry bleo;
	bleo.header.entryType = ENTRY_TYPE_BB_OFFSET;
	bleo.header.entryLength = sizeof(bleo.data.asBBOffset);
	bleo.data.asBBOffset.offset = offset;
	bleo.data.asBBOffset.cost = cost;
	bleo.data.asBBOffset.jumpType = jumpType;

	fwrite(&bleo, sizeof(bleo), 1, fLog);
	fflush(fLog);
	return true;
}

void BinLog::FlushLog() {
	fflush(fLog);
}*/


