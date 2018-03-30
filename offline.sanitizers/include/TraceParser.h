#ifndef __TRACE_PARSER__
#define __TRACE_PARSER__

#include "z3.h"
#include "Z3Handler.h"

#include "CommonCrossPlatform/Common.h"

#include <vector>

#ifndef FLAG_LEN
#define FLAG_LEN ((1 << 3) - 1)
#endif

struct BareBasicBlock {
	unsigned int offset;
	char module[MAX_PATH];
};

struct BasicBlock {
	struct BareBasicBlock current;
	union AssertionData {
		struct Address {
			unsigned int esp;
		} asAddress;

		struct Jcc {
			unsigned short jumpType;
			unsigned short jumpInstruction;
			struct BareBasicBlock next[2];
		} asJcc;
	} assertionData;
};

// address = base + scale x index + displacement
struct AddressAssertion {
	unsigned int offset;			//
	unsigned int symbolicBase;		//symbolic value of base
	unsigned int scale;				//concrete value of scale
	unsigned int symbolicIndex;		//symbolic value of index
	unsigned int displacement;		//concrete value of displacement
	bool input; bool output;		//read from or written to symbolicAddress
	struct BasicBlock basicBlock;   //basic block info to be used in assertions
	Z3_ast symbolicAddress;         //Z3 formula for address
};

struct JccCondition {
	unsigned int testFlags;			//flags that are tested by jump instruction
	unsigned int condition;			//symbolic value of jump condition
	unsigned int symbolicFlags[FLAG_LEN]; //symbolic values of all flags
	struct BasicBlock basicBlock;	//basic block info to be used in assertions
	Z3_ast symbolicCondition;		//Z3 formula for jump condition
};

class TraceParser {
	public:
		TraceParser();
		~TraceParser();

		bool Parse(FILE *input);

	private:
		std::vector<struct AddressAssertion> addrAssertions;
		std::vector<struct JccCondition> jccConditions;
		Z3Handler z3Handler;

		int ReadFromStream(unsigned char *buff, size_t size, FILE *input);
		int SmtToAst(Z3_ast *ast, char *smt, size_t size);
		void DebugPrint(unsigned type);
};
#endif
