#ifndef __INTERVAL_TREE__
#define __INTERVAL_TREE__

#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#define LEFT_CHILD	0
#define RIGHT_CHILD		1

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

	T GetKey() {
		return this->low;
	}

	void Print() {}

	~Interval() {}
};

template <>
inline void Interval<int>::Print() {
	printf("(%d %d)\n", low, high);
}

template <typename T>
struct IntervalNode {
	struct Interval<T> interval;
	T max;
	struct IntervalNode<T> *left;
	struct IntervalNode<T> *right;
	size_t height;
	bool has_max;

	IntervalNode()
		: interval(), left(nullptr), right(nullptr), height(0), has_max(false) {}

	IntervalNode(T low, T high)
		: interval(low, high), max(high), left(nullptr), right(nullptr), height(1), has_max(true) {}

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
		height = 1;
	}

	void SetMax(T max) {
		this->max = max;
		has_max = true;
	}

	bool HasMax() {
		return has_max;
	}

	void PrintMax() {};

	void Print();
	void RecursiveAdd(T low, T high);
	void AssignChild(bool direction, IntervalNode<T> *child);
};

template <typename T>
void IntervalNode<T>::RecursiveAdd(T low, T high) {
	T key = low;
	T current_key = this->interval.GetKey();

	if (key <= current_key) {
		if (this->left == nullptr) {
			AssignChild(LEFT_CHILD, new IntervalNode<T>(low, high));
		} else {
			this->left->RecursiveAdd(low, high);
		}
	} else if (key > current_key) {
		if (this->right == nullptr) {
			AssignChild(RIGHT_CHILD, new IntervalNode<T>(low, high));
		} else {
			this->right->RecursiveAdd(low, high);
		}
	}
	this->height += 1;

	if (!HasMax() || max < high) {
		SetMax(high);
	}
	return;
}

template <typename T>
void IntervalNode<T>::AssignChild(bool direction, IntervalNode<T> *child) {

	assert(child != nullptr);

	if (direction == LEFT_CHILD) {
		this->left = child;
	} else if (direction == RIGHT_CHILD) {
		this->right = child;
	}
	// do rebalance
}

template <>
inline void IntervalNode<int>::PrintMax() {
	printf("max: %d; ", max);
}

template <typename T>
void IntervalNode<T>::Print() {
	printf("height: %zu; ", this->height);
	PrintMax();
	interval.Print();

	if (this->left != nullptr) {
		this->left->Print();
	}

	if (this->right != nullptr) {
		this->right->Print();
	}
}


template <class T>
class IntervalTree {
	public:
		IntervalTree();
		IntervalTree(T low, T high);
		~IntervalTree();

		bool IsEmpty();
		void AddInterval(T low, T high);
		void RemoveInterval(T low, T high);
		bool Overlaps(T low, T high);

		void PrintTree();

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
		empty = false;
		return;
	}
	root.RecursiveAdd(low, high);
}

template <typename T>
void IntervalTree<T>::PrintTree() {
	if (IsEmpty())
		return;
	root.Print();
}

#endif
