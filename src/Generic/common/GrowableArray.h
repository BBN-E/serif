// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GROWABLEARRAY_H
#define GROWABLEARRAY_H

#include "Generic/common/InternalInconsistencyException.h"
template <class T>
class GrowableArray {
public:
	
	GrowableArray(int capacity = 16): _defaultValue(T()) {
		_capacity =	capacity;
		_length	= 0;
		_array = _new T[capacity];
	}

	GrowableArray(GrowableArray &other): _defaultValue(T()) {
		_capacity = other._capacity;
		_length = other._length; 
		_array = _new T[_capacity];
		for (int i=0; i<_length; i++)
			_array[i] = other._array[i];
	}

/*
	GrowableArray & operator=(const GrowableArray &other) {
		delete[] _array;
		new(this) GrowableArray(other);
		return *this;
	}
*/

	~GrowableArray() {
		delete[] _array; 
	}

	void reset() {
		delete[] _array; 
		_capacity = 0;
		_length = 0;
		_array = 0;
	}

	void add(T newElement) {
		if(_length == _capacity) grow();
		_array[_length]	= newElement;
		_length	++;
	}

	T removeLast() {
		if(_length == 0)
			throw InternalInconsistencyException("GrowableArray::removeLast()", "No elements in array.");
		return _array[--_length];
	}


	int	length() const {
		return _length;
	}

	void setLength(int length) {
		while(length > _capacity) grow();
		while(_length < length)
			_array[_length++] = _defaultValue;
		_length = length;
	}

	T& operator [](size_t index) const {
		return _array[index];
	}

	int find(T target) const {
		for(int i=0; i<_length; i++) {
			if(_array[i] == target)
				return i;
		}
		return -1;
	}

	bool remove(T target) {
		if (_length == 0)
			return false;
		T last = removeLast();
		if (last == target)
			return true;
		int index = find(target);
		if (index == -1) {
			add(last);
			return false;
		} else {
			_array[index] = last;
			return true;
		}
	}


private:

	void grow() {
		int	new_capacity = _capacity * 2;
		if (new_capacity < 16)
			new_capacity = 16;
		T *newArray	= _new T[new_capacity];
		for	(int i=0; i<_length; i++)	
			newArray[i]	= _array[i];
		delete[] _array;
		_array = newArray;
		_capacity =	new_capacity;
	}

	T *_array;
	int _length;
	int _capacity;	
	const T _defaultValue;
};

#endif
