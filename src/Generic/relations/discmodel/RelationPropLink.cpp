// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/relations/discmodel/TreeNodeChain.h"


void RelationPropLink::populate(Proposition *prop, Argument *a1, Argument *a2, const MentionSet *mentionSet) {
	_firstEl = _secondEl = _topEl = 0;
	_topProposition = prop;
	_arg1 = a1;
	_arg2 = a2;
	_intermediateProposition = 0;
	_intermediate_arg = 0;
	_nest_direction = NONE;

	// switch <sub> to an appropriate place (so that passives and prefixed PPs don't get confused)
	if (a2->getRoleSym() == Argument::SUB_ROLE) 
	{
		_arg1 = a2;
		_arg2 = a1;
	}

	if (!_topProposition->getPredSymbol().is_null())
		_topStemmedPred = RelationUtilities::get()->stemPredicate(_topProposition->getPredSymbol(),
			_topProposition->getPredType());
	else {
		if (_topProposition->getPredType() == Proposition::SET_PRED)
			_topStemmedPred = Symbol(L"<set>");
		else if (_topProposition->getPredType() == Proposition::LOC_PRED)
			_topStemmedPred = Symbol(L"<loc>");
		else if(_arg2->getRoleSym().is_null())
			_topStemmedPred = Symbol(L"-NULL_PRED-");
		else {
			_topStemmedPred = _arg2->getRoleSym();					
		}
	}

	_wcTop = WordClusterClass(_topStemmedPred, true);

	if (_arg1->getType() == Argument::TEXT_ARG) 
		_wcArg1 = WordClusterClass(_arg1->getNode()->getHeadWord(), true);
	else if (_arg1->getType() == Argument::MENTION_ARG) 
		_wcArg1 = WordClusterClass(_arg1->getMention(mentionSet)->getHead()->getHeadWord(), true);
	
	if (_arg2->getType() == Argument::TEXT_ARG) 
		_wcArg2 = WordClusterClass(_arg2->getNode()->getHeadWord(), true);
	else if (_arg2->getType() == Argument::MENTION_ARG) 
		_wcArg2 = WordClusterClass(_arg2->getMention(mentionSet)->getHead()->getHeadWord(), true);
		
	_n_offsets = SymbolUtilities::fillWordNetOffsets(_topStemmedPred, 
		_wordnetOffsets, MAX_WN_OFFSETS);

}

void RelationPropLink::populate(Proposition *prop, Proposition *intermedProp, 
			  Argument *a1, Argument *intermed, Argument *a2, const MentionSet *mentionSet, int nest)
{
	_firstEl = _secondEl = _topEl = 0;
	_topProposition = prop;
	_arg1 = a1;
	_arg2 = a2;
	_intermediateProposition = intermedProp;
	_intermediate_arg = intermed;
	_nest_direction = nest;

	// switch <sub> and <obj> to appropriate places
	/*if (a2->getRoleSym() == Argument::SUB_ROLE ||
	(a2->getRoleSym() == Argument::OBJ_ROLE &&
	a1->getRoleSym() != Argument::SUB_ROLE)) 
	{
	_arg1 = a2;
	_arg2 = a1;
	}*/

	_topStemmedPred = RelationUtilities::get()->stemPredicate(_topProposition->getPredSymbol(),
		_topProposition->getPredType());
	if(_topStemmedPred.is_null()){
		_topStemmedPred = Symbol(L"-NULL_PRED-");
	}


	_wcTop = WordClusterClass(_topStemmedPred);

	if (_arg1->getType() == Argument::TEXT_ARG) 
		_wcArg1 = WordClusterClass(_arg1->getNode()->getHeadWord(), true);
	else if (_arg1->getType() == Argument::MENTION_ARG) 
		_wcArg1 = WordClusterClass(_arg1->getMention(mentionSet)->getHead()->getHeadWord(), true);
	
	if (_arg2->getType() == Argument::TEXT_ARG) 
		_wcArg2 = WordClusterClass(_arg2->getNode()->getHeadWord(), true);
	else if (_arg2->getType() == Argument::MENTION_ARG) 
		_wcArg2 = WordClusterClass(_arg2->getMention(mentionSet)->getHead()->getHeadWord(), true);

	_n_offsets = SymbolUtilities::fillWordNetOffsets(_topStemmedPred, 
		_wordnetOffsets, MAX_WN_OFFSETS);

}


bool RelationPropLink::isNegative() {
	return (_topProposition->getNegation() != 0 ||
			(_intermediateProposition != 0 &&
			_intermediateProposition->getNegation() != 0));
}

