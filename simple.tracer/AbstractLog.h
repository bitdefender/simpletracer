#ifndef _ABSTRACT_LOG_H_
#define _ABSTRACT_LOG_H_

#define PATH_LEN 4096

class AbstractLog {
private :
	bool isLogOpen;

protected :
	virtual bool _OpenLog() = 0;
	virtual bool _CloseLog() = 0;


	bool OpenLog();
	bool CloseLog();
	bool IsLogOpen();
	
public :
	virtual bool WriteBytes(const unsigned char *buffer, const unsigned int size) = 0;
	virtual bool Flush() = 0;
};

class AbstractFormat {
protected :
	AbstractLog *log;

public :
	AbstractFormat(AbstractLog *l) {
		log = l;
	}
	
	virtual ~AbstractFormat(){}

	virtual bool WriteTestName(
		const char *testName
	) = 0;

	virtual bool WriteBasicBlock(
		const char *module,
		unsigned int offset,
		unsigned int cost,
		unsigned int jumpType
	) = 0;
	
	// Maybe these need a better name ?
	virtual void OnExecutionEnd() {} 
	virtual void OnExecutionBegin(const char* testName) { WriteTestName(testName); }

	/*virtual bool WriteInputUsage(
	unsigned int offset
	) = 0;

	virtual bool WriteTestResult(
	) = 0;*/
};

/*class AbstractLog {
private :
	bool isLogOpen;
protected :
	char logName[PATH_LEN];
	virtual bool _OpenLog() = 0;
	virtual bool _CloseLog() = 0;

	bool OpenLog();
	bool CloseLog();
	
	virtual bool _WriteTestName(
		const char *testName
	) = 0;

	virtual bool _WriteBasicBlock(
		const char *module,
		unsigned int offset,
		unsigned int cost,
		unsigned int jumpType
	) = 0;
public :
	virtual bool SetLogFile(
		const char *log
	);

	virtual void FlushLog() = 0;

	bool WriteTestName(
		const char *testName
	);

	bool WriteBasicBlock(
		const char *module,
		unsigned int offset,
		unsigned int cost,
		unsigned int jumpType
	);

	virtual bool WriteInputUsage(
		unsigned int offset
	) = 0;

	virtual bool WriteTestResult(
	) = 0;
};*/

#endif
