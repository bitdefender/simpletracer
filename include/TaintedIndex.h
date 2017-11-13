#ifndef __TAINTED_INDEX__
#define __TAINTED_INDEX__

#define MAX_FLAGS 7
#define MAX_OPERANDS 4

#include "AbstractLog.h"

class TaintedIndex {
	private:
		DWORD Index;

	public:
		TaintedIndex();
		TaintedIndex(const DWORD index);

		void NextIndex();
		DWORD GetIndex();
		void PrintIndex(AbstractFormat *aFormat);
};
#endif
