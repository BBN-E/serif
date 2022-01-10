// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___TREENODECHAIN_H___
#define ___TREENODECHAIN_H___

#include "Generic/SPropTrees/SPropTree.h"
#include "Generic/relations/xx_RelationUtilities.h"

//typedef std::pair<std::wstring,const STreeNode*> RoleLink;

struct TreeNodeElement {
	long _tnid;
	std::wstring _etype, _role, _text;
	const Proposition* _prop;
	const Mention* _men;
	int _arity;
	TreeNodeElement(const STreeNode* tn); 
	std::wstring toString() const;
	/*bool operator<(const TreeNodeElement& tne2) { 
		if ( _tnid < tne2._tnid ) return true;
		else if ( _tnid > tne2._tnid ) return false;
		else return _text < tne2._text;
	}*/
};

typedef std::map<long,TreeNodeElement*> TNEDictionary;

class TreeNodeChain {
	int _topLink;
	unsigned int _distance;
	std::vector<long> _chain;
	TNEDictionary& _dictionary;
public:
	TreeNodeChain(const SPropTree* t, const STreeNode* node1, const STreeNode* node2, TNEDictionary& dict);
	~TreeNodeChain() {};
	std::wstring toString() const;
	const TreeNodeElement* getElement(int i) const;
	int getTopLink() const { return _topLink; }
	int getSize() const { return (int) _chain.size(); }
	unsigned int getDistance() const { return _distance; }
};


#endif
