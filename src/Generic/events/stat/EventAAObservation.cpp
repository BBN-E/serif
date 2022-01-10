// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"

#include "Generic/common/UnicodeUtil.h"
#include <string>
//#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>


const Symbol EventAAObservation::_className(L"arg-attachment");
void EventAAObservation::initializeEventAAObservation(const TokenSequence *tokens, const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, const PropositionSet *propSet, 
		const EventMention *vMention, const Mention* mention)
{
	resetForNewSentence(tokens, parse, mentionSet, valueMentionSet, propSet);
	setEvent(vMention);
	setCandidateArgument(mention);
}
void EventAAObservation::initializeEventAAObservation(const TokenSequence *tokens, const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, const PropositionSet *propSet, 
		const EventMention *vMention, const ValueMention* valueMention)
{
	resetForNewSentence(tokens, parse, mentionSet, valueMentionSet, propSet);
	setEvent(vMention);
	setCandidateArgument(valueMention);
}

DTObservation *EventAAObservation::makeCopy() {
	EventAAObservation *copy = _new EventAAObservation();

	copy->resetForNewSentence(_tokens, _parse, _mentionSet, _valueMentionSet, _propSet);
	return copy;
}

void EventAAObservation::setEvent(const EventMention *vMention) {
	_vMention = vMention;

	_stemmedTrigger = SymbolUtilities::stemWord(_vMention->getAnchorNode()->getHeadWord(), 
		_vMention->getAnchorNode()->getTag());
	_triggerWC = WordClusterClass(_stemmedTrigger, true);

}

void EventAAObservation::setCandidateArgument(const Mention *mention) {
	_candidateArgument = mention;
	_candidateValue = 0;
	_candidateType = mention->getEntityType().getName();
	_candidateHeadword = mention->getNode()->getHeadWord();
	setCandidateRoleInTriggerProp();
	setConnectingProp();
	setConnectingString();
	setConnectingParsePath();
	setDistance();
	setNCandidatesOfSameType();

	//setSynFeatures();
	//setCandidatesOfSameType();		
	//setAnchorArgRelativePosition();
}


void EventAAObservation::setCandidateArgument(const ValueMention *valueMention) {
	_candidateArgument = findMentionForValue(valueMention);
	_candidateValue = valueMention;
	_candidateType = valueMention->getType();
	if (_candidateArgument) {
		_candidateHeadword = _candidateArgument->getNode()->getHeadWord();
	} else {
		_candidateHeadword = _parse->getRoot()->getNthTerminal(valueMention->getEndToken())->getHeadWord();
	}
	setCandidateRoleInTriggerProp();
	setConnectingProp();
	setConnectingString();
	setConnectingParsePath();
	setDistance();
	setNCandidatesOfSameType();

	//setSynFeatures();
	//setCandidatesOfSameType();		
	//setAnchorArgRelativePosition();	
}


// "the people were moved" --> "PER were moved"
// "he said by telephone from Belgium" --> "telephone from GPE"	
void EventAAObservation::setConnectingString() {
	int start_candidate;
	int end_candidate;
	if (_candidateArgument) {
		start_candidate = _candidateArgument->getNode()->getStartToken();
		end_candidate = _candidateArgument->getNode()->getEndToken();
	} else {
		start_candidate = _candidateValue->getStartToken();
		end_candidate = _candidateValue->getEndToken();
	}
	int trigger_index = _vMention->getAnchorNode()->getHeadPreterm()->getStartToken();
	

	_connectingString = Symbol();
	_stemmedConnectingString = Symbol();
	_posConnectingString = Symbol();
	_abbrevConnectingString = Symbol();
	if (trigger_index < start_candidate) {
		makeConnectingStringSymbol(trigger_index, start_candidate, false);
	} else if (end_candidate < trigger_index) {
		makeConnectingStringSymbol(end_candidate + 1, trigger_index + 1, true);	
	}  
}


void EventAAObservation::setDistance() {
	int start_candidate;
	int end_candidate;
	if (_candidateArgument) {
		start_candidate = _candidateArgument->getNode()->getStartToken();
		end_candidate = _candidateArgument->getNode()->getEndToken();
	} else {
		start_candidate = _candidateValue->getStartToken();
		end_candidate = _candidateValue->getEndToken();
	}
	int trigger_index = _vMention->getAnchorNode()->getHeadPreterm()->getStartToken();

	if (trigger_index < start_candidate)
		_distance = start_candidate - trigger_index;
	else if (end_candidate < trigger_index)
		_distance = trigger_index - end_candidate;
	else _distance = 0;

}



void EventAAObservation::setNCandidatesOfSameType() {
	_n_candidates_of_same_type = 0;
	if (_candidateValue != 0) {
		for (int i = 0; i < _valueMentionSet->getNValueMentions(); i++) {
			const ValueMention *other = _valueMentionSet->getValueMention(i);
			if (_candidateValue == other)
				continue;
			if (other->getFullType() == _candidateValue->getFullType())
				_n_candidates_of_same_type++;
		}
	}
	else if (_candidateArgument != 0) {
		for (int i = 0; i < _mentionSet->getNMentions(); i++) {
			const Mention *other = _mentionSet->getMention(i);
			if (other->getMentionType() != Mention::NAME &&
				other->getMentionType() != Mention::DESC &&
				other->getMentionType() != Mention::PRON)
				continue;
			if (_candidateArgument == other)
				continue;
			if (_candidateArgument->getEntityType() == other->getEntityType())
				_n_candidates_of_same_type++;
		}
	}
}




void EventAAObservation::setCandidateRoleInTriggerProp() {
	// "his trial" --> <poss>

	_roleInTriggerProp = Symbol();
	if (_vMention->getAnchorProp() == 0)
		return;
	for (int i = 0; i < _vMention->getAnchorProp()->getNArgs(); i++) {
		Argument *arg = _vMention->getAnchorProp()->getArg(i);
		if (arg->getType() == Argument::MENTION_ARG &&
			_candidateArgument != 0 &&
			arg->getMentionIndex() == _candidateArgument->getIndex()) 
		{
			_roleInTriggerProp = arg->getRoleSym();
			return;
		}
		else if (arg->getType() == Argument::TEXT_ARG &&
				_candidateArgument != 0 &&
				arg->getNode()->getStartToken() == _candidateArgument->getNode()->getStartToken() &&
				arg->getNode()->getEndToken() == _candidateArgument->getNode()->getEndToken())
		{
			_roleInTriggerProp = arg->getRoleSym();
			return;
		}
		else if (arg->getType() == Argument::TEXT_ARG &&
				_candidateValue != 0 &&
				arg->getNode()->getStartToken() == _candidateValue->getStartToken() &&
				arg->getNode()->getEndToken() == _candidateValue->getEndToken())
		{
			_roleInTriggerProp = arg->getRoleSym();
			return;
		}
	}
	
	if (_candidateArgument != 0) {
		for (int i = 0; i < _vMention->getAnchorProp()->getNArgs(); i++) {
			Argument *arg = _vMention->getAnchorProp()->getArg(i);
			if (arg->getType() == Argument::MENTION_ARG) 
			{
				Proposition *def = _propSet->getDefinition(arg->getMentionIndex());
				if (!def) continue;
				for (int j = 0; j < def->getNArgs(); j++) {
					Argument *defarg = def->getArg(j);
					if (defarg->getType() == Argument::MENTION_ARG &&
						defarg->getMentionIndex() == _candidateArgument->getIndex())
					{
						std::wstring str = arg->getRoleSym().to_string();
						str += L":";
						str += defarg->getRoleSym().to_string();
						_roleInTriggerProp = Symbol(str.c_str());
						return;
					}
				}
			}
		}
	}

}

