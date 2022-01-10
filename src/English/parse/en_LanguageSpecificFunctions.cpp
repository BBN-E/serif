// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/en_LanguageSpecificFunctions.h"
#include "English/common/en_WordConstants.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/wordnet/xx_WordNet.h"
/* added for speed up
#include "Generic/common/SymbolUtilities.h"
#include "Generic/WordClustering/WordClusterTable.h"
*/

bool EnglishLanguageSpecificFunctions::_standAloneParser = false;

/* _fixNameCommaNameHeads deals with a very specific situation:
*   (NPA (NPP ...) (, ,) (NPP ...))
* When this was found by the parser as (NPA (NPP ...) (, ,) (NNP oneword)),
* it will understand the head to be the second name, and we need to
* force the first name to be the head.
*
* Normally, we trust the parser on this kind of thing. For example:
*    IBM president George Bush
* will be rightly found as (NPA (NPP ...) (NN president) (NNP George Bush))
* and changed into (NPA (NPP ...) (NN president) (NPP (NNP George) (NNP Bush))
* and it correctly treats the second name as the head.
*
* In most cases where we don't trust the parser, we are dealing with things
* fixed by _fixNPList. The one outstanding case is what occurs in
* "city, state" constructions, and that is fixed here. (EMB 4/1/2003)
*/
void EnglishLanguageSpecificFunctions::_fixNameCommaNameHeads(ParseNode* node) {
	if (node->label != EnglishSTags::NPA)
		return;
	if (node->premods == 0 || node->premods->label != EnglishSTags::COMMA)
		return;
	if (node->headNode->label != EnglishSTags::NPP)
		return;
	if (node->premods->next == 0 || node->premods->next->label != EnglishSTags::NPP)
		return;
	if (node->premods->next->next != 0)
		return;

	Symbol s = Symbol(node->toWString().c_str());
	//		std::cerr << "\nfound: " << s.to_debug_string() << "\n";
	s = Symbol(node->headNode->toWString().c_str());
	//		std::cerr << "headnode: " << s.to_debug_string() << "\n";


	node->headNode->next = node->postmods;
	ParseNode *old_premods = node->premods;
	ParseNode *old_postmods = node->headNode;
	node->premods = 0;
	node->headNode = old_premods->next;
	old_premods->next = old_postmods;
	node->postmods = old_premods;
}

void EnglishLanguageSpecificFunctions::_fixNameCommaNameHeads(SynNode* node, SynNode* children[], int& n_children, int& head_index) {

	if (node->getTag() != EnglishSTags::NPA)
		return;
	if (head_index != 2)
		return;
	if (node->getChild(1)->getTag() != EnglishSTags::COMMA)
		return;
	if (node->getHead()->getTag() != EnglishSTags::NPP)
		return;
	if (node->getChild(2)->getTag() != EnglishSTags::NPP)
		return;


	//		Symbol s = Symbol(node->toString().c_str());
	//		std::cerr << "\nfound: " << s.to_debug_string() << "\n";
	//		s = Symbol(node->getHead()->toString().c_str());
	//		std::cerr << "headnode: " << s.to_debug_string() << "\n";

	head_index = 0;
	node->setHeadIndex(head_index);
}

SymbolHash * EnglishLanguageSpecificFunctions::_legalNominalAdjectives;
void EnglishLanguageSpecificFunctions::_fixLegalNominalAdjectives(ParseNode* node) {
	static int init = 0;
	if (init < 0)
		return;
	if (init == 0) {
		std::string adjectives = ParamReader::getParam("legal_nominal_adjectives");
		if (!adjectives.empty()) {
			_legalNominalAdjectives = _new SymbolHash(adjectives.c_str());
			init = 1;
		} else {
			_legalNominalAdjectives = _new SymbolHash(5);
			init = -1;
		}
	}

	if (node->label != EnglishSTags::NP &&
		node->label != EnglishSTags::NPA)
		return;
	if (node->premods == 0)
		return;
	ParseNode* last = node;
	for (ParseNode* pre = node->premods; pre != 0; last = pre, pre = pre->next) {
		if (pre->headNode == 0 || pre->headNode->headNode != 0)
			continue;
		if (_legalNominalAdjectives->lookup(pre->headNode->label)) {
			ParseNode* rest_of_the_premods = pre->next;
			pre->next = 0;
			ParseNode* newNode = _new ParseNode(EnglishSTags::NPA, pre->chart_start_index, pre->chart_end_index);
			newNode->headNode = pre;
			newNode->next = rest_of_the_premods;
			if (last == node) {
				last->premods = newNode;
			} else {
				last->next = newNode;
			}
			SessionLogger::dbg("_fixLegalNominalAdjectives") << "node: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
			SessionLogger::dbg("_fixLegalNominalAdjectives") << "newNode: " << newNode->toDebugString() << "(chart_start_index = " << newNode->chart_start_index << ", chart_end_index = " << newNode->chart_end_index << ")\n";
		}
	}

}


