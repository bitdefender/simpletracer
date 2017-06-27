#ifndef _FILE_LOG_H_
#define _FILE_LOG_H_

#include <stdio.h>
#include "AbstractLog.h"

class FileLog : public AbstractLog {
private :
	FILE *fLog;
	char logName[PATH_LEN];
protected :
	virtual bool _OpenLog();
	virtual bool _CloseLog();

public:
	virtual bool SetLogFileName(const char *name);
	virtual bool WriteBytes(unsigned char *buffer, unsigned int size);
	virtual bool Flush();
};

#endif // !_FILE_LOG_H_