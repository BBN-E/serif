// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"
#include "boost/pool/object_pool.hpp"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Sexp.h"
#include "Generic/SPropTrees/STreeNode.h"
#include "boost/lexical_cast.hpp"
#include "Generic/common/Sexp.h"


bool STreeNode::_ALLOW_EXPANSION=true;
long STreeNode::_FIRST_UNUSED_ID=0;
boost::object_pool<STreeNode> * STreeNode::_tndPool = NULL;

void STreeNode::initializeMemoryPool() { 
	if ( _tndPool != NULL ) delete _tndPool;
	_tndPool = _new boost::object_pool<STreeNode>;
	if ( _tndPool == NULL ) {
		SessionLogger::err("SERIF") << "failure in memory allocation: _tndPool; bailing out\n";
		exit(-2);
	}
	return;
}

void STreeNode::clearMemoryPool() { 
	if ( _tndPool != NULL ) delete _tndPool;
	_tndPool = NULL;
}


//==============================================================================

int STreeNode::populateChildren() {
	if ( ! _content || _content->getType() != SNodeContent::PROPOSITION || ! _content->_theProp ) return 0;
	const Proposition* prop=_content->_theProp;

	//_debug = ( prop->getID() == 403 ); //Common::DEBUG_MODE
	if ( _debug ) { SessionLogger::info("SERIF") << "populate: "; prop->dump(std::cerr); SessionLogger::info("SERIF") << "\n"; }
	int total=1;
	int nargs=prop->getNArgs();
	//bool isModifier=(prop->getPredType() == Proposition::MODIFIER_PRED);
	bool isSet=(prop->getPredType() == Proposition::SET_PRED);

	for ( int i=0; i < nargs; i++ ) {
		std::set<const Mention*> argMentions;
		Argument* arg=prop->getArg(i);
		Argument::Type atype=arg->getType();
		STreeNode* child=0;
		Symbol rsym=arg->getRoleSym();
		std::wstring r=( rsym.is_null() ) ? L"" : rsym.to_string();
		//if ( isModifier && r == L"<unknown>" ) continue; //!!! a HACK to eliminate this unnecessary tautology
		if ( isSet && r == L"<ref>" ) continue; //!!! a HACK to eliminate <ref> from sets. They are not handled right anyway:
		                                        //because function getAtomicHead(General) for "A and B" returns "B"!!!
		const Proposition* argProp=0;
		const STreeNode* par;
		const Mention* men=0;
		int menIndex;
		int depth=0;
		if ( _debug ) { SessionLogger::info("SERIF") << "\tconsider ARG[" << i << "]=" << arg->toString() << "; atype=" << atype << std::endl; }
		switch ( atype ) {
			case Argument::TEXT_ARG: 
				if ( _debug ) SessionLogger::info("SERIF") << "T" << "\n";
				//if ( r == L"" && prop->getPredType() == Proposition::NAME_PRED &&
				//		_parent->_parts.find(Argument::REF_ROLE.to_string()) == _parent->_parts.end() )
				//	r = L"<name>";
					
				//maybe we can still retract an entity mention from this synnode??
				men = findEntityMentionForSynNode(arg->getNode());
				if ( ! men ) { //maybe not...
					if ( r != L"" ) {
						child = _tndPool->construct(arg->getNode(), this, r);
						//STreeNode* tn_mp = (STreeNode*) _tndPool.alloc(STreeNode::treeNodePool); 
						//child = new (tn_mp) STreeNode(arg->getNode(), this, r); 
					}
					break;
				}
				//otherwize, yes, there's an entity mention under there!
				//and that's why: NO BREAK!
			case Argument::MENTION_ARG: 
				if ( _debug ) SessionLogger::info("SERIF") << "M" << "\n";
				if ( ! men ) men = arg->getMention(_common->senTheory->getMentionSet());
				if ( ! men ) break;
				argMentions.insert(men);
				menIndex = men->getIndex(); // arg->getMentionIndex();
				argProp = _common->senTheory->getPropositionSet()->getDefinition(menIndex);

				/*if ( _debug ) {
				std::wcerr << "---now: ";
				std::wcerr << "arg=" << arg->toString() << "\n";
				std::wcerr << "mention=" << men->getNode()->toTextString() << "\n";
				std::wcerr << "menind=" << menIndex << "\n";
				std::wcerr << "argprop="; argProp->dump(std::cerr); std::cerr << "\n";
				}*/
				//make sure a corresponding proposition exists and isn't used anywhere up the tree
				if ( ! argProp ) {
					child = _tndPool->construct(men, this, r);
					//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
					//child = new (tn_mp) STreeNode(men, this, r);
					break;
				}
				par = _parent;
                while ( par ) {
					if ( par->_content->_theProp == argProp ) break;
					par = par->_parent;
				}
				if ( par ) break;
				else if ( argProp == prop || ! _ALLOW_EXPANSION ) {
					child = _tndPool->construct(men, this, r);
					//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
					//child = new (tn_mp) STreeNode(men, this, r);
					//if ( _debug ) std::wcerr << "\ncreated " << child->toString(false) << " for argument " << r << ": " << men->getNode()->toTextString() << " of prop " << prop->toString();
					break;
				}
				//otherwise, yes, there's a proposition there
				//if ( argProp->getPredType() == Proposition::MODIFIER_PRED ) std::wcerr << "\n*** expanded menarg " << i << " of " << prop->toString() << " to mod: " << argProp->toString() << "\n";
				//...and that's why: NO BREAK!
			case Argument::PROPOSITION_ARG: 
				if ( _debug ) SessionLogger::info("SERIF") << "P" << "\n";
				if ( ! argProp ) argProp = arg->getProposition();
				if ( ! argProp ) break;

				//go down the <ref>-link of the <modifier>-propositions
				depth = 0;
				while ( argProp && argProp->getPredType() == Proposition::MODIFIER_PRED ) {

					/*depth++;
					std::wcerr << "\n";
					for ( int t=0; t < depth; t++ ) std::wcerr << "\t";
					std::wcerr << "consider: " << argProp->toString() << " for " << prop->toString();*/

					for ( int l=0; l < argProp->getNArgs(); l++ ) {
						Argument* theArg=argProp->getArg(l);
						Symbol theRole=theArg->getRoleSym();
						if ( theRole.is_null() || theRole != L"<ref>" ) continue;
						Argument::Type theAtype=theArg->getType();
						if ( theAtype == Argument::MENTION_ARG ) {
							const Mention* resMen=theArg->getMention(_common->senTheory->getMentionSet());
							argMentions.insert(resMen);
							const Proposition* resProp=getNameOrNounPropositionForMention(_common->senTheory,resMen);
							if ( resProp && resProp != prop ) child = _tndPool->construct(resProp, this, r);
							else child = _tndPool->construct(resMen, this, r);
							//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
							//if ( resProp && resProp != prop ) child = new (tn_mp) STreeNode(resProp, this, r);
							//else child = new (tn_mp) STreeNode(resMen, this, r);
							//std::wcerr << "\n            +++ end up with m: "; theArg->getMention(_common->senTheory->getMentionSet())->dump(std::cerr);
							argProp = 0;
							//!!!!!!!!! NOT CLEAR YET: here, we set argProp to 0 and thus the child proposition will not be checked
							//!!!!!!!!! as to its presence high up the already created part of the tree; looks like it's ok
							break;
						} else if ( theAtype == Argument::TEXT_ARG ) {
							child = _tndPool->construct(theArg->getNode()->toTextString(), this, r);
							//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
							//child = new (tn_mp) STreeNode(theArg->getNode()->toTextString().c_str(), this, r);
							//std::wcerr << "\n            +++ end up with text: " << theArg->getNode()->toTextString(); 
							argProp = 0;
							break;
						} else if ( theAtype == Argument::PROPOSITION_ARG ) {
							argProp = theArg->getProposition();
							//std::wcerr << "\n            +++ end up with prop: " << argProp->toString(0);
							break;
						} else {
							SessionLogger::info("SERIF") << "\nwhat a funny argument: " << theArg->toString() << "... ignore\n";
						}
					}
				}

				if ( ! argProp ) break;
				//if we got there, this means we have a regular proposition to attach!
				//So, make sure this proposition isn't used anywhere up the tree
				par = this;
                while ( par ) {
					if ( par->_content->_theProp == argProp ) break;
					par = par->_parent;
				}
				//std::cerr << "P" << (par!=0) << "\n";
				if ( ! par ) {
					child = _tndPool->construct(argProp, this, r);
					//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
					////if ( argProp->getPredType() == Proposition::MODIFIER_PRED ) std::wcerr << "\n*** expanded proparg " << i << " of " << prop->toString() << " to mod: " << argProp->toString() << "\n";
					//child = new (tn_mp) STreeNode(argProp, this, r); 
				}
				break;

			default:
				SessionLogger::info("SERIF") << "don't know what to do with " << atype << "\n";
		}
		if ( child && child->_role != L"" ) {
			if ( child->_isGood ) {
				std::set<const Mention*>::const_iterator smci;
				for ( smci = argMentions.begin(); smci != argMentions.end(); smci++ )
					child->addMention(*smci);
				total += child->size;
				_parts.insert(std::pair<std::wstring,STreeNode*>(r, child));
				/*
				std::wcerr << "--- added \"" << child->toString() << "\" to ";
				child->_parent->getProposition()->dump(std::cerr);
				std::cerr << std::endl;
				//*/
			} else {
				std::ostringstream ostr;
				ostr << "\tfailed to add child " << child << " of type " << atype << " and role \"" << r << "\" to " << toString(false) << "\n";
				SessionLogger::warn("SERIF") << ostr.str();
			}
		}
	}

	if ( _debug ) SessionLogger::info("prop_trees") << "\n====> " << toString() << "\n";
	int pp=0;
	for ( Roles::const_iterator rci=_parts.begin(); rci != _parts.end(); rci++ ) {
		pp++;
		if ( ! rci->second->_isGood ) {
			SessionLogger::err("SERIF") << "problem with " << pp << "th child of " << boost::lexical_cast<std::string>(_id) << ": \"" << toString(false) << "\"\n";
			exit(-4);
		}
	}

	return total;
}


