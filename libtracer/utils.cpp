#include "utils.h"
#include "CommonCrossPlatform/Common.h"
#include <string.h>

void TranslateAddressToBasicBlockPointer(
		struct BasicBlockPointer* bbp,
		unsigned int address, int mCount, ModuleInfo *mInfo) {
	int foundModule = -1;
	unsigned int offset = address;
	const char unkmod[MAX_PATH] = "???";

	memset(bbp, 0, sizeof(struct BasicBlockPointer));

	for (int i = 0; i < mCount; ++i) {
		if ((mInfo[i].ModuleBase <= address) &&
				(address < mInfo[i].ModuleBase + mInfo[i].Size)) {
			offset -= mInfo[i].ModuleBase;
			foundModule = i;
			break;
		}
	}

	bbp->offset = offset;
	if (foundModule == -1) {
		strncpy(bbp->modName, unkmod, MAX_PATH);
	} else {
		strncpy(bbp->modName, mInfo[foundModule].Name, MAX_PATH);
	}
}

unsigned int ReadFromFile(FILE* inputFile, unsigned char *buf, int sizeToRead) {
	unsigned int bSize = sizeToRead == -1 ? MAX_PAYLOAD_BUF : sizeToRead;
	unsigned int localread, read = 0;

	while ((localread = fread(buf + read, sizeof(unsigned char), bSize - read,
					inputFile)) != 0) {
		read += localread;
	}
	return read;
}
