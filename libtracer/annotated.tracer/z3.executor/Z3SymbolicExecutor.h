#ifndef _Z3_SYMBOLIC_EXECUTOR_
#define _Z3_SYMBOLIC_EXECUTOR_

#include "z3.h"

#include "SymbolicEnvironment/SymbolicEnvironment.h"
#include "SymbolicEnvironment/LargeStack.h"

#include "VariableTracker.h"
#include "AbstractLog.h"

#define OPERAND_BITMASK(idx) (0x00010000 << (idx))

struct SymbolicOperandsLazyFlags {
	void *svBefore[4];
	void *svAfter[4];

	void *svfBefore[7];
};

class Z3SymbolicExecutor : public sym::SymbolicExecutor {
private:
	stk::DWORD saveTop;
	stk::DWORD saveStack[0x10000];
	stk::LargeStack *ls;

	AbstractFormat *aFormat;

	struct SymbolicOperands {
		nodep::DWORD av;
		nodep::BOOL tr[4];
		nodep::DWORD cvb[4];
		nodep::DWORD cva[4];
		void *sv[4];

		nodep::BOOL trf[7];
		nodep::BYTE cvfb[7];
		nodep::BYTE cvfa[7];
		void *svf[7];
	};

	static const unsigned char flagList[7];
	static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);
	static const int opCount = 4;

	//void EvalZF(Z3_ast result);

	bool CheckSameSort(unsigned size, Z3_ast *ops);
	void InitLazyFlagsOperands(
			struct SymbolicOperandsLazyFlags *solf,
			struct SymbolicOperands *ops);
	void PrintSetOperands(unsigned idx);
	void PrintAST(Z3_ast ast);

	void SymbolicExecuteUnk(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteNop(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteCmpxchg(RiverInstruction *instruction, SymbolicOperands *ops);

	typedef Z3_ast(Z3SymbolicExecutor::*BVFunc)(SymbolicOperands * ops);

	template<unsigned int flag> Z3_ast Flag(SymbolicOperands * ops);
	template<Z3SymbolicExecutor::BVFunc func> Z3_ast Negate(SymbolicOperands * ops);
	template<Z3SymbolicExecutor::BVFunc func1, Z3SymbolicExecutor::BVFunc func2> Z3_ast Equals(SymbolicOperands * ops);
	template<Z3SymbolicExecutor::BVFunc func1, Z3SymbolicExecutor::BVFunc func2> Z3_ast Or(SymbolicOperands * ops);

	template<Z3SymbolicExecutor::BVFunc func> void SymbolicJumpCC(RiverInstruction * instruction, SymbolicOperands * ops);
	template<Z3SymbolicExecutor::BVFunc func> void SymbolicSetCC(RiverInstruction * instruction, SymbolicOperands * ops);
	template<Z3SymbolicExecutor::BVFunc func> void SymbolicCmovCC(RiverInstruction *instruction, SymbolicOperands *ops);

	void SymbolicExecuteMov(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteMovSx(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteMovS(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteMovZx(RiverInstruction *instruction, SymbolicOperands *ops);

	void SymbolicExecuteImul(RiverInstruction *instruction, SymbolicOperands *ops);

	typedef Z3_ast (Z3SymbolicExecutor::*CommonOperation)(unsigned nOps, SymbolicOperands *ops);
	template <Z3SymbolicExecutor::CommonOperation func, unsigned int funcCode>
	void SymbolicExecuteCommonOperation(RiverInstruction *instruction, SymbolicOperands *ops);

	Z3_ast ExecuteInc(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteDec(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteAdd(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteOr (unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteAdc(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteSbb(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteAnd(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteSub(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteXor(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteCmp(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteTest(unsigned nOps, SymbolicOperands *ops);

	Z3_ast ExecuteRol(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteRor(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteRcl(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteRcr(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteShl(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteShr(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteSal(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteSar(unsigned nOps, SymbolicOperands *ops);

	Z3_ast ExecuteNot(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteNeg(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteMul(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteImul(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteDiv(unsigned nOps, SymbolicOperands *ops);
	Z3_ast ExecuteIdiv(unsigned nOps, SymbolicOperands *ops);

	void GetSymbolicValues(RiverInstruction *instruction, SymbolicOperands *ops, nodep::DWORD mask);
public:
	int symIndex;

	Z3_config config;
	Z3_context context;
	Z3_sort dwordSort, wordSort, tbSort, byteSort, bitSort;
	Z3_sort sevenBitSort;

	Z3_ast zero32, zero16, zero7, zero8, zeroFlag, oneFlag;
	Z3_ast one8, one32;
	Z3_ast zeroScale, twoScale, fourScale;

	class Z3SymbolicCpuFlag {
	protected:
		Z3SymbolicExecutor *parent;
		virtual Z3_ast Eval() = 0;
		Z3_ast value;

		static const unsigned int lazyMarker;
	public :
		Z3SymbolicCpuFlag();
		void SetParent(Z3SymbolicExecutor *p);

		virtual void SetSource(struct SymbolicOperandsLazyFlags &ops,
				unsigned int op) = 0;
		virtual void SaveState(stk::LargeStack &stack) = 0;
		virtual void LoadState(stk::LargeStack &stack) = 0;

		void SetValue(Z3_ast val);
		void Unset();

		Z3_ast GetValue();
	} *lazyFlags[7];

	Z3_ast lastCondition;

	Z3_solver solver;

	//VariableTracker<Z3_ast> variableTracker;

	typedef void(Z3SymbolicExecutor::*SymbolicExecute)(RiverInstruction *instruction, SymbolicOperands *ops);
	template <Z3SymbolicExecutor::SymbolicExecute fSubOps[8]> void SymbolicExecuteSubOp(RiverInstruction *instruction, SymbolicOperands *ops);

	static SymbolicExecute executeFuncs[2][0x100];
	static SymbolicExecute executeAssignmentOperations[8];
	static SymbolicExecute executeRotationOperations[8];
	static SymbolicExecute executeAssignmentLogicalOperations[8];

	Z3SymbolicExecutor(sym::SymbolicEnvironment *e, AbstractFormat *aFormat);
	~Z3SymbolicExecutor();

	void StepForward();
	void StepBackward();
	//void Lock(Z3_ast t);


	virtual void *CreateVariable(const char *name, nodep::DWORD size);
	virtual void *MakeConst(nodep::DWORD value, nodep::DWORD bits);
	virtual void *ExtractBits(void *expr, nodep::DWORD lsb, nodep::DWORD size);
	virtual void *ConcatBits(void *expr1, void *expr2);
	virtual void Execute(RiverInstruction *instruction);
	virtual void *ExecuteResolveAddress(void *base, void *index, nodep::BYTE scale);
	virtual void ComposeScaleAndIndex(nodep::BYTE &scale, struct OperandInfo &indexOp);
	virtual void AddOperands(struct OperandInfo &left, struct OperandInfo &right, unsigned displacement, struct OperandInfo &result);
};

class Z3FlagZF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private :
	Z3_ast source;
protected :
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};


class Z3FlagSF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagPF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

#define Z3_FLAG_OP_ADD		0xA0
#define Z3_FLAG_OP_OR 		0xA1
#define Z3_FLAG_OP_ADC		0xA2
#define Z3_FLAG_OP_SBB		0xA3
#define Z3_FLAG_OP_AND		0xA4
#define Z3_FLAG_OP_SUB		0xA5
#define Z3_FLAG_OP_XOR		0xA6
#define Z3_FLAG_OP_CMP		0xA7
#define Z3_FLAG_OP_INC		0xA8

#define Z3_FLAG_OP_ROL		0xA9
#define Z3_FLAG_OP_ROR		0xAA
#define Z3_FLAG_OP_RCL		0xAB
#define Z3_FLAG_OP_RCR		0xAC
#define Z3_FLAG_OP_SHL		0xAD
#define Z3_FLAG_OP_SHR		0xAE
#define Z3_FLAG_OP_SAL		0xAF
#define Z3_FLAG_OP_SAR		0xB0

#define Z3_FLAG_OP_NOT		0xB1
#define Z3_FLAG_OP_NEG		0xB2
#define Z3_FLAG_OP_MUL		0xB3
#define Z3_FLAG_OP_DIV		0xB4

class Z3FlagCF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source, p[2];
	Z3_ast cf;
	unsigned int func;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagOF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source, p[2];
	unsigned int func;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagAF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagDF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};


#endif

