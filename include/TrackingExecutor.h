#ifndef __TRACKING_EXECUTOR__
#define __TRACKING_EXECUTOR__

#include "BitMap.h"
#include "SymbolicEnvironment/Environment.h"

class TrackingExecutor : public sym::SymbolicExecutor {
public :
	BitMap *lastCondition[3];
	unsigned int condCount;
	BitMap *bitMapZero;
	unsigned int varCount;

	TrackingExecutor(sym::SymbolicEnvironment *e, unsigned int varCount);

	virtual void *CreateVariable(const char *name, DWORD size);

	virtual void *MakeConst(DWORD value, DWORD bits);

	virtual void *ExtractBits(void *expr, DWORD lsb, DWORD size);
	virtual void *ConcatBits(void *expr1, void *expr2);

	#define OPERAND_BITMASK(idx) (0x00010000 << (idx))

	static const unsigned int flagValues[];

	struct Operands {
		bool useOp[4];
		BitMap *operands[4];
		bool useFlag[7];
		BitMap *flags[7];
	};

	void ExecuteJCC(unsigned int flag, RiverInstruction *instruction, const Operands &ops);
	void ExecuteJMPOp(unsigned int op, const Operands &ops);

	void ResetCond();

	virtual void Execute(RiverInstruction *instruction);
};



#endif
