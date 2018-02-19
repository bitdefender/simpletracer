#include "Z3SymbolicExecutor.h"

#include "CommonCrossPlatform/Common.h"

Z3SymbolicExecutor::Z3SymbolicCpuFlag::Z3SymbolicCpuFlag() {
	Unset();
}

void Z3SymbolicExecutor::Z3SymbolicCpuFlag::SetParent(Z3SymbolicExecutor *p) {
	parent = p;
}

void Z3SymbolicExecutor::Z3SymbolicCpuFlag::SetValue(Z3_ast val) {
	value = val;
}

void Z3SymbolicExecutor::Z3SymbolicCpuFlag::Unset() {
	value = nullptr;
}

Z3_ast Z3SymbolicExecutor::Z3SymbolicCpuFlag::GetValue() {
	if (value == (Z3_ast)&lazyMarker) {
		value = Eval();
	}

	return value;
}


// Zero flag - simple compare with zero
Z3_ast Z3FlagZF::Eval() {
	printf("<sym> lazyZF %p\n", source);

	return Z3_mk_ite(parent->context,
		Z3_mk_eq(
			parent->context,
			source,
			Z3_mk_int(parent->context, 0, Z3_get_sort(parent->context, source))
		),
		parent->oneFlag,
		parent->zeroFlag
	);
}

void Z3FlagZF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagZF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagZF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}


// Sign flag - extract msb
Z3_ast Z3FlagSF::Eval() {
	unsigned msb = Z3_get_bv_sort_size(parent->context,
			Z3_get_sort(parent->context, source)) - 1;
	Z3_ast res = Z3_mk_extract(
			parent->context,
			msb,
			msb,
			source
			);

	printf("<sym> lazySF: extract [%p] <= start[%d] size[%d] source[%p]\n",
			res, msb, 1, source);
	return res;
}

void Z3FlagSF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagSF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagSF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}

// Sign flag - extract msb
Z3_ast Z3FlagPF::Eval() {
	Z3_ast b4 = Z3_mk_bvxor(
		parent->context,
		source,
		Z3_mk_bvashr(
			parent->context,
			source,
			Z3_mk_int(
				parent->context,
				4,
				parent->dwordSort
			)
		)
	);

	Z3_ast b2 = Z3_mk_bvxor(
		parent->context,
		b4,
		Z3_mk_bvashr(
			parent->context,
			b4,
			Z3_mk_int(
				parent->context,
				2,
				parent->dwordSort
			)
		)
	);

	Z3_ast b1 = Z3_mk_bvxor(
		parent->context,
		b2,
		Z3_mk_bvashr(
			parent->context,
			b2,
			Z3_mk_int(
				parent->context,
				1,
				parent->dwordSort
			)
		)
	);

	printf("<sym> lazyPF %p\n", source);

	return Z3_mk_extract(
		parent->context,
		0,
		0,
		b1
	);
}

void Z3FlagPF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagPF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagPF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}

// Carry flag
// ADD: c = (a & b) | ((a | b) & ~r)
// SUB: c = ~((a & ~b) | ((a | ~b) & ~r))
// ADC: ?
// SBB: ?
// MUL: ?
// IMUL: ?
// CMP: c = ~((a & ~b) | ((a | ~b) & ~r))
// ??
Z3_ast Z3FlagCF::Eval() {
	switch (func) {
		case Z3_FLAG_OP_ADD:
		case Z3_FLAG_OP_SUB:
		case Z3_FLAG_OP_SBB:
		case Z3_FLAG_OP_CMP:
			break;
		default :
			DEBUG_BREAK;
	}


	unsigned sourceSize = Z3_get_bv_sort_size(parent->context,
			Z3_get_sort(parent->context, source));
	Z3_ast nr = Z3_mk_bvnot(
		parent->context,
		Z3_mk_extract(
			parent->context,
			sourceSize - 1,
			sourceSize - 1,
			source
		)
	);

	unsigned aSize = Z3_get_bv_sort_size(parent->context,
			Z3_get_sort(parent->context, p[0]));
	Z3_ast a = Z3_mk_extract(
		parent->context,
		aSize - 1,
		aSize - 1,
		p[0]
	);

	unsigned bSize = Z3_get_bv_sort_size(parent->context,
			Z3_get_sort(parent->context, p[1]));
	Z3_ast b = Z3_mk_extract(
		parent->context,
		bSize - 1,
		bSize - 1,
		p[1]
	);

	if (func == Z3_FLAG_OP_SUB || func == Z3_FLAG_OP_CMP) {
		b = Z3_mk_bvnot(
			parent->context,
			b
		);
	}

	Z3_ast c = Z3_mk_bvor(
		parent->context,
		Z3_mk_bvand(
			parent->context,
			a,
			b
		),
		Z3_mk_bvand(
			parent->context,
			nr,
			Z3_mk_bvor(
				parent->context,
				a,
				b
			)
		)
	);

	if (func == Z3_FLAG_OP_SUB || func == Z3_FLAG_OP_CMP) {
		c = Z3_mk_bvnot(
			parent->context,
			c
		);
	}

	// for sbb, verify result with zero. If zero, cf if previous cf
	if (func == Z3_FLAG_OP_SBB) {
		Z3_ast cond = Z3_mk_eq(parent->context, p[0], p[1]);
		c = Z3_mk_ite(parent->context, cond, cf, c);
	}

	printf("<sym> lazyCF %p <= %p, %p\n", source, p[0], p[1]);

	return c;
}

