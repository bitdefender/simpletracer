#ifndef __Z3_HANDLER__
#define __Z3_HANDLER__

#include "z3.h"
#include "IntervalTree.h"
#include "CommonCrossPlatform/Common.h"

#include <string.h>

class Z3Handler {
	const int MAX_LEN = 4;
	const int MAX_SYMBOL_NAME = 7;

	public:
		Z3Handler();
		~Z3Handler();
		Z3_ast toAst(char *smt, size_t size);
		void PrintAst(Z3_ast ast);

		template <class T>
		bool solve(Z3_ast ast, const char *variable_name, IntervalTree<T> &result);

	private:
		Z3_context context;
		Z3_optimize opt;

		template <typename T>
		bool check_opt_and_get_values(const char *variable_name, T &result);

		template <typename T>
		bool get_constants(Z3_model model, const char *variable_name, T &result);
};

template <class T>
bool Z3Handler::solve(Z3_ast ast, const char *variable_name, IntervalTree<T> &result) {

	unsigned ret;
	bool err;
	T minimum, maximum;

	Z3_optimize_push(context, opt);
	Z3_optimize_assert(context, opt, ast);

	Z3_symbol address_symbol = Z3_mk_string_symbol(context, "address_symbol");
	Z3_sort dwordSort = Z3_mk_bv_sort(context, 32);
	Z3_ast address_symbol_const = Z3_mk_const(context, address_symbol, dwordSort);

	Z3_optimize_push(context, opt);

	ret = Z3_optimize_maximize(context, opt, address_symbol_const);
	// get minimum values for address and input chars
	err = check_opt_and_get_values<T>(variable_name, maximum);
	if (err) {
		DEBUG_BREAK;
	}

	Z3_optimize_pop(context, opt);

	Z3_optimize_push(context, opt);
	ret = Z3_optimize_minimize(context, opt, address_symbol_const);
	// get maximum values for address and input chars
	err = check_opt_and_get_values<T>(variable_name, minimum);
	if (err) {
		DEBUG_BREAK;
	}

	result.AddInterval(minimum, maximum);

	Z3_optimize_pop(context, opt);
	Z3_optimize_pop(context, opt);
}

template <typename T>
bool Z3Handler::get_constants(Z3_model model, const char *variable_name, T &result) {
	unsigned int cnt = Z3_model_get_num_consts(context, model);

	for (unsigned int i = 0; i < cnt; ++i) {
		Z3_func_decl c = Z3_model_get_const_decl(context, model, i);

		Z3_ast ast = Z3_model_get_const_interp(context, model, c);
		Z3_symbol name = Z3_get_decl_name(context, c);
		Z3_string r = Z3_get_symbol_string(context, name);

		if (strcmp(r, variable_name) != 0) {
			continue;
		}

		Z3_ast val;
		Z3_bool pEval = Z3_model_eval(context, model, ast, Z3_TRUE, &val);

		unsigned ret;
		Z3_bool p = Z3_get_numeral_uint(context, val, &ret);

		result = (T)ret;
		return false;
	}

	return true;
}

template <typename T>
bool Z3Handler::check_opt_and_get_values(const char *variable_name, T &result) {
	Z3_model model;
	bool err;
	switch (Z3_optimize_check(context, opt)) {
		case Z3_L_FALSE:
			break;
		case Z3_L_UNDEF:
			break;
		case Z3_L_TRUE:
			model = Z3_optimize_get_model(context, opt);
			err = get_constants<T>(model, variable_name, result);
			return err;
		default:
			break;
	}
	return true;
}

#endif