// 10/16/2013 Yee Seng Chan
// _connectingProp is for experiment purposes. Would be useful to eyeball info from connecting proposition
void EventAAObservation::setConnectingProp() {
	// "he won the election" --> <sub> <obj> win

	_candidateRoleInCP = Symbol();
	_eventRoleInCP = Symbol();
	_stemmedCPPredicate = Symbol();	

	_connectingProp = 0;

	if (_candidateArgument == 0)
		return;

	for (int i = 0; i < _propSet->getNPropositions(); i++) {
		const Proposition *prop = _propSet->getProposition(i);
		for (int argnum = 0; argnum < prop->getNArgs(); argnum++) {
			Argument *arg = prop->getArg(argnum);
		//	wstring aa=_vMention->getAnchorNode()->toDebugTextString();
		//	wstring bb=_candidateArgument->toCasedTextString();
			if (arg->getType() == Argument::MENTION_ARG) {
				if (arg->getMentionIndex() == _candidateArgument->getIndex()) 
					_candidateRoleInCP = arg->getRoleSym();
				else if (_vMention->getAnchorNode() ==
					arg->getMention(_mentionSet)->getNode()->getHeadPreterm()) 
				{
					_eventRoleInCP = arg->getRoleSym();
				}
			} else if (arg->getType() == Argument::PROPOSITION_ARG &&
				arg->getProposition() == _vMention->getAnchorProp())
			{
				_eventRoleInCP = arg->getRoleSym();
			}
		}
		if (!_eventRoleInCP.is_null() && 
			!_candidateRoleInCP.is_null() &&
			prop->getPredHead() != 0) 
		{
			_stemmedCPPredicate = SymbolUtilities::stemWord(prop->getPredHead()->getHeadWord(),
				prop->getPredHead()->getHeadPreterm()->getTag());
			_connectingProp = _propSet->getProposition(i);
			break;
		} else {
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
		}
	}

	_oldRoleInTriggerProp = _roleInTriggerProp;
	_oldEventRoleInCP = _eventRoleInCP;
	_oldCandidateRoleInCP = _candidateRoleInCP;

	// handle cases where prop feature is <ref>
	Symbol refSym = Symbol(L"<ref>");
	std::string eventAARefMode = ParamReader::getParam("event_aa_ref_mode");

	if(eventAARefMode=="ALLSHIFT") {
		if(!_eventRoleInCP.is_null() && _eventRoleInCP==refSym) {		// eventRoleInCP=<ref>
			if(_roleInTriggerProp.is_null())
				_roleInTriggerProp = _candidateRoleInCP;
			// I had assumed that either roleInTriggerProp=NULL or roleInTriggerProp==candidateRoleInCP
			// there are a few cases where roleInTriggerProp!=NULL and !=candidateRoleInCP; but the logic here remains correct
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
		}	
		if(!_candidateRoleInCP.is_null() && _candidateRoleInCP==refSym) {	// candidateRoleInCP=<ref>
			if(_roleInTriggerProp.is_null())
				_roleInTriggerProp = _eventRoleInCP;
			// I had assumed that either roleInTriggerProp=NULL or roleInTriggerProp==eventRoleInCP
			// there are a few cases where roleInTriggerProp!=NULL and !=eventRoleInCP; but the logic here remains correct
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
		}	
		if(!_roleInTriggerProp.is_null() && !_eventRoleInCP.is_null() && !_candidateRoleInCP.is_null()) {
			// neither _eventRoleInCP nor _candidateRoleInCP is <ref>, but we will go ahead and remove these
			// it is mostly appropriate to do so (i.e. some examples would benefit from more information by keeping eventRoleInCP and candidateRoleInCP)
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
			//_oldRoleInTriggerProp = Symbol(L"OO:" + std::wstring(_oldRoleInTriggerProp.to_string()));
		}
	}
	else if(eventAARefMode=="SHIFT") {
		if(!_roleInTriggerProp.is_null() && !_eventRoleInCP.is_null() && !_candidateRoleInCP.is_null()) {
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
		}
	}
	else if(eventAARefMode=="EVENTCP") {
		if(!_eventRoleInCP.is_null() && _eventRoleInCP==refSym) {
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
			// if roleInTriggerProp==NULL, we are losing information when we do not transfer candidateRoleInCP to roleInTriggerProp
		}	
		if(!_roleInTriggerProp.is_null() && !_eventRoleInCP.is_null() && !_candidateRoleInCP.is_null()) {
			_eventRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
		}
	}

}

bool EventAAObservation::isDirectProp() const {
	if(!_roleInTriggerProp.is_null())
		return true;
	else
		return false;
}

bool EventAAObservation::isSharedProp() const {
	if(_roleInTriggerProp.is_null() && !_candidateRoleInCP.is_null() && !_eventRoleInCP.is_null()) 
		return true;
	else
		return false;
}

bool EventAAObservation::isUnconnectedProp() const {
	if(_roleInTriggerProp.is_null() && _candidateRoleInCP.is_null() && _eventRoleInCP.is_null()) 
		return true;
	else
		return false;
}

const Mention *EventAAObservation::findMentionForValue(const ValueMention *value) {
	for (int mentid = 0; mentid < _mentionSet->getNMentions(); mentid++) {
		const Mention *mention = _mentionSet->getMention(mentid);
		if (mention->getNode()->getStartToken() == value->getStartToken() &&
			mention->getNode()->getEndToken() == value->getEndToken())
			return mention;
	}
	return 0;
}

void EventAAObservation::makeConnectingStringSymbol(int start, int end, bool candidate_in_front) 
{
	if (end - start > 10) {
		return;
	}
	std::wstring str = L"";
	std::wstring sstr = L"";
	std::wstring pstr = L"";
	std::wstring astr = L"";
	if (candidate_in_front) {
		str += getCandidateType().to_string();
		str += L"_";
		sstr += getCandidateType().to_string();
		sstr += L"_";
		pstr += getCandidateType().to_string();
		pstr += L"_";
		astr += getCandidateType().to_string();
		astr += L"_";
	}
	for (int i = start; i < end; i++) {
		bool mention_found = false;
		bool included = false;
		bool included_in_abbrev = false;
		for (int j = 0; j < _mentionSet->getNMentions(); j++) {
			const Mention *ment = _mentionSet->getMention(j);
			if (ment->getNode()->getStartToken() == i &&
				ment->getNode()->getEndToken() < end &&
				ment->getEntityType().isRecognized()) 
			{
				str += ment->getEntityType().getName().to_string();
				sstr += ment->getEntityType().getName().to_string();
				pstr += ment->getEntityType().getName().to_string();
				astr += ment->getEntityType().getName().to_string();
				i = ment->getNode()->getEndToken();
				mention_found = true;
				included = true;
				included_in_abbrev = true;
				break;
			}
		}
		bool value_found = false;
		if (!mention_found) {
			for	(int k = 0;	k <	_valueMentionSet->getNValueMentions(); k++)	{
				const ValueMention *ment = _valueMentionSet->getValueMention(k);
				if (ment->getStartToken() == i &&
					ment->getEndToken()	< end &&
					ment->getFullType().isDetermined())	
				{
					str	+= ment->getType().to_string();
					sstr +=	ment->getType().to_string();
					pstr +=	ment->getType().to_string();
					astr +=	ment->getType().to_string();
					i =	ment->getEndToken();
					value_found	= true;
					included = true;
					included_in_abbrev = true;
					break;
				}
			}
		}
		if (!mention_found && !value_found) {
			Symbol word = SymbolUtilities::lowercaseSymbol(_tokens->getToken(i)->getSymbol());
			Symbol tag = _parse->getRoot()->getNthTerminal(i)->getParent()->getTag();

			Symbol stem = SymbolUtilities::stemWord(word, tag);
			Symbol next_tag = Symbol();
			if (i + 1 < end)
				next_tag = _parse->getRoot()->getNthTerminal(i+1)->getParent()->getTag();
			if (EventUtilities::includeInConnectingString(tag, next_tag)) {
				str += word.to_string();
				sstr += stem.to_string();
				pstr += tag.to_string();
				included = true;
			}
			if (EventUtilities::includeInAbbreviatedConnectingString(tag, next_tag)) {
				astr += stem.to_string();
				included_in_abbrev = true;
			}
		}
		if (i != end - 1) {
			if (included) {
				str += L"_";
				sstr += L"_";
				pstr += L"_";
			}
			if (included_in_abbrev) astr += L"_";
		}
	}
	if (!candidate_in_front) {
		str += L"_";
		str += getCandidateType().to_string();
		sstr += L"_";
		sstr += getCandidateType().to_string();
		pstr += L"_";
		pstr += getCandidateType().to_string();
		astr += L"_";
		astr += getCandidateType().to_string();
	}
	_connectingString = Symbol(str.c_str());
	_stemmedConnectingString = Symbol(sstr.c_str());
	_posConnectingString = Symbol(pstr.c_str());
	_abbrevConnectingString = Symbol(astr.c_str());
}

