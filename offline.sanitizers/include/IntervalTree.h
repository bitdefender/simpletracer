#ifndef __INTERVAL_TREE__
#define __INTERVAL_TREE__

#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include <algorithm>

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

	T GetHigh() {
		return this->high;
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

	void UpdateHeight() {
		if (this->left == nullptr && this->right == nullptr) {
			height = 1;
			return;
		}

		if (this->left == nullptr) {
			height = 1 + this->right->GetHeight();
		} else if (this->right == nullptr) {
			height = 1 + this->left->GetHeight();
		} else {
			height = 1 + std::max(this->left->GetHeight(),
					this->right->GetHeight());
		}
	}

	void SetMax(T max) {
		this->max = max;
		has_max = true;
	}

	bool HasMax() {
		return has_max;
	}

	void PrintMax() {};

	size_t GetHeight() {
		return height;
	}

	T GetKey() {
		return this->interval.GetKey();
	}

	int GetBalanceFactor() {
		int left_height = left == nullptr ? 0 : left->GetHeight();
		int right_height = right == nullptr ? 0 : right->GetHeight();

		return left_height - right_height;
	}


	void UpdateMax() {
		max = std::max(this->interval.GetHigh(), this->right->max);
		max = std::max(max, this->left->max);
	}

	struct IntervalNode<T> *LeftMost() {
		if (this->left != nullptr)
			return this->left->LeftMost();

		return this;
	}

	void Print();
	struct IntervalNode<T> *LeftRotate();
	struct IntervalNode<T> *RightRotate();
	struct IntervalNode<T> *RecursiveAdd(T low, T high);
	struct IntervalNode<T> *RecursiveDelete(T low, T high);
	void AssignChild(bool direction, IntervalNode<T> *child);
};

// left rotate subtree rooted with this
template <typename T>
struct IntervalNode<T> *IntervalNode<T>::LeftRotate() {
	struct IntervalNode<T> *x = this;
	struct IntervalNode<T> *y = x->right;

	struct IntervalNode<T> *T2 = y->left;

	//rotate
	y->left = x;
	x->right = T2;

	//update heights
	x->UpdateHeight();
	y->UpdateHeight();

	//update max
	x->UpdateMax();
	y->UpdateMax();

	//return new root
	return y;
}

template <typename T>
struct IntervalNode<T> *IntervalNode<T>::RightRotate() {
	struct IntervalNode<T> *z = this;
	struct IntervalNode<T> *y = z->left;

	struct IntervalNode<T> *T3 = y->right;

	//rotate
	y->right = z;
	z->right = T3;

	//update heights
	z->UpdateHeight();
	y->UpdateHeight();

	//update max
	z->UpdateMax();
	y->UpdateMax();

	//return new root
	return y;
}

template <typename T>
struct IntervalNode<T> *IntervalNode<T>::RecursiveAdd(T low, T high) {
	T key = low;
	T current_key = GetKey();

	if (key < current_key) {
		if (this->left == nullptr) {
			AssignChild(LEFT_CHILD, new IntervalNode<T>(low, high));
		} else {
			this->left = this->left->RecursiveAdd(low, high);
		}
	} else if (key > current_key) {
		if (this->right == nullptr) {
			AssignChild(RIGHT_CHILD, new IntervalNode<T>(low, high));
		} else {
			this->right = this->right->RecursiveAdd(low, high);
		}
	} else {
		return this;
	}

	// set max
	if (!HasMax() || max < high) {
		SetMax(high);
	}

	// increase height
	UpdateHeight();

	// compute balance factor
	int balance_factor = this->GetBalanceFactor();

	// left left or left right
	if (balance_factor > 1) {
		if (this->left != nullptr) {
			T left_key = this->left->GetKey();
			if (key < left_key) {
				// left left
				return RightRotate();
			} else if (key > left_key) {
				// left right
				this->left = this->left->LeftRotate();
				return RightRotate();
			}
		}
	} else if (balance_factor < -1) {
		// right right or right left
		if (this->right != nullptr) {
			T right_key = this->right->GetKey();
			if (key > right_key) {
				// right right
				return LeftRotate();
			} else if (key < right_key) {
				// right left
				this->right = this->right->RightRotate();
				return LeftRotate();
			}
		}
	} else {
		// balanced
	}

	return this;
}

template <typename T>
IntervalNode<T> *IntervalNode<T>::RecursiveDelete(T low, T high) {
	T key = low;
	T current_key = this->interval.GetKey();

	if (key < current_key) {
		if (this->left == nullptr)
			return nullptr;

		auto tmp = this->left;
		this->left = this->left->RecursiveDelete(low, high);

		if (this->left != tmp) {
			tmp->left = nullptr;
			tmp->right = nullptr;
			delete tmp;
		}
	} else if (key > current_key) {
		if (this->right == nullptr)
			return nullptr;

		auto tmp = this->right;
		this->right = this->right->RecursiveDelete(low, high);

		if (this->right != tmp) {
			tmp->left = nullptr;
			tmp->right = nullptr;
			delete tmp;
		}
	} else if (key == current_key) {
		T current_high = this->interval.GetHigh();
		if (high != current_high) {
			return this;
		}

		if (this->left == nullptr) {
			return this->right;
		} else if (this->right == nullptr) {
			return this->left;
		}

		// get leftmost in right subtree
		auto leftmost = this->right->LeftMost();
		this->interval = leftmost->interval;
		this->right = this->right->RecursiveDelete(this->interval.low,
				this->interval.high);
	}

	this->UpdateHeight();
	return this;
}

template <typename T>
void IntervalNode<T>::AssignChild(bool direction, IntervalNode<T> *child) {

	assert(child != nullptr);

	if (direction == LEFT_CHILD) {
		this->left = child;
	} else if (direction == RIGHT_CHILD) {
		this->right = child;
	}
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
		struct IntervalNode<T> *root;
		bool empty;

};

template <typename T>
IntervalTree<T>::IntervalTree()
	: root(nullptr) {}

template <typename T>
IntervalTree<T>::IntervalTree(T low, T high)
{
	root = new IntervalNode<T>(low, high);
}

template <typename T>
IntervalTree<T>::~IntervalTree() {
	if (!IsEmpty()) {
		delete root;
	}
}

template <typename T>
bool IntervalTree<T>::IsEmpty() {
	return root == nullptr;
}

template <typename T>
void IntervalTree<T>::AddInterval(T low, T high) {
	if (IsEmpty()) {
		root = new IntervalNode<T>(low, high);
		return;
	}
	root = root->RecursiveAdd(low, high);
}

template <typename T>
void IntervalTree<T>::RemoveInterval(T low, T high) {
	if (IsEmpty())
		return;

	root = root->RecursiveDelete(low, high);
}

template <typename T>
void IntervalTree<T>::PrintTree() {
	if (IsEmpty())
		return;
	root->Print();
	printf("===================================\n");
}

#endif