void EnglishLanguageSpecificFunctions::_fixNPPOS(ParseNode* node) {
	if (node->label != EnglishSTags::NPPOS)
		return;
	// odd if the head isn't POS
	if (node->headNode == 0 || node->headNode->label != EnglishSTags::POS) {
		SessionLogger::warn("unusual_parse") << "NPPOS without a POS head";
		return;
	}

	// odd if there are postmods
	if (node->postmods != 0) {
		SessionLogger::warn("unusual_parse") << "NPPOS with postmods";
		return;
	}

	// odd if there are no premods
	if (node->premods == 0) {
		SessionLogger::warn("unusual_parse") << "NPPOS with no premods";
		return;
	}
	// if the only premod is a single NP/NPA/NPP, no changes need to be made
	ParseNode* pre = node->premods;
	if (pre->next == 0 && (pre->label == EnglishSTags::NP  ||
		pre->label == EnglishSTags::NPA ||
		pre->label == EnglishSTags::NPP))
		return;
	int numPremods = 0;
	while (pre != 0) {
		numPremods++;
		pre = pre->next;
	}
	// create an array of premods for easier handling
	ParseNode** allPremods = _new ParseNode*[numPremods];
	pre = node->premods;
	int i = numPremods-1;
	while (pre != 0) {
		if (i < 0)
			throw InternalInconsistencyException("en_EnglishLanguageSpecificFunctions::_fixNPPOS()",
			"incorrect number of premods in array");
		allPremods[i--] = pre;
		pre = pre->next;
	}
	if (i != -1)
		throw InternalInconsistencyException("en_EnglishLanguageSpecificFunctions::_fixNPPOS()",
		"incorrect number of premods in array");
	// MEMORY: this newly inserted node should be deleted by its parent
	ParseNode* newNode = _new ParseNode(EnglishSTags::NP, node->chart_start_index, node->headNode->chart_start_index-1);
	_fillNPFromVector(newNode, allPremods, numPremods);

	// insert the new node into the nppos
	node->premods = newNode;
	// clean up the array we used
	delete[] allPremods;

	SessionLogger::dbg("_fixNPPOS") << "node: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
	SessionLogger::dbg("_fixNPPOS") << "newNode: " << newNode->toDebugString() << "(chart_start_index = " << newNode->chart_start_index << ", chart_end_index = " << newNode->chart_end_index << ")\n";

	return;

}

void EnglishLanguageSpecificFunctions::_fixNPPOS(SynNode* node, CorefItem* coref, SynNode *children[], int &n_children, int &head_index) {
	if (node->getTag() != EnglishSTags::NPPOS)
		return;
	// odd if the head isn't POS
	if (head_index == 0 || node->getHead()->getTag() != EnglishSTags::POS) {
		SessionLogger::warn("unusual_parse") << "NPPOS without a POS head";
		return;
	}

	// odd if there are postmods
	if (head_index < n_children - 1) {
		SessionLogger::warn("unusual_parse") << "NPPOS with postmods";
		return;
	}

	// odd if there are no premods
	if (head_index == 0) {
		SessionLogger::warn("unusual_parse") << "NPPOS with no premods";
		return;
	}
	// if the only premod is a single NP/NPA/NPP, no changes need to be made
	if (head_index == 1 && (children[0]->getTag() == EnglishSTags::NP  ||
		children[0]->getTag() == EnglishSTags::NPA ||
		children[0]->getTag()== EnglishSTags::NPP))
		return;
	int numPremods = head_index;

	// create an array of premods for easier handling
	SynNode** allPremods = _new SynNode*[numPremods];
	for (int i = 0; i < numPremods; i++) {
		allPremods[i] = children[i];
	}
	// MEMORY: this newly inserted node should be deleted by its parent
	SynNode* newNode = _new SynNode(0, node, EnglishSTags::NP, numPremods);
	newNode->setChildren(numPremods, findNPHead(allPremods, numPremods), allPremods);
	newNode->setTokenSpan(allPremods[0]->getStartToken(),allPremods[numPremods-1]->getEndToken());
	// Fix node's children array and coref info
	newNode->setMentionIndex(node->getMentionIndex());
	node->setMentionIndex(-1);
	if (coref != NULL)
		coref->node = newNode;
	children[0] = newNode;
	children[1] = children[head_index];
	n_children = 2;
	head_index = 1;
	node->setChildren(n_children, head_index, children);


	// clean up the arrays we used
	delete[] allPremods;
	return;

}



