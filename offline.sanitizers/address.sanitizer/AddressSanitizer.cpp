#include "AddressSanitizer.h"

#include <stdio.h>

AddressSanitizer::AddressSanitizer()
	: AbstractSanitizer(), z3Handler(nullptr)
{}

AddressSanitizer::~AddressSanitizer() {}

void AddressSanitizer::setZ3Handler(Z3Handler *z3Handler) {
	if (this->z3Handler != nullptr)
		return;

	this->z3Handler = z3Handler;
}

bool AddressSanitizer::sanitize(const struct AddressAssertion &addrAssertion) {
	if (z3Handler == nullptr)
		return false;
	return z3Handler->solve(addrAssertion.symbolicAddress,
			addrAssertion.basicBlock.assertionData.asAddress.esp);
}
