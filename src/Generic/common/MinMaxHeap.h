#ifndef _MIN_MAX_HEAP_
#define _MIN_MAX_HEAP_

/* Implementation of Min-Max Heap from

M.D. Atkinson, J.-R. Sack, N. Santoro, and T. Strothotte, "Min-Max Heaps and
Generalized Priority Queues." 1986. Communications of the ACM.

A structure with O(log n) insertion and deletion and O(1) access to the smallest
*and* largest elements.  Basically a more complicated binary heap where the layers 
alternate between having the max-heap property and the min-heap property with
respect to their children.

This particular implementation is designed to be easy to write and understand.
It could be made faster, if necessary.
*/

#include <vector>
#include <algorithm>
#include "UnrecoverableException.h"

// Compare returns -1 if less than, 0 if equal, 1 if greater
template <typename T, typename Comp>
class MinMaxHeap {
public:
	MinMaxHeap();
	
	void insert(const T& t);

	T popMax();
	T popMin();
	
	const T& peekMax() const;
	const T& peekMin() const;
	
	void deleteMin();
	void deleteMax();
	
	bool empty() const;
	void clear();

	size_t size() const;

	/*
	debugging tools
	void dumpHeap() const;
	void MinMaxHeap<T,Comp>::checkHeap() const;*/

private:
	std::vector<T> _heap;
	Comp _comp;

	/* core implementation */
	void trickleDown(size_t pos);
	void trickleDownMin(size_t pos);
	void trickleDownMax(size_t pos);

	void bubbleUp(size_t pos);
	void bubbleUpMin(size_t pos);
	void bubbleUpMax(size_t pos);
	
	/* utility methods core core implementation */
	size_t minIdx() const;

	bool less_than(size_t a, size_t b) const;
	bool greater_than(size_t a, size_t b) const;
	void swap(size_t a, size_t b);
	
	size_t lastNode() const;
	size_t parent(size_t pos) const;
	size_t grandparent(size_t pos) const;
	bool hasChild(size_t pos) const;
	size_t firstChild(size_t pos) const;
	size_t secondChild(size_t pos) const;
	bool hasParent(size_t pos) const;
	bool hasGrandparent(size_t pos) const;
	size_t firstGrandchild(size_t pos) const;
	size_t lastGrandchild(size_t pos) const;
	bool isGrandchildOf(size_t pos, size_t grandparent) const;
	size_t maxChildOrGrandchildOf(size_t pos) const;
	size_t minChildOrGrandchildOf(size_t pos) const;

	bool atMinLevel(size_t pos) const;
	size_t level(size_t pos) const;
};

namespace MinMaxHeapTest {
	void testMinMaxHeap();
};

template <typename T, typename Comp>
MinMaxHeap<T,Comp>::MinMaxHeap() : _heap(), _comp()
{
}

template <typename T, typename Comp>
void MinMaxHeap<T,Comp>::trickleDown(size_t pos) {
	if (atMinLevel(pos)) {
		trickleDownMin(pos);
	} else {
		trickleDownMax(pos);
	}
}

template <typename T, typename Comp>
void MinMaxHeap<T,Comp>::trickleDownMin(size_t pos) {
	if (hasChild(pos)) {
		size_t m = minChildOrGrandchildOf(pos);
		if (isGrandchildOf(m, pos)) {
			if (less_than(m, pos)) {
				swap(m, pos);
				if (greater_than(m, parent(m))) {
					swap(m, parent(m));
				}
				trickleDownMin(m);
			}
		} else {
			if (less_than(m, pos)) {
				swap(pos, m);
			}
		}
	}
}