// if the np is a flattened list, build np substructure
void EnglishLanguageSpecificFunctions::_fixNPList(ParseNode* node) {
	if (node->label != EnglishSTags::NP &&
		node->label != EnglishSTags::NPA &&
		node->label != EnglishSTags::NX)
		return;
	// if the original node is an NPA, the children will be, and it will be NP
	bool isNPA = node->label == EnglishSTags::NPA;
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
		if (pre->label == EnglishSTags::CC) {
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
			throw InternalInconsistencyException("en_EnglishLanguageSpecificFunctions::_fixNPList()",
			"incorrect number of premods in array");
		children[i--] = pre;
		pre = pre->next;
	}
	if (i != -1)
		throw InternalInconsistencyException("en_EnglishLanguageSpecificFunctions::_fixNPList()",
		"incorrect number of premods in array");
	int lhs_idx = 0;
	int rhs_idx = 0;
	for (i = 0; i < children_size; i++) {
		ParseNode* child = children[i];
		Symbol sym = child->label;
		// before the conjunction, fill lhs
		if (cc == 0) {
			if (sym == EnglishSTags::CC) {
				cc = child;
			}
			else {
				lhs_children[lhs_idx++] = child;
				if (sym == EnglishSTags::NP ||
					sym == EnglishSTags::NPA ||
					sym == EnglishSTags::NX ||
					sym == EnglishSTags::NPP) {

						lhs_has_np = true;
					}
				else if (sym == EnglishSTags::NN ||
					sym == EnglishSTags::NNS ||
					sym == EnglishSTags::NNP ||
					sym == EnglishSTags::NNPS) {
						lhs_has_nn = true;
						lhs_has_np = false;
					}
			}
		}
		// after the conjunction, fill rhs
		else {
			rhs_children[rhs_idx++] = child;
			if (sym == EnglishSTags::NP ||
				sym == EnglishSTags::NPA ||
				sym == EnglishSTags::NX ||
				sym == EnglishSTags::NPP) {

					rhs_has_np = true;
				}
			else if (sym == EnglishSTags::NN ||
				sym == EnglishSTags::NNS ||
				sym == EnglishSTags::NNP ||
				sym == EnglishSTags::NNPS) {
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
		SessionLogger::dbg("_fixNPList") << "node before: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";

		// the children vector is to be scrapped and filled with an np from either
		// side if possible, or simply the side's nodes, and of course the conjunction

		// MEMORY NOTE: the lhs and rhs nodes may be created here. They will be deleted
		// by their eventual parent
		ParseNode* lhs = 0;
		ParseNode* rhs = 0;
		if (!lhs_has_np) {
			lhs = _new ParseNode(isNPA ? EnglishSTags::NPA : EnglishSTags::NP, lhs_children[0]->chart_start_index, lhs_children[lhs_idx-1]->chart_end_index);
			_fillNPFromVector(lhs, lhs_children, lhs_idx);
			SessionLogger::dbg("_fixNPList") << "new LHS NP: " << lhs->toDebugString() << "(chart_start_index = " << lhs->chart_start_index << ", chart_end_index = " << lhs->chart_end_index << ")\n";
		}
		if (!rhs_has_np) {
			rhs = _new ParseNode(isNPA ? EnglishSTags::NPA : EnglishSTags::NP, rhs_children[0]->chart_start_index, rhs_children[rhs_idx-1]->chart_end_index);
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
			node->label = EnglishSTags::NP;

		SessionLogger::dbg("_fixNPList") << "node after: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
	}
	// clean up the child arrays
	delete [] children;
	delete [] lhs_children;
	delete [] rhs_children;
}

void EnglishLanguageSpecificFunctions::_fixNPList(SynNode* node, SynNode* children[], int& n_children, int& head_index) {
	if (node->getTag() != EnglishSTags::NP &&
		node->getTag() != EnglishSTags::NPA &&
		node->getTag() != EnglishSTags::NX)
		return;
	// if the original node is an NPA, the children will be, and it will be NP
	bool isNPA = node->getTag() == EnglishSTags::NPA;
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
		if (children[i]->getTag() == EnglishSTags::CC)
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
			if (sym == EnglishSTags::CC) {
				cc = child;
			}
			else {
				lhs_children[lhs_idx++] = child;
				if (sym == EnglishSTags::NP ||
					sym == EnglishSTags::NPA ||
					sym == EnglishSTags::NX ||
					sym == EnglishSTags::NPP) {

						lhs_has_np = true;
					}
				else if (sym == EnglishSTags::NN ||
					sym == EnglishSTags::NNS ||
					sym == EnglishSTags::NNP ||
					sym == EnglishSTags::NNPS) {
						lhs_has_nn = true;
						lhs_has_np = false;
					}
			}
		}
		// after the conjunction, fill rhs
		else {
			rhs_children[rhs_idx++] = child;
			if (sym == EnglishSTags::NP ||
				sym == EnglishSTags::NPA ||
				sym == EnglishSTags::NX ||
				sym == EnglishSTags::NPP) {

					rhs_has_np = true;
				}
			else if (sym == EnglishSTags::NN ||
				sym == EnglishSTags::NNS ||
				sym == EnglishSTags::NNP ||
				sym == EnglishSTags::NNPS) {
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
			lhs = _new SynNode(0, node, isNPA ? EnglishSTags::NPA : EnglishSTags::NP, lhs_idx);
			lhs->setChildren(lhs_idx, findNPHead(lhs_children, lhs_idx), lhs_children);
			lhs->setTokenSpan(lhs_children[0]->getStartToken(), lhs_children[lhs_idx-1]->getEndToken());
		}
		if (!rhs_has_np) {
			rhs = _new SynNode(0, node, isNPA ? EnglishSTags::NPA : EnglishSTags::NP, rhs_idx);
			rhs->setChildren(rhs_idx, findNPHead(rhs_children, rhs_idx), rhs_children);
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
		head_index = findNPHead(children, n_children);
		node->setChildren(n_children, head_index, children);

		// children are now the npas
		if (isNPA)
			node->setTag(EnglishSTags::NP);
	}
	// clean up the child arrays
	delete [] lhs_children;
	delete [] rhs_children;
}

// assign premods, head, and postmod to node, given kids of size size.
// links the premods and postmods too. Overwrites any previous info in node
// kids is assumed to be in normal order:
// first premod...last premod, head, first postmod...last postmod
void EnglishLanguageSpecificFunctions::_fillNPFromVector(ParseNode* node, ParseNode* kids[], int size) {
	int head = findNPHead(kids, size);
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
// code stolen and adapted from EnglishHeadFinder.cpp. Determine the head
// of the new NP that will wrap these nodes
// code borrowed from EnglishHeadFinder.cpp
int EnglishLanguageSpecificFunctions::findNPHead(ParseNode* arr[], int numNodes) {
	if (numNodes == 1)
		return 0;
	Symbol set1[8] = {EnglishSTags::NN,   EnglishSTags::NNP,  EnglishSTags::NPP,
		EnglishSTags::NNPS, EnglishSTags::DATE, EnglishSTags::NNS,
		EnglishSTags::NX,   EnglishSTags::JJR};
	int result = _scanBackward(arr, numNodes, set1, 8);
	if (result >= 0)
		return result;
	Symbol set2[3] = {EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::NPPOS};
	result = _scanForward(arr, numNodes, set2, 3);
	if (result >= 0)
		return result;
	Symbol set3[3] = {EnglishSTags::DOLLAR, EnglishSTags::ADJP, EnglishSTags::PRN};
	result = _scanBackward(arr, numNodes, set3, 3);
	if (result >= 0)
		return result;
	Symbol set4[1]= {EnglishSTags::CD};
	result = _scanBackward(arr, numNodes, set4, 1);
	if (result >= 0)
		return result;
	Symbol set5[4]= {EnglishSTags::JJ, EnglishSTags::JJS,
		EnglishSTags::RB, EnglishSTags::QP};
	result = _scanBackward(arr, numNodes, set5, 4);
	if (result >= 0)
		return result;
	return numNodes-1;

}
int EnglishLanguageSpecificFunctions::_scanForward(ParseNode* nodes[], int numNodes, Symbol syms[], int numSyms) {
	int i;
	for (i = 0; i < numNodes; i++) {
		int j;
		for (j = 0; j < numSyms; j++) {
			if (nodes[i]->label == syms[j])
				return i;
		}
	}
	return -1;

}
int EnglishLanguageSpecificFunctions::_scanBackward(ParseNode* nodes[], int numNodes, Symbol syms[], int numSyms) {
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
// code stolen and adapted from EnglishHeadFinder.cpp. Determine the head
// of the new NP that will wrap these nodes
// code borrowed from EnglishHeadFinder.cpp
int EnglishLanguageSpecificFunctions::findNPHead(const SynNode* const arr[], int numNodes) {
	if (numNodes == 1)
		return 0;
	Symbol set1[8] = {EnglishSTags::NN,   EnglishSTags::NNP,  EnglishSTags::NPP,
		EnglishSTags::NNPS, EnglishSTags::DATE, EnglishSTags::NNS,
		EnglishSTags::NX,   EnglishSTags::JJR};
	int result = _scanBackward(arr, numNodes, set1, 8);
	if (result >= 0)
		return result;
	Symbol set2[3] = {EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::NPPOS};
	result = _scanForward(arr, numNodes, set2, 3);
	if (result >= 0)
		return result;
	Symbol set3[3] = {EnglishSTags::DOLLAR, EnglishSTags::ADJP, EnglishSTags::PRN};
	result = _scanBackward(arr, numNodes, set3, 3);
	if (result >= 0)
		return result;
	Symbol set4[1]= {EnglishSTags::CD};
	result = _scanBackward(arr, numNodes, set4, 1);
	if (result >= 0)
		return result;
	Symbol set5[4]= {EnglishSTags::JJ, EnglishSTags::JJS,
		EnglishSTags::RB, EnglishSTags::QP};
	result = _scanBackward(arr, numNodes, set5, 4);
	if (result >= 0)
		return result;
	return numNodes-1;

}
int EnglishLanguageSpecificFunctions::_scanForward(const SynNode* const nodes[], int numNodes, Symbol syms[], int numSyms) {
	int i;
	for (i = 0; i < numNodes; i++) {
		int j;
		for (j = 0; j < numSyms; j++) {
			if (nodes[i]->getTag() == syms[j])
				return i;
		}
	}
	return -1;

}
int EnglishLanguageSpecificFunctions::_scanBackward(const SynNode* const nodes[], int numNodes, Symbol syms[], int numSyms) {
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

void EnglishLanguageSpecificFunctions::_fixHyphen(ParseNode* node) {
	if (node->headNode->label == ParserTags::HYPHEN)
		_fixHyphenRegular(node->headNode);
	ParseNode *post = node->postmods;
	while (post != 0) {
		if (post->label == ParserTags::HYPHEN)
			_fixHyphenRegular(post);
		post = post->next;
	}
	ParseNode *pre = node->premods;
	ParseNode *last = node;
	while (pre != 0) {
		if (pre->label == ParserTags::HYPHEN) {
			ParseNode* restOfPremods = pre->next;
			pre->next = 0;
			_fixHyphenPremod(pre);
			ParseNode *hyphen_end = pre;
			while (hyphen_end->next != 0)
				hyphen_end = hyphen_end->next;
			hyphen_end->next = restOfPremods;
			if (last == node)
				last->premods = pre;
			else last->next = pre;
			pre = hyphen_end;
		}
		last = pre;
		pre = pre->next;
	}
}

void EnglishLanguageSpecificFunctions::_fixHyphenRegular(ParseNode* node) {
	if (node->label != ParserTags::HYPHEN)
		return;
	node->label = getNameLabel();
	// node -- headNode & premods ==> node -- secondName -- headNode & premods
	ParseNode* secondName = _new ParseNode(getNameLabel());
	secondName->headNode = node->headNode;
	node->headNode = secondName;
	secondName->premods = node->premods;
	node->premods = 0;

	// find hyphen among premods
	ParseNode* prev = secondName->headNode;
	ParseNode* hyphen = secondName->premods;
	int hyphen_index = node->chart_end_index - 1;
	while (hyphen != 0) {
		if (hyphen->headNode->label == EnglishSTags::HYPHEN)
			break;
		prev = hyphen;
		hyphen = hyphen->next;
		hyphen_index--;
	}
	// break chain before hyphen
	if (prev == secondName->headNode)
		secondName->premods = 0;
	else prev->next = 0;
	// put hyphen up at level of secondName
	node->premods = hyphen;
	if (hyphen == 0 || hyphen->next == 0) {
		SessionLogger::warn("unusual_parse") << "weirdly formed hyphen constraint node";
		return;
	}
	// create firstName out of remaining premods, put as next premod after hyphen
	ParseNode* firstName = _new ParseNode(getNameLabel());
	firstName->headNode = hyphen->next;
	firstName->premods = firstName->headNode->next;
	firstName->headNode->next = 0;
	hyphen->next = firstName;

	// set chart_start_index and chart_end_index for all modified nodes
	firstName->chart_start_index = node->chart_start_index;
	firstName->chart_end_index = hyphen_index - 1;
	secondName->chart_start_index = hyphen_index + 1; 
	secondName->chart_end_index = node->chart_end_index;
	hyphen->chart_start_index = hyphen_index;
	hyphen->chart_end_index = hyphen_index;

	SessionLogger::dbg("_fixHyphenRegular") << "node: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
	SessionLogger::dbg("_fixHyphenRegular") << "hyphen: " << hyphen->toDebugString() << "(chart_start_index = " << hyphen->chart_start_index << ", chart_end_index = " << hyphen->chart_end_index << ")\n";
	SessionLogger::dbg("_fixHyphenRegular") << "firstName: " << firstName->toDebugString() << "(chart_start_index = " << firstName->chart_start_index << ", chart_end_index = " << firstName->chart_end_index << ")\n";
	SessionLogger::dbg("_fixHyphenRegular") << "secondName: " << secondName->toDebugString() << "(chart_start_index = " << secondName->chart_start_index << ", chart_end_index = " << secondName->chart_end_index << ")\n";

}

void EnglishLanguageSpecificFunctions::_fixHyphenPremod(ParseNode* node) {
	if (node->label != ParserTags::HYPHEN)
		return;
	node->label = getNameLabel();

	// find hyphen among premods
	ParseNode* prev = node;
	ParseNode* hyphen = node->premods;
	int hyphen_index = node->chart_end_index - 1;
	while (hyphen != 0) {
		if (hyphen->headNode->label == EnglishSTags::HYPHEN)
			break;
		prev = hyphen;
		hyphen = hyphen->next;
		hyphen_index--;
	}
	// break chain before hyphen
	if (prev == node)
		prev->premods = 0;
	else prev->next = 0;

	// put hyphen up at level of node
	node->next = hyphen;
	if (hyphen == 0 || hyphen->next == 0) {
		SessionLogger::warn("unusual_parse") << "weirdly formed hyphen constraint node";
		return;
	}
	// create firstName out of remaining premods, put as next premod after hyphen
	ParseNode* firstName = _new ParseNode(getNameLabel());
	firstName->headNode = hyphen->next;
	firstName->premods = firstName->headNode->next;
	firstName->headNode->next = 0;
	hyphen->next = firstName;

	// set chart_start_index and chart_end_index for all modified nodes
	firstName->chart_start_index = node->chart_start_index;
	firstName->chart_end_index = hyphen_index - 1;
	node->chart_start_index = hyphen_index + 1; 
	// node->chart_end_index unchanged
	hyphen->chart_start_index = hyphen_index;
	hyphen->chart_end_index = hyphen_index;


	SessionLogger::dbg("_fixHyphenPremod") << "node: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
	SessionLogger::dbg("_fixHyphenPremod") << "hyphen: " << hyphen->toDebugString() << "(chart_start_index = " << hyphen->chart_start_index << ", chart_end_index = " << hyphen->chart_end_index << ")\n";
	SessionLogger::dbg("_fixHyphenPremod") << "firstName: " << firstName->toDebugString() << "(chart_start_index = " << firstName->chart_start_index << ", chart_end_index = " << firstName->chart_end_index << ")\n";
}

static Symbol russianSym(L"russian");
static Symbol federationSym(L"federation");
void EnglishLanguageSpecificFunctions::_splitOrgNames(ParseNode* node)
{
	if (node->label != ParserTags::SPLIT)
		return;

	node->label = getNameLabel();

	if (node->premods == 0 || node->postmods != 0) {
		SessionLogger::warn("unusual_parse") << "weirdly formed split-org constraint node";
		return;
	}

	ParseNode* theNode = 0;
	ParseNode* preNationalityNode = node->headNode;
	ParseNode* nationalityNode = node->premods;

	bool is_russian_federation = false;
	while (nationalityNode->next != 0) {
		if (nationalityNode->next->next == 0 &&
			nationalityNode->next->headNode->label == EnglishWordConstants::THE)
		{
			theNode = nationalityNode->next;
			break;
		}
		if (nationalityNode->headNode->label == federationSym &&
			nationalityNode->next->headNode->label == russianSym)
		{
			is_russian_federation = true;
			if (nationalityNode->next->next != 0 &&
				nationalityNode->next->next->next == 0 &&
				nationalityNode->next->next->headNode->label == EnglishWordConstants::THE)
				theNode = nationalityNode->next->next;
            break;
		}
		preNationalityNode = nationalityNode;
		nationalityNode = nationalityNode->next;
	}

	if (is_russian_federation)
		nationalityNode->next->next = 0;
	else nationalityNode->next = 0;

	if (preNationalityNode == node->headNode)
		node->premods = 0;
	else preNationalityNode->next = 0;

	SessionLogger::dbg("_splitOrgNames") << "before: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
	ParseNode* orgName = _new ParseNode(getNameLabel(), preNationalityNode->chart_start_index, node->chart_end_index);
	orgName->headNode = node->headNode;
	orgName->premods = node->premods;
	node->headNode = orgName;
	ParseNode* gpeName = _new ParseNode(getNameLabel(), nationalityNode->chart_start_index, nationalityNode->chart_end_index);
	gpeName->headNode = nationalityNode;
	if (theNode != 0)
		gpeName->next = theNode;
	node->premods = gpeName;
	SessionLogger::dbg("_splitOrgNames") << "after: " << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
}

Symbol EnglishLanguageSpecificFunctions::getParseTagForWord(Symbol word) {
	if (word == EnglishWordConstants::_COMMA_)
		return EnglishSTags::COMMA;
	else if (word == EnglishWordConstants::AND || word == EnglishWordConstants::OR)
		return EnglishSTags::CC;
	else return EnglishSTags::X;
}

int EnglishLanguageSpecificFunctions::findNameListEnd(const TokenSequence *tokenSequence,
											   const NameTheory *nameTheory,
											   int start)
{
	EntityType listType = EntityType::getOtherType();
	int list_start = -1;
	int list_end = -1;
	bool same_types = true;
	bool seen_and = false;
	bool started_list = false;
	for (int i = 0; i < nameTheory->getNNameSpans(); i++) {
		NameSpan *span = nameTheory->getNameSpan(i);
		if (span->start < start)
			continue;
		if (span->start == start) {
			if (!span->type.isRecognized())
				return -1;
			if (span->end + 1 < tokenSequence->getNTokens()) {
				const Token *tok = tokenSequence->getToken(span->end + 1);
				if (tok->getSymbol() == EnglishWordConstants::_COMMA_) {
					list_start = span->start;
					list_end = span->end + 1;
					listType = span->type;
					seen_and = false;
					same_types = true;
					started_list = true;
				} else return -1;
			}
			continue;
		}

		if (!started_list)
			return -1;
		if (span->start != list_end + 1)
			return -1;

		if (span->type != listType)
			same_types = false;

		if (seen_and) {
			list_end = span->end;
			if (same_types) {
				return list_end;
			} else {
				return -1;
			}
		}

		if (span->end + 1 < tokenSequence->getNTokens()) {
			const Token *tok = tokenSequence->getToken(span->end + 1);
			if (tok->getSymbol() == EnglishWordConstants::_COMMA_) {
				list_end = span->end + 1;
				if (list_end + 1 < tokenSequence->getNTokens()) {
					tok = tokenSequence->getToken(list_end + 1);
					if (tok->getSymbol() == EnglishWordConstants::AND ||
						tok->getSymbol() == EnglishWordConstants::OR)
					{
						list_end = list_end + 1;
						seen_and = true;
					}
				}
			} else if (tok->getSymbol() == EnglishWordConstants::AND ||
						tok->getSymbol() == EnglishWordConstants::OR)
			{
				list_end = span->end + 1;
				seen_and = true;
			} else {
				return -1;
			}
		}
	}
	return -1;
}

bool EnglishLanguageSpecificFunctions::isTrulyUnknownWord(Symbol word) {
	// option 1 - speed up isInWordnet(word) 
	 
	return (!WordNet::getInstance()->isInWordnet(word) &&
			!WordConstants::isPunctuation(word));
	
	
	// option 2 - use WordCluster
	
	/*
	Symbol lc = SymbolUtilities::lowercaseSymbol(word);
	int* val = WordClusterTable::get(word, true);
	if ((val == NULL)){
		return true;
	}
	return false;
	*/
	
}
bool EnglishLanguageSpecificFunctions::isKnownNoun(Symbol word) {
	return (WordNet::getInstance()->isNounInWordnet(word));
}
bool EnglishLanguageSpecificFunctions::isKnownVerb(Symbol word) {
	return (WordNet::getInstance()->isVerbInWordnet(word));
}
bool EnglishLanguageSpecificFunctions::isPotentialGerund(Symbol word) {
	wstring wordStr = word.to_string();	
	return ( (wordStr.at(wordStr.length() - 3) == L'i' || wordStr.at(wordStr.length() - 3) == L'I') &&
		     (wordStr.at(wordStr.length() - 2) == L'n' || wordStr.at(wordStr.length() - 2) == L'N') &&
		     (wordStr.at(wordStr.length() - 1) == L'g' || wordStr.at(wordStr.length() - 1) == L'G'));
}
bool EnglishLanguageSpecificFunctions::isNounPOS(Symbol pos_tag) {
	return (pos_tag == EnglishSTags::NN || pos_tag == EnglishSTags::NNS || pos_tag == EnglishSTags::NNP || pos_tag == EnglishSTags::NNPS);
}
bool EnglishLanguageSpecificFunctions::isVerbPOS(Symbol pos_tag) {
	return (pos_tag == EnglishSTags::VB || pos_tag == EnglishSTags::VBD || pos_tag == EnglishSTags::VBG || pos_tag == EnglishSTags::VBN || pos_tag == EnglishSTags::VBP || pos_tag == EnglishSTags::VBZ);
}
void EnglishLanguageSpecificFunctions::setAsStandAloneParser(){
	EnglishLanguageSpecificFunctions::_standAloneParser = true;
}
bool EnglishLanguageSpecificFunctions::isStandAloneParser(){
	return _standAloneParser;};

Constraint* EnglishLanguageSpecificFunctions::getConstraints(const NPChunkTheory* theory, const TokenSequence* ts, int& num_constraints) {
  Constraint* constraints = _new Constraint[theory->n_npchunks];
  num_constraints = 0;
  int num_chunk = 0;
  if (theory->n_npchunks > 0) {
      constraints[num_constraints].left = theory->npchunks[num_constraints][0];
      constraints[num_constraints].right = theory->npchunks[num_constraints][1];
      constraints[num_constraints].type = Symbol();
      constraints[num_constraints].entityType = EntityType::getUndetType();    
      ++num_constraints;
  }
  for (num_chunk = 1; num_chunk < theory->n_npchunks; num_chunk++){
      char const * str = ts->getToken(theory->npchunks[num_chunk][0])->getSymbol().to_debug_string();
      if (str != 0 && str[0] == '\'') {
        ++(constraints[num_constraints - 1].right);
        constraints[num_constraints].left = theory->npchunks[num_chunk][0] + 1;
      } else {
        constraints[num_constraints].left = theory->npchunks[num_chunk][0];
      }
      constraints[num_constraints].right = theory->npchunks[num_chunk][1];
      constraints[num_constraints].type = Symbol();
      constraints[num_constraints].entityType = EntityType::getUndetType();    
      ++num_constraints;
  }
  return constraints;
}

