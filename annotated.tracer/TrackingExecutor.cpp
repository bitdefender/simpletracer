#include "SymbolicEnvironment/Environment.h"

#include "TrackingExecutor.h"
#include "annotated.tracer.h"

#include <stdlib.h>
#include <stdio.h>

TrackingExecutor::TrackingExecutor(sym::SymbolicEnvironment *e,
		AnnotatedTracer *at)
	: SymbolicExecutor(e), at(at) {
	condCount = 0;
	lastCondition[0] = lastCondition[1] = lastCondition[2] = nullptr;
}

void *TrackingExecutor::CreateVariable(const char *name, DWORD size) {
	BitMap *bmp = new BitMap(at->varCount, 1);
	bmp->SetBit(atoi(&name[2]));

	printf("Creating variable %s => ", name);
	bmp->Print();
	printf("\n");
	fflush(stdout);

	return bmp;
}

void *TrackingExecutor::MakeConst(DWORD value, DWORD bits) {
	at->bitMapZero->AddRef();
	return at->bitMapZero;
}

void *TrackingExecutor::ExtractBits(void *expr, DWORD lsb, DWORD size) {
	BitMap *bmp = (BitMap *)expr;

	if (nullptr == bmp) {
		return nullptr;
	}

	printf("Extract ");
	bmp->Print();
	printf(" %ld %ld => ", 4 - (lsb >> 3) - (size >> 3), size >> 3);

	BitMap *ret = new BitMap(*bmp, 4 - (lsb >> 3) - (size >> 3), size >> 3);
	ret->Print();
	printf("\n");
	fflush(stdout);
	return ret;
}

void *TrackingExecutor::ConcatBits(void *expr1, void *expr2) {
	BitMap *bmp1 = (BitMap *)expr1;
	BitMap *bmp2 = (BitMap *)expr2;

	if (nullptr == bmp1) {
		if (nullptr == bmp2) {
			return nullptr;
		}

		at->bitMapZero->AddRef();
		bmp1 = at->bitMapZero;
	}

	if (nullptr == bmp2) {
		at->bitMapZero->AddRef();
		bmp2 = at->bitMapZero;
	}

	BitMap *ret = new BitMap(*bmp1, *bmp2);
	printf("Concat ");
	bmp1->Print();
	printf(", ");
	bmp2->Print();
	printf("=> ");
	ret->Print();
	printf("\n");
	fflush(stdout);
	return ret;
}

void TrackingExecutor::ExecuteJCC(unsigned int flag, RiverInstruction *instruction, const Operands &ops) {
	condCount = 0;

	for (int i = 0; i < 7; ++i) {
		if ((1 << i) && instruction->testFlags) {
			if (ops.useFlag[i]) {
				lastCondition[condCount] = ops.flags[i];
				lastCondition[condCount]->AddRef();
				condCount++;
			}
		}
	}
}

void TrackingExecutor::ExecuteJMPOp(unsigned int op, const Operands &ops) {
	condCount = 0;
	if (ops.useOp[op]) {
		lastCondition[condCount] = ops.operands[op];
		lastCondition[condCount]->AddRef();
		condCount++;
	}
}

void TrackingExecutor::ResetCond() {
	condCount = 0;
}

void TrackingExecutor::Execute(RiverInstruction *instruction) {
	static const unsigned char flagList[] = {
		RIVER_SPEC_FLAG_CF,
		RIVER_SPEC_FLAG_PF,
		RIVER_SPEC_FLAG_AF,
		RIVER_SPEC_FLAG_ZF,
		RIVER_SPEC_FLAG_SF,
		RIVER_SPEC_FLAG_OF
	};

	static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);

	//printf("[%08lx] %02x\n", instruction->instructionAddress, instruction->opCode);

	Operands ops;
	memset(&ops, 0, sizeof(ops));
	unsigned int trk = 0;
	BitMap *lastOp = nullptr;

	for (int i = 0; i < 4; ++i) {
		BOOL isTracked;
		DWORD val;
		void *opVal;
		if (true == (ops.useOp[i] = env->GetOperand(i, isTracked, val, opVal))) {
			if (isTracked) {
				ops.operands[i] = (BitMap *)opVal;
				ops.operands[i]->AddRef();
				trk++;
				lastOp = ops.operands[i];
			} else {
				at->bitMapZero->AddRef();
				ops.operands[i] = at->bitMapZero;
			}
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		BOOL isTracked;
		BYTE val;
		void *opVal;
		if (true == (ops.useFlag[i] = env->GetFlgValue(flagList[i], isTracked, val, opVal))) {
			if (isTracked) {
				ops.flags[i] = (BitMap *)opVal;
				ops.flags[i]->AddRef();
				trk++;
				lastOp = ops.flags[i];
			} else {
				at->bitMapZero->AddRef();
				ops.operands[i] = at->bitMapZero;
			}
		}
	}

	if (0 == (instruction->modifiers & RIVER_MODIFIER_EXT)) {
		if ((0x70 <= instruction->opCode) && (instruction->opCode < 0x80)) {
			ExecuteJCC(instruction->opCode - 0x70, instruction, ops);
		}

		/*if (0xFF == instruction->opCode) {
		  if ((0x02 == instruction->subOpCode) || (0x04 == instruction->subOpCode)) {
		  ExecuteJMPOp(0, ops);
		  }
		  }*/
	} else {
		if ((0x80 <= instruction->opCode) && (instruction->opCode < 0x90)) {
			ExecuteJCC(instruction->opCode - 0x80, instruction, ops);
		}
	}

	if (trk) {
		BitMap *ret = nullptr;

		if (1 == trk) {
			lastOp->AddRef();
			ret = lastOp;
		} else {
			ret = new BitMap(at->varCount, 1);

			for (int i = 0; i < 4; ++i) {
				if (ops.useOp[i]) {
					ret->Union(*ops.operands[i]);
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (ops.useFlag[i]) {
					ret->Union(*ops.flags[i]);
				}
			}
		}

		if (ret->IsZero()) {
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
		} else {
			for (int i = 0; i < 4; ++i) {
				if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
					env->SetOperand(i, ret);
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (flagList[i] & instruction->modFlags) {
					env->SetFlgValue(flagList[i], ret);
				}
			}

			for (int i = 0; i < 4; ++i) {
				if (ops.useOp[i]) {
					ops.operands[i]->DelRef();
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (ops.useFlag[i]) {
					ops.flags[i]->DelRef();
				}
			}
		}

		ret->DelRef();
	} else {
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
}
