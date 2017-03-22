#include "Logger.h"

Logger::Logger() 
    : isLogEnabled(false) 
    , logFile(stdout){}

Logger::~Logger() 
{ 
	if (writeLogOnFile) 
		fclose(logFile);
}

void Logger::Log( const char * format, ... )
{
	if (!isLogEnabled)
		return;
		
	static const int buffSz = 2048;
	static char buffer[buffSz];
	va_list args;
	va_start (args, format);
	vsnprintf (buffer,buffSz,format, args);
	va_end (args);

	fprintf(logFile, "%s", buffer);
	fflush(logFile);
}

void Logger::EnableLog() 
{ 
    isLogEnabled = true; 
}

void Logger::SetLoggingToFile(const char* filename)
{
	logFile = fopen(filename);
	writeLogOnFile = true;
}

