// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/discmodel/TreeNodeChain.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include <algorithm>
#include "Generic/linuxPort/serif_port.h"

const int LARGE_INT=10000;
//long TreeNodeElement::_FIRST_UNUSED_ID=0;

TreeNodeElement::TreeNodeElement(const STreeNode* tn) : _men(0), _prop(0), _tnid(-1), _arity(0) {
	if ( ! tn ) return;
	_tnid = tn->getID();
	_etype = tn->getEntityType();
	if ( _etype == L"" ) _etype = L"-";
	_role = tn->getRole();
	if ( _role == L"" ) _role = L"-";
	_text = tn->getContent()->getPhrase();
	_arity = (int)tn->getRoles().size();
	const Proposition* prop=tn->getContent()->getProposition();
	if ( prop ) _prop = prop;
	AltMentions::const_iterator amci=tn->getContent()->getMentions().begin();
	if ( amci != tn->getContent()->getMentions().end() ) {
		_men = amci->first; //in the present version, there's at most one mention for each treenode
		if ( amci->first->getMentionType() == Mention::DESC ) {
			_text = RelationUtilities::get()->stemPredicate(_text.c_str(),Proposition::NOUN_PRED).to_string();
		}
	} else if ( prop ) {
		_text = RelationUtilities::get()->stemPredicate(_text.c_str(),prop->getPredType()).to_string();
		if ( PotentialRelationInstance::CONFUSED_SYM == _text.c_str() ) {
			if ( prop->getPredType() == Proposition::SET_PRED) _text = L"<set>";
			else if (prop->getPredType() == Proposition::LOC_PRED) _text = L"<loc>";
			else _text = L"-";
		}
	}
	if ( _text == L"" ) _text = L"-";
	//_cluster = WordClusterClass(_text, true);
}


std::wstring TreeNodeElement::toString() const {
	std::wostringstream s;
	s << "(" << _tnid << ":" << _etype << "/" << _role << "/" << _text << ")"; 
	return s.str();
}


/** connection between two STreeNodes in a tree. 
	It has 2 vectors each of them contains path from either one of the two nodes to their first shared ancestor-node.
	All nodes along the path (including the 2 nodes themselves and the ancestor are included in this vector
*/

TreeNodeChain::TreeNodeChain(const SPropTree* tree, const STreeNode* n1, const STreeNode* n2, TNEDictionary& dict)
	 : _dictionary(dict) {
	std::vector<long> chain1, chain2;
	_distance = LARGE_INT;
	_chain.clear();
	_topLink = -1;
	if ( ! tree || ! n1 || ! n2 ) return;
	//tree = t;
	const STreeNode* node1 = n1, *node2 = n2;
	long id1 = node1->getID();
	if ( _dictionary.find(id1) == _dictionary.end() ) _dictionary[id1] = _new TreeNodeElement(node1);
	chain1.push_back(id1);
	long id2 = node2->getID();
	if ( _dictionary.find(id2) == _dictionary.end() ) _dictionary[id2] = _new TreeNodeElement(node2);
	chain2.push_back(id2);
	SPropTree::iterator i1=tree->at(node1);
	SPropTree::iterator i2=tree->at(node2);
	if ( i1 == tree->end() || i2 == tree->end() ) return;

	if ( node1 == node2 ) { _distance = 0; _topLink = 0; return; }
	int lev1=i1.getLevel();
	int lev2=i2.getLevel();
	int l;
	//std::wstring lastText;
	for ( l = std::max(lev1,lev2); l >= 0 && node1 != node2; l-- ) {
		//std::wcerr << "\nl=" << l << "; l1=" << lev1 << "; l2=" << lev2;
		if ( ! node1 || ! node2 ) {
			std::wcerr << "\nerror: failed to connect nodes " << n1->toString(false) << " and " << n2->toString(false) 
				<< " in tree\n" << tree->toString(false);
			return;
		}
		if ( lev1 == l ) {
			node1 = node1->getParent();
			id1 = node1->getID();
			if ( _dictionary.find(id1) == _dictionary.end() ) _dictionary[id1] = _new TreeNodeElement(node1);
			chain1.push_back(id1);
			lev1--;
		}
		if ( lev2 == l ) {
			node2 = node2->getParent();
			id2 = node2->getID();
			if ( _dictionary.find(id2) == _dictionary.end() ) _dictionary[id2] = _new TreeNodeElement(node2);
			chain2.push_back(id2);
			lev2--;
		}
	}
	if ( l >= 0 ) {
		_chain.insert(_chain.end(),chain1.begin(),chain1.end());
		std::reverse(chain2.begin(),chain2.end());
		_chain.insert(_chain.end(),chain2.begin()+1,chain2.end());
		_topLink = (int) chain1.size() - 1;
		_distance = (unsigned int)(_chain.size()-1); //will always be non-negative
		//std::wcerr << "\nDIST BTW: " << n1->toString(false) << " and " << n2->toString(false) << ":\n" << toString();
	}
}

std::wstring TreeNodeChain::toString() const {
	std::wostringstream s;
	TNEDictionary::const_iterator tnedci;
	for ( int i = 0; i < (int) _chain.size(); i++ ) {
		tnedci = _dictionary.find(_chain[i]);
		if ( i > _topLink ) s << "<--";
		s << ( tnedci != _dictionary.end() ? tnedci->second->toString() : L"?" );
		if ( i < _topLink ) s << "-->";
	}
	s << ": d=" << _distance;
	return s.str();
}

const TreeNodeElement* TreeNodeChain::getElement(int i) const {
	if ( i < 0 || i >= (int) _chain.size() ) return 0;
	TNEDictionary::const_iterator tnedci=_dictionary.find(_chain[i]);
	if ( tnedci == _dictionary.end() ) return 0;
	else return tnedci->second;
}
