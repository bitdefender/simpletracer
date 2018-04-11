#include "IntervalTree.h"

int main() {
	IntervalTree<int> it_empty;

	it_empty.AddInterval(4, 10);
	it_empty.AddInterval(1, 6);
	it_empty.AddInterval(12, 13);
	it_empty.AddInterval(3, 5);

	it_empty.PrintTree();

	return 0;
}
