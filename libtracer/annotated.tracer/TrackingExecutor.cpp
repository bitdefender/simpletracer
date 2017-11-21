#include "SymbolicEnvironment/Environment.h"

#include "TrackingExecutor.h"
#include "TaintedIndex.h"
#include "annotated.tracer.h"
#include "revtracer/river.h"
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

#define MAX_DEPS 20

TrackingExecutor::TrackingExecutor(sym::SymbolicEnvironment *e,
		unsigned int varCount, AbstractFormat *aFormat)
	: SymbolicExecutor(e), varCount(varCount), aFormat(aFormat) {
	ti = new TaintedIndex();
}

void *TrackingExecutor::CreateVariable(const char *name, DWORD size) {
	unsigned int source = atoi(name + 2);
	if (source < varCount) {
		aFormat->WriteTaintedIndexPayload(ti->GetIndex(), source);
	} else {
		fprintf(stderr, "Error: Wrong index: I[%lu]\n", ti->GetIndex());
	}
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

void *TrackingExecutor::MakeConst(DWORD value, DWORD bits) {
	return nullptr;
}

void *TrackingExecutor::ExtractBits(void *expr, DWORD lsb, DWORD size) {
	DWORD index = (DWORD)expr;

	aFormat->WriteTaintedIndexExtract(ti->GetIndex(), index, lsb, size);
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

void *TrackingExecutor::ConcatBits(void *expr1, void *expr2) {
	DWORD index1 = (DWORD)expr1;
	DWORD index2 = (DWORD)expr2;
	unsigned int operands[] = {
		index1, index2
	};

	aFormat->WriteTaintedIndexConcat(ti->GetIndex(), operands);
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

static const unsigned char flagList[] = {
		RIVER_SPEC_FLAG_CF,
		RIVER_SPEC_FLAG_PF,
		RIVER_SPEC_FLAG_AF,
		RIVER_SPEC_FLAG_ZF,
		RIVER_SPEC_FLAG_SF,
		RIVER_SPEC_FLAG_OF
};

static const char flagNames[6][3] = {
	"CF", "PF", "AF", "ZF", "SF", "OF"
};

static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);

void TrackingExecutor::SetOperands(RiverInstruction *instruction, DWORD index) {
	for (int i = 0; i < 4; ++i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
			env->SetOperand(i, (void *)index);
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & instruction->modFlags) {
			env->SetFlgValue(flagList[i], (void *)index);
		}
	}
}

void TrackingExecutor::UnsetOperands(RiverInstruction *instruction) {
	for (int i = 0; i < 4; ++i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
			env->UnsetOperand(i);
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & instruction->modFlags) {
			env->UnsetFlgValue(flagList[i]);
		}
	}
}

void TrackingExecutor::Execute(RiverInstruction *instruction) {
	//printf("[%08lx] %02x\n", instruction->instructionAddress, instruction->opCode);

	Operands ops;
	memset(&ops, 0, sizeof(ops));
	unsigned int trk = 0;
	DWORD lastOp = -1;

	for (int i = 0; i < 4; ++i) {
		BOOL isTracked;
		DWORD val;
		void *opVal;
		if (true == (ops.useOp[i] = env->GetOperand(i, isTracked, val, opVal))) {
			if (isTracked) {
				ops.operands[i] = (DWORD)opVal;
				trk++;
				lastOp = ops.operands[i];
			} else {
				ops.operands[i] = -1;
			}
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		BOOL isTracked;
		BYTE val;
		void *opVal;
		if (true == (ops.useFlag[i] = env->GetFlgValue(flagList[i], isTracked, val, opVal))) {
			if (isTracked) {
				ops.flags[i] = (DWORD)opVal;
				trk++;
				lastOp = ops.flags[i];
			} else {
				ops.flags[i] = -1;
			}
		}
	}

	// clear registers means losing the reference to the
	// tacked memory location. We unset it
	if (0x30 <= instruction->opCode && instruction->opCode <= 0x33) {
		if (ops.operands[0] == ops.operands[1]) {
			UnsetOperands(instruction);
			return;
		}
	}

	if (trk == 0) {
		UnsetOperands(instruction);
		return;
	}

	DWORD index = 0;
	unsigned int depsSize = 0;
	unsigned int deps[MAX_DEPS];
	unsigned int dest = 0;
	unsigned int flags = 0;
	if (trk) {
		if (1 == trk) {
			index = lastOp;
		} else {
			dest = ti->GetIndex();

			for (int i = 0; i < flagCount; ++i) {
				if (ops.useFlag[i] && ops.flags[i] != -1) {
					deps[depsSize] = ops.flags[i];
					depsSize += 1;
					flags |= (1 << i);
				}
			}

			for (int i = 0; i < 4; ++i) {
				if (ops.useOp[i] && ops.operands[i] != -1) {
					deps[depsSize] = ops.operands[i];
					depsSize += 1;
				}
			}

			BasicBlockPointer bbp;
			TranslateAddressToBasicBlockPointer(&bbp,
					instruction->instructionAddress, mCount, mInfo);
			aFormat->WriteTaintedIndexExecute(dest, bbp, flags,
					depsSize, deps);
			index = ti->GetIndex();
			ti->NextIndex();
		}

		SetOperands(instruction, index);
	} else {
		//unset operands if none are symbolic
		UnsetOperands(instruction);
	}
}
