// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "docRelationsEvents/RelationTimexArgObservation.h"
#include "theories/TokenSequence.h"
#include "theories/Parse.h"
#include "theories/MentionSet.h"
#include "theories/ValueMentionSet.h"
#include "theories/PropositionSet.h"
#include "theories/Proposition.h"
#include "common/SymbolUtilities.h"
#include "common/SymbolConstants.h"
#include "events/EventUtilities.h"
#include "parse/LanguageSpecificFunctions.h"

const Symbol RelationTimexArgObservation::_className(L"relation-timex-arg-attachment");

RelationTimexArgObservation::RelationTimexArgObservation() : 
		DTObservation(_className), _relMention(0), _candidateArgument(0),
		_candidateValue(0), _tokens(0), _parse(0), _mentionSet(0), _propSet(0), 
		_connectingString(L""), _stemmedConnectingString(L""), _posConnectingString(L""),
	 _abbrevConnectingString(L""), _connectingCandParsePath(L""), _connectingPredicateParsePath(L""),	
	 _ment1ConnectingString(L""), _ment1StemmedConnectingString(L""), _ment1POSConnectingString(L""),
	 _ment1AbbrevConnectingString(L""), _ment2ConnectingString(L""), _ment2StemmedConnectingString(L""),
	 _ment2POSConnectingString(L""), _ment2AbbrevConnectingString(L""), _timexString(L"")
{
	_propLink = _new RelationPropLink();
}

RelationTimexArgObservation::~RelationTimexArgObservation() {
	delete _propLink;
}

DTObservation *RelationTimexArgObservation::makeCopy() {
	RelationTimexArgObservation *copy = _new RelationTimexArgObservation();
	copy->resetForNewSentence(_tokens, _parse, _mentionSet, _valueMentionSet, _propSet);
	copy->setRelation(_relMention);
	copy->setCandidateArgument(_candidateValue);
	return copy;
}

void RelationTimexArgObservation::setRelation(RelMention *relMention) {
	_relMention = relMention;
	findPropLink();
}

void RelationTimexArgObservation::setCandidateArgument(const ValueMention *valueMention) {
	_candidateArgument = findMentionForValue(valueMention);
	_candidateValue = valueMention;
	setCandidateRoleInRelationProp();
	setConnectingProp();
	setConnectingString();
	setMentionConnectingStrings();
	setConnectingParsePath();
	setTimexString();
	setGoverningPrep();
	setSentenceLocation();
	setDistance();
}

// "the people were moved" --> "PER were moved"
// "he said by telephone from Belgium" --> "telephone from GPE"	
void RelationTimexArgObservation::setConnectingString() {
	_connectingString = L"";
	_stemmedConnectingString = L"";
	_posConnectingString = L"";
	_abbrevConnectingString = L"";

	if (_propLink->isEmpty() || _propLink->getTopProposition()->getPredHead() == 0)
		return;

	int start_candidate;
	int end_candidate;
	if (_candidateArgument) {
		start_candidate = _candidateArgument->getNode()->getStartToken();
		end_candidate = _candidateArgument->getNode()->getEndToken();
	} else {
		start_candidate = _candidateValue->getStartToken();
		end_candidate = _candidateValue->getEndToken();
	}

	int predicate_index = _propLink->getTopProposition()->getPredHead()->getStartToken();
	if (predicate_index < start_candidate) {
		_connectingString = makeConnectingStringSymbol(predicate_index, start_candidate, false);
		_stemmedConnectingString = makeStemmedConnectingStringSymbol(predicate_index, start_candidate, false);
		_posConnectingString = makePOSConnectingStringSymbol(predicate_index, start_candidate, false);
		_abbrevConnectingString = makeAbbrevConnectingStringSymbol(predicate_index, start_candidate, false);
	} else if (end_candidate < predicate_index) {
		_connectingString = makeConnectingStringSymbol(end_candidate + 1, predicate_index + 1, true);	
		_stemmedConnectingString = makeStemmedConnectingStringSymbol(end_candidate + 1, predicate_index + 1, true);
		_posConnectingString = makePOSConnectingStringSymbol(end_candidate + 1, predicate_index + 1, true);
		_abbrevConnectingString = makeAbbrevConnectingStringSymbol(end_candidate + 1, predicate_index + 1, true);
	}  
}

