// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/sem_tree/SemNode.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/common/HeapChecker.h"


bool SemNode::_sem_nodes_in_use = false;
SemNode *SemNode::_allocatedSemNodes = 0;


SemNode::SemNode(SemNode *children, const SynNode *synNode)
	: _parent(0), _firstChild(0), _lastChild(0), _next(0),
	  _synNode(synNode), _tangential(true), _regularized(false)
{
	_sem_nodes_in_use = true;
	_nextAllocatedSemNode = _allocatedSemNodes;
	_allocatedSemNodes = this;

//	std::cout << "Coming to life: " << this << "\n";
//	std::cout.flush();

	claimChildren(children);
}

SemNode::~SemNode() {
	if (_sem_nodes_in_use) {
		throw InternalInconsistencyException("SemNode::~SemNode()",
			"Attempt to delete SemNode object prematurely.");
	}

//	std::cout << "Dying: " << this << "\n";
//	std::cout.flush();

//	for (SiblingIterator iter = _firstChild; iter.more(); ++iter) {
//		delete &*iter;
//	}
}


SemNode *SemNode::findPrev() const {
	SemNode *result = 0;
	SemNode *child = _parent->_firstChild;
	while (child != this) {
		result = child;
		child = child->_next;
	}

	return result;
}

int SemNode::countChildren() const {
	int n = 0;
	for (SemNode *child = _firstChild; child; child = child->_next)
		n++;
	return n;
}


void SemNode::prependChild(SemNode *newChild) {
	newChild->_parent = this;
	newChild->_next = _firstChild;
	_firstChild = newChild;
	if (_lastChild == 0)
		_lastChild = newChild;


	newChild->_tangential = true;
}

void SemNode::appendChild(SemNode *newChild) {
	newChild->_parent = this;
	newChild->_next = 0;
	if (_firstChild == 0) {
		_firstChild = newChild;
		_lastChild = newChild;
	}
	else {
		_lastChild->_next = newChild;
		_lastChild = newChild;
	}

	newChild->_tangential = true;
}


void SemNode::simplify() {
	for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
		(*iter).simplify();
	}
}

void SemNode::fixLinks() {
	for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
		(*iter).fixLinks();
	}
}

void SemNode::regularize() {
	for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
		if ((*iter)._regularized == false)
			(*iter).regularize();
	}

	_regularized = true;
}

void SemNode::createTraces() {
	createTraces(0);
}

// Returns true if a non-tangential child created a trace.
bool SemNode::createTraces(SemReference *awaiting_ref) {
	bool result = false;

	for (SemNode *child = _firstChild; child; child = child->_next) {
		if (child->_tangential) {
			child->createTraces(0);
		} else {
			result |= child->createTraces(awaiting_ref);
		}
	}

	return result;
}

bool SemNode::createTracesEarnestly(SemReference *awaiting_ref) {
	bool result = false;

	for (SemNode *child = _firstChild; child; child = child->_next) {
		if (child->_tangential == false) {
			result |= child->createTracesEarnestly(awaiting_ref);
		}
	}

	return result;
}

void SemNode::createPropositions(IDGenerator &propIDGenerator) {
	for (SemNode *node = _firstChild; node != 0; node = node->_next) {
		node->createPropositions(propIDGenerator);
	}
}

void SemNode::listPropositions(PropositionSet &result) {
	for (SemNode *node = _firstChild; node != 0; node = node->_next) {
		node->listPropositions(result);
	}
}


bool SemNode::verify(bool root) const {
	for (SemNode *node = _firstChild; node != 0; node = node->_next) {
		if (!root && node->getParent() != this) {
			std::ostringstream ostr;
			ostr << "\n ** Node has no parent:\n  ";
			node->dump(ostr, 2);
			ostr << "\n";
			SessionLogger::info("SERIF") << ostr.str();
			return false;
		}

		if (!node->verify())
			return false;
	}
	return true;
}


bool SemNode::containsReferences() const {
	for (SemNode *node = _firstChild; node != 0; node = node->_next) {
		if (node->containsReferences())
			return true;
	}
	return false;
}

// Remove links on our parents and siblings that point to us.
// Leave our pointers back to them untouched.
void SemNode::pruneOut() {
	if (_regularized == true) {
		throw InternalInconsistencyException("SemNode::pruneOut()",
			"This method doesn't work after the sem tree has been regularized!");
	}

	if (_parent == 0) {
		throw InternalInconsistencyException("SemNode::pruneOut()",
			"Illegal attempt to manipulate structure of root node");
	}

	if (_parent->_firstChild == this) {
		_parent->_firstChild = _next;
		if (_parent->_lastChild == this) {
			// only child; parent left with no children
			_parent->_lastChild = 0;
		}
	} else {
		SemNode *node = _parent->_firstChild;
		while (node != 0) {
			if (node->_next == this) {
				node->_next = _next;
			}

			_parent->_lastChild = node;
			node = node->_next;
		}
	}
}

void SemNode::replaceWithChildren() {
	if (_regularized)
		throw InternalInconsistencyException("SemNode::replaceWithChildren()",
			"This method doesn't work after the sem tree has been regularized!");

	if (_parent == 0) {
		throw InternalInconsistencyException("SemNode::replaceWithChildren()",
			"Illegal attempt to manipulate structure of root node");
	}

	if (_firstChild == 0) {
		// if this node has no children, it can just prune itself out
		pruneOut();
		return;
	}
	// the rest of this code assumes there is at least one child

	SemNode *prev = findPrev();

	if (prev) {
		prev->_next = _firstChild;
	} else {
		_parent->_firstChild = _firstChild;
	}

	for (SemNode *child = _firstChild; ; ) {
		child->_parent = _parent;

		if (child->_next == 0) {
			child->_next = _next;
			break;
		}
		child = child->_next;
	}

	if (_next == 0) {
		_parent->_lastChild = _lastChild;
	}

	// now that this is a free-floating node, we must remove references to
	// children. (otherwise, they'd get deleted along with this node)
//	_firstChild = 0;
//	_lastChild = 0;
}

void SemNode::replaceWithNode(SemNode *node) {
	if (_regularized) {
		throw InternalInconsistencyException("SemNode::replaceWithNode()",
			"This method doesn't work after the sem tree has been regularized!");
	}

	if (_parent == 0) {
		throw InternalInconsistencyException("SemNode::replaceWithNode()",
			"Illegal attempt to manipulate structure of root node");
	}

	SemNode *prev = findPrev();
	if (prev)
		prev->_next = node;
	else
		_parent->_firstChild = node;

	if (_parent->_lastChild == this)
		_parent->_lastChild = node;

	node->_next = _next;
	node->_parent = _parent;

	// now that this is a free-floating node, we must remove references to
	// children. (otherwise, they'd get deleted along with this node)
//	_firstChild = 0;
//	_lastChild = 0;
}

void SemNode::claimChildren(SemNode *childList) {
	_firstChild = childList;
	_lastChild = 0;

	SemNode *node = childList;
	while (node != 0) {
		node->_parent = this;

		_lastChild = node;

		node = node->_next;
	}
}


