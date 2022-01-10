// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/parse/STags.h"
#include "Generic/wordnet/xx_WordNet.h"

#include "Generic/common/version.h"


const Symbol EventModalityObservation::_className(L"event-modality");
const int EventModalityObservation::_MAX_OTHER_ARGS = ETO_MAX_OTHER_ARGS;

SymbolHash * EventModalityObservation::_nonAssertedIndicators;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsNoun;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsVerb;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsMD;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsAdj;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsAdv;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsNearby;
SymbolHash * EventModalityObservation::_nonAssertedIndicatorsOther;
SymbolHash * EventModalityObservation::_verbsCausingEvents;

namespace {
	static Symbol allegedly_sym(L"allegedly_sym");
}


boost::shared_ptr<EventModalityObservation::Factory> &EventModalityObservation::_factory() {
	static boost::shared_ptr<EventModalityObservation::Factory> factory(new GenericEventModalityObservationFactory());
	return factory;
}

DTObservation *EventModalityObservation::makeCopy() {
	EventModalityObservation *copy = EventModalityObservation::build();
	copy->populate(this);
	return copy;
}


void EventModalityObservation::populate(EventModalityObservation *other) {
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


	_ledbyAllegedAdverb = other->isLedbyAllegedAdverb();
	_ledbyModalWord = other->isLedbyModalWord();
	_followedbyIFWord = other->isFollowedbyIFWord();
	_ledbyIFWord = other->isLedbyIFWord();
	_parentIsLikelyNonAsserted = other->hasNonAssertedParent();
	_isNonAssertedbyRules = other->isNonAssertedbyRules();
	_modifyingNoun = other->isModifierOfNoun();
	_nounModifyingNoun = other->isNounModifierOfNoun();


	_hasIndicatorAboveS = other->hasIndicatorsAboveS();
	_hasIndicatorAboveVP = other->hasIndicatorsAboveVP();
	_hasIndicatorModifyingNoun = other->hasIndicatorsModifyingNoun();
	_hasIndicatorNearby = other->hasIndicatorsNearby();
	_hasIndicatorMDAboveVP = other->hasIndicatorsMDAboveVP();
	_numIndicatorModifyingNoun = other->getNIndicatorsModifyingNoun();	
	_numIndicatorAboveS = other->getNIndicatorsAboveS();
	_numIndicatorAboveVP = other->getNIndicatorsAboveVP();
	_numIndicatorNearby = other->getNIndicatorsNearby();
	_numIndicatorMDAboveVP = other->getNIndicatorsMDAboveVP();


	_hasRichFeatures=other->hasRichFeatures();
	_isPremodOfNP=other->isPremodOfNP();
	_isPremodOfMention=other->isPremodOfMention();
	_mentionType = other->getMentionType();
	_entityType = other->getEntityType();


	Symbol *tempIndicatorLs = other->getIndicatorsAboveS();
	for (int i = 0; i<_numIndicatorAboveS; i++){
		_indicatorAboveS[i] = tempIndicatorLs[i];
	}

	tempIndicatorLs = other->getIndicatorsAboveVP();
	for (int i = 0; i<_numIndicatorAboveVP; i++){
		_indicatorAboveVP[i] = tempIndicatorLs[i];
	}

	tempIndicatorLs = other->getIndicatorsModifyingNoun();
	for (int i = 0; i<_numIndicatorModifyingNoun; i++){
		_indicatorModifyingNoun[i] = tempIndicatorLs[i];
	}

	tempIndicatorLs = other->getIndicatorsNearby();
	for (int i = 0; i<_numIndicatorNearby; i++){
		_indicatorNearby[i] = tempIndicatorLs[i];
	}

	tempIndicatorLs = other->getIndicatorsMDAboveVP();
	for (int i = 0; i<_numIndicatorMDAboveVP; i++){
		_indicatorMDAboveVP[i] = tempIndicatorLs[i];
	}


}

