#include "SymbolicEnvironment/Environment.h"

#include "TrackingExecutor.h"
#include "TaintedIndex.h"
#include "annotated.tracer.h"

#include <stdlib.h>
#include <stdio.h>

TrackingExecutor::TrackingExecutor(sym::SymbolicEnvironment *e,
		unsigned int varCount)
	: SymbolicExecutor(e), varCount(varCount) {
	ti = new TaintedIndex();
}

void *TrackingExecutor::CreateVariable(const char *name, DWORD size) {
	if (atoi(name + 2) < varCount) {
		fprintf(stderr, "I[%lu] <= %s\n", ti->GetIndex(), name);
	} else {
		printf("Error: Wrong index: I[%lu]\n", ti->GetIndex());
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

	fprintf(stderr, "I[%lu] <= I[%lu][%lu:%lu]\n",
			ti->GetIndex(), index, size, lsb);
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

void *TrackingExecutor::ConcatBits(void *expr1, void *expr2) {
	DWORD index1 = (DWORD)expr1;
	DWORD index2 = (DWORD)expr2;

	fprintf(stderr, "I[%lu] <= I[%lu] ++ I[%lu]\n", ti->GetIndex(),
			index1, index2);
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

void TrackingExecutor::ExecuteJCC(unsigned int flag, RiverInstruction *instruction, const Operands &ops) {
}

void TrackingExecutor::ExecuteJMPOp(unsigned int op, const Operands &ops) {
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
	unsigned int usedOps = 0;
	unsigned int usedFlags = 0;
	DWORD lastOp = -1;

	for (int i = 0; i < 4; ++i) {
		BOOL isTracked;
		DWORD val;
		void *opVal;
		if (true == (ops.useOp[i] = env->GetOperand(i, isTracked, val, opVal))) {
			if (isTracked) {
				ops.operands[i] = (DWORD)opVal;
				trk++;
				usedOps += 1;
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
				usedFlags += 1;
				lastOp = ops.flags[i];
			} else {
				ops.flags[i] = -1;
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

	if (trk == 0) {
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
		return;
	}

	DWORD optIndex = 0, flagIndex = 0;
	if (trk) {
		if (1 == trk) {
			optIndex = flagIndex = lastOp;
		} else {
			if (usedOps) {
				fprintf(stderr, "operands: I[%lu] <= ", ti->GetIndex());
				for (int i = 0; i < 4; ++i) {
					if (ops.useOp[i] && ops.operands[i] != -1) {
						fprintf(stderr, "I[%lu] | ", ops.operands[i]);
					}
				}
				fprintf(stderr, "\n");
				optIndex = ti->GetIndex();
				ti->NextIndex();
			}

			if (usedFlags) {
				fprintf(stderr, "flags: I[%lu] <= ", ti->GetIndex());
				for (int i = 0; i < flagCount; ++i) {
					if (ops.useFlag[i] && ops.flags[i] != -1) {
						fprintf(stderr, "I[%lu] | ", ops.flags[i]);
					}
				}
				fprintf(stderr, "\n");
				flagIndex = ti->GetIndex();
				ti->NextIndex();
			}
		}


		for (int i = 0; i < 4; ++i) {
			if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
				env->SetOperand(i, (void *)optIndex);
			}
		}

		for (int i = 0; i < flagCount; ++i) {
			if (flagList[i] & instruction->modFlags) {
				env->SetFlgValue(flagList[i], (void *)flagIndex);
			}
		}

	}
}
