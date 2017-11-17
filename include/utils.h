#ifndef __utils__
#define __utils__

#include "AbstractLog.h"
#include "Execution/Execution.h"

void TranslateAddressToBasicBlockPointer(struct BasicBlockPointer *bbp,
		unsigned int address, int mCount, ModuleInfo *mInfo);

#endif
