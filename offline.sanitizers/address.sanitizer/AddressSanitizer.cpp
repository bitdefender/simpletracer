#include "AddressSanitizer.h"

#include <stdio.h>

AddressSanitizer::AddressSanitizer()
	: AbstractSanitizer()
{}

AddressSanitizer::~AddressSanitizer() {}

bool AddressSanitizer::Run(FILE *input) {
	return true;
}
