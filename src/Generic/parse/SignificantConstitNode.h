// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SIGCON_NODE_H
#define SIGCON_NODE_H

#include "Generic/common/Symbol.h"
#include <cstring>
#include <string>

#define SIGCON_NODE_BLOCK_SIZE 10000


class SignificantConstitNode {
private:

	// NB: "count" is not a reference count
	// count = the number of sigcon nodes contained in a chain
	// it is used for easy comparison of two chains and is only 
	// relevent or accurate at the front of a chain

	int left;
	int right;
	int count;
	SignificantConstitNode* next;
	SignificantConstitNode* tail;
	
public:

	SignificantConstitNode(bool left_is_constit, 
		SignificantConstitNode *leftNode, int lltok, int lrtok, 
		bool right_is_constit, 
		SignificantConstitNode *rightNode, int rltok, int rrtok);

	
	SignificantConstitNode (int _left, int _right);

	SignificantConstitNode (SignificantConstitNode *n);

	SignificantConstitNode ();

	void addNode(SignificantConstitNode *n);

	void addElement(int _left, int _right);

	void copyIn(SignificantConstitNode *n);

	
    ~SignificantConstitNode();

	bool operator==(const SignificantConstitNode& scNode2);

	std::string toString();

    static void* operator new(size_t);
    static void operator delete(void* object);
	static void* operator new(size_t n, int, char *, int);

private:
    static SignificantConstitNode* freeList;
    static const size_t blockSize;
	
};


#endif
