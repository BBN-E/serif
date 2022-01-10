// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/SPropTrees/SPropTree.h"
#include "boost/pool/object_pool.hpp"

enum SPropTree::Traverse SPropTree::traverseOrder = SPropTree::HeadLeftRight;

bool SPropTree::DEBUG_MODE=false;
long SPropTree::_FIRST_UNUSED_ID=0;

//set level of the iterator to be the level of the node it's pointing at
void SPropTree::iterator::synchLevel() {
	level = 0;
	const STreeNode* c=_current;
	while ( c && c != _root ) {
		level++;
		c = c->getParent();
	}
}

//declare a node to be root of a new tree (all objects that already exist in the nodes 
//under this node will become available for the new tree, no new memory will be allocated
SPropTree::SPropTree(const STreeNode* tn) : _isNodesOwner(false), _isCommonOwner(false) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Utilities::DEBUG_MODE ) std::wcerr << "\n%%%: ALLOCATE_TREE (1) " << _id << "\t" << "for " << tn->toString(false);
	_head = tn;
	if ( ! _head || ! _head->_isGood ) SessionLogger::warn("prop_trees") << "\tcreated proptree (1) with bad head node!\n";
	_common = tn->_common;
}

//create a new tree rooted in the given proposition in the sentence
SPropTree::SPropTree(const DocTheory* dt, const SentenceTheory* st, const Proposition* prop) : _isNodesOwner(true), _isCommonOwner(true) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Utilities::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_TREE (2) " << _id << "\t";
	_common = _new CommonInfo;
	_common->docTheory = dt;
	_common->senTheory = st;
	_common->_language = SNodeContent::UNKNOWN;
	PropositionSet* ps=st->getPropositionSet();
	//std::cerr << "U PS=" << ps << "; prop=" << prop->toDebugString() << ".";
	if ( ps == 0 ) return;
	_head = STreeNode::_tndPool->construct(prop, _common);
	//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
	//_head = new (tn_mp) STreeNode(prop, _common);
	if ( ! _head || ! _head->_isGood ) SessionLogger::warn("prop_trees") << "\tcreated proptree (2) with bad head node!\n";
	//std::wcerr << "create node owner: " << this << "; common=" << _common << "; _head=" << _head->toString() << "\n";
}


SPropTree::SPropTree(const DocTheory* dt, const SentenceTheory* st, const Sexp* se) : _isNodesOwner(true), _isCommonOwner(true) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Utilities::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_TREE (3) " << _id << "\t";
	if ( ! dt || ! st || ! se ) return;
	_common = _new CommonInfo;
	_common->docTheory = dt;
	_common->senTheory = st;
	_common->_language = SNodeContent::UNKNOWN;

	_head = STreeNode::_tndPool->construct(se, _common, (STreeNode*)0);
	//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
	//_head = new (tn_mp) STreeNode(se, _common, 0);
	if ( ! _head || ! _head->_isGood ) std::cerr << "\twarning: created proptree (3) with bad head node!\n";
}


SPropTree::~SPropTree() {
	//if ( Utilities::DEBUG_MODE ) std::cerr << "\n%%%: FREE_TREE (0) " << _id << "\t";
	if ( _isNodesOwner ) {
		SPropTree::Traverse old=traverseOrder;
		traverseOrder = LeftRightHead;
		iterator pti, pti2;
		pti = begin();
		while (  pti != end() ) {
			pti2 = pti;
			pti++;
			//????????? STreeNode::_tndPool->destroy(pti2.operator->());
			//pti2->~STreeNode();
		}
		traverseOrder = old;
	}
	if ( _isCommonOwner ) {
		//std::wcerr << "gonna delete " << _common << "\n";
		delete _common;
	}
}

SPropTree::iterator SPropTree::begin() const {
	const STreeNode* current=0;
	switch ( traverseOrder ) {
	case HeadLeftRight:	return at(_head); //return head
	case LeftRightHead: //return the leftest
		current = _head;
		while ( current && current->_parts.size() ) {
			current = current->_parts.begin()->second;
		}
		break;
	default: current = 0;
	}
	return at(current);
}

