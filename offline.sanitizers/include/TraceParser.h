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
	char module[MAX_PATH + 1];
};

struct BasicBlock {
	struct BareBasicBlock current;
	struct AssertionData {
		struct Address {
			unsigned int esp;
		} address;

		struct Jcc {
			unsigned short jumpType;
			unsigned short jumpInstruction;
			struct BareBasicBlock next[2];
		} jcc;
	} assertionData;

	struct BasicBlock & operator=(const struct BasicBlock &other) {
		if (this != &other) {
			this->current = other.current;
			this->assertionData = other.assertionData;
		}
		return *this;
	}
};

// address = base + scale x index + displacement
struct AddressAssertion {
	unsigned int offset;			//code offset in module
	unsigned int symbolicBase;		//symbolic value of base
	unsigned int scale;				//concrete value of scale
	unsigned int symbolicIndex;		//symbolic value of index
	unsigned int displacement;		//concrete value of displacement
	unsigned int composedAddress;   //symbolic value of address
	bool input; bool output;		//read from or written to symbolicAddress
	struct BasicBlock basicBlock;   //basic block info to be used in assertions
	Z3_ast symbolicAddress;         //Z3 formula for address
};


typedef int (*sanitizer_address)(const struct AddressAssertion &);

struct JccCondition {
	unsigned int testFlags;			//flags that are tested by jump instruction
	unsigned int condition;			//symbolic value of jump condition
	unsigned int symbolicFlags[FLAG_LEN]; //symbolic values of all flags
	struct BasicBlock basicBlock;	//basic block info to be used in assertions
	Z3_ast symbolicCondition;		//Z3 formula for jump condition
};

typedef int (*sanitizer_jcc)(const struct JccCondition &);

enum ParserState {
	NONE = 0,
	Z3_MODULE,
	Z3_SYMBOLIC_OBJECT,
	Z3_AST,
	Z3_OFFSET
};

class TraceParser {
	public:
		TraceParser();
		~TraceParser();

		bool Parse(FILE *input);
		bool GetAddressAssertion(size_t index, struct AddressAssertion *&addrAssertion);
		void GetHandler(Z3Handler *&handler);

	private:
		std::vector<struct AddressAssertion> addrAssertions;
		std::vector<struct JccCondition> jccConditions;
		Z3Handler z3Handler;
		int state, lviAddr, lviJcc;

		char lastModule[MAX_PATH + 1];
		char lastNextModule[MAX_PATH + 1];

		struct BasicBlock tmpBasicBlock;
		struct AddressAssertion tmpAddrAssertion;
		struct JccCondition tmpJccCond;

		int ReadFromStream(unsigned char *buff, size_t size, FILE *input);
		int SmtToAst(Z3_ast &ast, char *smt, size_t size);
		void DebugPrint(Z3_ast ast);
		void DebugPrint(unsigned type);
		void DebugPrint(const struct AddressAssertion &addrAssertion);
		void DebugPrint(const struct JccCondition &jccCondition);
		void PrintState();
		void PrintJump(unsigned short jumpType, unsigned short jumpInstruction);
		void CleanTempStructs();
		void ExchageModule(char *dest, char *moduleName, size_t size);
};
#endif