void EventModalityObservation::initializeStaticVariables() {
	static bool init = false;
	if (!init) {
		init = true;
		std::string filename = ParamReader::getParam("non_asserted_event_indicators");
		if (!filename.empty()) {
			_nonAssertedIndicators = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicators = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_noun");
		if (!filename.empty()) {
			_nonAssertedIndicatorsNoun = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsNoun = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_verb");
		if (!filename.empty()) {
			_nonAssertedIndicatorsVerb = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsVerb = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_adj");
		if (!filename.empty()) {
			_nonAssertedIndicatorsAdj = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsAdj = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_adv");
		if (!filename.empty()) {
			_nonAssertedIndicatorsAdv = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsAdv = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_MD");
		if (!filename.empty()) {
			_nonAssertedIndicatorsMD = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsMD = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_mixed");
		if (!filename.empty()) {
			_nonAssertedIndicatorsNearby = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsNearby = new SymbolHash(5);

		filename = ParamReader::getParam("non_asserted_event_indicators_other");
		if (!filename.empty()) {
			_nonAssertedIndicatorsOther = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicatorsOther = new SymbolHash(5);

		filename = ParamReader::getParam("verbs_causing_events");
		if (!filename.empty()) {
			_verbsCausingEvents = new SymbolHash(filename.c_str());
		}else _verbsCausingEvents = new SymbolHash(5);

	}
}

void EventModalityObservation::populate(int token_index, 
									   const TokenSequence *tokens, 
									   const Parse *parse, 
									   MentionSet *mentionSet, 
									   const PropositionSet *propSet, 
									   bool use_wordnet) 
{ 
	// Set default values.  All ints default to zero, and all booleans
	// default to false.
	_n_offsets = 0;
	_is_nominal_premod = false;
	_is_copula = false;
	_ledbyAllegedAdverb = false;
	_ledbyModalWord = false;
	_followedbyIFWord = false;
	_ledbyIFWord = false;
	_parentIsLikelyNonAsserted = false;
	_isNonAssertedbyRules = false;
	_modifyingNoun = false;
	_nounModifyingNoun = false;
	_hasIndicatorAboveS = false;
	_hasIndicatorAboveVP = false;
	_hasIndicatorModifyingNoun = false;
	_hasIndicatorNearby = false;
	_hasIndicatorMDAboveVP = false;
	_numIndicatorModifyingNoun = 0;	
	_numIndicatorAboveS = 0;
	_numIndicatorAboveVP = 0;
	_numIndicatorNearby = 0;
	_numIndicatorMDAboveVP = 0;
	_hasRichFeatures = false;
	_classifierNo = 0;
	_isPremodOfMention = false;
	_isPremodOfNP = false;

	// features borrowed from EventTriggerFinder
	
	const SynNode *currentNode = parse->getRoot()->getNthTerminal(token_index)->getParent();
	_word = tokens->getToken(token_index)->getSymbol();
	_lcWord = SymbolUtilities::lowercaseSymbol(_word);
	//_pos = parse->getRoot()->getNthTerminal(token_index)->getParent()->getTag();
	_pos = currentNode->getTag();
	
	_stemmedWord = SymbolUtilities::stemWord(_lcWord, _pos);
	
	if (tokens->getNTokens() - 1 == token_index){
		_nextWord = Symbol(L"END");
		_nextPOS = Symbol(L"END");
	}else{
		_nextWord = tokens->getToken(token_index+1)->getSymbol();
		_nextPOS = parse->getRoot()->getNthTerminal(token_index+1)->getParent()->getTag();
	}

	if (tokens->getNTokens() - 2 <= token_index)
		_secondNextWord = Symbol(L"END");
	else _secondNextWord = tokens->getToken(token_index+2)->getSymbol();
	
	if (token_index == 0){
		_prevWord = Symbol(L"START");
		_prevPOS = Symbol(L"START");
	}else {
		_prevWord = tokens->getToken(token_index-1)->getSymbol();
		_prevPOS = parse->getRoot()->getNthTerminal(token_index-1)->getParent()->getTag();
	}

	if (token_index <= 1)
		_secondPrevWord = Symbol(L"START");
	else _secondPrevWord = tokens->getToken(token_index-2)->getSymbol();
	if (use_wordnet)
		_n_offsets = SymbolUtilities::fillWordNetOffsets(_stemmedWord, _pos,
							_wordnetOffsets, ETO_MAX_WN_OFFSETS);
	else _n_offsets = 0;
	
	_wordCluster = WordClusterClass(_lcWord, true);
	_documentTopic = Symbol();
	_is_nominal_premod = NodeInfo::isNominalPremod(parse->getRoot()->getNthTerminal(token_index)->getParent());



	Proposition *prop = 0;
	const SynNode *triggerNode = parse->getRoot()->getNthTerminal(token_index);
	triggerNode = triggerNode->getParent();
	if (propSet != 0) {
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			if (propSet->getProposition(i)->getPredHead() != 0 &&
				propSet->getProposition(i)->getPredHead()->getHeadPreterm() == triggerNode) 
			{
				prop = propSet->getProposition(i);
				break;
			}
		}	
	}

	if (prop) {
		_is_copula = (prop->getPredType() == Proposition::COPULA_PRED);
		_ledbyAllegedAdverb = isLedbyAllegedAdverb(prop);
		_ledbyModalWord = isLedbyModalWord(prop);
		_followedbyIFWord = isFollowedbyIFWord(prop);
		_ledbyIFWord = isLedbyIFWord(prop);
		_parentIsLikelyNonAsserted = parentIsLikelyNonAsserted(prop, propSet, mentionSet);
	}
	
	// test whether the predicate word is a modifier noun  
	std::string POStxt;
	POStxt = _pos.to_debug_string();
	const SynNode *parentNode = currentNode->getParent() ;
	std::string parentPOStxt = parentNode->getTag().to_debug_string();
	if (parentPOStxt[0] == 'N' && parentPOStxt[1] == 'P'){
		if (currentNode != currentNode->getParent()->getHead()){
			_modifyingNoun = true;
			if ( POStxt[0] == 'N'){
				_nounModifyingNoun = true;
			}
		}
	}

    findIndicators(token_index, tokens, parse, mentionSet);

	// option 1
	_hasRichFeatures = _hasIndicatorNearby || _hasIndicatorAboveS || 
		_hasIndicatorAboveVP || _hasIndicatorMDAboveVP || _hasIndicatorModifyingNoun || 
		_isNonAssertedbyRules || _followedbyIFWord || _ledbyModalWord ||
	    _ledbyIFWord;
	
	// option 1.1
	/*
	_hasRichFeatures = _hasIndicatorNearby || _hasIndicatorAboveS || 
		_hasIndicatorAboveVP || _hasIndicatorMDAboveVP || _hasIndicatorModifyingNoun || 
		_isNonAssertedbyRules || _followedbyIFWord || _ledbyModalWord ||
	    _ledbyIFWord || _isPremodOfNP || _isPremodOfMention;
	*/

	// option 2
	/*
	_hasRichFeatures = _hasIndicatorAboveS || 
		_hasIndicatorAboveVP || _hasIndicatorMDAboveVP || _hasIndicatorModifyingNoun || 
		_isNonAssertedbyRules || _followedbyIFWord || _ledbyModalWord ||
	    _ledbyIFWord;
	*/

	// option 3
	/*
	_hasRichFeatures = _hasIndicatorAboveS || 
		_hasIndicatorAboveVP || _hasIndicatorModifyingNoun || 
		_isNonAssertedbyRules || _followedbyIFWord || _ledbyModalWord ||
	    _ledbyIFWord;
	*/

	// option 4
	/*
	_hasRichFeatures = _hasIndicatorNearby || _hasIndicatorAboveS || 
		_hasIndicatorAboveVP || _hasIndicatorModifyingNoun || 
		_isNonAssertedbyRules || _followedbyIFWord || _ledbyModalWord ||
	    _ledbyIFWord;
	*/

	/*
	_objectOfTrigger = Symbol();
	_indirectObjectOfTrigger = Symbol();
	_subjectOfTrigger = Symbol();
	for (int i = 0; i < _MAX_OTHER_ARGS; i++) {
		_otherArgsToTrigger[i] = Symbol();
	}
	int n_args = 0;
	if (prop != 0) {
		if (prop->getPredType() == Proposition::COPULA_PRED)
			_is_copula = true;
		for (int argnum = 0; argnum < prop->getNArgs(); argnum++) {
			Argument *arg = prop->getArg(argnum);
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
	*/
}

bool EventModalityObservation::isLikelyNonAsserted (Proposition *prop) const{
	return (prop->getAdverb() && 
			prop->getAdverb()->getHeadWord() == allegedly_sym);
}

std::vector<bool> EventModalityObservation::identifyNonAssertedProps(const PropositionSet *propSet, 
																	 const MentionSet *mentionSet) const
{
	std::vector<bool> isNonAsserted(propSet->getNPropositions(), false);

	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);
		if (isLikelyNonAsserted (prop)){
			isNonAsserted[prop->getIndex()] = true;
		}
	}

	return isNonAsserted;
}



bool EventModalityObservation::isLastWord() { 	
	return (_nextWord == Symbol(L"END"));
}

bool EventModalityObservation::isFirstWord() { 
	return (_prevWord == Symbol(L"START"));
}	

Symbol EventModalityObservation::getWord() { 
	return _word;
}

Symbol EventModalityObservation::getPOS() { 
	return _pos;
}

Symbol EventModalityObservation::getStemmedWord() { 
	return _stemmedWord;
}

Symbol EventModalityObservation::getLCWord() { 
	return _lcWord;
}

Symbol EventModalityObservation::getNextWord() { 
	return _nextWord;
}

Symbol EventModalityObservation::getNextPOS() { 
	return _nextPOS;
}

Symbol EventModalityObservation::getSecondNextWord() { 
	return _secondNextWord;
}

Symbol EventModalityObservation::getPrevWord() { 
	return _prevWord;
}

Symbol EventModalityObservation::getPrevPOS() { 
	return _prevPOS;
}

Symbol EventModalityObservation::getSecondPrevWord() { 
	return _secondPrevWord;
}

Symbol EventModalityObservation::getDocumentTopic() { 
	return _documentTopic;
}

void EventModalityObservation::setDocumentTopic(Symbol topic) { 
	_documentTopic = topic;
}

int EventModalityObservation::getReversedNthOffset(int n) {
	if (n < _n_offsets) {
		return _wordnetOffsets[_n_offsets - n - 1];
	} else return -1;	
}

int EventModalityObservation::getNthOffset(int n) {
	if (n < _n_offsets) {
		return _wordnetOffsets[n];
	} else return -1;	
}
int EventModalityObservation::getNOffsets() {
	return _n_offsets;
}

WordClusterClass EventModalityObservation::getWordCluster() {
	return _wordCluster;
}

Symbol EventModalityObservation::getObjectOfTrigger() {
	return _objectOfTrigger;
}
Symbol EventModalityObservation::getIndirectObjectOfTrigger() {
	return _indirectObjectOfTrigger;
}
Symbol EventModalityObservation::getSubjectOfTrigger() {
	return _subjectOfTrigger;
}
Symbol EventModalityObservation::getOtherArgToTrigger(int n) {
	if (n < 0 || n >= _MAX_OTHER_ARGS)
		return Symbol();
	return _otherArgsToTrigger[n];
}

bool EventModalityObservation::isNominalPremod() {
	return _is_nominal_premod;
}

bool EventModalityObservation::isCopula() {
	return _is_copula;
}


bool EventModalityObservation::isLedbyAllegedAdverb() {
	return _ledbyAllegedAdverb;
}

bool EventModalityObservation::isLedbyModalWord() {
	return _ledbyModalWord;
}

bool EventModalityObservation::isFollowedbyIFWord() {
	return _followedbyIFWord;
}

bool EventModalityObservation::isLedbyIFWord(){
	return _ledbyIFWord;
}

bool EventModalityObservation::hasNonAssertedParent(){
	return _parentIsLikelyNonAsserted;
}
	
bool EventModalityObservation::isNonAssertedbyRules(){
	return _isNonAssertedbyRules;
}

bool EventModalityObservation::isModifierOfNoun(){
	return _modifyingNoun;
}

bool EventModalityObservation::isNounModifierOfNoun(){
	return _nounModifyingNoun;
}


void EventModalityObservation::finalize()
{
	
	if (_nonAssertedIndicators != 0){
		delete _nonAssertedIndicators;
		_nonAssertedIndicators = 0;
	}

	if (_nonAssertedIndicatorsNoun != 0){
		delete _nonAssertedIndicatorsNoun;
		_nonAssertedIndicatorsNoun = 0;
	}

	if (_nonAssertedIndicatorsVerb != 0){
		delete _nonAssertedIndicatorsVerb;
		_nonAssertedIndicatorsVerb = 0;
	}
	
	if (_nonAssertedIndicatorsMD != 0){
		delete _nonAssertedIndicatorsMD;
		_nonAssertedIndicatorsMD = 0;
	}

	if (_nonAssertedIndicatorsAdj != 0){
		delete _nonAssertedIndicatorsAdj;
		_nonAssertedIndicatorsAdj = 0;
	}

	if (_nonAssertedIndicatorsAdv != 0){
		delete _nonAssertedIndicatorsAdv;
		_nonAssertedIndicatorsAdv =0;
	}

	if (_nonAssertedIndicatorsNearby != 0){
		delete _nonAssertedIndicatorsNearby;
		_nonAssertedIndicatorsNearby =0;
	}

	if (_nonAssertedIndicatorsOther != 0){
		delete _nonAssertedIndicatorsOther;
		_nonAssertedIndicatorsOther =0;
	}

	if (_verbsCausingEvents  != 0){
		delete _verbsCausingEvents ;
		_verbsCausingEvents  =0;
	}
	
}
