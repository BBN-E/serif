#ifndef _BOUNDED_PRIORITY_QUEUE_H_
#define _BOUNDED_PRIORITY_QUEUE_H_

#include "MinMaxHeap.h"

// this is a max-priority-queue with respect to comparator C
// comparator should return -1 if less than,
// 0 if equal, and 1 if greater than
template<typename T, typename Comp>
class BoundedPriorityQueue {
public:
	BoundedPriorityQueue(size_t max_items);

	void push(const T& t);
	const T& peek() const;
	T pop();
	void clear();

	size_t size() const;
	bool empty() const;
private:
	MinMaxHeap<T,Comp> _heap;
	size_t _max_items;
	Comp _comp;
};

template<typename T, typename C>
BoundedPriorityQueue<T,C>::BoundedPriorityQueue(size_t max_items) 
: _heap(), _max_items(max_items), _comp() {}

template<typename T, typename C>
size_t BoundedPriorityQueue<T,C>::size() const {
	return _heap.size();
}

template<typename T, typename C>
bool BoundedPriorityQueue<T,C>::empty() const {
	return _heap.empty();
}

template<typename T, typename C>
void BoundedPriorityQueue<T,C>::clear() {
	_heap.clear();
}

template<typename T, typename C>
T BoundedPriorityQueue<T,C>::pop() {
	return _heap.popMax();
}

template<typename T, typename C>
const T& BoundedPriorityQueue<T,C>::peek() const {
	return _heap.peekMax();
}

template<typename T, typename C>
void BoundedPriorityQueue<T,C>::push(const T& t) {
	if (_heap.size() < _max_items) {
		_heap.insert(t);
	} else {
		if (_comp(t, _heap.peekMin()) == 1) {
			_heap.deleteMin();
			_heap.insert(t);
		}
	}
}
#endif
