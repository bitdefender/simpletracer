#ifndef __ABSTRACT_SAN__
#define __ABSTRACT_SAN__

#include <stdio.h>

class AbstractSanitizer {
	public:
		AbstractSanitizer() {}
		virtual ~AbstractSanitizer() {}

		virtual bool Run(FILE *input) = 0;
};
#endif
