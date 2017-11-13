#ifndef __TRACKING_EXECUTOR__
#define __TRACKING_EXECUTOR__

#include "SymbolicEnvironment/Environment.h"
#include "TaintedIndex.h"

class TrackingExecutor : public sym::SymbolicExecutor {
public :
	unsigned int varCount;
	TaintedIndex *ti;

	TrackingExecutor(sym::SymbolicEnvironment *e, unsigned int varCount);

	virtual void *CreateVariable(const char *name, DWORD size);

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

	void ExecuteJCC(unsigned int flag, RiverInstruction *instruction, const Operands &ops);
	void ExecuteJMPOp(unsigned int op, const Operands &ops);

	virtual void Execute(RiverInstruction *instruction);
};



#endif