const Mention* STreeNode::findEntityMentionForSynNode(const SynNode* sn) {
	const Mention* theMention=0;
	const Mention* men=0;
	const Entity* ent=0;
	const SynNode* origsn=sn;
	sn = origsn->getHeadPreterm();
	while ( sn && sn != origsn ) {
		//std::wcerr << "\n??: " << sn << ": " << sn->toTextString();
		if ( sn->getMentionIndex() >= 0 ) { //maybe we found a mention
			men = _common->senTheory->getMentionSet()->getMention(sn->getMentionIndex());
			ent = _common->docTheory->getEntitySet()->getEntityByMention(men->getUID());
			if ( ent ) {
				//std::wcerr << "\nfor SN \"" << getSynNodeText(origsn) << "\" take part \"" << getSynNodeText(sn) << "\" for Entity Mention "; men->dump(std::cerr);
				theMention = men;
			}
		}
		sn = sn->getParent();
	}
	return men;
}


//------------------STreeNode constructors and distructors------------------

STreeNode::STreeNode() : _debug(false) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_NODE (1) " << _id << "\t";
}

STreeNode::~STreeNode() {
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: FREE_NODE (0) " << _id << "\t";
	//if ( _content ) SNodeContent::_nctPool->destroy(_content); //_content->~SNodeContent();
}