void EventAAObservation::setConnectingParsePath() {
	_connectingCandParsePath = Symbol(L"-NONE-");
	_connectingTriggerParsePath = Symbol(L"-NONE-");

	const SynNode *root = _parse->getRoot();
	const SynNode *triggerNode = _vMention->getAnchorNode();
	const SynNode *candNode;
	int c_start, c_end;
	int t_start = triggerNode->getStartToken();
	int t_end = triggerNode->getStartToken();
	
	if (_candidateArgument != 0) {
		candNode = _candidateArgument->getNode();
		c_start = candNode->getStartToken();
		c_end = candNode->getEndToken();
	}
	else {  // _candidateValue != 0
		c_start = _candidateValue->getStartToken();
		c_end = _candidateValue->getEndToken();
		candNode = root->getNodeByTokenSpan(c_start, c_end);
	}

	if(candNode == 0)
		return;

	const SynNode* coveringNode = 0;
	if (c_start < t_start)
		coveringNode = root->getCoveringNodeFromTokenSpan(c_start, t_end);
	else if (t_start < c_start)
		coveringNode = root->getCoveringNodeFromTokenSpan(t_start, c_end);

	if (coveringNode == 0)
		return;

	int cdist = candNode->getAncestorDistance(coveringNode);
	int tdist = triggerNode->getAncestorDistance(coveringNode);

	//paths >5, not interesting ???
	if((cdist > 5) || (tdist > 5))
		return;

	const int buffsize = 150;

	wchar_t buffer[buffsize];
	int remaining_buffsize = buffsize - 1;
	wcscpy(buffer, L"path:");
	remaining_buffsize -= 5;
	const SynNode* temp = candNode;	
	while (temp != coveringNode) {
		wcsncat(buffer, temp->getTag().to_string(), remaining_buffsize);
		remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
		if (remaining_buffsize <= 0)
			break;
		wcsncat(buffer, L"_", remaining_buffsize);
		remaining_buffsize -= 1;
		if (remaining_buffsize <= 0)
			break;
		temp = temp->getParent();
		if(temp == 0)
			return;
	}
	//include coveringnode tag
	if (remaining_buffsize >= 0) {
		wcsncat(buffer, temp->getTag().to_string(), remaining_buffsize);
		remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
	}
	if (remaining_buffsize >= 0) {
		wcsncat(buffer, L"_", remaining_buffsize);
		remaining_buffsize -= 1;
	}
	_connectingCandParsePath = Symbol(buffer);
	

	remaining_buffsize = buffsize - 1;
	wcscpy(buffer, L"path:");
	remaining_buffsize -= 5;
	temp = triggerNode;
	while (temp != coveringNode) {
		wcsncat(buffer, temp->getTag().to_string(), remaining_buffsize);
		remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
		if (remaining_buffsize <= 0)
			break;
		wcsncat(buffer, L"_", remaining_buffsize);
		remaining_buffsize -= 1;
		if (remaining_buffsize <= 0)
			break;
		temp = temp->getParent();
		if(temp == 0)
			return;
	}
	//include coveringnode tag
	if (remaining_buffsize >= 0) {
		wcsncat(buffer, temp->getTag().to_string(), remaining_buffsize);
		remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
	}
	if (remaining_buffsize >= 0) {
		wcsncat(buffer, L"_", remaining_buffsize);
		remaining_buffsize -= 1;
	}
	_connectingTriggerParsePath = Symbol(buffer);
}


bool EventAAObservation::hasArgumentWithThisRole(Symbol role) {
	return (_vMention->getFirstMentionForSlot(role) != 0 ||
			_vMention->getFirstValueForSlot(role) != 0);
}

int EventAAObservation::calculateNumOfPropFeatures() {
	int numOfFiredPropFeatures = 0;

	if(!_roleInTriggerProp.is_null())
		numOfFiredPropFeatures += 1;
	if(!_eventRoleInCP.is_null())
		numOfFiredPropFeatures += 1;
	if(!_candidateRoleInCP.is_null())
		numOfFiredPropFeatures += 1;

	return numOfFiredPropFeatures;
}


// Distributional Knowledge BEGIN HERE

void EventAAObservation::setTokens() {
	// word tokens and pos-tags for this sentence in an easily accessible vector
	_wordTokens.clear();
	_wordTokensLower.clear();
	_posTokens.clear();
	_lemmaTokens.clear();
	_contentWords.clear();	// set lemmatized content words ; I define content words as nouns and verbs
	for(int i=0; i<_tokens->getNTokens(); i++) {
		Symbol tag = _parse->getRoot()->getNthTerminal(i)->getParent()->getTag();	// pos-tag
		_posTokens.push_back(tag);

		Symbol tok = _tokens->getToken(i)->getSymbol();					// token
		_wordTokens.push_back(tok);
		Symbol lcTok = SymbolUtilities::lowercaseSymbol(tok);				// lower-case token
		_wordTokensLower.push_back(lcTok);

		Symbol lemma = SymbolUtilities::stemWord(lcTok, tag);				// based on above, get lemma
		_lemmaTokens.push_back(lemma);

		std::wstring posString = std::wstring(tag.to_string());	
		if( boost::algorithm::starts_with(posString, L"NN") || boost::algorithm::starts_with(posString, L"VB") ) {	
			_contentWords.insert(lemma);	// insert this (noun or verb) lemma into my bag of content words
		}
	}

	setNPChunks();
}

void EventAAObservation::setNPChunks() {
	// should perhaps be moved to some other locations
	std::set<Symbol> npPrefixPostags;

	npPrefixPostags.insert( Symbol(L"NN") );
	npPrefixPostags.insert( Symbol(L"NNS") );
	npPrefixPostags.insert( Symbol(L"NNP") );
	npPrefixPostags.insert( Symbol(L"NNPS") );
	npPrefixPostags.insert( Symbol(L"DATE-NNP") );
	npPrefixPostags.insert( Symbol(L"JJ") );
	npPrefixPostags.insert( Symbol(L"JJR") );
	npPrefixPostags.insert( Symbol(L"JJS") );
	npPrefixPostags.insert( Symbol(L"CD") );
	npPrefixPostags.insert( Symbol(L"DT") );
	npPrefixPostags.insert( Symbol(L"PDT") );

	_nounPostags.insert( Symbol(L"NN") );
	_nounPostags.insert( Symbol(L"NNS") );
	_nounPostags.insert( Symbol(L"NNP") );
	_nounPostags.insert( Symbol(L"NNPS") );
	_nounPostags.insert( Symbol(L"DATE-NNP") );
	_nounPostags.insert( Symbol(L"PRP") );
	_nounPostags.insert( Symbol(L"PRP$") );

	_npChunks.clear();
	for(unsigned i=0; i<_posTokens.size(); i++) {
		Symbol postag = _posTokens[i];
		if( _nounPostags.find(postag) != _nounPostags.end() ) 
			_npChunks.push_back( Symbol(L"I") );
		else 
			_npChunks.push_back( Symbol(L"O") );
	}

	for(unsigned i=0; i<_npChunks.size(); i++) {
		if(_npChunks[i]==Symbol(L"I")) {
			unsigned j = i;
			while((j>0) && npPrefixPostags.find(_posTokens[j-1])!=npPrefixPostags.end()) {
				_npChunks[j-1] = Symbol(L"I");
				j -= 1;
			}
			_npChunks[j] = Symbol(L"B");
		}
	}
}

