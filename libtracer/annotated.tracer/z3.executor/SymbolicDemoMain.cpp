#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdio.h>

#include "Execution/Execution.h"

#include "VariableTracker.h"
#include "TrackingCookie.h"

#include "SymbolicEnvironment/Environment.h"
#include "Z3SymbolicExecutor.h"

#include "ReversibleAction.h"

#include "z3.h"

#include "CommonCrossPlatform/Common.h"
#include "CommonCrossPlatform/BasicTypes.h"
#include "CommonCrossPlatform/CommonSpecifiers.h"

#include "FileLog.h"
#include "TextFormat.h"

ExecutionController *ctrl;

#ifdef _WIN32
HANDLE fDbg = ((HANDLE)(LONG_PTR)-1);
#else
FILE_T fDbg = -1;
#endif

#define PRINTF(fmt, ...) \
	do { \
		printf((fmt), ##__VA_ARGS__); \
		fflush(stdout); \
	} while (false);


ReversibleAction<TrackedVariableData, Unlock, 32> lockedVariables;
Z3SymbolicExecutor *executor = nullptr;
sym::SymbolicEnvironment *regEnv = nullptr;
sym::SymbolicEnvironment *revEnv = nullptr;
::DWORD lastDirection = EXECUTION_ADVANCE;

// here are the bindings to the payload
extern "C" unsigned int bufferDword[];
int PayloadDword();
int CheckDword(unsigned int *ptr);

/*extern "C" unsigned char bufferByte[];
int PayloadByte();
int CheckByte(unsigned char *ptr);*/

class TVDBuffer {
private:
	TrackedVariableData vars[8];
public :
	TrackedVariableData *GetVariable(rev::ADDR_TYPE addr) {
		DWORD idx = (unsigned int *)addr - bufferDword;

		if (idx < 8) {
			return &vars[idx];
		}

		return nullptr;
	}
} tvdTracker;

struct TrackedCondition {
	bool wasInverted;
	Z3_ast ast;
};
void UnloadCondition(void *ctx, TrackedCondition *itm);

struct ConditionTracker : public ReversibleAction<TrackedCondition, UnloadCondition, 64> {
public :
	TrackedCondition conditionPool[64];
	TrackedCondition *freeConditions[64];
	::DWORD freeCondCount;

	TrackedCondition *tvds[64];
	::DWORD revCount;

	TrackedCondition *lastCond;

	ConditionTracker() : ReversibleAction<TrackedCondition, UnloadCondition, 64>(this) {
		freeCondCount = sizeof(conditionPool) / sizeof(conditionPool[0]);
		revCount = 0;
		lastCond = nullptr;
		for (::DWORD i = 0; i < freeCondCount; ++i) {
			freeConditions[i] = &conditionPool[i];
		}
	}

	TrackedCondition *AllocCondition() {
		if (0 == freeCondCount) {
			return nullptr;
		}

		TrackedCondition *ret = freeConditions[--freeCondCount];
		ret->wasInverted = false;
		PRINTF("Alloc condition %08lx. Left: %lX\n", (DWORD)ret, freeCondCount);
		return ret;
	}

	void FreeCondition(TrackedCondition *cond) {
		PRINTF("Free condition %08lx. Left: %lX\n", (DWORD)cond, freeCondCount);
		freeConditions[freeCondCount++] = cond;
	}

	void Unload(TrackedCondition *tvd) {
		PRINTF("Tracking condition %08lx\n", (DWORD)tvd);
		tvds[revCount] = tvd;
		revCount++;

		lastCond = tvd;
	}

	TrackedCondition *Reload() {
		TrackedCondition *ret = tvds[--revCount];
		tvds[revCount] = nullptr;

		PRINTF("Restoring condition %08lx\n", (DWORD)ret);
		return ret;
	}

	void ResetConds() {
		for (::DWORD i = 0; i < revCount; ++i) {
			FreeCondition(tvds[i]);
		}
		revCount = 0;
	}

	bool TryPop(TrackedCondition *&cond) {
		if (0 < revCount) {
			cond = lastCond;
			lastCond = nullptr;

			if (nullptr != cond) {
				revCount--;
				return true;
			}
		}

		return false;
	}
};

void UnloadCondition(void *ctx, TrackedCondition *itm) {
	ConditionTracker *_this = (ConditionTracker *)ctx;

	_this->Unload(itm);
}

struct CustomExecutionContext : public ConditionTracker {
public :
	::DWORD executionPos;
	int backSteps;
	enum {
		EXPLORING,
		BACKTRACKING,
		PATCHING,
		RESTORING
	} executionState;

	void Init() {
		executionPos = 0;
		backSteps = 0;
		executionState = EXPLORING;
	}
};

TIME_FREQ_T liFreq;
TIME_T liStart, liStop,
	   liSymStart, liSymStop,
	   liBrStart, liBrStop;
TIME_RES_T liSymTotal, liBrTotal, liTotal;

//unsigned char newPass[9];
unsigned int newPass[9];

void AddRef(void *) {}
void DelRef(void *) {}

void AddReference(void *ref) {}
void DelReference(void *ref) {}

class SymbolicExecution : public ExecutionObserver {
private :
	CustomExecutionContext cec;
public:
	FileLog *aLog;
	AbstractFormat *aFormat;
public:

	virtual unsigned int ExecutionBegin(void *ctx, rev::ADDR_TYPE addr) {
		static bool ctxInit = false;

		START_COUNTER(liSymStart, liFreq);
		if (!ctxInit) {
			revEnv = NewX86RevtracerEnvironment(ctx, ctrl); //new RevSymbolicEnvironment(ctx, ctrl);
			regEnv = NewX86RegistersEnvironment(revEnv); //new OverlappedRegistersEnvironment();
			regEnv->SetReferenceCounting(AddRef, DelRef);
			executor = new Z3SymbolicExecutor(regEnv, nullptr);
			regEnv->SetExecutor(executor);
			regEnv->SetReferenceCounting(AddReference, DelReference);


			cec.Init();
			revEnv->SetSymbolicVariable("a[0]", (rev::ADDR_TYPE)(&bufferDword[0]), 4);
			revEnv->SetSymbolicVariable("a[1]", (rev::ADDR_TYPE)(&bufferDword[1]), 4);
			revEnv->SetSymbolicVariable("a[2]", (rev::ADDR_TYPE)(&bufferDword[2]), 4);
			revEnv->SetSymbolicVariable("a[3]", (rev::ADDR_TYPE)(&bufferDword[3]), 4);
			revEnv->SetSymbolicVariable("a[4]", (rev::ADDR_TYPE)(&bufferDword[4]), 4);
			revEnv->SetSymbolicVariable("a[5]", (rev::ADDR_TYPE)(&bufferDword[5]), 4);
			revEnv->SetSymbolicVariable("a[6]", (rev::ADDR_TYPE)(&bufferDword[6]), 4);
			revEnv->SetSymbolicVariable("a[7]", (rev::ADDR_TYPE)(&bufferDword[7]), 4);

			ctxInit = true;
			START_COUNTER(liStart, liFreq);
			//TODO liSymTotal.QuadPart = 0;
		}

		nodep::DWORD ret = EXECUTION_ADVANCE;

		if (cec.BACKTRACKING == cec.executionState) {
			GET_COUNTER(liStart, liStop, liFreq, liTotal);

			TIME_RES_T result;
			GET_AGGREGATE_RESULT(liSymTotal, liFreq, result);
			fprintf(stderr, "$$ Symbolic time: %lfms\n", result);
			GET_AGGREGATE_RESULT(liBrTotal, liFreq, result);
			fprintf(stderr, "$$ Branching time: %lfms\n", result);
			fprintf(stderr, "$$ Execution time: %lfms\n", liTotal);
			ret = EXECUTION_TERMINATE;
		}

		GET_COUNTER_AGGREGATE(liSymStart, liSymStop, liFreq, liSymTotal);
		return ret;
	}

	bool DonePatching(void *ctx, unsigned int *orig, unsigned int *newVal, unsigned int size) {
		bool ret = true;
		for (unsigned int i = 0; i < size; ++i) {
			if (orig[i] != newVal[i]) {
				/*Z3_ast ast = (Z3_ast)ctrl->GetMemoryInfo(ctx, &orig[i]);
				TrackedVariableData *tvd = (TrackedVariableData *)Z3_get_user_ptr(executor->context, ast);*/
				TrackedVariableData *tvd = tvdTracker.GetVariable(&orig[i]);

				if (tvd->IsLocked()) {
					ret = false;
				}
				else {
					orig[i] = newVal[i];
				}
			}
		}

		return ret;
	}

	virtual unsigned int ExecutionControl(void *ctx, rev::ADDR_TYPE addr) {
		nodep::DWORD ret = EXECUTION_ADVANCE;
		TrackedCondition *lastCondition;

		START_COUNTER(liSymStart, liFreq);
		static const char c[][32] = {
			"EXPLORING",
			"BACKTRACKING",
			"PATCHING",
			"REVERTING"
		};

		if (EXECUTION_BACKTRACK == lastDirection) {
			//cec.executionPos--;
			executor->StepBackward();
			lockedVariables.Backward();
			cec.Backward();

			switch (cec.executionState) {
			case CustomExecutionContext::BACKTRACKING:
				ret = EXECUTION_BACKTRACK;

				if (cec.TryPop(lastCondition)) {
					PRINTF("Trying to invert condition %p - %s\n", lastCondition->ast, lastCondition->wasInverted ? "true" : "false");

					if (!lastCondition->wasInverted) {
						lastCondition->ast = Z3_simplify(
							executor->context,
							Z3_mk_not(
								executor->context,
								lastCondition->ast
							)
						);

						//PRINTF("Invert condition %08x\n", cond);
						lastCondition->ast = Z3_simplify(executor->context, lastCondition->ast);
						//PRINTF("(tryinvert %s)\n\n", Z3_ast_to_string(executor->context, lastCondition->ast));

						if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, lastCondition->ast)) {
							Z3_solver_assert(executor->context, executor->solver, lastCondition->ast);

							PRINTF("(tryassert %s)\n\n", Z3_ast_to_string(executor->context, lastCondition->ast));

							//unsigned int actualPass[10] = { 0 };
							Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

							if (Z3_L_TRUE == ret) {
								PRINTF("==============================================================\n");
								Z3_model model = Z3_solver_get_model(executor->context, executor->solver);

								unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

								for (unsigned int i = 0; i < 8; ++i) {
									newPass[i] = 0;
								}

								for (unsigned int i = 0; i < cnt; ++i) {
									Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

									//PRINTF("%s\n", Z3_func_decl_to_string(executor->context, c));

									Z3_ast ast = Z3_func_decl_to_ast(executor->context, c);
									Z3_symbol sName = Z3_get_decl_name(executor->context, c);
									Z3_string r = Z3_get_symbol_string(executor->context, sName);

									Z3_ast val;
									Z3_bool pEval = Z3_eval_func_decl(
										executor->context,
										model,
										c,
										&val
									);

									nodep::DWORD ret;
									Z3_bool p = Z3_get_numeral_uint(executor->context, val, (unsigned int *)&ret);

									newPass[r[2] - '0'] = (unsigned char)ret;
									//actualPass[r[2] - '0'] = ret;
								}

								PRINTF("NEW INPUT \"");
								for (int i = 0; newPass[i]; ++i) {
									PRINTF("%c", newPass[i]);
								}
								PRINTF("\" OUTPUT %04x\n", CheckDword(newPass));

								fprintf(stderr, "NEW INPUT \"");
								for (int i = 0; newPass[i]; ++i) {
									fprintf(stderr, "%c", newPass[i]);
								}
								fprintf(stderr, "\" OUTPUT %04x\n", CheckDword(newPass));

								cec.executionState = cec.PATCHING;
							}
							else if (Z3_L_FALSE == ret) {
							}
							else {
							}

							lastCondition->wasInverted = true;
							cec.Unload(lastCondition);
						}
					}
				}

				break;
			case CustomExecutionContext::PATCHING:
				ret = EXECUTION_BACKTRACK;
				cec.backSteps++;

				/* this should happen now automagically
				
				  if (cec.TryPop(lastCondition)) {
					cec.Unload(lastCondition);
				}*/

				if (DonePatching(ctx, (unsigned int *)bufferDword, (unsigned int *)newPass, 8)) {
					cec.executionState = cec.RESTORING;
					ret = EXECUTION_ADVANCE;
				}
				break;

			case CustomExecutionContext::EXPLORING:
			case CustomExecutionContext::RESTORING:
				DEBUG_BREAK;
			}

			printf(
				"BACKTRACK *** step %d; addr %p; state %s; backSteps %d, coundCount %d revCount %lu\n",
				cec.GetStep(),
				addr,
				c[cec.executionState],
				cec.backSteps,
				cec.GetTop(),
				cec.revCount
			);

			fflush(stdout);
		}
		else if (EXECUTION_ADVANCE == lastDirection) {
			PRINTF(
				"ADVANCE *** step %d; addr %p; state %s; backSteps %d, coundCount %d revCount %lu\n",
				cec.GetStep(),
				addr,
				c[cec.executionState],
				cec.backSteps,
				cec.GetTop(),
				cec.revCount
			);

			fflush(stdout);

			switch (cec.executionState) {
			case CustomExecutionContext::EXPLORING:
				if (nullptr != executor->lastCondition) {
					printf("!@(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

					if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, executor->lastCondition)) {
						TrackedCondition *cond = cec.AllocCondition();
						if (!cond)
							DEBUG_BREAK;
						cond->wasInverted = false;
						cond->ast = executor->lastCondition;
						//Z3_set_user_ptr(executor->context, executor->lastCondition, cond, CustomExecutionContext::DummyFreeCondition);

						cec.Push(cond);
						Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

						PRINTF("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

						executor->lastCondition = nullptr;
					}
				}
				break;

			case CustomExecutionContext::RESTORING:
				if (nullptr != executor->lastCondition) {

					if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, executor->lastCondition)) {
						TrackedCondition *cond = cec.Reload();
						//Z3_set_user_ptr(executor->context, executor->lastCondition, cond, CustomExecutionContext::DummyFreeCondition);
						cond->ast = executor->lastCondition;

						cec.Push(cond);
						Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

						PRINTF("(assert %s)\n", Z3_ast_to_string(executor->context, executor->lastCondition));
						PRINTF("wasInverted: %s\n\n", cond->wasInverted ? "true" : "false");

						executor->lastCondition = nullptr;
					}
				}

				cec.backSteps--;
				if (0 > cec.backSteps) {
					cec.executionState = cec.EXPLORING;
					cec.ResetConds();
					cec.backSteps = 0;
				}
				break;
			case CustomExecutionContext::BACKTRACKING:
			case CustomExecutionContext::PATCHING:
				DEBUG_BREAK;
			}
		}



		lastDirection = ret;

		if (EXECUTION_ADVANCE == ret) {
			executor->StepForward();
			lockedVariables.Forward();
			//cec.executionPos++;
			cec.Forward();
		}

		GET_COUNTER_AGGREGATE(liSymStart, liSymStop, liFreq, liSymTotal);
		return ret;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {
		START_COUNTER(liSymStart, liFreq);

		if (cec.EXPLORING != cec.executionState) {
			DEBUG_BREAK;
		}

		fprintf(stderr, "Check(\"");
		for (int i = 0; bufferDword[i]; ++i) {
			fprintf(stderr, "%c", bufferDword[i]);
		}
		fprintf(stderr, "\") = %04x\n", CheckDword(bufferDword));

		PRINTF("Check(\"");
		for (int i = 0; bufferDword[i]; ++i) {
			PRINTF("%c", bufferDword[i]);
		}
		PRINTF("\") = %04x\n", CheckDword(bufferDword));

		lastDirection = EXECUTION_BACKTRACK;
		cec.executionState = cec.BACKTRACKING;

		GET_COUNTER_AGGREGATE(liSymStart, liSymStop, liFreq, liSymTotal);

		return EXECUTION_BACKTRACK;
	}

	virtual void TerminationNotification(void *ctx) { }

	virtual unsigned int TranslationError(void *ctx, void *address) {
		printf("Translation error @%p\n", address);
		return EXECUTION_TERMINATE;
	}

} symbolicExecution;

