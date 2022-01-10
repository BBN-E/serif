// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef QUEUE_H
#define QUEUE_H

// brain-dead queue class. 

template <class T>
class SimpleQueue {
private:
	struct ListNode {
		T data;
		ListNode *prev, *next;
	};
	
	ListNode *_head, *_tail;

public:
	SimpleQueue() {
		_head = _tail = NULL;
	}

	~SimpleQueue() {
		ListNode *iterator = _head;
		while(_head != NULL) {
			_head = iterator->next;
			delete iterator;
			iterator = _head;
		}
	}

	bool isEmpty() { return _head == NULL; }

	void add(T newT) {
		ListNode *newNode = _new ListNode;
		newNode->data = newT;
		newNode->prev = _tail;
		newNode->next = NULL;
		if(_tail != NULL)
			_tail->next = newNode;
		else
			_head = newNode;
		_tail = newNode;
	}

	T remove() {
		T result;
		if(!isEmpty()) {
			ListNode *oldHead = _head;
			if(_head == _tail) 
				_tail = NULL;
			else _head->next->prev = NULL;
			_head = _head->next;
			result = oldHead->data;
			delete oldHead;
		}
		return result;
	}
};

#endif
