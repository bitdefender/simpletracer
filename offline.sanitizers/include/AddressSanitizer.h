#ifndef __ADDRESS_SAN__
#define __ADDRESS_SAN__

#include "AbstractSanitizer.h"

class AddressSanitizer : public AbstractSanitizer {
	public:
		AddressSanitizer();
		~AddressSanitizer();
};
#endif