//re-create a new node of a tree from an Sexp expression;
//use given tree node as parent or (if parent is 0), create a new root node
//all the subordinate propositions under this node will be created as well

//!!!!!!!!!!! TODO: integrate "_thePhrase" !!!!!!!!!!!!!

STreeNode::STreeNode(const Sexp* se, const CommonInfo* ci, const STreeNode* par) : _isGood(false), _debug(false) {
	_id = _FIRST_UNUSED_ID++;
	if ( Common::DEBUG_MODE ) SessionLogger::info("SERIF") << "\n%%%: ALLOCATE_NODE (2) " << boost::lexical_cast<std::string>(_id) << "\t";

	if ( ! se || ( ! ci && ! par ) ) return;
	std::wstring text=se->to_string();
	//if ( prop )	std::wcerr << "create new node from \"" << se->to_string() << "\"\n";
	_parent = par;
	_common = ( par ) ? _parent->_common : (CommonInfo*) ci;

	int numFields=se->getNumChildren();
	if ( numFields != 6 && numFields != 5) {
		SessionLogger::warn("prop_trees") << "invalid entry (tn): " << text << std::endl;
		return;
	}

	_role = se->getFirstChild()->getValue().to_string();
	if ( Argument::REF_ROLE != _role.c_str() && 
		 Argument::SUB_ROLE != _role.c_str() && 
		 Argument::OBJ_ROLE != _role.c_str() && 
		 Argument::IOBJ_ROLE != _role.c_str() && 
		 Argument::POSS_ROLE != _role.c_str() && 
		 Argument::TEMP_ROLE != _role.c_str() && 
		 Argument::LOC_ROLE != _role.c_str() &&
		 Argument::MEMBER_ROLE != _role.c_str() && 
		 Argument::UNKNOWN_ROLE != _role.c_str() && 
		 _role.length() ) _role = _role.substr(1,_role.length()-2);
	_content = SNodeContent::_nctPool->construct();
	//SNodeContent* nc_mp = (SNodeContent*) mp_alloc(SNodeContent::nodeContentPool); 
	//_content = new (nc_mp) SNodeContent();
	std::wstring ntype=se->getSecondChild()->getValue().to_string();
	int peId=_wtoi(se->getThirdChild()->getValue().to_string());
	if ( ntype == L"P" ) {
		_content->_type = SNodeContent::PROPOSITION;
		_content->_theProp = _common->senTheory->getPropositionSet()->findPropositionByUID(peId);
		if ( ! _content->_theProp ) return;
		_content->_theSynnode = _content->_theProp->getPredHead();
	} else if ( ntype == L"E" ) {
		_content->_type = SNodeContent::ENTITY;
		_content->_theEntity = _common->docTheory->getEntitySet()->getEntity(peId);
		if ( ! _content->_theEntity ) return;
		_content->_theSynnode = 0;
	} else if ( ntype == L"T" ) {
		_content->_type = SNodeContent::PHRASE;
		_content->_theSynnode = 0;
	}

	size = _wtoi(se->getNthChild(3)->getValue().to_string());
	Sexp* mentions=se->getNthChild(4);
	_content->_hasName = false;
	for ( int k=0; mentions && k < mentions->getNumChildren(); k++ ) {
		Sexp* nextPair=mentions->getNthChild(k);
		int numChildren=nextPair->getNumChildren();
		if ( numChildren != 2 && numChildren != 1 ) {
			SessionLogger::warn("prop_trees") << "invalid entry (wd): " << text << std::endl;
			continue;
		}
		MentionUID menuid(_wtoi(mentions->getFirstChild()->getValue().to_string()));
		int sentNum=Mention::getSentenceNumberFromUID(menuid);
		SentenceTheory* st=_common->docTheory->getSentenceTheory(sentNum);
		int menid=Mention::getIndexFromUID(menuid);
		if ( ! st ) continue;
		const Mention* men=st->getMentionSet()->getMention(menid);
		if ( ! men ) {
			SessionLogger::info("prop_trees") << "failed to find mention " << menid << " in sentence " << sentNum << " (" << menuid << ")" << std::endl;
			continue;
		}
		addMention(men);
	}
	//std::wcerr << "got " << _content->_expandedPhrases.size() << " phrases\n";

	Sexp* arguments=se->getNthChild(5); // that is: 6th element
	for ( int a=0; arguments && a < arguments->getNumChildren(); a++ ) {
		Sexp* theArg=arguments->getNthChild(a);
		STreeNode* son = _tndPool->construct(theArg,(CommonInfo*)0,this);
		//STreeNode* tn_mp = (STreeNode*) mp_alloc(STreeNode::treeNodePool); 
		//STreeNode* son = new (tn_mp) STreeNode(theArg,0,this);
		if ( son && son->_isGood && son->_common ) _parts.insert(std::pair<std::wstring,STreeNode*>(son->_role, son));
	}
	_isGood = true;

	//std::wcerr << "\n***RESURECTED:\n" << toString() << std::endl;
}

	

	


