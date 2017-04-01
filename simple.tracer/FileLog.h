#ifndef _FILE_LOG_H_
#define _FILE_LOG_H_

#include <stdio.h>
#include "AbstractLog.h"

class FileLog : public AbstractLog {
private :

	FileLog() : fLog(nullptr), isLogToStdout(false), logName[0](0) {}
	FILE *fLog;
	char logName[PATH_LEN];
	bool	isLogToStdout;		// True if logging to stdout. Maybe we need a better abstraction here...
protected :
	virtual bool _OpenLog();
	virtual bool _CloseLog();

public:
	virtual bool SetLogFileName(const char *name);
	virtual void SetAsStdout();
	virtual bool WriteBytes(unsigned char *buffer, unsigned int size);
	virtual bool Flush();
};

#endif // !_FILE_LOG_H_