// Gather features from propositions that have the _candidateArgument as one of their args
// We go through each proposition in the current sentence. If a proposition has _candidateArgument as one of its args:
// - note the prop-role between proposition and _candidateArgument. Store pair <prop-role, prop predicate headword lemma> in _predAssocPropFeas
// - we consider all other arguments in the same proposition as neighbors of _candidateArgument. For each such 'neighbor' arg:
//     - note the prop-role between proposition and arg. Store pair <prop-role, headword lemma of arg> in _argAssocPropFeas
void EventAAObservation::setAssociatedProps() {

	_predAssocPropFeas.clear();
	_argAssocPropFeas.clear();

	// store mentions by their id
	std::map<int, Mention*> mentionMap;
	for(int i=0; i<_mentionSet->getNMentions(); i++) {
		Mention* m = _mentionSet->getMention(i);
		int id = m->getIndex();
		mentionMap[id] = m;
	}

	// store value mentions by their id
	std::map<int, ValueMention*> valueMap;
	for(int i=0; i<_valueMentionSet->getNValueMentions(); i++) {
		ValueMention* m = _valueMentionSet->getValueMention(i);
		int id = m->getIndex();
		valueMap[id] = m;
	}

	// store propositions by their id
	std::map<int, const Proposition*> propMap; 
	for(int i=0; i<_propSet->getNPropositions(); i++) {
		const Proposition *prop = _propSet->getProposition(i);
		int id = (prop->getID() % MAX_SENTENCE_PROPS);
		propMap[id] = prop;
	}


	std::vector<std::wstring> outBufLines;
	outBufLines.push_back(L"<associatedProps>");

	if(_candidateArgument!=0) {

		for(int i=0; i<_propSet->getNPropositions(); i++) {
			const Proposition *prop = _propSet->getProposition(i);
			std::string predTypeString(prop->getPredTypeString(prop->getPredType()));

			const SynNode* predHead = prop->getPredHead();
			if( (predHead!=0 && _candidateArgument!=0 && predHead==_candidateArgument->getHead()) || 
			     predTypeString.compare("set")==0 || predTypeString.compare("member")==0 ) {
				continue;
			}

			// check whether this proposition have the _candidateArgument as one of its args
			bool found=false;
			for(int argnum=0; argnum<prop->getNArgs(); argnum++) {	// go through each arg of current proposition
				Argument *arg = prop->getArg(argnum);
				if(arg->getType()==Argument::MENTION_ARG) {
					if(arg->getMentionIndex() == _candidateArgument->getIndex()) {
						found = true;
						break;
					}
				}
			}

			if(found) {		// this proposition has the _candidateArgument as one of its args
				std::wstring outBuf;
				Symbol predHeadSym, predHeadPosSym, predHeadLemma;

				predHeadLemma = Symbol();
				if(predHead!=0) {
					predHeadSym = predHead->getHeadWord();			// predicate headword of current proposition
					predHeadPosSym = predHead->getHeadPreterm()->getTag();	// pos-tag of predicate headword
					outBuf = std::wstring(predHeadSym.to_string()) + L"/" + std::wstring(predHeadPosSym.to_string()) + 
								L"<" + UnicodeUtil::toUTF16StdString(predTypeString) + L">(";
				}
				else {
					predHeadSym = Symbol();
					predHeadPosSym = Symbol();
					outBuf = L"_/_<" + UnicodeUtil::toUTF16StdString(predTypeString) + L">(";
				}

				// if the predicate headword is a noun or verb, get its lemma form
				if( !predHeadSym.is_null() && 
					(boost::algorithm::starts_with(predHeadPosSym.to_string(), L"NN") || 
					 boost::algorithm::starts_with(predHeadPosSym.to_string(), L"VB")) ) {
					predHeadLemma = SymbolUtilities::stemWord(predHeadSym, predHeadPosSym);
				}

				for(int argIndex=0; argIndex<prop->getNArgs(); argIndex++) {	// go through each arg of current proposition
					std::wstring roleString=L"";
					Symbol argSym, argPosSym, argLemma;
					bool isCandArg=false;

					Argument* arg = prop->getArg(argIndex);
					Symbol roleSym = arg->getRoleSym();
					if(!roleSym.is_null()) {
						roleString = roleSym.to_string();
					}
					if(predTypeString.compare("poss")==0) {
						roleString = L"poss";
						roleSym = Symbol(L"poss");
					}

					if(arg->getType() == Argument::MENTION_ARG) {		// arg is a mention
						int argMentionIndex = arg->getMentionIndex();
						if(mentionMap.find(argMentionIndex)==mentionMap.end()) {
							//std::cout << "mentionMap.find(" << argMentionIndex << ") returns null" << std::endl;
							continue;
						}
						Mention* m = mentionMap.find(argMentionIndex)->second;
						argSym = m->getHead()->getHeadWord();			// headword of argument
						argPosSym = m->getHead()->getHeadPreterm()->getTag();	// pos-tag of argument
	
						if(argMentionIndex == _candidateArgument->getIndex()) {	// if this argument is the candidate itself, pair it with the proposition
							isCandArg = true;
							if(!predHeadLemma.is_null()) {
								_predAssocPropFeas.insert( std::pair<Symbol, Symbol>(roleSym, predHeadLemma) );
							}
						}
						else {		// this argument is not the _candidateArgument (its neighbor) ; pair it with the candidate
							if(roleString.compare(L"")!=0 && !(roleString.compare(L"<ref>")==0 && predTypeString.compare("noun")==0)) {
								if( !argSym.is_null() &&
								    	(boost::algorithm::starts_with(argPosSym.to_string(), L"NN") || 
									boost::algorithm::starts_with(argPosSym.to_string(), L"VB")) ) {
									argLemma = SymbolUtilities::stemWord(argSym, argPosSym);
									_argAssocPropFeas.insert( std::pair<Symbol, Symbol>(roleSym, argLemma) );
								}
							}
						}
					}
					else if(arg->getType() == Argument::PROPOSITION_ARG) {	// arg points to a proposition
						int id = (arg->getProposition()->getID() % MAX_SENTENCE_PROPS);
						if(propMap.find(id)==propMap.end()) {
							//std::cout << "propMap.find(" << id << ") returns null" << std::endl;
							continue;
						}	
						const Proposition* p = propMap.find(id)->second;
						const SynNode* pHead = p->getPredHead();
						if(pHead!=0) {
							argSym = pHead->getHeadWord();
							argPosSym = pHead->getHeadPreterm()->getTag();
	
							if(roleString.compare(L"")!=0) {
								if( !argSym.is_null() &&
								    	(boost::algorithm::starts_with(argPosSym.to_string(), L"NN") || 
									boost::algorithm::starts_with(argPosSym.to_string(), L"VB")) ) {
									argLemma = SymbolUtilities::stemWord(argSym, argPosSym);
									_argAssocPropFeas.insert( std::pair<Symbol, Symbol>(roleSym, argLemma) );
								}
							}
						}
					}
					else if(arg->getType() == Argument::TEXT_ARG) {
						// 24/10/2013. Yee Seng Chan.
						// we can ignore TEXT_ARG arguments, as it is just the textual representation of the _candidateArgument
						// but let's keep this code around for the moment in case we need it for experiments
						/*
						// text
						Symbol symArray[MAX_SENTENCE_TOKENS];
						int n_tokens = arg->getNode()->getTerminalSymbols(symArray, MAX_SENTENCE_TOKENS);
						std::wstring str = L"";
						for(int j=0; j<n_tokens; j++) {
							if(j>0) 
								str += L" ";
							str += symArray[j].to_string();
						}		

						// pos
						Symbol posArray[MAX_SENTENCE_TOKENS];
						n_tokens = arg->getNode()->getPOSSymbols(posArray, MAX_SENTENCE_TOKENS);
						str = L"";
						for(int j=0; j<n_tokens; j++) {
							if(j>0)
								str += L" ";
							str += posArray[j].to_string();
						}						
						*/
					}

					if(!argSym.is_null()) {
						if(argIndex>0)
							outBuf += L", ";
						if(isCandArg)		// current arg of this loop is the _candidateArgument
							outBuf += L"*";
						outBuf += roleString + L":" + std::wstring(argSym.to_string()) + L"/" + std::wstring(argPosSym.to_string());
					}
				}
				// went through all the args of current proposition

				outBuf += L")";
				outBufLines.push_back(outBuf);	
			} // if found
		} // went through all the propositions

	}

	outBufLines.push_back(L"</associatedProps>");

	//for(unsigned i=0; i<outBufLines.size(); i++) {
	//	std::cout << outBufLines[i] << std::endl;
	//}
}

