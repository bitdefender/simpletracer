#ifndef _FILE_LOG_H_
#define _FILE_LOG_H_

#include <stdio.h>
#include "AbstractLog.h"

class FileLog : public AbstractLog {
private :
	FILE *fLog;
	char logName[PATH_LEN];
	bool	isLogToStdout;		// True if logging to stdout. Maybe we need a better abstraction here...
protected :
	virtual bool _OpenLog();
	virtual bool _CloseLog();

public:
	FileLog() : fLog(nullptr), isLogToStdout(false) { logName[0] = 0;}
	virtual bool SetLogFileName(const char *name);
	virtual bool SetAsStdout();
	virtual bool WriteBytes(const unsigned char *buffer, const unsigned int size);
	virtual bool Flush();
};

#endif // !_FILE_LOG_H_