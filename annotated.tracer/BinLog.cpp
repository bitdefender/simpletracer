#include "BinLog.h"

#include <string.h>
#include <cstdlib>

BinLogWriter::BinLogWriter(FILE *log, const bool shouldBufferEntries) {
	fLog = log;
	lastModule[0] = '\0';

	bufferingEntries 	= shouldBufferEntries;
	bufferEntries 		= shouldBufferEntries ? new char[MAX_ENTRIES_BUFFER_SIZE] : nullptr;
	bufferHeaderPos 	= 0;
}

BinLogWriter::~BinLogWriter()
{
	if (bufferEntries)
	{
		delete [] bufferEntries;
		bufferEntries = nullptr;
	}
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

	if (!bufferingEntries)
	{						
		fwrite(&ble, sizeof(ble), 1, fLog);
		if (ble.modNameLength) {
			fwrite(module, 1, ble.modNameLength, fLog);
		}
	}
	else
	{
		if (bufferHeaderPos + sizeof(ble) + ble.modNameLength >= MAX_ENTRIES_BUFFER_SIZE)
		{
			fprintf(stderr, "Already reached the end of the buffer :( exiting\n");
			exit(1);
		}

		memcpy(&bufferEntries[bufferHeaderPos], &ble, sizeof(ble));
		bufferHeaderPos += sizeof(ble);

		if (ble.modNameLength) {
			memcpy(&bufferEntries[bufferHeaderPos], module, ble.modNameLength*sizeof(char));
			bufferHeaderPos += ble.modNameLength*sizeof(char);
		}
	}
	
	return true;
}

void BinLogWriter::ExecutionEnd()
{
	// If using buffer mode, write it to stdout
	if (!bufferEntries) { 
		if (fLog) {
			fflush(fLog);
		}
	}
	else {
		if (fLog) {	
			// Write the total number of bytes of the output buffer, then the buffered data
			const size_t totalSizeToWrite = sizeof(bufferEntries[0]) * bufferHeaderPos;
			fwrite(&totalSizeToWrite, sizeof(totalSizeToWrite), 1, fLog);
			fwrite(bufferEntries, sizeof(bufferEntries[0]), bufferHeaderPos, fLog);

			fflush(fLog);
		}
	}
}

void BinLogWriter::ExecutionBegin()
{
	bufferHeaderPos = 0;
	lastModule[0] = '\0';
}