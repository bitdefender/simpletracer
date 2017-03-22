#ifndef _LOGGER_H_
#define _LOGGER_H_

class Logger
{
public:
	Logger();
	~Logger();

	void Log( const char * format, ... );
	void EnableLog();
	void SetLoggingToFile(const char* filename);

private:	
	bool writeLogOnFile;
	FILE* logFile; 
	bool isLogEnabled;
};



#endif