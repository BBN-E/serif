// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/parse/ch_LanguageSpecificFunctions.h"
#include "Chinese/parse/ch_STags.h"

/**
* _fixPartitives attempts to fix the case of a flat NP with a partitive
* word as head. It inserts a new NP node to cover all premods, so that
* they can be recognized as a separate mention.
*
* Example: (NP .... (NPA ... (NN partitive-word))) ->
*          (NP (NP ... (NPA ...)) (NPA (NN partitive-word)))
*/
void ChineseLanguageSpecificFunctions::_fixPartitives(ParseNode *node) {
	if (node->label != ChineseSTags::NP)
		return;
	if (node->headNode == 0 || node->headNode->label != ChineseSTags::NPA)
		return;
	

	ParseNode* child = node->headNode;
	ParseNode* grandchild = child->headNode;
	ParseNode* headWordNode = grandchild;

	while (headWordNode->headNode != 0)
		headWordNode = headWordNode->headNode;

	if (! WordConstants::isPartitiveWord(headWordNode->label))
		return;

	if (node->postmods != 0 || child->postmods != 0)
		return;

	if (grandchild->label != ChineseSTags::NN &&
		grandchild->label != ChineseSTags::NR &&
		grandchild->label != ChineseSTags::NT)
		return;

	// Remove grandchild from Child's children and make new head
	ParseNode* childPre = child->premods;
	if (childPre == 0)
		return;

	SessionLogger::dbg("_fixPartitives") << "before: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";


	// remove first premod (closest to head) and make it the new head
	child->premods = childPre->next;
	childPre->next = 0;
	child->headNode = childPre;

	// Create a new NPA node for the partitive word alone
	ParseNode* newHeadNode = _new ParseNode(ChineseSTags::NPA, grandchild->chart_start_index, grandchild->chart_end_index);
	newHeadNode->headNode = grandchild;

	// Create a new NP node for the premods
	ParseNode* newPreNode = _new ParseNode(ChineseSTags::NP, node->chart_start_index, grandchild->chart_start_index-1);
	newPreNode->premods = node->premods;
	newPreNode->headNode = node->headNode;
	newPreNode->isFirstWord = node->isFirstWord;
	newPreNode->isName = false;

	node->premods = newPreNode;
	node->headNode = newHeadNode;

	SessionLogger::dbg("_fixPartitives") << "after: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";

}