//get next node in the tree; the traversing order is given by the static variable traverseOrder
const STreeNode* SPropTree::nextNode(const STreeNode* current, const STreeNode* root) {
	if ( ! current ) return 0;
	//std::wcerr << "---CUR NODE: " << current->toString() << std::endl;
	const STreeNode* toret=0;
	switch ( traverseOrder ) {
	case HeadLeftRight:
		if ( current->_parts.size() ) toret = current->_parts.begin()->second;
		else { //find the first sibling that is elder or (if none) the eldest uncle
			const STreeNode* c=current;
			while ( c && c != root && c->_parent ) {
				Roles rls=c->_parent->_parts;
				Roles::const_iterator rlc=rls.find(c->_role);
				if ( rlc == rls.end() ) {
					SessionLogger::info("SERIF") << "oh, node " << c->_role << " doesn't exist in the tree (HLR)...\n";
					return 0;
				}
				//we found the first sibling with the same role; let's now move ahead until we find
				//exactly the sibling
				while ( rlc != rls.end() && rlc->second != c ) rlc++;
				//here, we either found the current one, or reached the end
				rlc++;
				if ( rlc != rls.end() ) {
					toret = rlc->second;
					c = 0; // ==> break
				} else c = c->_parent;
			}
		}
		//if ( toret) std::wcerr << "---NEXT NODE: " << toret->toString() << std::endl;
		break;
	case LeftRightHead: //always the youngest descendant of my older brother; and if none -> then parent
		if ( current != root && current->_parent ) {
			Roles rls=current->_parent->_parts;
			Roles::const_iterator rlc=rls.find(current->_role);
			if ( rlc == rls.end() ) {
				SessionLogger::info("SERIF") << "oh, node " << current->_role << " doesn't exist in the tree (LRH)...\n"; return 0;
			}
			while ( rlc != rls.end() && rlc->second != current ) rlc++;
			rlc++;
			if ( rlc != rls.end() ) {
				toret =	rlc->second;
				while ( toret->_parts.size() ) toret = toret->_parts.begin()->second;
			} else toret = current->_parent;
		} else toret = 0;
		break;		
	}
	return toret;
}



/*
//check if the two treenodes form a noun-verb pair like "Clinton visited" and "Clinton visit" or "visit of Clinton"
//if not, return PTMCOST_MAX; otherwise, return the distance between them.
//no penalty for going down the reference link is incurred here.
//first argument must be tree node of the pattern , second - tree node of the document
double SPropTree::isNounVerbPair(const STreeNode* tn1, const STreeNode* tn2) {
	const Proposition* p1,*p2;
	Proposition::PredType pt1,pt2;
	if ( ! tn1 || ! tn2 || ! (p1=tn1->getProposition()) || ! (p2=tn2->getProposition()) ) return PTMCOST_MAX;
	pt1 = p1->getPredType();
	pt2 = p2->getPredType();
	if ( ! (pt1 == Proposition::VERB_PRED && pt2 == Proposition::NOUN_PRED || 
			pt2 == Proposition::VERB_PRED && pt1 == Proposition::NOUN_PRED) ) return PTMCOST_MAX;

	//now check if the pair shares common action (like "Clinton's _visit_" and "Clinton _visited_")
	//go all the way done the <ref> branch for the noun
	const STreeNode* n_tn=( pt1 == Proposition::NOUN_PRED ? tn1 : tn2 );
	const STreeNode* v_tn=( pt1 == Proposition::VERB_PRED ? tn1 : tn2 );
	const STreeNode* theNode=n_tn;
	Roles::const_iterator rci;
	while ( theNode ) {
		Roles::const_iterator rci=theNode->_parts.find(L"<ref>");
		if ( rci == theNode->_parts.end() ) {
			double dist=SNodeContent::distance(v_tn->_content, theNode->_content);
			//if ( dist < PTMCOST_MAX ) {
			//	std::wcerr << "FOUND N-V-PAIR:\n" << SPropTree(tn1).toString(false) << "\nand\n" << SPropTree(tn2).toString(false) << "\n";
				//exit(0);
			//}
			return dist;
		} else {
			theNode = rci->second;
		}
	}
	return PTMCOST_MAX;
}
*/