void RelationTimexArgObservation::setMentionConnectingStrings() {
	_ment1ConnectingString = L"";
	_ment1StemmedConnectingString = L"";
	_ment1POSConnectingString = L"";
	_ment1AbbrevConnectingString = L"";
	_ment2ConnectingString = L"";
	_ment2StemmedConnectingString = L"";
	_ment2POSConnectingString = L"";
	_ment2AbbrevConnectingString = L"";

	int start_candidate;
	int end_candidate;
	if (_candidateArgument) {
		start_candidate = _candidateArgument->getNode()->getStartToken();
		end_candidate = _candidateArgument->getNode()->getEndToken();
	} else {
		start_candidate = _candidateValue->getStartToken();
		end_candidate = _candidateValue->getEndToken();
	}

	int ment1_index = _relMention->getLeftMention()->getHead()->getStartToken();
	int ment2_index = _relMention->getRightMention()->getHead()->getStartToken();

	if (ment1_index < start_candidate) {
		_ment1ConnectingString = makeConnectingStringSymbol(ment1_index, start_candidate, false);
		_ment1StemmedConnectingString = makeStemmedConnectingStringSymbol(ment1_index, start_candidate, false);
		_ment1POSConnectingString = makePOSConnectingStringSymbol(ment1_index, start_candidate, false);
		_ment1AbbrevConnectingString = makeAbbrevConnectingStringSymbol(ment1_index, start_candidate, false);
	} else if (end_candidate < ment1_index) {
		_ment1ConnectingString = makeConnectingStringSymbol(end_candidate + 1, ment1_index + 1, true);
		_ment1StemmedConnectingString = makeStemmedConnectingStringSymbol(end_candidate + 1, ment1_index + 1, true);	
		_ment1POSConnectingString = makePOSConnectingStringSymbol(end_candidate + 1, ment1_index + 1, true);	
		_ment1AbbrevConnectingString = makeAbbrevConnectingStringSymbol(end_candidate + 1, ment1_index + 1, true);	
	}  

	if (ment2_index < start_candidate) {
		_ment2ConnectingString = makeConnectingStringSymbol(ment2_index, start_candidate, false);
		_ment2StemmedConnectingString = makeStemmedConnectingStringSymbol(ment2_index, start_candidate, false);	
		_ment2POSConnectingString = makePOSConnectingStringSymbol(ment2_index, start_candidate, false);	
		_ment2AbbrevConnectingString = makeAbbrevConnectingStringSymbol(ment2_index, start_candidate, false);	
	} else if (end_candidate < ment2_index) {
		_ment2ConnectingString = makeConnectingStringSymbol(end_candidate + 1, ment2_index + 1, true);
		_ment2StemmedConnectingString = makeStemmedConnectingStringSymbol(end_candidate + 1, ment2_index + 1, true);
		_ment2POSConnectingString = makePOSConnectingStringSymbol(end_candidate + 1, ment2_index + 1, true);
		_ment2AbbrevConnectingString = makeAbbrevConnectingStringSymbol(end_candidate + 1, ment2_index + 1, true);
	}  
}

void RelationTimexArgObservation::setConnectingParsePath() {
	_connectingCandParsePath = L"-NONE-";
	_connectingPredicateParsePath = L"-NONE-";

	if (_propLink->isEmpty() || _propLink->getTopProposition()->getPredHead() == 0)
		return;

	const SynNode *root = _parse->getRoot();
	const SynNode *predicateNode =  _propLink->getTopProposition()->getPredHead();
	const SynNode *candNode;
	int c_start, c_end;
	int p_start = predicateNode->getStartToken();
	int p_end = predicateNode->getStartToken();
	
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
	if (c_start < p_start)
		coveringNode = root->getCoveringNodeFromTokenSpan(c_start, p_end);
	else if (p_start < c_start)
		coveringNode = root->getCoveringNodeFromTokenSpan(p_start, c_end);

	if (coveringNode == 0)
		return;

	int cdist = candNode->getAncestorDistance(coveringNode);
	int pdist = predicateNode->getAncestorDistance(coveringNode);

	//paths >5, not interesting ???
	if((cdist > 5) || (pdist > 5))
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
	_connectingCandParsePath = buffer;
	

	remaining_buffsize = buffsize - 1;
	wcscpy(buffer, L"path:");
	remaining_buffsize -= 5;
	temp = predicateNode;
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
	_connectingPredicateParsePath = buffer;
}

void RelationTimexArgObservation::setTimexString() {
	_timexString = L"";

	if (_candidateValue != 0) { 
		makeTimexString(_candidateValue->getStartToken(), 
			_candidateValue->getEndToken());
	}
}