/**
* _fixNPList attempts to add NP substructure to the case of a flat NP. 
* It inserts a new NP node for each list item, so that it can be 
* recognized as a separate mention.
*
* Example: (NPA (NN ...) (CC ...) (NN ...)) ->
*          (NP (NPA (NN ...)) (CC ...) (NPA (NN ...)))
*/
void ChineseLanguageSpecificFunctions::_fixNPList(ParseNode* node) {
	if (node->label != ChineseSTags::NP &&
		node->label != ChineseSTags::NPA)
		return;
	// if the original node is an NPA, the children will be, and it will be NP
	bool isNPA = node->label == ChineseSTags::NPA;
	// I assume a flattened list is going to have premods and no postmods
	if (node->postmods != 0)
		return;
	if (node->premods == 0)
		return;
	// check for something with a cc in premods, before we go to any trouble
	bool seenCC = false;
	int premodLength = 0;
	ParseNode* pre = node->premods;
	while (pre != 0) {
		premodLength++;
		if (pre->label == ChineseSTags::CC) {
			seenCC = true;
		}
		pre = pre->next;
	}
	if (!seenCC)
		return;

	// now the actual transmogrification
	// this code is largely borrowed from parsetree/ParseNode.java::fixCommonParseProblems

	// form an array of the elements, by reversing the premods and adding the head
	// this will make the scanning easier to deal with
	int children_size = premodLength+1;
	// MEMORY: these must be cleaned at the end!
	ParseNode** children = _new ParseNode*[children_size];
	ParseNode** lhs_children = _new ParseNode*[children_size];
	ParseNode** rhs_children = _new ParseNode*[children_size];
	bool lhs_has_np = false;
	bool lhs_has_nn = false;
	bool rhs_has_np = false;
	bool rhs_has_nn = false;
	ParseNode* cc = 0;
	int i = children_size-1;
	children[i--] = node->headNode;
	pre = node->premods;
	while (pre != 0) {
		if (i < 0)
			throw InternalInconsistencyException("ch_ChineseLanguageSpecificFunctions::_fixNPList()",
			"incorrect number of premods in array");
		children[i--] = pre;
		pre = pre->next;
	}
	if (i != -1)
		throw InternalInconsistencyException("ch_ChineseLanguageSpecificFunctions::_fixNPList()",
		"incorrect number of premods in array");
	int lhs_idx = 0;
	int rhs_idx = 0;
	for (i = 0; i < children_size; i++) {
		ParseNode* child = children[i];
		Symbol sym = child->label;
		// before the conjunction, fill lhs
		if (cc == 0) {
			if (sym == ChineseSTags::CC) {
				cc = child;
			}
			else {
				lhs_children[lhs_idx++] = child;
				if (sym == ChineseSTags::NP ||
					sym == ChineseSTags::NPA ||
					sym == ChineseSTags::NPP) {

						lhs_has_np = true;
					}
				else if (sym == ChineseSTags::NN ||
					sym == ChineseSTags::NR ||
					sym == ChineseSTags::NT) {
						lhs_has_nn = true;
						lhs_has_np = false;
					}
			}
		}
		// after the conjunction, fill rhs
		else {
			rhs_children[rhs_idx++] = child;
			if (sym == ChineseSTags::NP ||
				sym == ChineseSTags::NPA ||
				sym == ChineseSTags::NPP) {

					rhs_has_np = true;
				}
			else if (sym == ChineseSTags::NN ||
				sym == ChineseSTags::NR ||
				sym == ChineseSTags::NT) {
					rhs_has_nn = true;
					rhs_has_np = false;
				}
		}
	}
	// all nodes have been assigned. If we are indeed in a node changing
	// situation (and we should be) then proceed
	if (cc != 0 &&
		(lhs_has_nn || lhs_has_np) && (rhs_has_nn || rhs_has_np))
	{

		// the children vector is to be scrapped and filled with an np from either
		// side if possible, or simply the side's nodes, and of course the conjunction

		// MEMORY NOTE: the lhs and rhs nodes may be created here. They will be deleted
		// by their eventual parent
		ParseNode* lhs = 0;
		ParseNode* rhs = 0;
		if (!lhs_has_np) {
			lhs = _new ParseNode(isNPA ? ChineseSTags::NPA : ChineseSTags::NP, lhs_children[0]->chart_start_index, lhs_children[lhs_idx-1]->chart_end_index);
			_fillNPFromVector(lhs, lhs_children, lhs_idx);
			SessionLogger::dbg("_fixNPList") << "new LHS NP: " << lhs->toDebugString() << "(chart_start_index = " << lhs->chart_start_index << ", chart_end_index = " << lhs->chart_end_index << ")\n";
		}
		if (!rhs_has_np) {
			rhs = _new ParseNode(isNPA ? ChineseSTags::NPA : ChineseSTags::NP, rhs_children[0]->chart_start_index, rhs_children[rhs_idx-1]->chart_end_index);
			_fillNPFromVector(rhs, rhs_children, rhs_idx);
			SessionLogger::dbg("_fixNPList") << "new RHS NP: " << rhs->toDebugString() << "(chart_start_index = " << rhs->chart_start_index << ", chart_end_index = " << rhs->chart_end_index << ")\n";
		}

		// scrap and fill children
		int children_new_idx = 0;
		if (lhs == 0)
			for (i = 0; i < lhs_idx; i++)
				children[children_new_idx++] = lhs_children[i];
		else
			children[children_new_idx++] = lhs;
		children[children_new_idx++] = cc;
		if (rhs == 0)
			for (i = 0; i < rhs_idx; i++)
				children[children_new_idx++] = rhs_children[i];
		else
			children[children_new_idx++] = rhs;
		for (i = children_new_idx; i < children_size; i++)
			children[i] = 0;

		// now transmogrify the original node with this new children set
		_fillNPFromVector(node, children, children_new_idx);
		// children are now the npas
		if (isNPA)
			node->label = ChineseSTags::NP;
	}
	// clean up the child arrays
	delete [] children;
	delete [] lhs_children;
	delete [] rhs_children;
}

