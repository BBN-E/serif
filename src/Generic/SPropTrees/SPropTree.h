// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___SPROPTREE_H___
#define ___SPROPTREE_H___

#include "Generic/SPropTrees/STreeNode.h"
//#include "Generic/SPropTreeInfo.h"

class SPropTree {
	static long _FIRST_UNUSED_ID;
	static const STreeNode* nextNode(const STreeNode*, const STreeNode*);
	long _id;
	const STreeNode* _head;
	bool _isNodesOwner, _isCommonOwner;
	CommonInfo* _common;
public:
	static bool DEBUG_MODE;
	enum Traverse { HeadLeftRight=0, LeftRightHead };
	static Traverse traverseOrder;

	class iterator {
		friend class SPropTree;
		const STreeNode* _root;
		const STreeNode* _current;
		int level;
		void synchLevel();
	protected:
		iterator(const STreeNode* tn=0, const STreeNode* r=0) { 
			_current = (STreeNode*) tn; 
			_root = ( r ) ? (STreeNode*) r : _current; 
			synchLevel(); 
		}
	public:
		//iterator(const iterator& i) : _current(i._current) {};
		bool operator==(const iterator& i) { return _current == i._current; }
		bool operator!=(const iterator& i) { return _current != i._current; }
		STreeNode& operator*() { return (STreeNode&) *_current; }
		STreeNode* operator->() { return (STreeNode*) _current; }
		iterator& operator++() { 
			_current = nextNode(_current,_root); 
			synchLevel(); 
			return *this; }
		iterator operator++(int) { 
			const STreeNode* old=_current; 
			_current = nextNode(_current,_root);
			synchLevel();
			return iterator(old, _root); 
		}
		int getLevel() const { return level; }
	};

	SPropTree(const DocTheory*, const SentenceTheory*, const Proposition*); 
	SPropTree(const DocTheory*, const SentenceTheory*, const Sexp*);
	SPropTree(const STreeNode*);
	SPropTree() : _id(0),_head(0),_isNodesOwner(false),_isCommonOwner(false){ };

	~SPropTree();
	void setLanguage(SNodeContent::Language lang) { _common->_language = lang; }
	long getID() const { return _id; }
	int getSize() const { return (_head ? _head->getSize() : 0); }
	iterator find(const Proposition*) const;
	const Proposition* getHeadProposition() const { 
		return ( _head && _head->getContent() && _head->getContent()->getType() == SNodeContent::PROPOSITION ) ? 
			_head->_content->getProposition() : 0; 
	}
	iterator at(const STreeNode* tn) const { return iterator(tn, _head); }
	iterator begin() const;
	iterator end() const { return iterator(0, _head); }
	//check if this pair is a "noun-verb" with a matching function
	//static double isNounVerbPair(const STreeNode* tn1, const STreeNode* tn2);
	//convert proposition tree into a string representation
	std::wstring toString(bool=true) const;
	void serialize(std::wostream&, int) const;
	void serialize(std::wstring&, int) const;
	//find a proposition in a tree that resolves pronoun into what it refers to
	const SynNode* resolvePronouns(const STreeNode*) const;
	void resolveAllPronouns();
	void includeModifiers();
	//void expandAllReferences(bool=true, bool=false, bool=false, bool=false);
	void collectAllMentions();
	int getNodeDepth(const STreeNode*) const;
	int getAbsoluteNodeDepth(const STreeNode*) const;
	const STreeNode* getHead() const { return _head; }
	//size_t getAllMentions(AltMentions& mns, bool onlyHead=false, Mention::Type=Mention::NONE) const;
	size_t getAllEntityMentions(RelevantMentions& rmns, bool unique=true, bool onlyHead=false, const std::wstring& etypeName=L"UNDET", Mention::Type mtype=Mention::NONE) const;
	double getMaximumScore() const;
	const SentenceTheory* getSentenceTheory() const { return _common ? _common->senTheory : 0; }
};

#endif