void EventAAObservation::setAnchorArgRelativePositionNP() {
        std::pair<int,int> anchorOffset = getAnchorStartEndTokenOffset();
        Symbol anchorPostag = getAnchorPostag();
        if(boost::algorithm::starts_with( std::wstring(anchorPostag.to_string()) , L"NN")) {
                anchorOffset = getNounPhraseBoundary(anchorOffset.first, anchorOffset.second);
        }
        std::pair<int,int> argOffset = getNounPhraseBoundaryOfArg();

	if(anchorOffset.second < argOffset.first) {
		_anchorArgRelativePositionNP = Symbol(L"anchor_arg");
	}
	else if(argOffset.second < anchorOffset.first) {
		_anchorArgRelativePositionNP = Symbol(L"arg_anchor");
	}
	else {
		_anchorArgRelativePositionNP = Symbol(L"overlap");
	}
}

void EventAAObservation::setEntityTypesInNPSpanOfArg() {
	std::pair<int, int> argStartEndTokenIndex = getNounPhraseBoundaryOfArg();
	std::vector<Symbol> np;

	// first, populate with pos tags
	for(int i=argStartEndTokenIndex.first; i<=argStartEndTokenIndex.second; i++) {
		np.push_back(_posTokens[i]);
	}

	std::set<int> populatedIndices;
	for(int i=0; i<_mentionSet->getNMentions(); i++) {
		const Mention *m = _mentionSet->getMention(i);
		if(m->getMentionType()==Mention::NAME || m->getMentionType()==Mention::DESC || m->getMentionType()==Mention::PRON) {
			if(mentionIsBetweenSpanInclusive(m, argStartEndTokenIndex.first, argStartEndTokenIndex.second)) {
				int index = (m->getNode()->getHead()->getEndToken() - argStartEndTokenIndex.first);
				Symbol entityType = m->getEntityType().getName();
				np[index] = entityType;
				populatedIndices.insert(index);
			}	
		}
	}

	if(_candidateArgument) {
		int index = (_candidateArgument->getNode()->getHead()->getEndToken() - argStartEndTokenIndex.first);
		np[index] = Symbol(L"*" + std::wstring(_candidateArgument->getEntityType().getName().to_string()));
		populatedIndices.insert(index);
	}
	else {
		int index = (_candidateValue->getEndToken() - argStartEndTokenIndex.first);
		np[index] = Symbol(L"*" + std::wstring(_candidateValue->getType().to_string()));
		populatedIndices.insert(index);
	}

	std::wstring buf = L"";
	bool emptyBuf = true;
	Symbol poss = Symbol(L"POS");
	for(unsigned i=0; i<np.size(); i++) {
		if( (populatedIndices.find(i)!=populatedIndices.end()) || (np[i]==poss) ) {
			if(!emptyBuf)
				buf += L"_";
			if(np[i]==poss)
				buf += L"'s";
			else
				buf += np[i].to_string();
			emptyBuf = false;		
		}
	}

	_entityTypesInNPSpanOfArg = buf;
}

void EventAAObservation::setAAPrepositions() {
	_prepBeforeArg = Symbol();

        std::pair<int,int> anchorTokenOffset = getAnchorStartEndTokenOffset();
        std::pair<int,int> argTokenOffset = getArgStartEndTokenOffset();

	// check whether there is a preposition before/after anchor, arg
	Symbol prepIn = Symbol(L"IN");
	Symbol prepTo = Symbol(L"TO");
	if(argTokenOffset.first > 0) {
		Symbol postag = _posTokens[argTokenOffset.first-1];
		if( (postag == prepIn) || (postag == prepTo) ) {
			_prepBeforeArg = _wordTokensLower[argTokenOffset.first-1];
		}
	}
}

Symbol EventAAObservation::getAnchorHwLemma() {
	Symbol anchor = _vMention->getAnchorNode()->getHeadWord();
	Symbol anchorPosTag = _vMention->getAnchorNode()->getHeadPreterm()->getTag();
	return SymbolUtilities::stemWord(anchor, anchorPosTag);	
}

Symbol EventAAObservation::getAnchorPostag() {
	return _vMention->getAnchorNode()->getHeadPreterm()->getTag();
}

Symbol EventAAObservation::getArgumentHwLemma() {
	if(_candidateArgument)
		return getCandidateArgumentHwLemma();
	else
		return getCandidateValueHwLemma();
}

Symbol EventAAObservation::getCandidateArgumentHwLemma() {
	Symbol arg = _candidateHeadword;
	Symbol argPosTag = _candidateArgument->getNode()->getHeadPreterm()->getTag();
	return SymbolUtilities::stemWord(arg, argPosTag);
}

Symbol EventAAObservation::getCandidateValueHwLemma() {
	Symbol arg = _candidateHeadword;
	Symbol argPosTag = _parse->getRoot()->getNthTerminal(_candidateValue->getEndToken())->getParent()->getTag();
	return SymbolUtilities::stemWord(arg, argPosTag);
}

Symbol EventAAObservation::getArgumentPostag() {
	if(_candidateArgument!=0)
		return _candidateArgument->getNode()->getHeadPreterm()->getTag();
	else
		return _parse->getRoot()->getNthTerminal(_candidateValue->getEndToken())->getParent()->getTag();
}

std::pair<int,int> EventAAObservation::getAnchorArgBoundaryTokenOffset() {
	std::pair<int,int> anchorOffset = getAnchorStartEndTokenOffset();
	std::pair<int,int> argOffset = getArgStartEndTokenOffset();

	int i1=-1, i2=-1;
	if(anchorOffset.second < argOffset.first) {
		i1 = anchorOffset.second;
		i2 = argOffset.first;
	}
	else if(argOffset.second < anchorOffset.first) {	
		i1 = argOffset.second;
		i2 = anchorOffset.first;
	}

	std::pair<int,int> p = std::pair<int,int>(i1,i2);
	return p;
}

std::wstring EventAAObservation::getNounPhraseTokensOfArg() {
	std::pair<int,int> p = getNounPhraseBoundaryOfArg();

	std::wstring buf = L"";
	for(int i=p.first; i<=p.second; i++) {
		if(i > p.first)
			buf += L" ";
		buf += std::wstring(_wordTokens[i].to_string());
	}

	return buf;
}

std::pair<int, int> EventAAObservation::getNounPhraseBoundaryOfArg() {
	std::pair<int,int> p = getArgStartEndTokenOffset();
	std::pair<int, int> argStartEndTokenOffset = getNounPhraseBoundary(p.first, p.second);

	return argStartEndTokenOffset;	
}

