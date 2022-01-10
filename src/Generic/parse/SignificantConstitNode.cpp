// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstddef>
#include <iostream>
#include "Generic/parse/SignificantConstitNode.h"
#include <stdio.h>

using namespace std;

const size_t SignificantConstitNode::blockSize = SIGCON_NODE_BLOCK_SIZE;
SignificantConstitNode* SignificantConstitNode::freeList = 0;

string SignificantConstitNode::toString() 
{
	if (count == 0)
		return "";

	string result;

	result += "(";
	char temp[10];
	sprintf(temp, "%d", left);
	result += temp;
	result += " ";
	sprintf(temp, "%d", right);
	result += temp;
	result += ") ";

	if (next != 0)
		result += next->toString();

	return result;
	
}

void* SignificantConstitNode::operator new(size_t n, int, char *, int) { return operator new(n); }

SignificantConstitNode::SignificantConstitNode () {
	left = -1;
	right = -1;
	next = 0;
	tail = this;
	count = 0;
}

SignificantConstitNode::SignificantConstitNode (SignificantConstitNode *n) {
	copyIn(n);
}

SignificantConstitNode::SignificantConstitNode (int _left, int _right) {
	left = _left;
	right = _right;
	next = 0;
	tail = this;
	count = 1;

}

SignificantConstitNode::SignificantConstitNode(bool left_is_constit, 
	SignificantConstitNode *leftNode, int lltok, int lrtok, bool right_is_constit, 
	SignificantConstitNode *rightNode, int rltok, int rrtok)
{
	left = -1;
	right = -1;
	next = 0;
	tail = this;
	count = 0;

	if (leftNode->count != 0) {
		copyIn(leftNode);
	}

	if (left_is_constit) {
		if (left >= 0) 
			addElement(lltok, lrtok);
		else {
			left = lltok;
			right = lrtok;
			count = 1;
		}
		
	}
	
	if (rightNode->count != 0) {
		if (left >= 0) 
			addNode(rightNode);
		else copyIn(rightNode);
	}

	if (right_is_constit) {
		if (left >= 0) 
			addElement(rltok, rrtok);
		else {
			left = rltok;
			right = rrtok;
			count = 1;
		}
	}
}

SignificantConstitNode::~SignificantConstitNode()
{
	delete next;
}

void* SignificantConstitNode::operator new(size_t)
{
    SignificantConstitNode* p = freeList;
    if (p) {
        freeList = p->next;
    } else {
		//cerr << "allocating new SignificantConstitNode block" << endl;
        SignificantConstitNode* newBlock = 
			static_cast<SignificantConstitNode*>(::operator new(
				blockSize * sizeof(SignificantConstitNode)));
        for (size_t i = 1; i < (blockSize - 1); i++)
            newBlock[i].next = &newBlock[i + 1];
        newBlock[blockSize - 1].next = 0;
        p = newBlock;
        freeList = &newBlock[1];
    }
    return p;
}

void SignificantConstitNode::operator delete(void* object)
{
    SignificantConstitNode* p = static_cast<SignificantConstitNode*>(object);
    p->next = freeList;
    freeList = p;
}

void SignificantConstitNode::addNode(SignificantConstitNode *n) {
	tail->next = _new SignificantConstitNode(n);
	tail = tail->next->tail;
	count = count + n->count;
}

void SignificantConstitNode::addElement(int _left, int _right) {
	tail->next = _new SignificantConstitNode(_left, _right);
	tail = tail->next;
	count++;
}

void SignificantConstitNode::copyIn(SignificantConstitNode *n) {
	left = n->left;
	right = n->right;
	if (n->next != 0) {
		next = _new SignificantConstitNode(n->next);
		tail = next->tail;
	} else {
		next = 0;
		tail = this;
	}
	count = n->count;
}

bool SignificantConstitNode::operator==(const SignificantConstitNode& scNode2) {
	if (count != scNode2.count ||
		left != scNode2.left ||
		right != scNode2.right)
		return false;
	if (next == 0)
		return true;
	if (*next == *(scNode2.next))
		return true;
	return false;
}