void Z3FlagCF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	p[0] = (Z3_ast)ops.svBefore[0];
	p[1] = (Z3_ast)ops.svBefore[1];
	cf = (Z3_ast)ops.svfBefore[RIVER_SPEC_IDX_CF];
	func = op;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagCF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
	stack.Push((DWORD)p[0]);
	stack.Push((DWORD)p[1]);
	stack.Push((DWORD)func);
}

void Z3FlagCF::LoadState(stk::LargeStack &stack) {
	func = (unsigned int)stack.Pop();
	p[1] = (Z3_ast)stack.Pop();
	p[0] = (Z3_ast)stack.Pop();
	source = (Z3_ast)stack.Pop();
}



// Overflow flag
// ADD: c = (a ^ r) & (b ^ r)
// SUB: c = (a ^ r) & (~b ^ r)
// ADC: ?
// SBB: ?
// MUL: ?
// IMUL: ?
// AND : c = 0
// ??
Z3_ast Z3FlagOF::Eval() {
	switch (func) {
		case Z3_FLAG_OP_ADD:
		case Z3_FLAG_OP_SUB:
		case Z3_FLAG_OP_CMP:
			break;
		case Z3_FLAG_OP_AND:
			return parent->zeroFlag;
		default:
			DEBUG_BREAK;
	}

	unsigned resSize = Z3_get_bv_sort_size(parent->context,
			Z3_get_sort(parent->context, source));
	unsigned opSize = Z3_get_bv_sort_size(parent->context,
			Z3_get_sort(parent->context, p[0]));

	if (opSize != resSize) DEBUG_BREAK;

	unsigned msb = resSize - 1;

	Z3_ast r = Z3_mk_extract(
		parent->context,
		msb,
		msb,
		source
	);

	Z3_ast a = Z3_mk_extract(
		parent->context,
		msb,
		msb,
		p[0]
	);

	Z3_ast b = Z3_mk_extract(
		parent->context,
		msb,
		msb,
		p[1]
	);

	if ((func == Z3_FLAG_OP_SUB) || (func == Z3_FLAG_OP_CMP)) {
		b = Z3_mk_bvnot(
			parent->context,
			b
		);
	}

	Z3_ast res = Z3_mk_bvand(
			parent->context,
			Z3_mk_bvxor(
				parent->context,
				a,
				r
				),
			Z3_mk_bvxor(
				parent->context,
				b,
				r
				)
	);
	printf("<sym> lazyOF %p <= source[%p], p0[%p] p1[%p]\n", res, source, p[0], p[1]);
	return res;
}

void Z3FlagOF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	p[0] = (Z3_ast)ops.svBefore[0];
	p[1] = (Z3_ast)ops.svBefore[1];
	func = op;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagOF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
	stack.Push((DWORD)p[0]);
	stack.Push((DWORD)p[1]);
	stack.Push((DWORD)func);
}

void Z3FlagOF::LoadState(stk::LargeStack &stack) {
	func = (unsigned int)stack.Pop();
	p[1] = (Z3_ast)stack.Pop();
	p[0] = (Z3_ast)stack.Pop();
	source = (Z3_ast)stack.Pop();
}

// Adjust flag
Z3_ast Z3FlagAF::Eval() {
	DEBUG_BREAK;
}

void Z3FlagAF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagAF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagAF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}

// Direction flag
Z3_ast Z3FlagDF::Eval() {
	DEBUG_BREAK;
}

void Z3FlagDF::SetSource(struct SymbolicOperandsLazyFlags &ops, unsigned int op) {
	source = (Z3_ast)ops.svAfter[0];
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagDF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagDF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}