// given a (start, end) offset, get the noun phrase offsets they are in
std::pair<int, int> EventAAObservation::getNounPhraseBoundary(const int& start, const int& end) {
	int i1, i2;

	Symbol before = Symbol(L"B");
	Symbol in = Symbol(L"I");

	i1 = start;
	while( (i1>0) && (_npChunks[i1-1]==before || _npChunks[i1-1]==in) ) {
		i1 -= 1;
	}
	i2 = end;
	while( (((unsigned)i2+1)<_posTokens.size()) && (_npChunks[i2+1]==in) ) {
		i2 += 1;
	}

	std::pair<int, int> p = std::pair<int, int>(i1, i2);

	return p;
}

std::pair<int,int> EventAAObservation::getAnchorStartEndTokenOffset() {
	int start = _vMention->getAnchorNode()->getHead()->getStartToken();
	int end = _vMention->getAnchorNode()->getHead()->getStartToken();	// we assume anchor is just a unigram

	std::pair<int,int> p = std::pair<int,int>(start, end);
	return p;
}

std::pair<int,int> EventAAObservation::getArgStartEndTokenOffset() {
	int start, end;

	if(_candidateArgument) {
		start = _candidateArgument->getNode()->getHead()->getStartToken();
		end = _candidateArgument->getNode()->getHead()->getEndToken();
	}
	else {
		start = _candidateValue->getStartToken();
		end = _candidateValue->getEndToken();
	}

	std::pair<int,int> p = std::pair<int,int>(start, end);
	return p;
}

int EventAAObservation::numberOfTokensBetweenAnchorArg() {
	std::pair<int,int> p = getAnchorArgBoundaryTokenOffset();

	if(p.first==-1 && p.second==-1)
		return -1;
	else
		return ((p.second-1) - (p.first+1) +1);
}

// bag of words between anchor and arg
void EventAAObservation::setBOWBetweenAnchorArg() {
	std::pair<int,int> p = getAnchorArgBoundaryTokenOffset();

	_bowBetween.clear();
	for(int i=(p.first+1); i<p.second; i++) {
		Symbol tok = _lemmaTokens[i];
		std::wstring tag = std::wstring(_posTokens[i].to_string());
		if( boost::algorithm::starts_with(tag, L"NN") || boost::algorithm::starts_with(tag, L"VB") || 
		    boost::algorithm::starts_with(tag, L"JJ") || boost::algorithm::starts_with(tag, L"RB") || 
		    tag.compare(L"IN")==0 || tag.compare(L"TO")==0 || 
		    tag.compare(L"POS")==0 || boost::algorithm::starts_with(tag, L"PRP") || 
		    boost::algorithm::starts_with(tag, L"WP") || tag.compare(L"WRB")==0 ) {
			_bowBetween.insert(tok);
		}
	}
}

void EventAAObservation::setBigramsBetweenAnchorArg() {
	std::pair<int,int> p = getAnchorArgBoundaryTokenOffset();

	_bigramsBetween.clear();
	for(int i=(p.first+1); (i+1)<p.second; i+=1) {
		Symbol tok1 = _lemmaTokens[i];
		if( (i+1) < p.second) {
			Symbol tok2 = _lemmaTokens[i+1];
			std::pair<Symbol,Symbol> bigram = std::pair<Symbol,Symbol>(tok1, tok2);
			_bigramsBetween.insert(bigram);
		}
		else {
			break;
		}
	}
}

// the 'NP' version ; by first expanding anchor and arg to their associated NPs
void EventAAObservation::setLocalContextWordsNP() {
	_firstWordBeforeM1NP = Symbol();

        std::pair<int,int> anchorOffset = getAnchorStartEndTokenOffset();
        Symbol anchorPostag = getAnchorPostag();
        if(boost::algorithm::starts_with( std::wstring(anchorPostag.to_string()) , L"NN")) {
                anchorOffset = getNounPhraseBoundary(anchorOffset.first, anchorOffset.second);
        }
        std::pair<int,int> argOffset = getNounPhraseBoundaryOfArg();

	if(anchorOffset.second < argOffset.first) {		// anchor before arg
		setLocalContextWordsNP(anchorOffset, argOffset);
	}
	else if(argOffset.second < anchorOffset.first) {	// arg before anchor
		setLocalContextWordsNP(argOffset, anchorOffset);
	}
}

void EventAAObservation::setLocalContextWordsNP(const std::pair<int,int>& m1Offset, const std::pair<int,int>& m2Offset) {
	if(m1Offset.first > 0)
		_firstWordBeforeM1NP = _lemmaTokens[m1Offset.first-1];
}

// grab the entity mentions between anchor and arg
// no expansion of anchor/arg to associated NP
std::set<const Mention*> EventAAObservation::getMentionsBetweenAnchorArg() {
	std::set<const Mention*> mentionsBetweenAnchorAndArg;

	for(int i=0; i<_mentionSet->getNMentions(); i++) {
		const Mention *m = _mentionSet->getMention(i);
		if(m->getMentionType()==Mention::NAME || m->getMentionType()==Mention::DESC || m->getMentionType()==Mention::PRON) {
			if(mentionIsBetweenAnchorAndArg(m)) {		
				mentionsBetweenAnchorAndArg.insert(m);
			}	
		}
	}

	return mentionsBetweenAnchorAndArg;
}

// sets : _mentionEntityTypeBetween
void EventAAObservation::setMentionInfoBetweenAnchorArg(std::set<const Mention*> mentionsBetweenAnchorArg) {
	_mentionEntityTypeBetween.clear();
	for(std::set<const Mention*>::iterator it=mentionsBetweenAnchorArg.begin(); it!=mentionsBetweenAnchorArg.end(); it++) {
		const Mention* m = *it;
		_mentionEntityTypeBetween.insert(m->getEntityType().getName());
	}
}

// if arg is an entity mention, is/are there any more entity mention(s) with same entity-type between anchor and arg
void EventAAObservation::setHasSameEntityTypeBetween() {
	_hasSameEntityTypeBetween = Symbol();
	if(_candidateArgument) {
		Symbol et = _candidateArgument->getEntityType().getName();
		if(_mentionEntityTypeBetween.find(et)!=_mentionEntityTypeBetween.end())
			_hasSameEntityTypeBetween = Symbol(L"Y");
		else
			_hasSameEntityTypeBetween = Symbol(L"N");
	}
}

bool EventAAObservation::mentionIsBetweenAnchorAndArg(const Mention* m) {
	std::pair<int,int> p = getAnchorArgBoundaryTokenOffset();	// no expansion of anchor/arg to associated NP
	int i1 = p.first + 1;
	int i2 = p.second - 1;

	if(i1 <= i2) {
		if(mentionIsBetweenSpanInclusive(m, i1, i2))
			return true;
	}

	return false;
}

bool EventAAObservation::mentionIsBetweenSpanInclusive(const Mention* m, const int& startIndex, const int& endIndex) {
	int start = m->getNode()->getHead()->getStartToken();
	int end = m->getNode()->getHead()->getEndToken();
	if( (startIndex<=start) && (end<=endIndex) ) 
		return true;
	else
		return false;
}