//return the treenode that contains given proposition or 0 otherwise
SPropTree::iterator SPropTree::find(const Proposition* prop) const {
	if ( ! prop ) return end();
	SPropTree::Traverse old=traverseOrder;
	traverseOrder = HeadLeftRight;
	SPropTree::iterator pti;
	for ( pti = begin(); pti != end(); pti++ ) {
		//std::wcerr << "\npti=" << pti->toString() << "\n";
		if ( pti->getContent() && 
			pti->getContent()->getType() == SNodeContent::PROPOSITION && 
			pti->getContent()->getProposition()->getID() == prop->getID() ) {
				traverseOrder = old;
				return pti;
			}
	}
	traverseOrder = old;
	return end();
}


void SPropTree::collectAllMentions() {
	iterator pti;
	traverseOrder = HeadLeftRight;
	//from all nodes...
	for ( pti = begin(); pti != end(); pti++ ) {
		//std::wcerr << "**COLLECT FOR " << pti->toString();
		//if ( pti->getContent() && pti->getContent()->getType() == SNodeContent::ENTITY ) 
		pti->collectMentions();
		//std::cerr << "DONE\n";
	}
}


void SPropTree::resolveAllPronouns() {
	if ( _common->_language != SNodeContent::ENGLISH ) return;

	traverseOrder = HeadLeftRight;
	//from all nodes...
	for ( iterator pti=begin(); pti != end(); pti++ ) {
		//keep only pronoun propositions
		if ( ! pti->getContent() || pti->getContent()->getType() != SNodeContent::PROPOSITION || 
			pti->getProposition()->getPredType() != Proposition::PRONOUN_PRED || ! pti->_parent ) continue;

		//std::wcerr << "trying to resolve " << pti.operator->() << std::endl;
		//find what SynNode it would be mapped onto
		const SynNode* newSN=resolvePronouns(pti.operator->());
		if ( ! newSN ) continue;
		//std::wcerr << "resolve " << pti->toString() << " to " << newSN->toTextString() << "\n";

		//check if it is a pronoun with parent
		STreeNode* theParent=(STreeNode*)pti->_parent;
		std::wstring role=pti->_role;
		Roles::iterator rci=theParent->_parts.find(role);
		//find how parent refers to me
		while ( rci != theParent->_parts.end() && rci->second != &(*pti) ) rci++;
		if ( rci == theParent->_parts.end() ) {
			SessionLogger::err("SERIF") << "resolveAllPronouns: I'm not a child of my parent!\n";
			exit(-2);
		}
		int removedSize=pti->getSize();
		//and while creating a new one to the noun the pronoun refers to
		STreeNode* newNode=STreeNode::_tndPool->construct(newSN, theParent, role);
		//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
		//STreeNode* newNode = new (tn_mp) STreeNode(newSN, theParent, role);
		if ( ! newNode || newNode->getContent()->getType() == SNodeContent::EMPTY ) return;
		//std::wcerr << "*** GONNA REPLACE\n" << pti->toString() << "\nby\n" << tn_mp->toString() << "\n";
		
		const SynNode* predSN=newSN; //->getHeadPreterm();
		while ( predSN && ! predSN->hasMention() ) predSN = predSN->getParent();
		if ( predSN ) newNode->addMention(_common->senTheory->getMentionSet()->getMention(predSN->getMentionIndex()));

		//remove this reference
		theParent->_parts.erase(rci);
		SPropTree* tmp = _new SPropTree(&(*pti));
		tmp->_isNodesOwner = true; //this is a trick that will make all node to be deleted when we destruct the tree
		delete tmp;
		theParent->_parts.insert(std::pair<std::wstring,STreeNode*>(role, newNode));

		STreeNode* thep=theParent;
		while ( thep ) {
			thep->size += newNode->size - removedSize;
			//if ( ! thep->_parent ) std::wcerr << "add " << thep->toString() << "\n";
			thep = (STreeNode*)thep->_parent;
		}
		pti = iterator(newNode,_head); 
		//!!!!!! this is a little dangerous, because we might miss some nodes 
		//!!!!!! that were after the replaced proposition but turned out to be before the new one
	}
}


