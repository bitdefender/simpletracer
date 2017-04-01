#include "BinFormat.h"

#include <string.h>
#include <cstdlib>
#include "Logger.h"

BinFormat::BinFormat(AbstractLog *l, bool shouldBufferEntries, Logger& logger) 
: AbstractFormat(l) 
, logger(_logger)
{
	lastModule[0] 		= '\0';
	bufferingEntries 	= shouldBufferEntries;
	bufferEntries 		= shouldBufferEntries ? new char[MAX_ENTRIES_BUFFER_SIZE] : nullptr;
	bufferHeaderPos 	= 0;
}

BinFormat::~BinFormat()
{
	if (bufferEntries)
	{
		delete [] bufferEntries;
		bufferEntries = nullptr;
	}
}

void BinFormat::OnExecutionBegin(const char* testName)
{
	bufferHeaderPos = 0;
	lastModule[0] = '\0';
	
	WriteTestName(testName);
}

void BinFormat::OnExecutionEnd()
{
	if (!bufferEntries) { 		
		log->Flush();
	}
	else {
		// Write the total number of bytes of the output buffer, then the buffered data
		const size_t totalSizeToWrite = sizeof(bufferEntries[0]) * bufferHeaderPos;
		log->WriteBytes(&totalSizeToWrite, sizeof(totalSizeToWrite));
		log->WriteBytes(bufferEntries, sizeof(bufferEntries[0]) * bufferHeaderPos);
		log->Flush();
	}
}

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

		WriteData(buff, sizeof(blem->header) + blem->header.entryLength);
	}

	BinLogEntry bleo;
	bleo.header.entryType = ENTRY_TYPE_BB_OFFSET;
	bleo.header.entryLength = sizeof(bleo.data.asBBOffset);
	bleo.data.asBBOffset.offset = offset;
	bleo.data.asBBOffset.cost = cost;
	bleo.data.asBBOffset.jumpType = jumpType;

	WriteData((unsigned char *)&bleo, sizeof(bleo));
	return true;
}

void BinFormat::WriteData(const unsigned char* data, const unsigned int size, const bool ignoreInBufferedMode)
{
	if (!bufferingEntries)
	{
		log->WriteBytes(data, size);
	}
	else
	{
		if (!ignoreInBufferedMode)
		{
			if (bufferHeaderPos + size >= MAX_ENTRIES_BUFFER_SIZE)
			{
				logger.Log("Already reached the end of the buffer :( exiting\n");
				exit(1);
			}

			memcpy(&bufferEntries[bufferHeaderPos], data, size);
			bufferHeaderPos += size;
		}
	}
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


	WriteData(buff, sizeof(*bleh) + bleh->entryLength, true);
	//fwrite(buff, sizeof(*bleh) + bleh->entryLength, 1, fLog);

	// also reset current module
	lastModule[0] = '\0';
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