//create a new tree node based in the given proposition in the sentence;
//all the subordinate propositions under this nore will be created as well
//the parent of the current proposition is also given
STreeNode::STreeNode(const Proposition* prop, const STreeNode* par, const std::wstring& rl) : _isGood(false), _debug(false) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_NODE (3) " << _id << "\t";
	
	/*if ( prop ) {
		std::cerr << "create new prop node for \"";
		prop->dump(std::cerr, 0);
		std::cerr << "\"\n";
	}*/
	_content = SNodeContent::_nctPool->construct();
	//SNodeContent* nc_mp = (SNodeContent*) mp_alloc(SNodeContent::nodeContentPool); 
	//_content = new (nc_mp) SNodeContent();
	_content->_type = SNodeContent::PROPOSITION;
	_content->_theProp = prop;
	Symbol psym=prop->getPredSymbol();
	_content->_thePhrase = ( psym.is_null() ) ? L"" : psym.to_string();
	if ( _content->_theProp ) _content->_theSynnode = _content->_theProp->getPredHead();
	_parent = par;
	if ( _parent ) _common = _parent->_common;
	_role = rl;
	size = populateChildren();
	_isGood = true;
}


//create a new root node of a tree based in the given proposition in the sentence;
//all the subordinate propositions under this nore will be created as well
STreeNode::STreeNode(const Proposition* prop, const CommonInfo* ci) : _isGood(false), _debug(false) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_NODE (4) " << _id << "\t";
	/*
	if ( prop ) {
		std::cerr << "create new root prop node for \"";
		prop->dump(std::cerr, 0);
		std::cerr << "\"\n";
	}*/
	_content = SNodeContent::_nctPool->construct();
	//SNodeContent* nc_mp = (SNodeContent*) mp_alloc(SNodeContent::nodeContentPool); 
	//_content = new (nc_mp) SNodeContent();
	_content->_type = SNodeContent::PROPOSITION;
	_content->_theProp = prop;
	Symbol psym=prop->getPredSymbol();
	_content->_thePhrase = ( psym.is_null() ) ? L"" : psym.to_string();
	if ( _content->_theProp ) _content->_theSynnode = _content->_theProp->getPredHead();
	_parent = 0;
	_common = (CommonInfo*) ci;
	_role = L"";
	size = populateChildren();
	_isGood = true;
}