//actually, this functions resolved relative pronouns (that, which, who) AND all references to the best alternative

//we know, it's a WORD
const SynNode* SPropTree::resolvePronouns(const STreeNode* tn) const {
	const SynNode* toret=0;
	if ( ! tn || ! tn->getParent() ) return 0;
	const SynNode* theSN=tn->getSynNode();
	const Proposition* prop=tn->getProposition();
	const Proposition* parProp=tn->getParent()->getProposition();
	const SNodeContent* nc=tn->getContent();
	if ( ! theSN || ! prop || ! parProp || ! nc ) return 0;
	if ( nc->getType() != SNodeContent::PROPOSITION && prop->getPredType() != Proposition::PRONOUN_PRED ) return 0;
	Symbol psym=prop->getPredSymbol();
	std::wstring phrase=( psym.is_null() ) ? L"" : psym.to_string();
	///*
	//std::wcerr << "\n\n***CHECK: node=" << tn->toString() << "; synnode=" << theSN->toString(0) << "; phrase=" << phrase << std::endl;
	//std::wcerr << "\n\tmenID=" << theSN->getMentionIndex() << "mention=";
	//_common->senTheory->getMentionSet()->getMention(theSN->getMentionIndex())->dump(std::cerr);
	//*/

	//first resolve relative pronouns to mentions they are pointing at
	if ( phrase == L"that" || phrase == L"which" || phrase == L"who" ) { //"who" is being taken care of by coreference resolution for PER entities
		//look at the SynNode of the pronoun and based on the proposition tree, 
		//find the proposition that a pronoun refers to
		while ( theSN ) {
			Symbol hw=theSN->getHeadWord();
			if ( hw.is_null() ) return 0;
			//std::wcerr << "\nfor \"" << phrase.to_string() << "\" consider \"" << theSN->toTextString() << "\" with head=" << theSN->getHeadWord().to_string() << "\n";
			if ( hw != phrase.c_str() ) { 
				toret = theSN; 
				//std::wcerr << " ... accepted"; exit(0);
				break; 
			}
			theSN = theSN->getParent();
		}
	}

	return toret;
}

/*
//for each node of the tree collect all words associated with it from the following sources:
// 1) wordnet stemming/synonyms/hyponyms/hypernyms
// 2) external (my own) dictionaries of synonyms
void SPropTree::expandAllReferences(bool doStemming, bool doSynonyms, bool doHypernyms, bool doHyponyms) {
	//make sure WordNet is initialized
	if ( ! SNodeContent::wn ) {
		std::cerr << "initializing WordNet...";
		SNodeContent::wn = WordNet::getInstance();
		std::cerr << " done\n";
	}
	//WordNet can only be done for English
	if ( _common->_language != DistillUtilities::ENGLISH ) {
		doStemming = doSynonyms = doHypernyms = doHyponyms = false;
	}
	iterator pti;
	traverseOrder = HeadLeftRight;
	//from all nodes...
	for ( pti = begin(); pti != end(); pti++ ) {
		//std::wcerr << "trying to expand " << pti->toString() << std::endl;
		if ( pti->getContent() ) pti->getContent()->expand(doStemming, doSynonyms, doHypernyms, doHyponyms);
	}
}
*/

int SPropTree::getNodeDepth(const STreeNode* tn) const {
	const STreeNode* par=tn;
	int level=0;
	while ( par && par != _head ) {
		par = par->_parent;
		level++;
	}
	return ( par ? level : -1 );
}

//go all the way up to a node with no parent, consider it absolute head 
int SPropTree::getAbsoluteNodeDepth(const STreeNode* tn) const {
	const STreeNode* par=tn;
	int level=0;
	while ( par ) {
		par = par->_parent;
		level++;
	}
	return level;
}

