#ifndef __ADDRESS_SAN__
#define __ADDRESS_SAN__

#include "AbstractSanitizer.h"

class AddressSanitizer : public AbstractSanitizer {
	public:
		AddressSanitizer();
		~AddressSanitizer();

		void setZ3Handler(Z3Handler *z3Handler);
		bool sanitize(const struct AddressAssertion &addrAssertion);
		void LogBacktrace(const struct AddressAssertion &addrAssertion);
		void LogSymbolicReturnAddress(const struct AddressAssertion &addrAssertion);

	private:
		Z3Handler *z3Handler;
};
#endif