STreeNode::STreeNode(const Mention* men, const STreeNode* par, const std::wstring& rl) : _isGood(false), _debug(false) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_NODE (5) " << _id << "\t";
	/*
	if ( men ) {
		std::cerr << "create new mention node for \"";
		//men->getNode()->dump(std::cerr); //getHead()->toString(0)
		std::cerr << "\"; MInd=" << men->getNode()->getMentionIndex() << "\n";
	}*/
	_content = SNodeContent::_nctPool->construct();
	//SNodeContent* nc_mp = (SNodeContent*) mp_alloc(SNodeContent::nodeContentPool); 
	//_content = new (nc_mp) SNodeContent();
	_content->_type = SNodeContent::EMPTY;
	_content->_theSynnode = men->getNode();
	_parent = par;
	_common = _parent->_common;
	_role = rl;

	const SynNode *atomNode=getAtomicHeadGeneral(_content->_theSynnode);
	if ( ! atomNode ) return;
	_content->_thePhrase = getSynNodeText(atomNode);

	if ( ! _common || ! _common->docTheory || ! _common->docTheory->getEntitySet() ) return;
	_content->_theEntity = _common->docTheory->getEntitySet()->getEntityByMention(men->getUID());
	_content->_type = ( _content->_theEntity ) ? SNodeContent::ENTITY : SNodeContent::PHRASE;
	addMention(men);
	size = 1;
	_isGood = true;
}

STreeNode::STreeNode(const SynNode* st, const STreeNode* par, const std::wstring& rl) : _isGood(false), _debug(false) {
	const SynNode* sn=st;
	_id = _FIRST_UNUSED_ID++;
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_NODE (6) " << _id << "\t";
	/*
	if ( sn ) {
		std::cerr << "create new synnode node for " << sn << ": \"";
		//sn->dump(std::cerr); //getHead()->toString(0)
		std::cerr << "\"; MInd=" << sn->getMentionIndex() << "\n";
	}*/
	_common = par->_common;
	//safety check
	if ( ! sn || ! _common || ! _common->docTheory || ! _common->docTheory->getEntitySet()	|| 
		 ! _common->senTheory || ! _common->senTheory->getMentionSet() ) return;
	_content = SNodeContent::_nctPool->construct();
	//SNodeContent* nc_mp = (SNodeContent*) mp_alloc(SNodeContent::nodeContentPool); 
	//_content = new (nc_mp) SNodeContent();
	_content->_type = SNodeContent::EMPTY; 
	_parent = par;
	_role = rl;
	_content->_type = SNodeContent::EMPTY;
	_content->_theSynnode = st;
	const SynNode *atomNode=getAtomicHeadGeneral(st);
	if ( ! atomNode ) return;
	_content->_thePhrase = getSynNodeText(atomNode);
	//std::wcerr << "\nfor " << st << " ADD PHRASE: \"" << p.to_string() << "\" FOR \"" << getSynNodeText(st) << "\"";
	_content->_type = SNodeContent::PHRASE;
	size = 1;
	_isGood = true;
}