/**
* _fixNPList attempts to add NP substructure to the case of a flat NP. 
* It inserts a new NP node for each list item, so that it can be 
* recognized as a separate mention.
*
* Example: (NPA (NN ...) (CC ...) (NN ...)) ->
*          (NP (NPA (NN ...)) (CC ...) (NPA (NN ...)))
*/
void ChineseLanguageSpecificFunctions::_fixNPList(SynNode* node, SynNode* children[], int& n_children, int& head_index) {
	if (node->getTag() != ChineseSTags::NP &&
		node->getTag() != ChineseSTags::NPA)
		return;
	// if the original node is an NPA, the children will be, and it will be NP
	bool isNPA = node->getTag() == ChineseSTags::NPA;
	// I assume a flattened list is going to have premods and no postmods
	if (head_index != n_children - 1)
		return;
	if (head_index == 0)
		return;
	// check for something with a cc in premods, before we go to any trouble
	bool seenCC = false;
	int premodLength = head_index;
	int i = 0;
	for (i = 0; i < premodLength; i++) {
		if (children[i]->getTag() == ChineseSTags::CC) 
			seenCC = true;
	}
	if (!seenCC)
		return;

	// now the actual transmogrification
	// this code is largely borrowed from parsetree/ParseNode.java::fixCommonParseProblems

	// form an array of the elements, by reversing the premods and adding the head
	// this will make the scanning easier to deal with

	// MEMORY: these must be cleaned at the end!
	SynNode** lhs_children = _new SynNode*[n_children];
	SynNode** rhs_children = _new SynNode*[n_children];
	bool lhs_has_np = false;
	bool lhs_has_nn = false;
	bool rhs_has_np = false;
	bool rhs_has_nn = false;
	SynNode* cc = 0;
	int lhs_idx = 0;
	int rhs_idx = 0;
	for (int j = 0; j < n_children; j++) {
		SynNode* child = children[j];
		Symbol sym = child->getTag();
		// before the conjunction, fill lhs
		if (cc == 0) {
			if (sym == ChineseSTags::CC) {
				cc = child;
			}
			else {
				lhs_children[lhs_idx++] = child;
				if (sym == ChineseSTags::NP ||
					sym == ChineseSTags::NPA ||
					sym == ChineseSTags::NPP) {

						lhs_has_np = true;
					}
				else if (sym == ChineseSTags::NN ||
					sym == ChineseSTags::NR ||
					sym == ChineseSTags::NT) {
						lhs_has_nn = true;
						lhs_has_np = false;
					}
			}
		}
		// after the conjunction, fill rhs
		else {
			rhs_children[rhs_idx++] = child;
			if (sym == ChineseSTags::NP ||
				sym == ChineseSTags::NPA ||
				sym == ChineseSTags::NPP) {

					rhs_has_np = true;
				}
			else if (sym == ChineseSTags::NN ||
				sym == ChineseSTags::NR ||
				sym == ChineseSTags::NT) {
					rhs_has_nn = true;
					rhs_has_np = false;
				}
		}
	}
	// all nodes have been assigned. If we are indeed in a node changing
	// situation (and we should be) then proceed
	if (cc != 0 &&
		(lhs_has_nn || lhs_has_np) && (rhs_has_nn || rhs_has_np))
	{

		// the children vector is to be scrapped and filled with an np from either
		// side if possible, or simply the side's nodes, and of course the conjunction

		// MEMORY NOTE: the lhs and rhs nodes may be created here. They will be deleted
		// by their eventual parent
		SynNode* lhs = 0;
		SynNode* rhs = 0;
		if (!lhs_has_np) {
			lhs = _new SynNode(0, node, isNPA ? ChineseSTags::NPA : ChineseSTags::NP, lhs_idx);
			lhs->setChildren(lhs_idx, _findNPHead(lhs_children, lhs_idx), lhs_children);
			lhs->setTokenSpan(lhs_children[0]->getStartToken(), lhs_children[lhs_idx-1]->getEndToken());
		}
		if (!rhs_has_np) {
			rhs = _new SynNode(0, node, isNPA ? ChineseSTags::NPA : ChineseSTags::NP, rhs_idx);
			rhs->setChildren(rhs_idx, _findNPHead(rhs_children, rhs_idx), rhs_children);
			rhs->setTokenSpan(rhs_children[0]->getStartToken(), rhs_children[rhs_idx-1]->getEndToken());
		}

		// scrap and fill children
		int children_new_idx = 0;
		if (lhs == 0)
			for (i = 0; i < lhs_idx; i++)
				children[children_new_idx++] = lhs_children[i];
		else
			children[children_new_idx++] = lhs;
		children[children_new_idx++] = cc;
		if (rhs == 0)
			for (i = 0; i < rhs_idx; i++)
				children[children_new_idx++] = rhs_children[i];
		else
			children[children_new_idx++] = rhs;
		for (i = children_new_idx; i < n_children; i++)
			children[i] = 0;

		// now transmogrify the original node with this new children set
		n_children = children_new_idx;
		head_index = _findNPHead(children, n_children);
		node->setChildren(n_children, head_index, children);

		// children are now the npas
		if (isNPA)
			node->setTag(ChineseSTags::NP);
	}
	// clean up the child arrays
	delete [] lhs_children;
	delete [] rhs_children;
}

