#ifndef __TRACKING_EXECUTOR__
#define __TRACKING_EXECUTOR__

#include "SymbolicEnvironment/Environment.h"
#include "TaintedIndex.h"
#include "Execution/Execution.h"

class TrackingExecutor : public sym::SymbolicExecutor {
public :
	TaintedIndex *ti;
	unsigned int varCount;
	AbstractFormat *aFormat;

	TrackingExecutor(sym::SymbolicEnvironment *e, unsigned int varCount,
			AbstractFormat *aFormat);

	virtual void *CreateVariable(const char *name, DWORD size);
	void SetModuleData(int mCount, ModuleInfo *mInfo);

	virtual void *MakeConst(DWORD value, DWORD bits);

	virtual void *ExtractBits(void *expr, DWORD lsb, DWORD size);
	virtual void *ConcatBits(void *expr1, void *expr2);

	#define OPERAND_BITMASK(idx) (0x00010000 << (idx))

	static const unsigned int flagValues[];

	struct Operands {
		bool useOp[4];
		DWORD operands[4];
		bool useFlag[7];
		DWORD flags[7];
	};

	virtual void Execute(RiverInstruction *instruction);
private:
	int mCount;
	ModuleInfo *mInfo;

	void SetOperands(RiverInstruction *instruction, DWORD index);
	void UnsetOperands(RiverInstruction *instruction);
};



#endif
