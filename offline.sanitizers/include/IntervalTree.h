#ifndef __INTERVAL_TREE__
#define __INTERVAL_TREE__

template <typename T>
struct Interval {
	T low;
	T high;

	Interval() {}

	Interval(T low, T high) {
		this->low = low;
		this->high = high;
	}

	void Set(T low, T high) {
		this->low = low;
		this->high = high;
	}

	~Interval(){}
};

template <typename T>
struct IntervalNode {
	struct Interval<T> interval;
	T max;
	struct IntervalNode<T> *left;
	struct IntervalNode<T> *right;

	IntervalNode()
		: interval(), left(nullptr), right(nullptr) {}

	IntervalNode(T low, T high)
		: interval(low, high), max(high), left(nullptr), right(nullptr) {}

	~IntervalNode() {
		if (left != nullptr)
			delete left;
		if (right != nullptr)
			delete right;
	}

	bool IsLeaf() {
		return left != nullptr || right != nullptr;
	}

	void SetInterval(T low, T high) {
		interval.Set(low, high);
	}

	void SetMax(T max) {
		this->max = max;
	}
};

template <class T>
class IntervalTree {
	public:
		IntervalTree();
		IntervalTree(T low, T high);
		~IntervalTree();

		bool IsEmpty();
		void AddInterval(T low, T high);

	private:
		struct IntervalNode<T> root;
		bool empty;

};

template <typename T>
IntervalTree<T>::IntervalTree()
	: empty(true) {}

template <typename T>
IntervalTree<T>::IntervalTree(T low, T high)
	: root(low, high), empty(false) {}

template <typename T>
IntervalTree<T>::~IntervalTree() {}

template <typename T>
bool IntervalTree<T>::IsEmpty() {
	return empty;
}

template <typename T>
void IntervalTree<T>::AddInterval(T low, T high) {
	if (IsEmpty()) {
		root.SetInterval(low, high);
		return;
	}

	//TODO traverse and add
}

#endif