STreeNode::STreeNode(const std::wstring& text, const STreeNode* par, const std::wstring& rl) : _isGood(false), _debug(false) {
	_id = _FIRST_UNUSED_ID++;
	//if ( Common::DEBUG_MODE ) std::cerr << "\n%%%: ALLOCATE_NODE (7) " << _id << "\t";
	_content = SNodeContent::_nctPool->construct();
	//SNodeContent* nc_mp = (SNodeContent*) mp_alloc(SNodeContent::nodeContentPool); 
	//_content = new (nc_mp) SNodeContent();
	_content->_type = SNodeContent::PHRASE;  //CHANGE ME!!!
	_content->_theSynnode = 0;
	_content->_thePhrase = text; 
	_parent = par;
	_common = _parent->_common;
	//_content->_theSynnode = 0;
	//_content->_theEntity = 0;
	_role = rl;
	size = 1;
	_isGood = true;
}

//only for TreeNodes with entities!!!
int STreeNode::collectMentions() {
	if ( ! _content || ! _common || ! _common->senTheory || ! _common->docTheory || ! _isGood ) return 0;
	//std::wcerr << "*** collect for " << toString() << "\n";
	const Entity* ent=0;
	SNodeContent::ContentType ctype=_content->getType();
	if ( ctype == SNodeContent::PROPOSITION ) {
		/*!!!! NEW COMMENT
		const SynNode* sn=_content->_theProp->getPredHead();
		if ( ! sn ) return 0;
		int menID=sn->getMentionIndex();  //within Sentence
		if ( menID <= 0 ) return 0;
		const Mention* men=_common->senTheory->getMentionSet()->getMention(menID);
		if ( ! men ) return 0;
		ent = _common->docTheory->getEntitySet()->getEntityByMention(men->getUID());
		*/
	} else if ( ctype == SNodeContent::ENTITY ) {
		ent = _content->_theEntity;
	} else return 0;
	if ( ent == 0 ) return 0;

	int numAdded=0;
	int num=ent->getNMentions();

	//if ( ent->getID() != 10 ) return 0;
	//std::cerr << "look at entity: "; ((Entity*)ent)->dump(std::cerr); std::cerr << "\n";

	for ( int i=0; i < num; i++ ) {
		MentionUID newMenUID=ent->getMention(i); //global UID
		const Mention* newMen=_common->docTheory->getEntitySet()->getMention(newMenUID);
		addMention(newMen);
		numAdded++;
	}
	return numAdded;
}


//go top-down and collect mentions associated with treenodes.
size_t STreeNode::getAllEntityMentions(RelevantMentions& rmns, 
									   bool unique, bool onlyHead, 
									   const std::wstring& etypeName, Mention::Type mtype) const{
	if ( ! _isGood ) return 0;
	const STreeNode* tn=this;
	try {
		EntityType etype=Symbol(etypeName.c_str());

		AltMentions::const_iterator amit;
		for ( amit = _content->_theMentions.begin(); amit != _content->_theMentions.end(); amit++ ) {
			if ( mtype != Mention::NONE && amit->first->getMentionType() != mtype ) continue;
			if ( ! amit->second || ( etype.isDetermined() && !(etype == amit->second->getType()) ) ) continue;
			if ( unique && rmns.find(amit->first) != rmns.end() ) continue;
			rmns.insert(std::make_pair<const Mention*,MentionInfo>(amit->first,MentionInfo(this,amit->second->getType())));
		}

		if ( ! onlyHead ) 
			for ( Roles::const_iterator rci=_parts.begin(); rci != _parts.end(); rci++ )
				rci->second->getAllEntityMentions(rmns, unique, onlyHead, etypeName, mtype);
	} catch ( UnexpectedInputException ) {
		std::wcerr << "unknown entity type specified";
	}
	return rmns.size();
}

std::wstring STreeNode::getEntityType() const {
	AltMentions::const_iterator amit=_content->_theMentions.begin();
	const Entity* ent;
	return ( amit == _content->_theMentions.end() || !(ent=amit->second) ) ? 
		EntityType::getUndetType().getName().to_string() :
		ent->getType().getName().to_string();
}