void TrackCallback(nodep::DWORD value, nodep::DWORD address, nodep::DWORD segSel) {
	if (EXECUTION_ADVANCE == lastDirection) {
		TrackedVariableData *tvd = tvdTracker.GetVariable((rev::ADDR_TYPE)address);

		if ((nullptr != tvd) && (!tvd->IsLocked())) {
			tvd->Lock();
			lockedVariables.Push(tvd);
		}
	}
}

void MarkCallback(nodep::DWORD oldValue, nodep::DWORD newValue, nodep::DWORD address, nodep::DWORD segSel) {
	//__asm int 3;
}

void __stdcall SymbolicHandler(void *ctx, void *offset, void *addr) {
	RiverInstruction *instr = (RiverInstruction *)addr;

	regEnv->SetCurrentInstruction(instr, offset);
	executor->Execute(instr);
}

int main(int argc, char *argv[]) {


	GET_FREQ(liFreq);
	START_COUNTER(liStart, liFreq);
	PayloadDword();
	GET_COUNTER(liStart, liStop, liFreq, liTotal);

	fprintf(stderr, "$$ Native time: %lfms\n", liTotal);

	ctrl = NewExecutionController(EXECUTION_INPROCESS);
	ctrl->SetEntryPoint((void*)PayloadDword);

	FileLog *fLog = new FileLog();
	fLog->SetLogFileName("z3demo.log");
	symbolicExecution.aLog = fLog;

	symbolicExecution.aFormat = new TextFormat(symbolicExecution.aLog);

	ctrl->SetExecutionFeatures(EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_SYMBOLIC);
	ctrl->SetExecutionObserver(&symbolicExecution);
	ctrl->SetSymbolicHandler(SymbolicHandler);
	ctrl->SetTrackingObserver(TrackCallback, MarkCallback);

	ctrl->Execute();
	ctrl->WaitForTermination();

	return 0;
}
