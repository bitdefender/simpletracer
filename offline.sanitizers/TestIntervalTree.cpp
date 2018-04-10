#include "IntervalTree.h"

int main() {
	IntervalTree<int> it(0, 0);
	IntervalTree<int> it_empty;

	it_empty.AddInterval(0, 10);

	return 0;
}
