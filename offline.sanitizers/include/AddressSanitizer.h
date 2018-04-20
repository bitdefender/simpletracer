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

		template <typename T>
		void LogCrashingInput(const std::map<std::string, T> &input);

	private:
		Z3Handler *z3Handler;
};

template <typename T>
void AddressSanitizer::LogCrashingInput(const std::map<std::string, T> &input) {
	if (!input.size())
		return;

	printf("Valid input: ");
	for (auto i : input) {
		printf("%s <= 0x%02X; ", i.first.c_str(), i.second);
	}
	printf("\n");
}
#endif
