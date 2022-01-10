// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_NODE_H
#define SEM_NODE_H

#include "Generic/common/InternalInconsistencyException.h"

#include <iostream>

class SynNode;
class PropositionSet;

class SemBranch;
class SemOPP;
class SemReference;
class SemMention;
class SemTrace;
class SemLink;
class IDGenerator;


class SemNode {
protected:
	SemNode *_parent;
	SemNode *_firstChild;
	SemNode *_lastChild;
	SemNode *_next; /// next sibling

	const SynNode *_synNode;

	bool _tangential;
	bool _regularized;

private:
	static bool _sem_nodes_in_use;
	static SemNode *_allocatedSemNodes;
	SemNode *_nextAllocatedSemNode;

public:
	enum Type {BRANCH_TYPE,
			   OPP_TYPE,
			   REFERENCE_TYPE,
			   MENTION_TYPE,
			   TRACE_TYPE,
			   LINK_TYPE};


	/** Subclasses chain to this constructor.
	  * synNode is the corresponding syntactic node.
	  * children is a linked list of children. */
	SemNode(SemNode *children, const SynNode *synNode);
	virtual ~SemNode();


	/** Any non-abstract subclass must implement this to return one of the
	  * valid Types. */
	virtual Type getSemNodeType() const = 0;

	virtual bool isReference() const { return false; }

	// Casting shortcuts -- each subclass overrides one of these to return
	// itself
	virtual SemBranch &asBranch()
		{ throw InternalInconsistencyException("SemNode::asBranch()",
											   "wrong type"); }
	virtual SemOPP &asOPP()
		{ throw InternalInconsistencyException("SemNode::asOPP()",
											   "wrong type"); }
	virtual SemReference &asReference()
		{ throw InternalInconsistencyException("SemNode::asReference()",
											   "wrong type"); }
	virtual SemMention &asMention()
		{ throw InternalInconsistencyException("SemNode::asMention()",
											   "wrong type"); }
	virtual SemTrace &asTrace()
		{ throw InternalInconsistencyException("SemNode::asTrace()",
											   "wrong type"); }
	virtual SemLink &asLink()
		{ throw InternalInconsistencyException("SemNode::asLink()",
											   "wrong type"); }

	// accessors
	SemNode *getParent() const			{ return _parent; }
	SemNode *getFirstChild() const		{ return _firstChild; }
	SemNode *getLastChild()	const		{ return _lastChild; }
	SemNode *getNext() const			{ return _next; } /// (next sibling)

	/// Use with caution...
	void setNext(SemNode *next) { _next = next; }

	bool isTangential() const { return _tangential; }
	void setTangential(bool val) { _tangential = val; }
	bool isRegularized() const { return _regularized; }
	const SynNode *getSynNode() const	{ return _synNode; }

	SemNode *findPrev() const;
	int countChildren() const;

	/// in principle you shouldn't have to use this
	void setSynNode(const SynNode *synNode) { _synNode = synNode; }

	void prependChild(SemNode *newChild);
	void appendChild(SemNode *newChild); /// does NOT clear child's _next ptr


	/** Call simplify() right after a tree is created */
	virtual void simplify();

	/** Call fixLinks() after simplify(). It turns some links into OPPs
	  * and attaches others to separate OPPs. */
	virtual void fixLinks();

	/** Call regularize after fixLinks(). It takes the list-of-children
	  * tree representation and assigns certain children to one of each
	  * sem-node type's statically defined role slots. */
	virtual void regularize();

	/** Call createTraces() after regularize() (on the root node, with no
	  * argument. It creates and resolves traces. */
	void createTraces();

	/** This is called by createTraces(void), and overridden by derived
	  * classes. */
	virtual bool createTraces(SemReference *awaiting_ref);
	virtual bool createTracesEarnestly(SemReference *awaiting_ref);

	/** Call createPropositions() after createTraces() to create the
	  * actual propositions() */
	virtual void createPropositions(IDGenerator &propIDGenerator);

	/** Call listPropositions() after createPropositions() to populate
	  * a PropositionSet with all the propositions that got created. */
	virtual void listPropositions(PropositionSet &result);


	virtual bool verify(bool root = false) const;


	static void deallocateAllSemNodes() {
		_sem_nodes_in_use = false;
		while (_allocatedSemNodes != 0) {
			SemNode *toDel = _allocatedSemNodes;
			_allocatedSemNodes = _allocatedSemNodes->_nextAllocatedSemNode;
			delete toDel;
		}
	}

	virtual void dump(std::ostream &out, int indent = 0) const = 0;
	friend std::ostream &operator <<(std::ostream &out, const SemNode &it)
	{	it.dump(out, 0); return out; }


	/** This business is necessary when looping through children if the
	  * operation performed in the loop may modify the tree structure. 
	  *
	  * IMPORTANT NOTE: Even if you use SiblingIterator, you're NOT safe
	  * if the action performed in the loop changes the structure of the
	  * (old) next child, or prunes it out of the tree.
	  *
	  * In short, there is subtle trickiness here, so if you write a loop
	  * over a node's children which modifies their structure, make sure
	  * you know exactly what's going on.
	  */
	class SiblingIterator {
	private:
		SemNode *_currValue;
		SemNode *_nextValue;
	public:
		SiblingIterator(SemNode *startNode) {
			_currValue = startNode;
			if (_currValue)
				_nextValue = _currValue->_next;
		}
		SiblingIterator &operator=(SemNode *rhs) {
			this->_currValue = rhs;
			if (this->_currValue)
				this->_nextValue = _currValue->_next;
			return *this;
		}
		bool operator==(SiblingIterator &that) {
			return this->_currValue == that._currValue;
		}
		SiblingIterator &operator++() {
			_currValue = _nextValue;
			if (_currValue)
				_nextValue = _currValue->_next;
			return *this;
		}
		bool more() {
			return _currValue != 0;
		}
		SemNode &operator*() {
			return *_currValue;
		}
	};

	/** Heuristics sometimes want to know if a subtree contains any
	  * SemReferences */
	virtual bool containsReferences() const;

	/** Removes node (and thus subtree rooted at node) from tree */
	void pruneOut();

	/** Removes this node but attaches its children to its parent where
	  * it used to be. Note that this changes the parent's child list, so
	  * be careful calling this while looping through children.
	  * If this is called on the root it does nothing. */
	void replaceWithChildren();

	void replaceWithNode(SemNode *node);

protected:
	/* Make all children point to this node as their parent. */
//XXX	void claimChildren();
	/** Set _lastChild to correct value */
	void claimChildren(SemNode *childList);

};

#endif