void RelationTimexArgObservation::setGoverningPrep() {
	_governingPrep = Symbol();

	if (_candidateArgument != 0) { 
		const SynNode *node = _candidateArgument->getNode();
		if (node->getParent() != 0 &&
			LanguageSpecificFunctions::isPPLabel(node->getParent()->getTag()))
		{
			_governingPrep = node->getParent()->getHeadWord();
		}
	}
	else if (_candidateValue != 0) {
		const SynNode *node = findSynNodeForValue(_parse->getRoot(), _candidateValue);
		if (node != 0 && node->getParent() != 0 &&
			LanguageSpecificFunctions::isPPLabel(node->getParent()->getTag())) 
		{
			_governingPrep = node->getParent()->getHeadWord();
		}
	}
}

void RelationTimexArgObservation::setSentenceLocation() {
	_sentenceLocation = Symbol();

	int start_ment1 = _relMention->getLeftMention()->getNode()->getStartToken();
	int start_ment2 = _relMention->getRightMention()->getNode()->getStartToken();
	int end_ment1 = _relMention->getLeftMention()->getNode()->getEndToken();
	int end_ment2 = _relMention->getRightMention()->getNode()->getEndToken();

	int start_candidate;
	int end_candidate;
	if (_candidateArgument) {
		start_candidate = _candidateArgument->getNode()->getStartToken();
		end_candidate = _candidateArgument->getNode()->getEndToken();
	} else {
		start_candidate = _candidateValue->getStartToken();
		end_candidate = _candidateValue->getEndToken();
	}

	if (start_candidate == 0)
		_sentenceLocation = Symbol(L"sentence-begin");
	else if (end_candidate < start_ment1 && end_candidate < start_ment2)
		_sentenceLocation = Symbol(L"before-both");
	else if (start_candidate > end_ment1 && start_candidate > end_ment2)
		_sentenceLocation = Symbol(L"after-both");
	else if (start_candidate > end_ment1 && end_candidate < start_ment2)
		_sentenceLocation = Symbol(L"between");
	else if (start_candidate >= start_ment1 && end_candidate <= end_ment1 &&
			 start_candidate >= start_ment2 && end_candidate <= end_ment2)
		_sentenceLocation = Symbol(L"both-extents");
	else if (start_candidate >= start_ment1 && end_candidate <= end_ment1)
		_sentenceLocation = Symbol(L"ment1-extent");
	else if (start_candidate >= start_ment2 && end_candidate <= end_ment2)
		_sentenceLocation = Symbol(L"ment2-extent");
	else 
		_sentenceLocation = Symbol(L"undefined");
	
}

void RelationTimexArgObservation::setDistance() {
	if (_propLink->isEmpty() || _propLink->getTopProposition()->getPredHead() == 0) {
		_distance = -1;
		return;
	}
	
	int start_candidate;
	int end_candidate;
	if (_candidateArgument) {
		start_candidate = _candidateArgument->getNode()->getStartToken();
		end_candidate = _candidateArgument->getNode()->getEndToken();
	} else {
		start_candidate = _candidateValue->getStartToken();
		end_candidate = _candidateValue->getEndToken();
	}
	int predicate_index = _propLink->getTopProposition()->getPredHead()->getStartToken();

	if (predicate_index < start_candidate)
		_distance = start_candidate - predicate_index;
	else if (end_candidate < predicate_index)
		_distance = predicate_index - end_candidate;
	else _distance = 0;

}

void RelationTimexArgObservation::setCandidateRoleInRelationProp() {
	// "his trial" --> <poss>

	_roleInRelationProp = Symbol();
	if (_propLink->isEmpty() || _candidateArgument == 0)
		return;
	for (int i = 0; i < _propLink->getTopProposition()->getNArgs(); i++) {
		Argument *arg = _propLink->getTopProposition()->getArg(i);
		if (arg->getType() == Argument::MENTION_ARG &&
			arg->getMentionIndex() == _candidateArgument->getIndex()) 
		{
			_roleInRelationProp = arg->getRoleSym();
			return;
		}
	}
}

