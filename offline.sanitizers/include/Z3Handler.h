#ifndef __Z3_HANDLER__
#define __Z3_HANDLER__

#include "z3.h"
#include "IntervalTree.h"
#include "CommonCrossPlatform/Common.h"

#include <string.h>
#include <string>
#include <map>

class Z3Model {
	public:
		Z3Model(Z3_context context, Z3_optimize opt);
		~Z3Model() {}

		template <typename T>
		int get_values(const char *variable_name, T &result);

		Z3_string get_symbol_string(unsigned index);

		unsigned get_num_constants() {
			return num_constants;
		}

	private:
		Z3_model model;
		Z3_context context;
		Z3_optimize opt;
		bool valid;
		unsigned num_constants;

		Z3_ast get_ast(unsigned index);
};

template <typename T>
int Z3Model::get_values(const char *variable_name, T &result) {
	if (!valid)
		return -1;

	int err;

	for (unsigned index = 0; index < get_num_constants(); ++index) {
		Z3_string name = get_symbol_string(index);

		if (strcmp(name, variable_name) != 0) {
			continue;
		}

		Z3_ast ast = get_ast(index);

		Z3_ast val;
		Z3_bool pEval = Z3_model_eval(context, model,
				ast, Z3_TRUE, &val);

		unsigned ret;
		Z3_bool p = Z3_get_numeral_uint(context, val, &ret);

		result = (T)ret;
		return 0;
	}
	return -1;
}


class Z3Handler {
	const int MAX_LEN = 4;
	const int MAX_SYMBOL_NAME = 7;

	public:
		Z3Handler();
		~Z3Handler();
		Z3_ast toAst(char *smt, size_t size);
		void PrintAst(Z3_ast ast);

		template <class T>
		int solve(Z3_ast ast, const char *variable_name,
				IntervalTree<T> &result);

		template <typename T>
		int solveEq(Z3_ast ast, const unsigned concrete,
				const char *variable_name, std::map<std::string, T> &input);

	private:
		Z3_context context;
		Z3_optimize opt;
};

template <typename T>
int Z3Handler::solveEq(Z3_ast ast, const unsigned concrete,
		const char *variable_name, std::map <std::string, T> &input) {
	int err;

	Z3_optimize_push(context, opt);
	Z3_optimize_assert(context, opt, ast);

	Z3_symbol address_symbol = Z3_mk_string_symbol(context, variable_name);
	Z3_sort dwordSort = Z3_mk_bv_sort(context, 32);
	Z3_ast address_symbol_const = Z3_mk_const(context, address_symbol, dwordSort);

	Z3_ast eq = Z3_mk_eq(context, address_symbol_const,
			Z3_mk_unsigned_int(context, concrete, dwordSort));

	Z3_optimize_assert(context, opt, eq);
	Z3_optimize_push(context, opt);

	// get all input names excepting address_symbol

	Z3Model model(context, opt);

	for (unsigned index = 0; index < model.get_num_constants(); ++index) {
		Z3_string z3_symbol_string = model.get_symbol_string(index);
		std::string symbol(z3_symbol_string);

		int input_id;
		if (sscanf(z3_symbol_string, "s[%d]", &input_id) == 0) {
			continue;
		}

		unsigned char c;
		err = model.get_values<unsigned char>(symbol.c_str(), c);

		if (err) {
			DEBUG_BREAK;
		}

		input[symbol] = c;
	}

	Z3_optimize_pop(context, opt);
	Z3_optimize_pop(context, opt);
	return err;
}

template <class T>
int Z3Handler::solve(Z3_ast ast, const char *variable_name, IntervalTree<T> &result) {

	unsigned ret;
	int err;
	T minimum, maximum;

	Z3_optimize_push(context, opt);
	Z3_optimize_assert(context, opt, ast);

	Z3_symbol address_symbol = Z3_mk_string_symbol(context, "address_symbol");
	Z3_sort dwordSort = Z3_mk_bv_sort(context, 32);
	Z3_ast address_symbol_const = Z3_mk_const(context, address_symbol, dwordSort);

	Z3_optimize_push(context, opt);

	ret = Z3_optimize_maximize(context, opt, address_symbol_const);
	// get minimum values for address and input chars

	Z3Model model_max(context, opt);
	err = model_max.get_values<T>(variable_name, maximum);
	if (err) {
		DEBUG_BREAK;
	}

	Z3_optimize_pop(context, opt);

	Z3_optimize_push(context, opt);
	ret = Z3_optimize_minimize(context, opt, address_symbol_const);

	// get maximum values for address and input chars
	Z3Model model_min(context, opt);
	err = model_min.get_values<T>(variable_name, minimum);
	if (err) {
		DEBUG_BREAK;
	}

	result.AddInterval(minimum, maximum);

	Z3_optimize_pop(context, opt);
	Z3_optimize_pop(context, opt);

	return err;
}

#endif