/*serialize a node in the following format:
	(<role> node_type node_id_within_type size (((word_1) cost_1)((word_2) cost_2)...((word_k) cost_k)))

	where <role> is the role of the node content in the _parent-proposition always enclosed in '<' '>',
	so that for role symbols like <ref> this will be <ref> and for proposition roles like "in" this will be <in>
*/
void STreeNode::serialize(std::wostream& wos, int depth) const {
	if ( ! _content || ! _isGood ) return;
	SNodeContent::ContentType ntype=_content->getType();
	if ( ntype == SNodeContent::EMPTY ) return;
	std::wstring r=_role[0] == '<' ? _role : std::wstring(L"<") + _role + L">";
	for ( int i=0; i < depth; i++ ) wos << "\t";
	wos << "(" << r;
	if ( ntype == SNodeContent::PROPOSITION ) {
		wos << "\tP\t" << _content->getProposition()->getID();
	} else if ( ntype == SNodeContent::ENTITY ) {
		wos << "\tE\t" << _content->getEntity()->getID();
	} else { //if ( ntype == SNodeContent::PHRASE ) {
		wos << "\tT\t" << 0;
	}
	wos << "\t" << size << "\t(";
	AltMentions::const_iterator amci;
	size_t n=0;
	for ( amci = _content->_theMentions.begin(); amci != _content->_theMentions.end(); amci++ ) {
		wos << "(" << amci->first->getUID() << ")";
		n++;
		if ( n < _content->_theMentions.size() ) wos << "\t";
	}
	wos << ")";
	if ( _parts.size() ) {
		wos << "\t(\n";
		Roles::const_iterator rci;
		for ( rci = _parts.begin(); rci != _parts.end(); rci++ ) rci->second->serialize(wos,depth+1);
		for ( int i=0; i < depth; i++ ) wos << "\t";
		wos << ")";
	}
	wos << ")\n";
}



/*same but the node is APPENDED to a wstring
*/
void STreeNode::serialize(std::wstring& wstr, int depth) const {
	if ( ! _content || ! _isGood ) return;
	SNodeContent::ContentType ntype=_content->getType();
	if ( ntype == SNodeContent::EMPTY ) return;
	std::wstring r=_role[0] == '<' ? _role : std::wstring(L"<") + _role + L">";
	for ( int i=0; i < depth; i++ ) wstr += L"\t";
	wstr += L"(";
	wstr += r;
	if ( ntype == SNodeContent::PROPOSITION ) {
		wstr += L"\tP\t";
		std::wostringstream s;
		s << _content->getProposition()->getID();
		wstr += s.str();
	} else if ( ntype == SNodeContent::ENTITY ) {
		wstr += L"\tE\t";
		std::wostringstream s;
		s << _content->getEntity()->getID();
		wstr += s.str();
	} else { //if ( ntype == SNodeContent::PHRASE ) {
		wstr += L"\tT\t0";
	}
	wstr += L"\t";
	std::wostringstream s;
	s << size;
	wstr += s.str();
	wstr += L"\t(";
	AltMentions::const_iterator amci;
	size_t n=0;
	for ( amci = _content->_theMentions.begin(); amci != _content->_theMentions.end(); amci++ ) {
		wstr += L"(";
		std::wostringstream s;
		s << amci->first->getUID();
		wstr += s.str();
		wstr += L")";
		n++;
		if ( n < _content->_theMentions.size() ) wstr += L"\t";
	}
	wstr += L")";
	if ( _parts.size() ) {
		wstr += L"\t(\n";
		Roles::const_iterator rci;
		for ( rci = _parts.begin(); rci != _parts.end(); rci++ ) rci->second->serialize(wstr,depth+1);
		for ( int i=0; i < depth; i++ ) wstr += L"\t";
		wstr += L")";
	}
	wstr += L")\n";
}



std::wstring STreeNode::toString(bool printMentionText) const {
	std::wostringstream s;
	if ( ! _content || ! _isGood ) return L"";
	s << "<" << _role << ">";
	SNodeContent::ContentType ctype=_content->getType();
	if ( ctype == SNodeContent::PROPOSITION ) {
		const Proposition* p=_content->getProposition();
		if ( ! p ) s << "XXX";
		else s << "P/" << p->getID() << "/<" << Proposition::getPredTypeString(p->getPredType()) << ">/" << _content->getPhrase() << "/";
		//_content->getProposition()->dump(s,0,true);
	} else if ( ctype == SNodeContent::PHRASE ) {
		s << "T/" << _content->getPhrase() << "/";
	} else if ( ctype == SNodeContent::ENTITY ) {
		if ( ctype == SNodeContent::ENTITY ) {
			s << "E/";
			if ( _content->_theEntity ) s << _content->_theEntity->getID();
			s << "/" << _content->getPhrase() << "/";
		}
	} else s << "./././";
	s << "#" << _id << " (" << size << ")";
	AltMentions::const_iterator amci;
	s << "[";
	size_t n=0;
	for ( amci = _content->_theMentions.begin(); amci != _content->_theMentions.end(); amci++ ) {
		s << amci->first->getUID();
		if ( amci->second ) s << "-" << Entity::getTypeString(amci->second->getType());
		
		if ( printMentionText ) {
			std::wstring firstNode = amci->first->getNode()->toTextString();
			s << ": \"" << Common::stripTrailingWSymbols(firstNode) << "\"";
		}
		n++;
		if ( n < _content->_theMentions.size() ) s << ", ";
	}
	s << "]";
	return s.str();
}