template <typename T, typename Comp>
bool MinMaxHeap<T,Comp>::hasChild(size_t pos) const {
	return firstChild(pos) <= lastNode();
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::firstChild(size_t pos) const {
	return 2*pos + 1;
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::secondChild(size_t pos) const {
	return 2*pos+2;
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::firstGrandchild(size_t pos) const {
	return 4*pos + 3;
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::lastGrandchild(size_t pos) const {
	return 4*pos + 6;
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::isGrandchildOf(size_t pos, size_t grandparent) const {
	return (pos >= firstGrandchild(grandparent)) && (pos <= lastGrandchild(grandparent));
}

// assumes we have at least one child
template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::maxChildOrGrandchildOf(size_t pos) const {
	size_t limit = (std::min)(secondChild(pos), lastNode());
	size_t node = firstChild(pos);
	size_t best_node = node;

	for (++node; node <= limit; ++node) {
		if (greater_than(node, best_node)) {
			best_node = node;
		}
	}

	
	limit = (std::min)(lastGrandchild(pos), lastNode());
	for (node = firstGrandchild(pos); node <= limit; ++node) {
		if (greater_than(node, best_node)) {
			best_node = node;
		}
	}
	
	return best_node;
}

// assumes we have at least one child
template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::minChildOrGrandchildOf(size_t pos) const {
	size_t limit = (std::min)(secondChild(pos), lastNode());
	size_t node = firstChild(pos);
	size_t best_node = node;

	for (++node; node <= limit; ++node) {
		if (less_than(node, best_node)) {
			best_node = node;
		}
	}

	
	limit = (std::min)(lastGrandchild(pos), lastNode());
	for (node = firstGrandchild(pos); node <= limit; ++node) {
		if (less_than(node, best_node)) {
			best_node = node;
		}
	}
	
	return best_node;
}


// only call if we know heap to be non-empty
template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::lastNode() const {
	return _heap.size() - 1;
}

template <typename T, typename Comp>
void MinMaxHeap<T,Comp>::trickleDownMax(size_t pos) {
	if (hasChild(pos)) {
		size_t m = maxChildOrGrandchildOf(pos);
		if (isGrandchildOf(m, pos)) {
			if (greater_than(m, pos)) {
				swap(m, pos);
				if (less_than(m, parent(m))) {
					swap(m, parent(m));
				}
				trickleDownMax(m);
			}
		} else {
			if (greater_than(m, pos)) {
				swap(pos, m);
			}
		}
	}
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::bubbleUp(size_t pos) {
	if (atMinLevel(pos)) {
		if (hasParent(pos) && greater_than(pos, parent(pos))) {
			swap(pos, parent(pos));
			bubbleUpMax(parent(pos));
		} else {
			bubbleUpMin(pos);
		}
	} else {
		if (hasParent(pos) && less_than(pos, parent(pos))) {
			swap(pos, parent(pos));
			bubbleUpMin(parent(pos));
		} else {
			bubbleUpMax(pos);
		}
	}
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::bubbleUpMin(size_t pos) {
	if (hasGrandparent(pos)) {
		if (less_than(pos, grandparent(pos))) {
			swap(pos, grandparent(pos));
			bubbleUpMin(grandparent(pos));
		}
	}
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::bubbleUpMax(size_t pos) {
	if (hasGrandparent(pos)) {
		if (greater_than(pos, grandparent(pos))) {
			swap(pos, grandparent(pos));
			bubbleUpMax(grandparent(pos));
		}
	}
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::insert(const T& t) {
	size_t pos = lastNode() + 1;
	_heap.push_back(t);
	bubbleUp(pos);
}

template<typename T, typename Comp>
const T& MinMaxHeap<T,Comp>::peekMax() const {
	if (!_heap.empty()) {
		return _heap[0];
	} else {
		throw UnrecoverableException("MinMaxHeap::peekMax", "Heap is empty");
	}
}

template<typename T, typename Comp>
const T& MinMaxHeap<T,Comp>::peekMin() const {
	return _heap[minIdx()];
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::deleteMax() {
	if (_heap.size() > 0) {
		if (_heap.size() == 1) {
			_heap.resize(0);
		} else {
			_heap[0] = _heap[lastNode()];
			_heap.resize(_heap.size()-1);
			trickleDown(0);
		}
	}
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::minIdx() const {
	switch (_heap.size()) {
		case 0:
			throw UnrecoverableException("MinMaxHeap::peekMin", "Heap is empty");
		case 1:
			return 0;
		case 2:
			return 1;
		default:
			if (_heap[1] < _heap[2]) {
				return 1;
			} else {
				return 2;
			}
	}
}


template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::deleteMin() {
	if (_heap.size() > 0) {
		size_t min_idx = minIdx();

		if (min_idx == 0) {
			_heap.resize(0);
		} else {
			_heap[min_idx] = _heap[lastNode()];
			_heap.resize(_heap.size()-1);
			trickleDown(min_idx);
		}
	}
}

template<typename T, typename Comp>
T MinMaxHeap<T,Comp>::popMax() {
	T ret = peekMax();
	deleteMax();
	return ret;
}

template<typename T, typename Comp>
T MinMaxHeap<T,Comp>::popMin() {
	T ret = peekMin();
	deleteMin();
	return ret;
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::less_than(size_t a, size_t b) const {
	return _comp(_heap[a], _heap[b]) == -1;
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::greater_than(size_t a, size_t b) const {
	return _comp(_heap[a], _heap[b]) == 1;
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::swap(size_t a, size_t b) {
	//std::cout << "Swapping positions " << a << " and " << b << std::endl;
	(std::swap)(_heap[a], _heap[b]);
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::parent(size_t pos) const {
	if (!hasParent(pos)) {
		throw UnrecoverableException("MinMaxHeap::parent", "Root has no parent");
	} else {
		return (pos-1)/2;
	}
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::grandparent(size_t pos) const {
	return parent(parent(pos));
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::hasParent(size_t pos) const {
	return pos!=0;
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::hasGrandparent(size_t pos) const {
	return pos > 2;
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::atMinLevel(size_t pos) const {
	return level(pos) % 2 == 1;
}

// this can be done significantly faster at the cost of obscurity if 
// this ends up being a bottleneck
template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::level(size_t pos) const {
	++pos;
	size_t r = 0;
	while (pos >>=1) {
		++r;
	}
	return r;
}

template<typename T, typename Comp>
bool MinMaxHeap<T,Comp>::empty() const {
	return _heap.empty();
}

template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::clear() {
	_heap.clear();
}

template<typename T, typename Comp>
size_t MinMaxHeap<T,Comp>::size() const {
	return _heap.size();
}

/*template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::checkHeap() const {
	for (size_t i=0; i<_heap.size(); ++i) {
		if (atMinLevel(i)) {
			if (hasChild(i)) {
				if (!less_than(i, minChildOrGrandchildOf(i))) {
					std::cout << "Violation: " << _heap[i] << " and " << _heap[minChildOrGrandchildOf(i)] << std::endl;
				}
			}
		} else {
			if (hasChild(i)) {
				if (!greater_than(i, maxChildOrGrandchildOf(i))) {
					std::cout << "Violation: " << _heap[i] << " and " << _heap[maxChildOrGrandchildOf(i)] << std::endl;
				}
			}
		}
	}
}*/

/*template<typename T, typename Comp>
void MinMaxHeap<T,Comp>::dumpHeap() const {
	for (size_t i=0; i<_heap.size(); ++i) {
		std::cout << _heap[i] << " ";
	} 
	std::cout << std::endl;
}*/
#endif
