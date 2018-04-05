#ifndef __ABSTRACT_SAN__
#define __ABSTRACT_SAN__

#include <stdio.h>

#include "Z3Handler.h"
#include "TraceParser.h"

class AbstractSanitizer {
	public:
		AbstractSanitizer() {}
		virtual ~AbstractSanitizer() {}
};
#endif