/*
size_t SPropTree::getAllMentions(AltMentions& mns, bool onlyHead, Mention::Type mtype) const {
	return _head->getAllMentions(mns, _head, onlyHead, mtype);
}
*/

size_t SPropTree::getAllEntityMentions(RelevantMentions& rmns, 
							bool unique, bool onlyHead, 
							const std::wstring& etypeName, Mention::Type mtype) const {
	return _head->getAllEntityMentions(rmns, unique, onlyHead, etypeName, mtype);
}


/*
//same but only names are considered
size_t SPropTree::getAllNames(PhrasesInTree& phs, bool onlyHead) const {
	return _head->getAllNames(phs, _head, onlyHead);
}
*/

std::wstring SPropTree::toString(bool printMentionText) const {
	std::wstring toret=L"";
	SPropTree::iterator pti;
	Traverse old=traverseOrder;
	traverseOrder = HeadLeftRight;

	//toret += L"ID=";
	//toret = toret + _id;
	//toret += L"\n";
	for ( pti = begin(); pti != end(); pti++ ) {
		int lev=pti.getLevel();
		for ( int i=0; i < lev; i++ ) toret += L"\t";
		toret += pti->toString(printMentionText) + L"\n";
	}
	traverseOrder = old;
	return toret;
}

/*serialize prop tree in an Sexp format
following format is observed:
	( sent_number (
		head
		node_2
		...
		node_N
	))
	where all nodes (including the head) are serialized as explained in STreeNode::serialize()
*/
void SPropTree::serialize(std::wostream& wos, int sent) const {
	if ( ! _head ) return;
	SPropTree::iterator pti;
	Traverse old=traverseOrder;
	traverseOrder = HeadLeftRight;

	wos << "(" << sent << "\n";
	_head->serialize(wos,0);
	wos << ")\n";
	traverseOrder = old;
}

/*same but the node is APPENDED to a wstring
*/
void SPropTree::serialize(std::wstring& wstr, int sent) const {
	if ( ! _head ) return;
	SPropTree::iterator pti;
	Traverse old=traverseOrder;
	traverseOrder = HeadLeftRight;

	wstr += L"(";
	std::wostringstream s;
	s << sent;
	wstr += s.str();
	wstr += L"\n";
	_head->serialize(wstr,0);
	wstr += L")\n";
	traverseOrder = old;
}