void RelationPropLink::populate(const TreeNodeChain* tnChain) {
	reset();

	if ( ! tnChain || tnChain->getSize() <= 1 ) return;
	int topLink = tnChain->getTopLink();
	_topEl = tnChain->getElement(topLink);

	_firstEl = tnChain->getElement(0);
	_secondEl = tnChain->getElement(tnChain->getSize()-1);


//version 1: take roles from the actual mentions
	_altRole1 =	_firstEl->_role;
	_altRole2 = _secondEl->_role;

	//ignore non-essential roles
	int i;
	for ( i = 0; i < tnChain->getTopLink(); i++ ) {
		std::wstring r=tnChain->getElement(i)->_role;
		if ( r != L"<ref>" && r != L"<member>" ) {
			_altRole1 = r;
			break;
		}
	}
	if ( i == tnChain->getTopLink() ) _altRole1 = L"-NULL_ROLE-";

	for ( i = tnChain->getSize()-1; i > tnChain->getTopLink(); i-- ) {
		std::wstring r=tnChain->getElement(i)->_role;
		if ( r != L"<ref>" && r != L"<member>" ) {
			_altRole2 = r;
			break;
		}
	}
	if ( i == tnChain->getTopLink() ) _altRole2 = L"-NULL_ROLE-";
//end version 1


/*/version 2: take roles from the top:
	const TreeNodeElement* leftTopEl = tnChain->getElement( topLink > 0 ? topLink-1 : topLink);
	const TreeNodeElement* rightTopEl = tnChain->getElement( topLink < (int)tnChain->getSize()-1 ? topLink+1 : topLink);
	_altRole1 = leftTopEl->_role;
	if ( leftTopEl == _topEl || _altRole1 == L"<ref>" || _altRole1 == L"<member>" ) _altRole1 = L"-NULL_ROLE-";
	_altRole2 = rightTopEl->_role;
	if ( rightTopEl == _topEl || _altRole2 == L"<ref>" || _altRole2 == L"<member>" ) _altRole2 = L"-NULL_ROLE-";
//end version 2 */


	// switch <sub> to an appropriate place (so that passives and prefixed PPs don't get confused)
	if (Argument::SUB_ROLE == _altRole2.c_str() ) {
		const TreeNodeElement* tmp=_firstEl;
		_firstEl = _secondEl;
		_secondEl = tmp;
		std::wstring tmps=_altRole1;
		_altRole1 = _altRole2;
		_altRole2 = tmps;
	}

	_topProposition = (Proposition*)_topEl->_prop;
	_topStemmedPred = _topEl->_text.c_str();
	_wcTop = WordClusterClass(_topStemmedPred, true);
	_wcArg1 = WordClusterClass(_firstEl->_text.c_str(), true);
	_wcArg2 = WordClusterClass(_secondEl->_text.c_str(), true);
	_n_offsets = SymbolUtilities::fillWordNetOffsets(_topStemmedPred, _wordnetOffsets, MAX_WN_OFFSETS);

}


void RelationPropLink::reset() {
	 _firstEl = _secondEl = _topEl = 0;
	_topProposition = 0;
	_arg1 = 0;
	_arg2 = 0;
	_intermediateProposition = 0;
	_intermediate_arg = 0;
	_nest_direction = NONE;
}

//We need to check for empty symbols in the role, to enforce this, I'm making the actual 
//arguments inaccessible
//getArgument1()/getArgument2()/getIntermedArg() have been replaced with functions that get
//arguments and roles for each of the arguments
Symbol RelationPropLink::getArg1Role() const {
	if ( _firstEl ) return _altRole1.c_str();
	if(_arg1->getRoleSym().is_null()) {
		return Symbol(L"-NULL_ROLE-");
	} else {
		return _arg1->getRoleSym();
	}
}

Symbol RelationPropLink::getArg2Role() const {
	if ( _secondEl ) return _altRole2.c_str();
	if(_arg2->getRoleSym().is_null()) {
		return Symbol(L"-NULL_ROLE-");
	} else {
		return _arg2->getRoleSym();
	}
}
Symbol RelationPropLink::getIntermedArgRole() const {
	if( ! _intermediate_arg || _intermediate_arg->getRoleSym().is_null()) {
		return Symbol(L"-NULL_ROLE-");
	} else {
		return _intermediate_arg->getRoleSym();
	}
}
const Mention* RelationPropLink::getArg1Ment(const MentionSet* mentionSet) const {
	if ( _firstEl ) return _firstEl->_men;
	if(_arg1 == 0){ 
		//std::cout<<"empty arg1: "<<std::endl;
		return 0;
	}
	if(_arg1->getType() != Argument::MENTION_ARG){
		SessionLogger::info("SERIF")<<"return 0 arg1: "<<std::endl;
		return 0;
	}
	return _arg1->getMention(mentionSet);
}
const Mention* RelationPropLink::getArg2Ment(const MentionSet* mentionSet) const {
	if ( _secondEl ) return _secondEl->_men;
	if(_arg2 == 0){ 
		return 0;
	}
	if(_arg2->getType() != Argument::MENTION_ARG){
		SessionLogger::info("SERIF")<<"return 0 arg2: "<<std::endl;
		return 0;
	}
	return _arg2->getMention(mentionSet);
}

