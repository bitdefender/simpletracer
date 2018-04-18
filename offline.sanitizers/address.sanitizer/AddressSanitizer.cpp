#include "AddressSanitizer.h"

#include <stdio.h>
#include "IntervalTree.h"

AddressSanitizer::AddressSanitizer()
	: AbstractSanitizer(), z3Handler(nullptr)
{}

AddressSanitizer::~AddressSanitizer() {}

void AddressSanitizer::setZ3Handler(Z3Handler *z3Handler) {
	if (this->z3Handler != nullptr)
		return;

	this->z3Handler = z3Handler;
}

void AddressSanitizer::LogBacktrace(const struct AddressAssertion &addrAssertion) {

}

void AddressSanitizer::LogSymbolicReturnAddress(const struct AddressAssertion &addrAssertion) {
	printf("Error type: symbolic return address\n");
	printf("Module name: %s + 0x%08X\n",
			addrAssertion.basicBlock.current.module,
			addrAssertion.basicBlock.current.offset);
	printf("Address operation: %s\n",
			(addrAssertion.input && addrAssertion.output) ? "input/output" :
			((addrAssertion.input) ? "input" : "output"));
	printf("Stack address: 0x%08X\n",
			addrAssertion.basicBlock.assertionData.asAddress.esp);
	printf("Symbolic Address: 0x%08X <= 0x%08X + 0x%08X x 0x%08X + 0x%08X\n",
			addrAssertion.composedAddress,
			addrAssertion.symbolicBase,
			addrAssertion.symbolicIndex,
			addrAssertion.scale,
			addrAssertion.displacement);
}

bool AddressSanitizer::sanitize(const struct AddressAssertion &addrAssertion) {
	if (z3Handler == nullptr)
		return false;
	IntervalTree<unsigned> address_space;
	//const unsigned esp = addrAssertion.basicBlock.assertionData.asAddress.esp;
	bool err;

	err = z3Handler->solve<unsigned>(
			addrAssertion.symbolicAddress, "address_symbol", address_space);
	const unsigned esp = 0x11;

	if (address_space.HasValue(esp)) {
		bool valid_address = true;
		unsigned char input[10];
		bool err = z3Handler->solveEq<unsigned char>(
				addrAssertion.symbolicAddress, esp,
				"address_symbol", input, valid_address);
		if (!valid_address) {
			LogSymbolicReturnAddress(addrAssertion);
			LogBacktrace(addrAssertion);
		}
	}
	printf("Checked esp: 0x%08X\nInterval tree: ", esp);
	address_space.PrintTree();
}