void RelationTimexArgObservation::setConnectingProp() {
	// "he won the election" --> <sub> <obj> win

	_candidateRoleInCP = Symbol();
	_relationRoleInCP = Symbol();
	_stemmedCPPredicate = Symbol();	

	if (_candidateArgument == 0 || _propLink->isEmpty())
		return;

	for (int i = 0; i < _propSet->getNPropositions(); i++) {
		const Proposition *prop = _propSet->getProposition(i);
		for (int argnum = 0; argnum < prop->getNArgs(); argnum++) {
			Argument *arg = prop->getArg(argnum);
			if (arg->getType() == Argument::MENTION_ARG) {
				if (arg->getMentionIndex() == _candidateArgument->getIndex()) 
					_candidateRoleInCP = arg->getRoleSym();
				else if (_propLink->getTopProposition()->getPredHead() ==
					arg->getMention(_mentionSet)->getNode()->getHeadPreterm()) 
				{
					_relationRoleInCP = arg->getRoleSym();
				}
			} else if (arg->getType() == Argument::PROPOSITION_ARG &&
				arg->getProposition() == _propLink->getTopProposition())
			{
				_relationRoleInCP = arg->getRoleSym();
			}
		}
		if (!_relationRoleInCP.is_null() && 
			!_candidateRoleInCP.is_null() &&
			prop->getPredHead() != 0) 
		{
			_stemmedCPPredicate = SymbolUtilities::stemWord(prop->getPredHead()->getHeadWord(),
				prop->getPredHead()->getHeadPreterm()->getTag());
			break;
		} else {
			_relationRoleInCP = Symbol();
			_candidateRoleInCP = Symbol();
		}
	}
}

const Mention *RelationTimexArgObservation::findMentionForValue(const ValueMention *value) {
	for (int mentid = 0; mentid < _mentionSet->getNMentions(); mentid++) {
		const Mention *mention = _mentionSet->getMention(mentid);
		if (mention->getNode()->getStartToken() == value->getStartToken() &&
			mention->getNode()->getEndToken() == value->getEndToken())
			return mention;
	}
	return 0;
}

const SynNode *RelationTimexArgObservation::findSynNodeForValue(const SynNode *node,
															   const ValueMention *value) 
{
	if (node->getStartToken() == value->getStartToken() &&
		node->getEndToken() == value->getEndToken())
		return node;
	else if (node->getStartToken() <= value->getStartToken() &&
		node->getEndToken() >= value->getEndToken())
	{
		for (int i = 0; i < node->getNChildren(); i++) {
			const SynNode *t = findSynNodeForValue(node->getChild(i), value);
			if (t != 0)
				return t;
		}
	}

	return 0;
}


wstring RelationTimexArgObservation::makeConnectingStringSymbol(int start, int end, bool candidate_in_front) 
{
	if (end - start > 10) {
		return L"";
	}
	std::wstring str = L"";

	if (candidate_in_front) {
		str += getCandidateType().to_string();
		str += L"_";
	}
	for (int i = start; i < end; i++) {
		bool mention_found = false;
		bool included = false;
		for (int j = 0; j < _mentionSet->getNMentions(); j++) {
			const Mention *ment = _mentionSet->getMention(j);
			if (ment->getNode()->getStartToken() == i &&
				ment->getNode()->getEndToken() < end &&
				ment->getEntityType().isRecognized()) 
			{
				str += ment->getEntityType().getName().to_string();
				i = ment->getNode()->getEndToken();
				mention_found = true;
				included = true;
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
					i =	ment->getEndToken();
					value_found	= true;
					included = true;
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
				included = true;
			}
		}
		if (i != end - 1) {
			if (included) {
				str += L"_";
			}
		}
	}
	if (!candidate_in_front) {
		str += L"_";
		str += getCandidateType().to_string();
	}

	return str;
}

