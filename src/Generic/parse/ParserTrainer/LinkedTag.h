// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LINKED_TAG_H
#define LINKED_TAG_H

class LinkedTag {
public:
	Symbol tag;
	LinkedTag* next;
	int count;
	LinkedTag () {	next = 0; }
	LinkedTag (Symbol _tag) {
		tag = _tag;
		next = 0;
		count = 1;
	}

	LinkedTag (Symbol _tag, int _count) {
		tag = _tag;
		next = 0;
		count = _count;
	}
};

#endif