/**
* _fixHeadlessQP adds an NP above a QP that does not have a NP-like
* node as its parent.  This allows headless QPs to be mentions.
*
* Example: (VP (VE ...) (QP ...)) ->
*          (VP (VE ...) (NP (QP ...)))
*/
void ChineseLanguageSpecificFunctions::_fixHeadlessQP(ParseNode* node) {
	if (node->label == ChineseSTags::NP ||
		node->label == ChineseSTags::NPA ||
		node->label == ChineseSTags::DNP)
	{
		return;
	}

	ParseNode* pre = node->premods;
	ParseNode *prev = 0;
	while (pre != 0) {
		if (pre->label == ChineseSTags::QP) {
			SessionLogger::dbg("_fixHeadlessQP") << "before: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
			// Create a new NP node to surround the QP
			ParseNode* newNPNode = _new ParseNode(ChineseSTags::NP, pre->chart_start_index, pre->chart_end_index);
			newNPNode->headNode = pre;
			newNPNode->isFirstWord = pre->isFirstWord;
			newNPNode->isName = false;
			newNPNode->next = pre->next;
			pre->next = 0;
			if (prev != 0)
				prev->next = newNPNode;
			else
				node->premods = newNPNode;
			pre = newNPNode;
			SessionLogger::dbg("_fixHeadlessQP") << "after: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
		}
		prev = pre;
		pre = pre->next;
	}

	ParseNode* head = node->headNode;
	if (head->label == ChineseSTags::QP) {
		SessionLogger::dbg("_fixHeadlessQP") << "before: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
		// Create a new NP node to surround the QP
		ParseNode* newNPNode = _new ParseNode(ChineseSTags::NP, head->chart_start_index, head->chart_end_index);
		newNPNode->headNode = head;
		newNPNode->isFirstWord = head->isFirstWord;
		newNPNode->isName = false;
		node->headNode = newNPNode;
		SessionLogger::dbg("_fixHeadlessQP") << "after: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
	}

	ParseNode* post = node->postmods;
	prev = 0;
	while (post != 0) {
		if (post->label == ChineseSTags::QP) {
			SessionLogger::dbg("_fixHeadlessQP") << "before: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
			// Create a new NP node to surround the QP
			ParseNode* newNPNode = _new ParseNode(ChineseSTags::NP, post->chart_start_index, post->chart_end_index);
			newNPNode->headNode = post;
			newNPNode->isFirstWord = post->isFirstWord;
			newNPNode->isName = false;
			newNPNode->next = post->next;
			post->next = 0;				
			if (prev != 0)
				prev->next = newNPNode;
			else
				node->postmods = newNPNode;
			post = newNPNode;
			SessionLogger::dbg("_fixHeadlessQP") << "after: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
		}
		prev = post;
		post = post->next;
	}
}

/**
* _fixHeadlessQP adds an NP above a QP that does not have a NP-like
* node as its parent.  This allows headless QPs to be mentions.
*
* Example: (VP (VE ...) (QP ...)) ->
*          (VP (VE ...) (NP (QP ...)))
*/
void ChineseLanguageSpecificFunctions::_fixHeadlessQP(SynNode* node, SynNode* children[], int& n_children, int& head_index) {
	if (node->getTag() == ChineseSTags::NP ||
		node->getTag() == ChineseSTags::NPA ||
		node->getTag() == ChineseSTags::DNP)
	{
		return;
	}

	for (int i = 0; i < n_children; i++) {
		if (children[i]->getTag() == ChineseSTags::QP) {
			// Create a new NP node to surround the QP
			SynNode *newNPNode = _new SynNode(0, node, ChineseSTags::NP, 1);
			newNPNode->setChildren(1, 0, &children[i]);
			newNPNode->setTokenSpan(children[i]->getStartToken(), children[i]->getEndToken());

			children[i] = newNPNode;
			node->setChildren(n_children, node->getHeadIndex(), children);
		}
	}
}