void EventAAObservation::setAADistributionalKnowledge() {
	Symbol anchorLemma = getAnchorHwLemma();
	Symbol argLemma = getArgumentHwLemma();

	Symbol anchorPosTag = _vMention->getAnchorNode()->getHeadPreterm()->getTag();
	Symbol argPosTag = getArgumentPostag();

	setAssociatedProps();			// sets: _predAssocPropFeas , _argAssocPropFeas

	_aaDK = DistributionalKnowledgeClass(anchorLemma, anchorPosTag, argLemma, argPosTag, _vMention->getEventType());
	_aaDK.assignAssocPropFeatures(_predAssocPropFeas);
	_aaDK.assignCausalFeatures(_predAssocPropFeas);

	setAnchorArgRelativePositionNP();	// sets: _anchorArgRelativePositionNP ; expand both anchor and arg to its associated NP

	setAAPrepositions();			

	std::set<const Mention*> mentionsBetweenAnchorArg = getMentionsBetweenAnchorArg();	// grabs entity mentions between anchor,arg ; no expansion of a/a to NP
	setMentionInfoBetweenAnchorArg(mentionsBetweenAnchorArg);	// sets : _mentionEntityTypeBetween
	setHasSameEntityTypeBetween();					// requires: _mentionEntityTypeBetween
									// sets: _hasSameEntityTypeBetween

	setEntityTypesInNPSpanOfArg();		// sets: _entityTypesInNPSpanOfArg (wstring '_' separated sequence of entity types in NP associated with arg

	setBOWBetweenAnchorArg();
	setBigramsBetweenAnchorArg();

	setLocalContextWordsNP();		// sets: _firstWordBeforeM1NP

	/*
	std::vector<std::wstring> bufLines = displayDistributionalKnowledgeInfo();
	std::cout << "<analysis>" << std::endl;
	for(unsigned i=0; i<bufLines.size(); i++) {
		std::cout << UnicodeUtil::toUTF8StdString(bufLines[i]) << std::endl;
	}
	for(std::set<const Mention*>::iterator it=mentionsBetweenAnchorArg.begin(); it!=mentionsBetweenAnchorArg.end(); it++) {
		const Mention* m = *it;
		std::cout << "mentionsInBetween: " << UnicodeUtil::toUTF8StdString(m->getEntityType().getName().to_string()) << " " << UnicodeUtil::toUTF8StdString(m->getHead()->getHeadWord().to_string()) << std::endl;
	}
	std::cout << "</analysis>" << std::endl;
	*/
}

// 10/16/2013, Yee Seng Chan
// The code below is for experimental purposes. Prints useful information about this EventAAObservation.
std::vector<std::wstring> EventAAObservation::displayDistributionalKnowledgeInfo() {
	std::vector<std::wstring> outBufLines;
	std::wstring outBuf = L"";

	//if(_docId!=0)
	//	outBufLines.push_back(std::wstring(_docId.to_string()));
	//else
		outBufLines.push_back(L"docId=NULL");
	outBuf = L"event-aa-role:" + std::wstring(_vMention->getEventType().to_string());

	int argStartTokenIndex, argEndTokenIndex;
	if(_candidateArgument) {
		outBuf += L" " + std::wstring(_candidateArgument->getEntityType().getName().to_string());
		outBuf += L" " + std::wstring(_candidateArgument->getEntitySubtype().getName().to_string());
		outBuf += L" mrole=" + std::wstring(_candidateArgument->getRoleType().getName().to_string());
		argStartTokenIndex = _candidateArgument->getNode()->getHead()->getStartToken();
		argEndTokenIndex = _candidateArgument->getNode()->getHead()->getEndToken();
	}
	else {
		outBuf += L" " + std::wstring(_candidateValue->getFullType().getBaseTypeSymbol().to_string());
		outBuf += L" " + std::wstring(_candidateValue->getFullType().getSubtypeSymbol().to_string());
		argStartTokenIndex = _candidateValue->getStartToken();
		argEndTokenIndex = _candidateValue->getEndToken();
	}

	int anchorStartTokenIndex = _vMention->getAnchorNode()->getHead()->getStartToken();
	int anchorEndTokenIndex = _vMention->getAnchorNode()->getHead()->getEndToken();
	int anchorStartOffset = _tokens->getToken(anchorStartTokenIndex)->getStartEDTOffset().value();
	int anchorEndOffset = _tokens->getToken(anchorEndTokenIndex)->getEndEDTOffset().value();
	int argStartOffset = _tokens->getToken(argStartTokenIndex)->getStartEDTOffset().value();
	int argEndOffset = _tokens->getToken(argEndTokenIndex)->getEndEDTOffset().value();
	outBuf += L" " + boost::lexical_cast<std::wstring>(anchorStartOffset) + L"," + boost::lexical_cast<std::wstring>(anchorEndOffset);
	outBuf += L" " + boost::lexical_cast<std::wstring>(argStartOffset) + L"," + boost::lexical_cast<std::wstring>(argEndOffset);
	outBufLines.push_back(outBuf);
	// set: anchorStartTokenIndex , anchorEndTokenIndex , argStartTokenIndex , argEndTokenIndex
	// set: anchorStartOffset , anchorEndOffset , argStartOffset , argEndOffset

	int i1 = 0;
	int i2 = _tokens->getNTokens()-1;

	std::wstring connectingString = L"";
	for(int i=i1; i<=i2; i++) {
		if(i>i1)
			connectingString += L" ";
		if(i==argStartTokenIndex)
			connectingString += L"[";
		if(i==anchorStartTokenIndex)
			connectingString += L"<";
		connectingString += _tokens->getToken(i)->getSymbol().to_string();
		if(i==argEndTokenIndex)
			connectingString += L"]";
		if(i==anchorEndTokenIndex)
			connectingString += L">";
	}
	outBufLines.push_back(L"TEXT: " + connectingString);

	std::wstring connectingLemmaString = L"";
	for(int i=i1; i<=i2; i++) {
		if(i>i1)
			connectingLemmaString += L" ";
		if(i==argStartTokenIndex)
			connectingLemmaString += L"[";
		if(i==anchorStartTokenIndex)
			connectingLemmaString += L"<";
		connectingLemmaString += _lemmaTokens[i].to_string();
		if(i==argEndTokenIndex)
			connectingLemmaString += L"]";
		if(i==anchorEndTokenIndex)
			connectingLemmaString += L">";
	}
	outBufLines.push_back(L"LEMMA: " + connectingLemmaString);

	std::wstring connectingPosString = L"";
	for(int i=i1; i<=i2; i++) {
		if(i>i1)
			connectingPosString += L" ";
		if(i==argStartTokenIndex)
			connectingPosString += L"[";
		if(i==anchorStartTokenIndex)
			connectingPosString += L"<";
		Symbol tag = _parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
		connectingPosString += tag.to_string();
		if(i==argEndTokenIndex)
			connectingPosString += L"]";
		if(i==anchorEndTokenIndex)
			connectingPosString += L">";
	}
	outBufLines.push_back(L"POS: " + connectingPosString);

	std::wstring npChunkString = L"";
	for(int i=i1; i<=i2; i++) {
		if(i>i1)
			npChunkString += L" ";
		npChunkString += std::wstring(_npChunks[i].to_string());
	}
	outBufLines.push_back(L"NPCHUNKS: " + npChunkString);

	outBufLines.push_back(L"connectingCandParsePath: " + std::wstring(_connectingCandParsePath.to_string()));
	outBufLines.push_back(L"connectingTriggerParsePath: " + std::wstring(_connectingTriggerParsePath.to_string()));

	Symbol nnPos = Symbol(L"NN");
	Symbol nnsPos = Symbol(L"NNS");
	Symbol nnpPos = Symbol(L"NNP");
	Symbol nnpsPos = Symbol(L"NNPS");
	Symbol jjPos = Symbol(L"JJ");
	Symbol jjsPos = Symbol(L"JJS");
	Symbol cdPos = Symbol(L"CD");
	Symbol dtPos = Symbol(L"DT");
	Symbol pdtPos = Symbol(L"PDT");
	Symbol posPos = Symbol(L"POS");
	Symbol prpPos = Symbol(L"PRP$");


	int npStartIndex=argStartTokenIndex-1;
	while( 	(npStartIndex>=0) && 
		(_posTokens[npStartIndex]==nnPos || _posTokens[npStartIndex]==nnsPos || 
		_posTokens[npStartIndex]==nnpPos || _posTokens[npStartIndex]==nnpsPos ||
		_posTokens[npStartIndex]==jjPos || _posTokens[npStartIndex]==jjsPos || 
		_posTokens[npStartIndex]==cdPos || 
		_posTokens[npStartIndex]==dtPos || _posTokens[npStartIndex]==pdtPos ||
		_posTokens[npStartIndex]==posPos || _posTokens[npStartIndex]==prpPos) ) {
		npStartIndex -= 1;
	}
	std::wstring beforeNpPostag = L"NULL";
	std::wstring beforeNpWord = L"NULL";
	if(npStartIndex>=0) {
		beforeNpPostag = std::wstring(_posTokens[npStartIndex].to_string());
		beforeNpWord = std::wstring(_wordTokens[npStartIndex].to_string());
	}
	outBufLines.push_back(L" beforeNp=" + beforeNpWord + L"/" + beforeNpPostag);

	if(_connectingProp!=0) {
		std::wstring cpHw = std::wstring(_connectingProp->getPredHead()->getHeadWord().to_string());
		int cpStartTokenIndex = _connectingProp->getPredHead()->getHead()->getStartToken();
		int cpEndTokenIndex = _connectingProp->getPredHead()->getHead()->getEndToken();
		int cpStartOffset = _tokens->getToken(cpStartTokenIndex)->getStartEDTOffset().value();
		int cpEndOffset = _tokens->getToken(cpEndTokenIndex)->getEndEDTOffset().value();
		outBufLines.push_back(L"<connectingProp start=\"" + boost::lexical_cast<std::wstring>(cpStartOffset) + L"\" end=\"" + 
					boost::lexical_cast<std::wstring>(cpEndOffset) + L"\">" + cpHw + L"</connectingProp>");
	}
	else {
		outBufLines.push_back(L"<connectingProp start=\"_\" end=\"_\"></connectingProp>");
	}

	if(_candidateArgument!=0 && !_vMention->getRoleForMention(_candidateArgument).is_null())
		outBuf = L"  role=" + std::wstring(_vMention->getRoleForMention(_candidateArgument).to_string());
	else if(_candidateValue!=0 && !_vMention->getRoleForValueMention(_candidateValue).is_null())
		outBuf = L"  role=" + std::wstring(_vMention->getRoleForValueMention(_candidateValue).to_string());
	else
		outBuf = L"  role=NONE";
	if(!_candidateRoleInCP.is_null())
		outBuf += L"\targRoleInCP=" + std::wstring(getCandidateRoleInCP().to_string());
	else
		outBuf += L"\targRoleInCP=NULL";
	if(!_eventRoleInCP.is_null())
		outBuf += L"\teventRoleInCP=" + std::wstring(getEventRoleInCP().to_string());
	else
		outBuf += L"\teventRoleInCP=NULL";
	if(!_roleInTriggerProp.is_null())
		outBuf += L"\troleInTriggerProp=" + std::wstring(_roleInTriggerProp.to_string());
	else
		outBuf += L"\troleInTriggerProp=NULL";
	outBufLines.push_back(outBuf);

	outBuf = L"\t";
	if(!_oldCandidateRoleInCP.is_null())
		outBuf += L"\targRoleInCP=" + std::wstring(_oldCandidateRoleInCP.to_string());
	else
		outBuf += L"\targRoleInCP=NULL";
	if(!_oldEventRoleInCP.is_null())
		outBuf += L"\teventRoleInCP=" + std::wstring(_oldEventRoleInCP.to_string());
	else
		outBuf += L"\teventRoleInCP=NULL";
	if(!_oldRoleInTriggerProp.is_null())
		outBuf += L"\troleInTriggerProp=" + std::wstring(_oldRoleInTriggerProp.to_string());
	else
		outBuf += L"\troleInTriggerProp=NULL";
	outBufLines.push_back(outBuf);

	outBufLines.push_back(prepositionInfoToString());

	outBuf = std::wstring(L"subScoreSB:" + boost::lexical_cast<wstring>(_aaDK.subScoreSB()));
	outBuf += L" objScoreSB:" + boost::lexical_cast<wstring>(_aaDK.objScoreSB());
	outBuf += L" avgSubObjScoreSB:" + boost::lexical_cast<wstring>(_aaDK.avgSubObjScoreSB());
	outBuf += L" maxSubObjScoreSB:" + boost::lexical_cast<wstring>(_aaDK.maxSubObjScoreSB());
	outBufLines.push_back(outBuf);

	outBufLines.push_back(L"anchorArgSim " + boost::lexical_cast<std::wstring>(_aaDK.anchorArgSim()) + L" SB:" + boost::lexical_cast<wstring>(_aaDK.anchorArgSimSB()));
	outBufLines.push_back(L"anchorArgPmi " + boost::lexical_cast<std::wstring>(_aaDK.anchorArgPmi()) + L" SB:" + boost::lexical_cast<wstring>(_aaDK.anchorArgPmiSB()));
	outBufLines.push_back(L"assocPropSim " + boost::lexical_cast<std::wstring>(_aaDK.assocPropSim()) + L" SB:" + boost::lexical_cast<wstring>(_aaDK.assocPropSimSB()));
	outBufLines.push_back(L"assocPropPmi " + boost::lexical_cast<std::wstring>(_aaDK.assocPropPmi()) + L" SB:" + boost::lexical_cast<wstring>(_aaDK.assocPropPmiSB()));
	outBufLines.push_back(L"causalScore " + boost::lexical_cast<std::wstring>(_aaDK.causalScore()) + L" SB:" + boost::lexical_cast<wstring>(_aaDK.causalScoreSB()));

	outBuf = L"contentWords:";
	for(std::set<Symbol>::iterator it=_contentWords.begin(); it!=_contentWords.end(); it++) {
		outBuf += L" " + std::wstring((*it).to_string());
	}
	outBufLines.push_back(outBuf);

	outBufLines.push_back(L"getDistance=" + boost::lexical_cast<std::wstring>(getDistance()));

	// local context words around anchor and arg
	outBufLines.push_back(localContextWordsNPToString());
	outBufLines.push_back(bowBetweenToString());
	outBufLines.push_back(bigramsBetweenToString());

	outBufLines.push_back(L"npTokensOfArg: " + getNounPhraseTokensOfArg());
	outBufLines.push_back(L"numOfTokensBetweenAnchorArg: " + boost::lexical_cast<std::wstring>(numberOfTokensBetweenAnchorArg()));

	outBufLines.push_back(L"entityTypesInNPSpanOfArg: " + _entityTypesInNPSpanOfArg);


	return outBufLines;
}

