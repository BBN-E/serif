// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/parse/LanguageSpecificFunctions.h"

const Symbol EventTriggerObservation::_className(L"event-trigger");
const int EventTriggerObservation::_MAX_OTHER_ARGS = ETO_MAX_OTHER_ARGS;

DTObservation *EventTriggerObservation::makeCopy() {
	EventTriggerObservation *copy = _new EventTriggerObservation();
	copy->populate(this);
	return copy;
}

void EventTriggerObservation::populate(EventTriggerObservation *other) {
	_word = other->getWord();
	_nextWord = other->getNextWord();
	_secondNextWord = other->getSecondNextWord();
	_prevWord = other->getPrevWord();
	_secondPrevWord = other->getSecondPrevWord();
	_lcWord = other->getLCWord();
	_stemmedWord = other->getStemmedWord();
	_pos = other->getPOS();
	_documentTopic = other->getDocumentTopic();
	_is_nominal_premod = other->isNominalPremod();
	_is_copula = other->isCopula();
	for (int i = 0; i < other->getNOffsets(); i++) {
		_wordnetOffsets[i] = other->getNthOffset(i);
	}
	_n_offsets = other->getNOffsets();
	_objectOfTrigger = other->getObjectOfTrigger();
	_indirectObjectOfTrigger = other->getIndirectObjectOfTrigger();
	_subjectOfTrigger = other->getSubjectOfTrigger();
	for (int j = 0; j < _MAX_OTHER_ARGS; j++) {
		_otherArgsToTrigger[j] = other->getOtherArgToTrigger(j);
	}
	_triggerSyntacticType = other->_triggerSyntacticType;
}

void EventTriggerObservation::populate(int token_index, 
									   const TokenSequence *tokens, 
									   const Parse *parse, 
									   MentionSet *mentionSet, 
									   const PropositionSet *propSet, 
									   bool use_wordnet) 
{ 
	_word = tokens->getToken(token_index)->getSymbol();
	_lcWord = SymbolUtilities::lowercaseSymbol(_word);
	_pos = parse->getRoot()->getNthTerminal(token_index)->getParent()->getTag();
	_stemmedWord = SymbolUtilities::stemWord(_lcWord, _pos);

	const SynNode* anchorHead = parse->getRoot()->getNthTerminal(token_index)->getParent()->getHeadPreterm();
	_triggerSyntacticType = Symbol(L"OTHER");	
	if(LanguageSpecificFunctions::isNounPOS(anchorHead->getTag())){
		if(NodeInfo::isNominalPremod(anchorHead)){ 
			_triggerSyntacticType = Symbol(L"NOM_PREMOD");	
		}
		else{
			_triggerSyntacticType = Symbol(L"NOM_HEAD");	
		}
	}
	else if(LanguageSpecificFunctions::isVerbPOS(anchorHead->getTag())){
		_triggerSyntacticType = Symbol(L"VERB_HEAD");	
	}

	if (tokens->getNTokens() - 1 == token_index)
		_nextWord = Symbol(L"END");
	else _nextWord = tokens->getToken(token_index+1)->getSymbol();
	if (tokens->getNTokens() - 2 <= token_index)
		_secondNextWord = Symbol(L"END");
	else _secondNextWord = tokens->getToken(token_index+2)->getSymbol();
	if (token_index == 0)
		_prevWord = Symbol(L"START");
	else _prevWord = tokens->getToken(token_index-1)->getSymbol();
	if (token_index <= 1)
		_secondPrevWord = Symbol(L"START");
	else _secondPrevWord = tokens->getToken(token_index-2)->getSymbol();
	if (use_wordnet)
		_n_offsets = SymbolUtilities::fillWordNetOffsets(_stemmedWord, _pos,
							_wordnetOffsets, ETO_MAX_WN_OFFSETS);
	else _n_offsets = 0;
	_wordCluster = WordClusterClass(_lcWord, true);
	_wordClusterMC = WordClusterClass(_word);
	_documentTopic = Symbol();
	_is_nominal_premod = NodeInfo::isNominalPremod(parse->getRoot()->getNthTerminal(token_index)->getParent());
	_is_copula = false;	

	Proposition *triggerProp = 0;
	const SynNode *triggerNode = parse->getRoot()->getNthTerminal(token_index);
	triggerNode = triggerNode->getParent();
	if (propSet != 0) {
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			if (propSet->getProposition(i)->getPredHead() != 0 &&
				propSet->getProposition(i)->getPredHead()->getHeadPreterm() == triggerNode) 
			{
				triggerProp = propSet->getProposition(i);
				break;
			}
		}	
	}
	_objectOfTrigger = Symbol();
	_indirectObjectOfTrigger = Symbol();
	_subjectOfTrigger = Symbol();
	for (int i = 0; i < _MAX_OTHER_ARGS; i++) {
		_otherArgsToTrigger[i] = Symbol();
	}
	int n_args = 0;
	if (triggerProp != 0) {
		if (triggerProp->getPredType() == Proposition::COPULA_PRED)
			_is_copula = true;
		for (int argnum = 0; argnum < triggerProp->getNArgs(); argnum++) {
			Argument *arg = triggerProp->getArg(argnum);
			if (arg->getType() != Argument::MENTION_ARG) 
				continue;
			if (arg->getRoleSym() == Argument::REF_ROLE ||
				arg->getRoleSym() == Argument::TEMP_ROLE ||
				arg->getRoleSym() == Argument::LOC_ROLE ||
				arg->getRoleSym() == Argument::MEMBER_ROLE ||
				arg->getRoleSym() == Argument::UNKNOWN_ROLE)
			{
				continue;
			}

			const Mention *ment = arg->getMention(mentionSet);
			Symbol sym;
			if (ment->getEntityType().isRecognized())
				sym = ment->getEntityType().getName();
			else sym = ment->getNode()->getHeadWord();
			
			if (arg->getRoleSym() == Argument::SUB_ROLE)
				_subjectOfTrigger = sym;
			else if (arg->getRoleSym() == Argument::OBJ_ROLE)
				_objectOfTrigger = sym;
			else if (arg->getRoleSym() == Argument::IOBJ_ROLE)
				_indirectObjectOfTrigger = sym;
			else if (arg->getRoleSym() == Argument::POSS_ROLE && n_args < _MAX_OTHER_ARGS) {
				std::wstring str = L"poss:";
				str += sym.to_string();
				_otherArgsToTrigger[n_args++] = Symbol(str.c_str());
			} else if (n_args < _MAX_OTHER_ARGS) {
				std::wstring str = arg->getRoleSym().to_string();
				str += L":";
				str += sym.to_string();
				_otherArgsToTrigger[n_args++] = Symbol(str.c_str());
			} 
		}
	}
}