// assign premods, head, and postmod to node, given kids of size size.
// links the premods and postmods too. Overwrites any previous info in node
// kids is assumed to be in normal order: 
// first premod...last premod, head, first postmod...last postmod
void ChineseLanguageSpecificFunctions::_fillNPFromVector(ParseNode* node, ParseNode* kids[], int size) {
	int head = _findNPHead(kids, size);
	// set premod chain (must be reversed for the node)
	int i;
	for (i = head-1; i >0; i--)
		kids[i]->next = kids[i-1];
	kids[0]->next = 0;
	// head has no next
	kids[head]->next = 0;
	// postmod chain already in proper order
	for (i = head+1; i < size-1; i++)
		kids[i]->next = kids[i+1];
	kids[size-1]->next = 0;
	// now insert premod, head, and post into node, or insert
	// 0 if no such thing

	// premods 
	if (head-1 >= 0)
		node->premods = kids[head-1];
	else 
		node->premods = 0;

	// head
	node->headNode = kids[head];

	// postmods
	if (head+1 < size)
		node->postmods = kids[head+1];
	else
		node->postmods = 0;
}

// assumes the array is in normal order: premods, head, postmods
// code adapted (with lots of liberties) from ChineseHeadFinder.cpp. Determine the head
// of the new NP that will wrap these nodes
int ChineseLanguageSpecificFunctions::_findNPHead(ParseNode* arr[], int numNodes)  {
	if (numNodes == 1)
		return 0;
	Symbol set1[7] = {ChineseSTags::NP, ChineseSTags::NPA, ChineseSTags::NPP, ChineseSTags::NN, ChineseSTags::NR,  
						ChineseSTags::NT, ChineseSTags::DATE};
	int result = _scanBackward(arr, numNodes, set1, 7);
	if (result >= 0)
		return result;
	Symbol set2[12] = {ChineseSTags::CP, ChineseSTags::QP, ChineseSTags::DP, ChineseSTags::DNP, 
						ChineseSTags::CLP, ChineseSTags::PRN, ChineseSTags::LCP, ChineseSTags::IP, 
						ChineseSTags::PP, ChineseSTags::ADJP, ChineseSTags::ADVP, ChineseSTags::UCP};
	result = _scanBackward(arr, numNodes, set2, 12);
	if (result >= 0)
		return result;
	return numNodes-1;

}

int ChineseLanguageSpecificFunctions::_scanBackward(ParseNode* nodes[], int numNodes, Symbol syms[], int numSyms) {
	int i;
	for (i = numNodes-1; i >= 0; i--) {
		int j;
		for (j = 0; j < numSyms; j++) {
			if (nodes[i]->label == syms[j])
				return i;
		}
	}
	return -1;

}

// assumes the array is in normal order: premods, head, postmods
// code adapted (with liberties) from ChineseHeadFinder.cpp. Determine the head
// of the new NP that will wrap these nodes
int ChineseLanguageSpecificFunctions::_findNPHead(SynNode* arr[], int numNodes) {
	if (numNodes == 1)
		return 0;
	Symbol set1[7] = {ChineseSTags::NP, ChineseSTags::NPA, ChineseSTags::NPP, ChineseSTags::NN, ChineseSTags::NR,  
						ChineseSTags::NT, ChineseSTags::DATE};
	int result = _scanBackward(arr, numNodes, set1, 7);
	if (result >= 0)
		return result;
	Symbol set2[12] = {ChineseSTags::CP, ChineseSTags::QP, ChineseSTags::DP, ChineseSTags::DNP, 
						ChineseSTags::CLP, ChineseSTags::PRN, ChineseSTags::LCP, ChineseSTags::IP, 
						ChineseSTags::PP, ChineseSTags::ADJP, ChineseSTags::ADVP, ChineseSTags::UCP};
	result = _scanBackward(arr, numNodes, set2, 12);
	if (result >= 0)
		return result;
	return numNodes-1;

}

int ChineseLanguageSpecificFunctions::_scanBackward(SynNode* nodes[], int numNodes, Symbol syms[], int numSyms) {
	int i;
	for (i = numNodes-1; i >= 0; i--) {
		int j;
		for (j = 0; j < numSyms; j++) {
			if (nodes[i]->getTag() == syms[j])
				return i;
		}
	}
	return -1;

}