// invoked by: displayDistributionalKnowledgeInfo()
std::wstring EventAAObservation::prepositionInfoToString() {
	std::wstring outBuf = L"prepositions: " + std::wstring(_anchorArgRelativePositionNP.to_string()) + L" prepBeforeArg:";
	if(_prepBeforeArg.is_null())
		outBuf += L"_ ";
	else
		outBuf += std::wstring(_prepBeforeArg.to_string()) + L" ";

	return outBuf;
}

// invoked by: displayDistributionalKnowledgeInfo()
std::wstring EventAAObservation::localContextWordsNPToString() {
	std::wstring localContextWords = L"localContextWordsNP: ";
	if(_firstWordBeforeM1NP.is_null())
		localContextWords += L"_ (m1) ";
	else
		localContextWords += std::wstring(_firstWordBeforeM1NP.to_string()) + L" (m1) ";

	return localContextWords;
}

// invoked by: displayDistributionalKnowledgeInfo()
std::wstring EventAAObservation::bowBetweenToString() {
	std::wstring buf = L"BOWBetween:";

	for(std::set<Symbol>::iterator it=_bowBetween.begin(); it!=_bowBetween.end(); it++) {
		buf += L" ";
		buf += std::wstring((*it).to_string());
	}

	return buf;
}

// invoked by: displayDistributionalKnowledgeInfo()
std::wstring EventAAObservation::bigramsBetweenToString() {
	std::wstring buf = L"BigramsBetween:";

	for(std::set< std::pair<Symbol,Symbol> >::iterator it=_bigramsBetween.begin(); it!=_bigramsBetween.end(); it++) {
		buf += L" ";
		buf += std::wstring((*it).first.to_string());
		buf += L"_";
		buf += std::wstring((*it).second.to_string());
	}

	return buf;
}


