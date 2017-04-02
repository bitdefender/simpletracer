#ifndef _FILE_LOG_H_
#define _FILE_LOG_H_

#include <stdio.h>
#include "AbstractLog.h"

class FileLog : public AbstractLog {
private :
	FILE *fLog;
	char logName[PATH_LEN];
	bool isExternalFileSet;		// True if an external file was set directly instead of giving a filename to open
protected :
	virtual bool _OpenLog();
	virtual bool _CloseLog();

public:
	FileLog() : fLog(nullptr), isExternalFileSet(false) { logName[0] = 0;}
	virtual bool SetLogFileName(const char *name);
	virtual bool SetExternalFile(FILE* externalFile);
	virtual bool WriteBytes(const unsigned char *buffer, const unsigned int size);
	virtual bool Flush();
};

#endif // !_FILE_LOG_H_