//must be run straight after SPropTree constructor!!!
void SPropTree::includeModifiers() {
	//find all modifier propositions
	typedef std::map<const void*, std::vector<const Proposition*> > Mods;
	Mods modifiers;
	Mods::const_iterator mi;
	const PropositionSet* ps=_common->senTheory->getPropositionSet();
	for ( int k=0; k < ps->getNPropositions(); k++ ) {
		const Proposition* p=ps->getProposition(k);
		//make sure it's a modifier proposition and
		//descend the <ref> link all the way down to a non-modifier proposition or mention or text
		const Proposition* p2=p;
		const Argument* arg=0;
		Symbol role2;
		/*if ( p->getPredType() == Proposition::MODIFIER_PRED )
			std::wcerr << "\n*** consider modprop "; p->dump(std::cerr); std::wcerr << std::endl;*/
		while ( p2 && p2->getPredType() == Proposition::MODIFIER_PRED ) {
			int i;
			for ( i = 0; i < p2->getNArgs(); i++ ) //find the <ref> argument
				if ( (arg=p2->getArg(i)) && !(role2=arg->getRoleSym()).is_null() && role2 == L"<ref>" ) 
					break;
			if ( i == p2->getNArgs() ) { SessionLogger::info("SERIF") << "\nmodifier proposition " << p2->toString() << " has no reference!"; arg = 0; break; }
			p2 = ( arg->getType() == Argument::PROPOSITION_ARG ) ? arg->getProposition() : 0;
		}
		if ( ! arg ) continue;
		const void* point=0;
		const Proposition* resProp=0;
		const Mention* resMen=0;
		//std::wcerr << "\nfor modifier " << p->toString() << " consider \"" << ((Argument*)arg)->toString() << "\" of type " << arg->getType();
		switch ( arg->getType() ) {
			case Argument::MENTION_ARG:
				//std::wcerr << " (mention: " << arg->getMention(_common->senTheory->getMentionSet())->getNode()->toTextString() << ")";
				point = _common->senTheory->getPropositionSet()->getDefinition(arg->getMentionIndex());
				if ( ! point || point == (const void*)p ) {
					resMen = arg->getMention(_common->senTheory->getMentionSet());
					//resProp = STreeNode::getNameOrNounPropositionForMention(_common->senTheory,resMen);
					//if ( resProp ) point = resProp;
					//else 
					point = _common->docTheory->getEntitySet()->getEntityByMention(resMen->getUID());
				}
				arg = 0;
				break;
			case Argument::TEXT_ARG:
				point=arg->getNode(); 
				/*resProp = STreeNode::getNameOrNounPropositionForSynNode(_common->senTheory,arg->getNode());
				if ( resProp ) {
					std::wcerr << "\treplace it by " << resProp->toString();
					point = resProp; //?????????
				}*/
				arg = 0;
				break;
			case Argument::PROPOSITION_ARG:
				const Proposition* arg2=arg->getProposition();
				point = arg2; 
				break;
		}
		if ( ! point ) continue;
		modifiers[point].push_back(p);
	}

	//now for all proposition TreeNodes check if they can be augmented by a modifier argument
	Traverse oldTrav=traverseOrder;
	traverseOrder = HeadLeftRight;
	std::set<const void*> replaced;
	for ( iterator iter=begin(); iter != end(); iter++ ) {
		STreeNode* tn=iter.operator->();
		SNodeContent::ContentType type=tn->_content->getType();
		const void* candidate=0;
		switch ( type ) {
		case SNodeContent::PROPOSITION: candidate = tn->getProposition(); break;
		case SNodeContent::ENTITY: candidate = tn->getEntity(); break;
		case SNodeContent::PHRASE: candidate = tn->getSynNode(); break;
        default: break;
		}
		int depth=0;
		const void* mod=candidate;
		if ( replaced.find(candidate) != replaced.end() ) continue;
		Mods::const_iterator mci=modifiers.find(mod);
		if ( mci == modifiers.end() ) continue;
		//ok, we found a bunch of modifier propositions that all cover this node; let's augment:
		STreeNode* tnAttachTo=tn;
		if ( type != SNodeContent::PROPOSITION ) {
			//we can not attach modifiers to entity- and synnode-treenodes, but we can use their parents instead!
			tnAttachTo = (STreeNode*)tn->_parent;
			if ( ! tnAttachTo ) continue;
			//std::wcerr << "try attaching modifiers like " << mci->second[0]->toString() << " to " << tnAttachTo->toString(false);
			const Proposition* propAtTo=tnAttachTo->getProposition();
			if ( ! propAtTo || tn->_role != L"<ref>" || 
				(propAtTo->getPredType() != Proposition::NAME_PRED &&
				 propAtTo->getPredType() != Proposition::NOUN_PRED) ) continue;
		}
		//for each found modifier for this treenode...
		const std::vector<const Proposition*>& modProps=mci->second;
		int numAdded=0;
		for ( size_t j=0; j < modProps.size(); j++ ) {
			//create children (text-treenodes) for each non reference argument of this modifier proposition
			for ( int k=0; k < modProps[j]->getNArgs(); k++ ) {
				Argument* arg=modProps[j]->getArg(k);
				Argument::Type atype=arg->getType();
				Symbol rsym=arg->getRoleSym();
				std::wstring r=( rsym.is_null() ) ? L"" : rsym.to_string();
				if ( r == L"" || r == L"<ref>" ) continue;
				if ( atype == Argument::TEXT_ARG ) {
					const SynNode* sn=arg->getNode();
					//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
					STreeNode* modArgTn;
					const Proposition* resProp=STreeNode::getNameOrNounPropositionForSynNode(_common->senTheory,sn);
					if ( resProp ) modArgTn = STreeNode::_tndPool->construct(resProp, tnAttachTo, r);
					else modArgTn = STreeNode::_tndPool->construct(sn, tnAttachTo, r);
					//if ( resProp ) modArgTn = new (tn_mp) STreeNode(resProp, tnAttachTo, r); 
					//else modArgTn = new (tn_mp) STreeNode(sn, tnAttachTo, r); 
					tnAttachTo->_parts.insert(std::pair<std::wstring,STreeNode*>(r,modArgTn));
					numAdded += modArgTn->getSize();
				} else if ( atype == Argument::PROPOSITION_ARG ) {
					const Proposition* argProp=arg->getProposition();
					if ( ! argProp ) continue;
					STreeNode* modArgTn=STreeNode::_tndPool->construct(argProp, tnAttachTo, r);
					//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
					//STreeNode* modArgTn = new (tn_mp) STreeNode(argProp, tnAttachTo, r); 
					tnAttachTo->_parts.insert(std::pair<std::wstring,STreeNode*>(r,modArgTn));
					numAdded += modArgTn->getSize();
				} else if ( atype == Argument::MENTION_ARG ) {
					const Mention* argMen=arg->getMention(_common->senTheory->getMentionSet());
					if ( ! argMen ) continue;
					//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
					STreeNode* modArgTn;
					const Proposition* resProp=STreeNode::getNameOrNounPropositionForMention(_common->senTheory,argMen);
					//if ( resProp ) modArgTn = new (tn_mp) STreeNode(resProp, tnAttachTo, r); 
					//else modArgTn = new (tn_mp) STreeNode(argMen, tnAttachTo, r); 
					if ( resProp ) modArgTn = STreeNode::_tndPool->construct(resProp, tnAttachTo, r);
					else modArgTn = STreeNode::_tndPool->construct(argMen, tnAttachTo, r);
					tnAttachTo->_parts.insert(std::pair<std::wstring,STreeNode*>(r,modArgTn));
					numAdded += modArgTn->getSize();
				} else {
					SessionLogger::info("SERIF") << "\ncan't attach non-textual argument (type " << atype << ") of a modifier to a reference of this modifier\n";
					SessionLogger::info("SERIF") << "\tmod: " << STreeNode(modProps[j],_common).toString(false) << "\n\targ: " << arg->toString() << "\n\tref: " << tnAttachTo->toString(false);
					continue;
				}
			}
			//and the modifier text...
			std::wstring predHead = modProps[j]->getPredHead()->toTextString();
			std::wstring modText = Common::stripTrailingWSymbols(predHead);
			if ( modText != L"" && tnAttachTo->_parts.find(modText) == tnAttachTo->_parts.end() ) {
				//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
				//STreeNode* modTn = _new (tn_mp) STreeNode(modText.c_str(), tnAttachTo, L"<mod>");
				STreeNode* modTn=STreeNode::_tndPool->construct(modText.c_str(), tnAttachTo, L"<mod>");
				tnAttachTo->_parts.insert(std::pair<std::wstring,STreeNode*>(L"<mod>",modTn));
				//finally, if the mod's predicate corresponds to a mention, add this mention to the list
				const SynNode* predSN=modProps[j]->getPredHead();
				if ( predSN ) predSN = predSN->getHeadPreterm();
				if ( predSN && predSN->hasMention() )
					modTn->addMention(_common->senTheory->getMentionSet()->getMention(predSN->getMentionIndex()));

				numAdded++;
			}
		}
		STreeNode* thep=tnAttachTo;
		while ( thep ) {
			thep->size += numAdded;
			thep = (STreeNode*)thep->_parent;
		}
		replaced.insert(candidate);
		iter = iterator(tnAttachTo,_head); //iterate over the whole subtree again (also: maybe we can change what we added)
	}

	traverseOrder = oldTrav;
	return;
}