std::wstring STreeNode::getSynNodeText(const SynNode* sn) {
	std::wstring atomHead=L"";
	const int MAX_SN_LEN=500;
	Symbol names[MAX_SN_LEN];
	sn->getTerminalSymbols(names,MAX_SN_LEN);
	int numWordsInHead=sn->getNTerminals();
	if ( numWordsInHead > MAX_SN_LEN ) {
		SessionLogger::warn("SERIF") << "encountered very long SynNode (len=" << numWordsInHead << "), cut off at " << MAX_SN_LEN << "\n";
		numWordsInHead = MAX_SN_LEN;
	}
	for ( int k=0; k < numWordsInHead; k++ ) {
		atomHead += names[k].to_string();
		if ( k != numWordsInHead-1 ) atomHead += L" ";
	}

	return atomHead.c_str();
}


//like DistillUtilities::getAtomiHead() but operates on SynNodes and can start anywhere (not only on NAME mentions!
const SynNode* STreeNode::getAtomicHeadGeneral(const SynNode* sn) {
	const SynNode *headPreterm=sn->getHeadPreterm();
	const SynNode *potentialName=headPreterm->getParent();
	//!!! faking name tag here, this is not standard policy
	return (potentialName && potentialName->getTag() == Symbol(L"NPP")) ? potentialName : headPreterm;
}


const Proposition* STreeNode::getNameOrNounPropositionForMention(const SentenceTheory* st, const Mention* m) {
	const PropositionSet* ps;
	const MentionSet* ms;
	if ( ! st || ! m ||
		 !(ps=st->getPropositionSet()) || 
		 !(ms=st->getMentionSet()) ) return 0;

	for ( int k=0; k < ps->getNPropositions(); k++ ) {
		const Proposition* p=ps->getProposition(k);
		if ( ! p ) continue;
		Proposition::PredType ptype=p->getPredType();
		if ( ptype != Proposition::NAME_PRED && ptype != Proposition::NOUN_PRED ) continue;
		const Argument* arg;
		Symbol rsym;
		for ( int i=0; i < p->getNArgs(); i++ ) //find the <ref> argument
			if ( (arg=p->getArg(i)) && 
				 !(rsym=arg->getRoleSym()).is_null() && 
				 rsym == L"<ref>" && 
				 arg->getType() == Argument::MENTION_ARG && 
				 arg->getMention(ms) == m ) 
				return p;
	}
	return 0;
}


const Proposition* STreeNode::getNameOrNounPropositionForSynNode(const SentenceTheory* st, const SynNode* sn) {
	const PropositionSet* ps;
	if ( ! st || ! sn || !(ps=st->getPropositionSet()) ) return 0;
	const MentionSet* ms=st->getMentionSet();


	for ( int k=0; k < ps->getNPropositions(); k++ ) {
		const Proposition* p=ps->getProposition(k);
		if ( ! p ) continue;
		Proposition::PredType ptype=p->getPredType();
		if ( ptype != Proposition::NAME_PRED && ptype != Proposition::NOUN_PRED ) continue;
		const Argument* arg;
		Symbol rsym;
		for ( int i=0; i < p->getNArgs(); i++ ) //find the <ref> argument
			if ( (arg=p->getArg(i)) && 
				 !(rsym=arg->getRoleSym()).is_null() && 
				 rsym == L"<ref>" && 
				 ( ( arg->getType() == Argument::MENTION_ARG && ms && arg->getMention(ms)->getNode() == sn ) ||
                   ( arg->getType() == Argument::TEXT_ARG && arg->getNode() == sn ) ) )
				return p;
	}
	return 0;
}
