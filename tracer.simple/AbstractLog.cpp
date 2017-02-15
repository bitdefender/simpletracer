#include "AbstractLog.h"

#include <string.h>

bool AbstractLog::OpenLog() {
	if (isLogOpen) {
		return true;
	}

	if (_OpenLog()) {
		isLogOpen = true;
		return true;
	}

	return false;
}

bool AbstractLog::CloseLog() {
	if (_CloseLog()) {
		isLogOpen = false;
		return true;
	}

	return false;
}

bool AbstractLog::IsLogOpen() {
	return isLogOpen;
}

/*bool AbstractFormat::SetLogFile(const char *log) {
	if (isLogOpen) {
		_CloseLog();
		isLogOpen = false;
	}

	strcpy(logName, log);

	return false;
}

bool AbstractFormat::WriteTestName(const char *testName) {
	if (!OpenLog()) {
		return false;
	}
	return _WriteTestName(testName);
}

bool AbstractFormat::WriteBasicBlock(const char *module, unsigned int offset, unsigned int cost, unsigned int jumpType) {
	if (!OpenLog()) {
		return false;
	}
	return _WriteBasicBlock(module, offset, cost, jumpType);
}*/
