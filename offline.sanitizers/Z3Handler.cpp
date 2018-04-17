#include "Z3Handler.h"

#include <stdio.h>
#include <stdlib.h>

#include "Common.h"

Z3_context mk_context();
Z3_context mk_context_custom(Z3_config cfg, Z3_error_handler err);
Z3_optimize mk_optimize(Z3_context context);
void del_optimize(Z3_context context, Z3_optimize opt);

Z3_context mk_context() {
	Z3_config  cfg;
	cfg = Z3_mk_config();
	Z3_context ctx = mk_context_custom(cfg, nullptr);
	Z3_del_config(cfg);
	return ctx;
}

Z3_context mk_context_custom(Z3_config cfg, Z3_error_handler err) {

	Z3_set_param_value(cfg, "model", "true");
	Z3_context ctx = Z3_mk_context(cfg);

	return ctx;
}

Z3_optimize mk_optimize(Z3_context context) {
	Z3_optimize opt = Z3_mk_optimize(context);
	Z3_optimize_inc_ref(context, opt);
	return opt;
}

void del_optimize(Z3_context context, Z3_optimize opt) {
	Z3_optimize_dec_ref(context, opt);
}

Z3Handler::Z3Handler() {
	context = mk_context();
	opt = mk_optimize(context);
}

Z3Handler::~Z3Handler() {
	del_optimize(context, opt);
	Z3_del_context(context);
}

Z3_ast Z3Handler::toAst(char *smt, size_t size) {

	// max lex bitvec[32]
	Z3_symbol symbols[MAX_LEN];
	for (int i = 0; i < MAX_LEN; ++i) {
		char s[MAX_SYMBOL_NAME];
		sprintf(s, "s[%d]", i);
		symbols[i] = Z3_mk_string_symbol(context, s);
	}

	Z3_sort sorts[MAX_LEN];
	for (int i = 0; i < MAX_LEN; ++i) {
		sorts[i] = Z3_mk_bv_sort(context, 8);
	}

	Z3_set_ast_print_mode(context, (Z3_ast_print_mode)2);
	Z3_ast res = Z3_parse_smtlib2_string(context, smt,
	//		0, 0, 0, 0, 0, 0);
			MAX_LEN, symbols, sorts, 0, 0, 0);

	Z3_error_code e = Z3_get_error_code(context);
	if (e != Z3_OK) {
		return 0;
	} else {
		return res;
	}

	return 0;
}

void Z3Handler::PrintAst(Z3_ast ast) {
	Z3_set_ast_print_mode(context, (Z3_ast_print_mode)2);
	Z3_set_ast_print_mode(context, Z3_PRINT_SMTLIB2_COMPLIANT);
	PRINT("%s\n", Z3_ast_to_string(context, ast));
}