wstring RelationTimexArgObservation::makeAbbrevConnectingStringSymbol(int start, 
																	  int end, 
																	  bool candidate_in_front) 
{
	if (end - start > 10) {
		return L"";
	}
	std::wstring astr = L"";
	if (candidate_in_front) {
		astr += getCandidateType().to_string();
		astr += L"_";
	}
	for (int i = start; i < end; i++) {
		bool mention_found = false;
		bool included_in_abbrev = false;
		for (int j = 0; j < _mentionSet->getNMentions(); j++) {
			const Mention *ment = _mentionSet->getMention(j);
			if (ment->getNode()->getStartToken() == i &&
				ment->getNode()->getEndToken() < end &&
				ment->getEntityType().isRecognized()) 
			{
				astr += ment->getEntityType().getName().to_string();
				i = ment->getNode()->getEndToken();
				mention_found = true;
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
					astr +=	ment->getType().to_string();
					i =	ment->getEndToken();
					value_found	= true;
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
			if (EventUtilities::includeInAbbreviatedConnectingString(tag, next_tag)) {
				astr += stem.to_string();
				included_in_abbrev = true;
			}
		}
		if (i != end - 1) {
			if (included_in_abbrev) astr += L"_";
		}
	}
	if (!candidate_in_front) {
		astr += L"_";
		astr += getCandidateType().to_string();
	}

	return astr;
}

wstring RelationTimexArgObservation::makePOSConnectingStringSymbol(int start, 
																   int end, 
																   bool candidate_in_front) 
{
	if (end - start > 10) {
		return L"";
	}
	std::wstring pstr = L"";
	if (candidate_in_front) {
		pstr += getCandidateType().to_string();
		pstr += L"_";
	}
	for (int i = start; i < end; i++) {
		bool mention_found = false;
		bool included = false;
		for (int j = 0; j < _mentionSet->getNMentions(); j++) {
			const Mention *ment = _mentionSet->getMention(j);
			if (ment->getNode()->getStartToken() == i &&
				ment->getNode()->getEndToken() < end &&
				ment->getEntityType().isRecognized()) 
			{
				pstr += ment->getEntityType().getName().to_string();
				i = ment->getNode()->getEndToken();
				mention_found = true;
				included = true;
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
					pstr +=	ment->getType().to_string();
					i =	ment->getEndToken();
					value_found	= true;
					included = true;
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
				pstr += tag.to_string();
				included = true;
			}
		}
		if (i != end - 1) {
			if (included) {
				pstr += L"_";
			}
		}
	}
	if (!candidate_in_front) {
		pstr += L"_";
		pstr += getCandidateType().to_string();
	}

	return pstr;
}

wstring RelationTimexArgObservation::makeStemmedConnectingStringSymbol(int start, 
																	   int end, 
																	   bool candidate_in_front) 
{
	if (end - start > 10) {
		return L"";
	}
	std::wstring sstr = L"";
	if (candidate_in_front) {
		sstr += getCandidateType().to_string();
		sstr += L"_";
	}
	for (int i = start; i < end; i++) {
		bool mention_found = false;
		bool included = false;
		for (int j = 0; j < _mentionSet->getNMentions(); j++) {
			const Mention *ment = _mentionSet->getMention(j);
			if (ment->getNode()->getStartToken() == i &&
				ment->getNode()->getEndToken() < end &&
				ment->getEntityType().isRecognized()) 
			{
				sstr += ment->getEntityType().getName().to_string();
				i = ment->getNode()->getEndToken();
				mention_found = true;
				included = true;
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
					sstr +=	ment->getType().to_string();
					i =	ment->getEndToken();
					value_found	= true;
					included = true;
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
				sstr += stem.to_string();
				included = true;
			}
		}
		if (i != end - 1) {
			if (included) {
				sstr += L"_";
			}
		}
	}
	if (!candidate_in_front) {
		sstr += L"_";
		sstr += getCandidateType().to_string();
	}

	return sstr;
}

void RelationTimexArgObservation::makeTimexString(int start, int end) 
{
	std::wstring str = L"";
	for (int i = start; i <= end; i++) {
		str += _tokens->getToken(i)->getSymbol().to_string();
		if (i != end - 1) {
			str += L"_";
		}
	}
	_timexString = str;
}


void RelationTimexArgObservation::findPropLink() {
	int mappedArg1 = _relMention->getLeftMention()->getIndex();
	int mappedArg2 = _relMention->getRightMention()->getIndex();

	// props that link them both
	for (int i = 0; i < _propSet->getNPropositions(); i++) {
		Proposition *p = _propSet->getProposition(i);
		Argument *a1 = 0;
		Argument *a2 = 0;
		for (int j = p->getNArgs() - 1; j >= 0; j--) { // this need not be in reversed order, though results might change slightly if it wasn't
			Argument *arg = p->getArg(j);
			if (arg->getType() == Argument::MENTION_ARG) {
				int index = arg->getMentionIndex();
				if (index == mappedArg1)
					a1 = arg;
				else if (index == mappedArg2)
					a2 = arg;
				else {
					Proposition *set = _propSet->getDefinition(index);
					if (set != 0 && set->getPredType() == Proposition::SET_PRED) {
						for (int k = 1; k < set->getNArgs(); k++) {
							Argument *setArg = set->getArg(k);
							if (setArg->getType() == Argument::MENTION_ARG) {
								int setIndex = setArg->getMentionIndex();
								// we need these breaks here so we don't take both mentions
								// from the same set
								if (setIndex == mappedArg1) {
									a1 = arg;
									break;
								} else if (setIndex == mappedArg2) {
									a2 = arg;
									break;
								}
							}
						}
					}
				}
				// if we find both in one proposition, yay!
				if (a1 != 0 && a2 != 0) {
					_propLink->populate(p, a1, a2, _mentionSet);
					/* TODO: fix this part??
					if (RelationUtilities::get()->isPrepStack(this)) {
					Proposition *def = _propSet->getDefinition(a1->getMentionIndex());
					if (def != 0)
					_propLink->populate(def, def->getArg(0), a2, _mentionSet);
					}*/
					return;
				}
			}

		}

	}
	_propLink->reset();
}
