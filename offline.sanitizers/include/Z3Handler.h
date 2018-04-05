#ifndef __Z3_HANDLER__
#define __Z3_HANDLER__

#include "z3.h"

class Z3Handler {
	const int MAX_LEN = 4;
	const int MAX_SYMBOL_NAME = 7;

	public:
		Z3Handler();
		~Z3Handler();
		Z3_ast toAst(char *smt, size_t size);
		void PrintAst(Z3_ast ast);
		bool solve(Z3_ast ast, unsigned address);

	private:
		Z3_context context;
		Z3_optimize opt;

		void check_opt_and_get_values(void);
		void get_constants(Z3_model model);
};

#endif