bool EventTriggerObservation::isLastWord() { 	
	return (_nextWord == Symbol(L"END"));
}

bool EventTriggerObservation::isFirstWord() { 
	return (_prevWord == Symbol(L"START"));
}	

Symbol EventTriggerObservation::getWord() { 
	return _word;
}

Symbol EventTriggerObservation::getPOS() { 
	return _pos;
}

Symbol EventTriggerObservation::getStemmedWord() { 
	return _stemmedWord;
}

Symbol EventTriggerObservation::getLCWord() { 
	return _lcWord;
}

Symbol EventTriggerObservation::getNextWord() { 
	return _nextWord;
}

Symbol EventTriggerObservation::getSecondNextWord() { 
	return _secondNextWord;
}

Symbol EventTriggerObservation::getPrevWord() { 
	return _prevWord;
}

Symbol EventTriggerObservation::getSecondPrevWord() { 
	return _secondPrevWord;
}

Symbol EventTriggerObservation::getDocumentTopic() { 
	return _documentTopic;
}

void EventTriggerObservation::setDocumentTopic(Symbol topic) { 
	_documentTopic = topic;
}

int EventTriggerObservation::getReversedNthOffset(int n) {
	if (n < _n_offsets) {
		return _wordnetOffsets[_n_offsets - n - 1];
	} else return -1;	
}

int EventTriggerObservation::getNthOffset(int n) {
	if (n < _n_offsets) {
		return _wordnetOffsets[n];
	} else return -1;	
}
int EventTriggerObservation::getNOffsets() {
	return _n_offsets;
}

WordClusterClass EventTriggerObservation::getWordCluster() {
	return _wordCluster;
}

WordClusterClass EventTriggerObservation::getWordClusterMC() {
	return _wordClusterMC;
}

Symbol EventTriggerObservation::getObjectOfTrigger() {
	return _objectOfTrigger;
}
Symbol EventTriggerObservation::getIndirectObjectOfTrigger() {
	return _indirectObjectOfTrigger;
}
Symbol EventTriggerObservation::getSubjectOfTrigger() {
	return _subjectOfTrigger;
}
Symbol EventTriggerObservation::getOtherArgToTrigger(int n) {
	if (n < 0 || n >= _MAX_OTHER_ARGS)
		return Symbol();
	return _otherArgsToTrigger[n];
}

bool EventTriggerObservation::isNominalPremod() {
	return _is_nominal_premod;
}

bool EventTriggerObservation::isCopula() {
	return _is_copula;
}
