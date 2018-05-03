#ifndef __utils__
#define __utils__

#include "AbstractLog.h"
#include "Execution/Execution.h"
#include "revtracer/revtracer.h"

#define VALID_ESP(esp) ((esp) != INVALID_ADDRESS)

void TranslateAddressToBasicBlockPointer(struct BasicBlockPointer *bbp,
		unsigned int address, int mCount, ModuleInfo *mInfo);
unsigned int ReadFromFile(FILE* inputFile, unsigned char *buf, int sizeToRead = -1);
void ClearExecutionRegisters(struct rev::ExecutionRegs *regs);

#endif
