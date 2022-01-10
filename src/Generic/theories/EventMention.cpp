// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"

#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventMentionSet.h"


#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include "boost/foreach.hpp"

using namespace std;

EventMention::EventMention(int sentno, int relno)
		: _score(1.0), _sent_no(sentno), 
		_eventType(SymbolConstants::nullSymbol),
		_patternId(SymbolConstants::nullSymbol),
		_indicator(SymbolConstants::nullSymbol),
		_gainLoss(SymbolConstants::nullSymbol),
		_event_id(-1), 
		_anchorProp(0), _anchorNode(0), _annotationID(SymbolConstants::nullSymbol),
		_modality(Modality::ASSERTED), _tense(Tense::UNSPECIFIED),
		_genericity(Genericity::SPECIFIC), _polarity(Polarity::POSITIVE),
		_uid(sentno, relno)
{
}

EventMention::EventMention(EventMention &other) : _score(other._score), 
		_uid(other._uid), _eventType(other._eventType),
		_patternId(other._patternId),
		_indicator(other._indicator),
		_gainLoss(other._gainLoss),
		_anchorProp(other._anchorProp),
		_anchorNode(other._anchorNode),	_event_id(other._event_id),
		_sent_no(other._sent_no), _annotationID(other._annotationID),
		_modality(other._modality), _tense(other._tense),
		_genericity(other._genericity), _polarity(other._polarity)
{
	EventArgument emptyEA;
	EventValueArgument emptyEVA;
	EventSpanArgument emptyESA;
	int i;
	for (i = 0; i < (int) other._arguments.size(); i++) {
		_arguments.push_back(emptyEA);
		_arguments[i].mention = other._arguments[i].mention;
		_arguments[i].role = other._arguments[i].role;
		_arguments[i].score = other._arguments[i].score;
	}
	for (i = 0; i < (int) other._valueArguments.size(); i++) {
		_valueArguments.push_back(emptyEVA);
		_valueArguments[i].valueMention = other._valueArguments[i].valueMention;
		_valueArguments[i].role = other._valueArguments[i].role;
		_valueArguments[i].score = other._valueArguments[i].score;
	}
	for (i = 0; i < (int) other._spanArguments.size(); i++) {
		_spanArguments.push_back(emptyESA);
		_spanArguments[i].start_token = other._spanArguments[i].start_token;
		_spanArguments[i].end_token = other._spanArguments[i].end_token;
		_spanArguments[i].role = other._spanArguments[i].role;
		_spanArguments[i].score = other._spanArguments[i].score;
	}
}

	
EventMention::EventMention(EventMention &other, int index) : _score(other._score), 
		_eventType(other._eventType),
		_patternId(other._patternId),
		_indicator(other._indicator),
		_gainLoss(other._gainLoss),
		_anchorProp(other._anchorProp),
		_anchorNode(other._anchorNode),	_event_id(other._event_id),
		_sent_no(other._sent_no), _annotationID(other._annotationID),
		_modality(other._modality), _tense(other._tense),
		_genericity(other._genericity), _polarity(other._polarity),
		_uid(other._uid.sentno(), index)
{
	EventArgument emptyEA;
	EventValueArgument emptyEVA;
	EventSpanArgument emptyESA;
	int i;	
	for (i = 0; i < (int) other._arguments.size(); i++) {
		_arguments.push_back(emptyEA);
		_arguments[i].mention = other._arguments[i].mention;
		_arguments[i].role = other._arguments[i].role;
		_arguments[i].score = other._arguments[i].score;
	}
	for (i = 0; i < (int) other._valueArguments.size(); i++) {
		_valueArguments.push_back(emptyEVA);
		_valueArguments[i].valueMention = other._valueArguments[i].valueMention;
		_valueArguments[i].role = other._valueArguments[i].role;
		_valueArguments[i].score = other._valueArguments[i].score;
	}
	for (i = 0; i < (int) other._spanArguments.size(); i++) {
		_spanArguments.push_back(emptyESA);
		_spanArguments[i].start_token = other._spanArguments[i].start_token;
		_spanArguments[i].end_token = other._spanArguments[i].end_token;
		_spanArguments[i].role = other._spanArguments[i].role;
		_spanArguments[i].score = other._spanArguments[i].score;
	}
}

EventMention::EventMention(const EventMention &other, int sent_offset, int event_offset, const Parse* parse, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet, const PropositionSet* propSet, const ValueMentionSet* documentValueMentionSet, ValueMentionSet::ValueMentionMap &documentValueMentionMap)
: _score(other._score), 
_eventType(other._eventType),
_patternId(other._patternId),
_indicator(other._indicator),
_gainLoss(other._gainLoss),
_annotationID(other._annotationID),
_modality(other._modality), _tense(other._tense),
_genericity(other._genericity), _polarity(other._polarity)
{
	_sent_no = other._sent_no + sent_offset;
	_uid = EventMentionUID(_sent_no, other._uid.index());
	_event_id = other._event_id + event_offset;

	// Reconstruct the arguments
	BOOST_FOREACH(EventArgument arg, other._arguments) {
		addArgument(arg.role, mentionSet->getMention(arg.mention->getIndex()), arg.score);
	}
	BOOST_FOREACH(EventValueArgument arg, other._valueArguments) {
		if (documentValueMentionMap.find(arg.valueMention->getUID().toInt()) != documentValueMentionMap.end()) {
			// This ValueMention is a document-level ValueMention, look it up in the map we created when merging the document ValueMentionSets.
			addValueArgument(arg.role, documentValueMentionSet->getValueMention(documentValueMentionMap[arg.valueMention->getUID().toInt()]), arg.score);
		} else {
			addValueArgument(arg.role, valueMentionSet->getValueMention(arg.valueMention->getUID().index()), arg.score);
		}
	}
	BOOST_FOREACH(EventSpanArgument arg, other._spanArguments) {
		addSpanArgument(arg.role, arg.start_token, arg.end_token, arg.score);
	}

	// Look up anchors, if any
	if (other._anchorProp != NULL)
		_anchorProp = propSet->findPropositionByUID(other._anchorProp->getID() + sent_offset*MAX_SENTENCE_PROPS);
	else
		_anchorProp = NULL;
	if (other._anchorNode != NULL)
		_anchorNode = parse->getSynNode(other._anchorNode->getID());
	else
		_anchorProp = NULL;
}

void EventMention::setAnchor(const Proposition* prop) { 
	_anchorProp = prop; 
	_anchorNode = _anchorProp->getPredHead()->getHeadPreterm();
}

void EventMention::setAnchor(const SynNode *node, const PropositionSet *propSet) {
	_anchorNode = node;
	_anchorProp = 0; 
	if (propSet != 0) {
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			if (propSet->getProposition(i)->getPredHead() != 0 &&
				propSet->getProposition(i)->getPredHead()->getHeadPreterm() == _anchorNode) {
					_anchorProp = propSet->getProposition(i);
					break;
				}
		}	
	}
}

void EventMention::addArgument(Symbol role, const Mention *mention, float score) {
	for (int i = 0; i < (int) _arguments.size(); i++) {
		if (_arguments[i].role == role && _arguments[i].mention == mention)  {
			SessionLogger::warn("duplicate_event_argument") << "Tried to add duplicate argument to EventMention.";
			return;
		}
	}
    	EventArgument empty;
  	size_t size = _arguments.size();
    	_arguments.push_back(empty);
	_arguments[size].role = role;
	_arguments[size].mention = mention;
	_arguments[size].score = score;
}

void EventMention::addValueArgument(Symbol role, const ValueMention *valueMention, float score) {
	for (int i = 0; i < (int) _valueArguments.size(); i++) {
		if (_valueArguments[i].role == role && _valueArguments[i].valueMention == valueMention)  {
			SessionLogger::warn("duplicate_event_argument") << "Tried to add duplicate value argument to EventMention.";
			return;
		}
	}
    	size_t size = _valueArguments.size();
    	EventValueArgument empty;
    	_valueArguments.push_back(empty);
	_valueArguments[size].role = role;
	_valueArguments[size].valueMention = valueMention;
	_valueArguments[size].score = score;
}

void EventMention::addSpanArgument(Symbol role, int startToken, int endToken, float score) {
	for (int i = 0; i < (int) _spanArguments.size(); i++) {
		if (_spanArguments[i].role == role && _spanArguments[i].start_token == startToken && _spanArguments[i].end_token == endToken)  {
			SessionLogger::warn("duplicate_event_argument") << "Tried to add duplicate span argument to EventMention.";
			return;
		}
	}
	size_t size = _spanArguments.size();
	EventSpanArgument empty;
	_spanArguments.push_back(empty);
	_spanArguments[size].role = role;
	_spanArguments[size].start_token = startToken;
	_spanArguments[size].end_token = endToken;
	_spanArguments[size].score = score;
}

// will just return the first one found: used in EELD where there is only one per role type
const Mention *EventMention::getFirstMentionForSlot(Symbol slotName) const {
	for (int i = 0; i < (int) _arguments.size(); i++) {
		if (_arguments[i].role == slotName) 
			return _arguments[i].mention;
	}
	return 0;
}
// will just return the first one found
const ValueMention *EventMention::getFirstValueForSlot(Symbol slotName) const {
	for (int i = 0; i < (int) _valueArguments.size(); i++) {
		if (_valueArguments[i].role == slotName) 
			return _valueArguments[i].valueMention;
	}
	return 0;
}


Symbol EventMention::getRoleForMention(const Mention *mention) const {
	for (int arg = 0; arg < getNArgs(); arg++) {
		if (getNthArgMention(arg) == mention) {
			return getNthArgRole(arg);
		}
	}
	return Symbol();
}

Symbol EventMention::getRoleForValueMention(const ValueMention *valueMention) const {
	for (int arg = 0; arg < getNValueArgs(); arg++) {
		if (getNthArgValueMention(arg) == valueMention) {
			return getNthArgValueRole(arg);
		}
	}
	return Symbol();
}

Event* EventMention::getEvent(const DocTheory* docTheory) const {
	return docTheory->getEventSet()->getEvent(_event_id);
}

std::wstring EventMention::toString(const TokenSequence *tokens) const {
	int i;
	std::wstring result = L"\n";
	result += _eventType.to_string();
	result += L":\n  anchor ==> ";
	result += getAnchorNode()->getHeadWord().to_string();
	result += L"\n";
	if (_patternId != SymbolConstants::nullSymbol){
		result += L"  pattern id ==> ";
		result += _patternId.to_string();
		result += L"\n";
	}
	if (_gainLoss != SymbolConstants::nullSymbol){
		result += L"  gain/Loss ==> ";
		result += _gainLoss.to_string();
		result += L"\n";
	}
	if (_indicator != SymbolConstants::nullSymbol){
		result += L"  indicator ==> ";
		result += _indicator.to_string();
		result += L"\n";
	}
	if (getAnchorProp() != 0) {
		result += L"  anchor prop ==> ";
		result += getAnchorProp()->toString();
		result += L"\n";
	}		
	for (i = 0; i < (int) _arguments.size(); i++) {
		result += L"  ";
		result += _arguments[i].role.to_string();
		result += L" ==> ";
		result += _arguments[i].mention->getNode()->toTextString();
		wchar_t buffer[100];
		swprintf(buffer, 100, L" (%f)", _arguments[i].score);
		result += buffer;
		result += L"\n";
	}
	for (i = 0; i < (int) _valueArguments.size(); i++) {
		result += L"  ";
		result += _valueArguments[i].role.to_string();
		result += L" ==> ";
		if (tokens) {
			result += tokens->toString(_valueArguments[i].valueMention->getStartToken(), _valueArguments[i].valueMention->getEndToken());
		}  else {
			result += L"(not printed)";
		}
		wchar_t buffer[100];
		swprintf(buffer, 100, L" (%f)", _valueArguments[i].score);
		result += buffer;
		result += L"\n";
	}
	for (i = 0; i< (int) _spanArguments.size(); i++) {
		result += L"  ";
		result += _spanArguments[i].role.to_string();
		result += L" ==> ";
		if (tokens) {
			result += tokens->toString(_spanArguments[i].start_token, _spanArguments[i].end_token);
		}  else {
			result += L"(not printed)";
		}
		wchar_t buffer[100];
		swprintf(buffer, 100, L" %i %i (%f)", _spanArguments[i].start_token, _spanArguments[i].end_token, _spanArguments[i].score);
		result += buffer;
		result += L"\n";
	}
	return result;
}

std::string EventMention::toDebugString(const TokenSequence *tokens) const {
	int i;
	std::string result = "\n";
	result += _eventType.to_debug_string();
	result += ":\n  anchor ==> ";
	result += getAnchorNode()->getHeadWord().to_debug_string();
	result += "\n";
	if (_patternId != SymbolConstants::nullSymbol){
		result += "  pattern id ==> ";
		result += _patternId.to_debug_string();
		result += "\n";
	}
	if (_gainLoss != SymbolConstants::nullSymbol){
		result += "  gain/Loss ==> ";
		result += _gainLoss.to_debug_string();
		result += "\n";
	}
	if (_indicator != SymbolConstants::nullSymbol){
		result += "  indicator ==> ";
		result += _indicator.to_debug_string();
		result += "\n";
	}
	if (getAnchorProp() != 0) {
		result += "  anchor prop ==> ";
		result += getAnchorProp()->toDebugString();
		result += "\n";
	}		
	for (i = 0; i < (int) _arguments.size(); i++) {
		result += "  ";
		result += _arguments[i].role.to_debug_string();
		result += " ==> ";
		result += _arguments[i].mention->getNode()->toDebugTextString();
		char buffer[100];
		sprintf(buffer, " (%f)", _arguments[i].score);
		result += buffer;
		result += "\n";
	}		
	for (i = 0; i < (int) _valueArguments.size(); i++) {
		result += "  ";
		result += _valueArguments[i].role.to_debug_string();
		result += " ==> ";
		if (tokens) {
			result += tokens->toDebugString(_valueArguments[i].valueMention->getStartToken(), _valueArguments[i].valueMention->getEndToken());
		}  else {
			result += "(not printed)";
		}
		char buffer[100];
		sprintf(buffer, " %i %i (%f)", _valueArguments[i].valueMention->getStartToken(), _valueArguments[i].valueMention->getEndToken(), _valueArguments[i].score);
		result += buffer;		
		result += "\n";
	}
	for (i = 0; i< (int) _spanArguments.size(); i++) {
		result += "  ";
		result += _spanArguments[i].role.to_debug_string();
		result += " ==> ";
		if (tokens) {
			result += tokens->toDebugString(_spanArguments[i].start_token, _spanArguments[i].end_token);
		}  else {
			result += "(not printed)";
		}
		char buffer[100];
		sprintf(buffer, " %i %i (%f)", _spanArguments[i].start_token, _spanArguments[i].end_token, _spanArguments[i].score);
		result += buffer;
		result += "\n";
	}
	//char buffer[200];
	//sprintf(buffer, "score: %f, simple score: %f, prior score: %f", _score,
	//	_simple_score, _score_with_prior);
	//result += buffer;
	result += "\n";
	return result;
}

void EventMention::dump(UTF8OutputStream &out, int indent) const {
	out << L"Event Mention " << _uid.toInt() << L": ";
	out << this->toString();
}
void EventMention::dump(std::ostream &out, int indent) const {
	out << "Event Mention " << _uid.toInt() << ": ";
	out << this->toDebugString();
}

// For saving state:
void EventMention::updateObjectIDTable() const { ObjectIDTable::addObject(this); }
void EventMention::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"EventMention", this);
	stateSaver->saveInteger(_uid.toInt());
	stateSaver->saveInteger(_event_id);
	stateSaver->saveSymbol(_eventType);
	stateSaver->saveSymbol(_patternId);
	stateSaver->saveSymbol(_gainLoss);
	stateSaver->saveSymbol(_indicator);
	stateSaver->savePointer(_anchorNode);
	stateSaver->savePointer(_anchorProp);
	stateSaver->saveInteger(_modality.toInt());
	stateSaver->saveInteger(_genericity.toInt());
	stateSaver->saveInteger(_tense.toInt());
	stateSaver->saveInteger(_polarity.toInt());
	
	// Added 6/12/06
	stateSaver->saveInteger(_sent_no);
	stateSaver->saveReal(_score);
	/////////////////////////////////////

	stateSaver->saveInteger(_arguments.size());
	stateSaver->saveInteger(_valueArguments.size());
	stateSaver->saveInteger(_spanArguments.size());

	int i;
	for (i = 0; i < (int) _arguments.size(); i++) {
		stateSaver->saveSymbol(_arguments[i].role);
		stateSaver->savePointer(_arguments[i].mention);
		stateSaver->saveReal(_arguments[i].score);
	}		
	for (i = 0; i < (int) _valueArguments.size(); i++) {
		stateSaver->saveSymbol(_valueArguments[i].role);
		stateSaver->savePointer(_valueArguments[i].valueMention);
		stateSaver->saveReal(_valueArguments[i].score);
	}
	for (i = 0; i < (int) _spanArguments.size(); i++) {
		stateSaver->saveSymbol(_spanArguments[i].role);
		stateSaver->saveInteger(_spanArguments[i].start_token);
		stateSaver->saveInteger(_spanArguments[i].end_token);
		stateSaver->saveReal(_spanArguments[i].score);
	}
	stateSaver->endList();
}
// For loading state:
void EventMention::loadState(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"EventMention");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	_uid = EventMentionUID(stateLoader->loadInteger());
	_event_id = stateLoader->loadInteger();
	_eventType = stateLoader->loadSymbol();
	_patternId = stateLoader->loadSymbol();
	_gainLoss = stateLoader->loadSymbol();
	_indicator = stateLoader->loadSymbol();
	_anchorNode = (SynNode *) stateLoader->loadPointer();
	_anchorProp = (Proposition *) stateLoader->loadPointer();
	_modality = ModalityAttribute::getFromInt(stateLoader->loadInteger());	
	_genericity = GenericityAttribute::getFromInt(stateLoader->loadInteger());		
	_tense = TenseAttribute::getFromInt(stateLoader->loadInteger());		
	_polarity = PolarityAttribute::getFromInt(stateLoader->loadInteger());

	// Added 6/12/06
	_sent_no = stateLoader->loadInteger();
	_score = stateLoader->loadReal();
	/////////////////////////////////////

	int numArguments = stateLoader->loadInteger();
	int numValueArguments = stateLoader->loadInteger();
	int numSpanArguments = stateLoader->loadInteger();

	int i;
	for (i = 0; i < numArguments; i++) {
		EventArgument empty;
		_arguments.push_back(empty);
		_arguments[i].role = stateLoader->loadSymbol();
		_arguments[i].mention = (Mention *) stateLoader->loadPointer();
		_arguments[i].score = stateLoader->loadReal();
	}				
	for (i = 0; i < numValueArguments; i++) {
		EventValueArgument empty;
		_valueArguments.push_back(empty);
		_valueArguments[i].role = stateLoader->loadSymbol();
		_valueArguments[i].valueMention = (ValueMention *) stateLoader->loadPointer();
		_valueArguments[i].score = stateLoader->loadReal();
	}
	for (i = 0; i < numSpanArguments; i++) {
		EventSpanArgument empty;
		_spanArguments.push_back(empty);
		_spanArguments[i].role = stateLoader->loadSymbol();
		_spanArguments[i].start_token = stateLoader->loadInteger();
		_spanArguments[i].end_token = stateLoader->loadInteger();
		_spanArguments[i].score = stateLoader->loadReal();
	}
	stateLoader->endList();
}

void EventMention::resolvePointers(StateLoader * stateLoader) {
	int i;
	_anchorNode = (SynNode *) stateLoader->getObjectPointerTable().getPointer(_anchorNode);	
	_anchorProp = (Proposition *) stateLoader->getObjectPointerTable().getPointer(_anchorProp);	
	for (i = 0; i < (int) _arguments.size(); i++) {
		_arguments[i].mention 
			= (Mention *) stateLoader->getObjectPointerTable().getPointer(_arguments[i].mention);	
	}
	for (i = 0; i < (int) _valueArguments.size(); i++) {
		_valueArguments[i].valueMention 
			= (ValueMention *) stateLoader->getObjectPointerTable().getPointer(_valueArguments[i].valueMention);	
	}		

}


// For ModalityClassifier - 2008/01/17
// this part was not effecient for Modality Classifier model itself
// the purpose of designing code in this way is to keep the rest part of EventMention.cpp untouched
// may need to refine this part in the future for efficiency purpose
Symbol EventMention::getModalityType() const {
	if (_modality.is_defined()){
		return Symbol(_modality.toString());
	} else {
		return Symbol(L"NULL");
	}	
}


void EventMention::setModalityType(Symbol mtype){
	if (mtype == Symbol(L"Asserted")){
		setModality(Modality::ASSERTED);
	}else if (mtype == Symbol(L"Non-Asserted")){
		setModality(Modality::OTHER);
	}else{
		string msg = "Invalid modality Symbol ";
		msg += mtype.to_debug_string();
		throw UnexpectedInputException("EventMention::setModalityType()", msg.c_str());
	}
}

const wchar_t* EventMention::XMLIdentifierPrefix() const {
	return L"eventmention";
}

void EventMention::saveXML(SerifXML::XMLTheoryElement eventmentionElem, const Theory *context) const {
	using namespace SerifXML;
	const EventMentionSet *emSet = dynamic_cast<const EventMentionSet*>(context);
	if (context == 0)
		throw InternalInconsistencyException("EventMention::saveXML", "Expected context to be an EventMentionSet");

	// Attributes of the EventMention
	eventmentionElem.setAttribute(X_score, _score);
	eventmentionElem.setAttribute(X_event_type, _eventType);
	if (_patternId != SymbolConstants::nullSymbol){
		eventmentionElem.setAttribute(X_pattern_id, _patternId);
	}
	if (_gainLoss != SymbolConstants::nullSymbol){
		eventmentionElem.setAttribute(X_gainLoss, _gainLoss);
	}
	if (_indicator != SymbolConstants::nullSymbol){
		eventmentionElem.setAttribute(X_indicator, _indicator);
	}
	// _sent_no is redundant -- so we don't bother to record it.
	eventmentionElem.setAttribute(X_genericity, _genericity.toString());
	eventmentionElem.setAttribute(X_polarity, _polarity.toString());
	eventmentionElem.setAttribute(X_tense, _tense.toString());
	eventmentionElem.setAttribute(X_modality, _modality.toString());
	// Anchor proposition
	if (_anchorProp)
		eventmentionElem.saveTheoryPointer(X_anchor_prop_id, _anchorProp);
	// Anchor node
	eventmentionElem.saveTheoryPointer(X_anchor_node_id, _anchorNode);
	// Arguments
	for (int i = 0; i < (int) _arguments.size(); i++) {
		XMLTheoryElement argElem = eventmentionElem.addChild(X_EventMentionArg);
		argElem.saveTheoryPointer(X_mention_id, _arguments[i].mention);
		argElem.setAttribute(X_role, _arguments[i].role);
		argElem.setAttribute(X_score, _arguments[i].score);
	}
	// Value Arguments
	for (int j = 0; j < (int) _valueArguments.size(); j++) {
		XMLTheoryElement argElem = eventmentionElem.addChild(X_EventMentionArg);
		argElem.saveTheoryPointer(X_value_mention_id, _valueArguments[j].valueMention);
		argElem.setAttribute(X_role, _valueArguments[j].role);
		argElem.setAttribute(X_score, _valueArguments[j].score);
	}
	// Span Arguments
	for (int k = 0; k < (int) _spanArguments.size(); k++) {
		XMLTheoryElement argElem = eventmentionElem.addChild(X_EventMentionArg);
		argElem.setAttribute(X_role, _spanArguments[k].role);
		

		const TokenSequence* tokSeq = emSet->getParse()->getTokenSequence();
		// Pointers to start/end tokens:
		const Token *startTok = tokSeq->getToken( _spanArguments[k].start_token);
		argElem.saveTheoryPointer(X_start_token, startTok);
		const Token *endTok = tokSeq->getToken(_spanArguments[k].end_token);
		argElem.saveTheoryPointer(X_end_token, endTok);
		// Start/end offsets:
		const OffsetGroup &startOffset = startTok->getStartOffsetGroup();
		const OffsetGroup &endOffset = endTok->getEndOffsetGroup();
		argElem.saveOffsets(startOffset, endOffset);
		
		
		//argElem.setAttribute(X_start_token, _spanArguments[k].start_token);
		//argElem.setAttribute(X_end_token, _spanArguments[k].end_token);
		argElem.setAttribute(X_score, _spanArguments[k].score);
	}
	if (eventmentionElem.getOptions().include_mentions_as_comments)
		eventmentionElem.addComment(toString());
	// We don't bother to serialize event_id, since we can reconstruct it when 
	// we deserialize the event (in the Event::Event(XMLTheoryElement) constructor)
}

EventMention::EventMention(SerifXML::XMLTheoryElement eventMentionElem, int sentno, int relno)
: _sent_no(sentno), _uid(sentno, relno), _event_id(-1)
{
	using namespace SerifXML;
	eventMentionElem.loadId(this);
	_score = eventMentionElem.getAttribute<float>(X_score, 0);
	// _event_id get set by the Event::Event(XMLTheoryElement) constructor
	_eventType = eventMentionElem.getAttribute<Symbol>(X_event_type);
	if (eventMentionElem.hasAttribute(X_pattern_id)){
		_patternId = eventMentionElem.getAttribute<Symbol>(X_pattern_id);
	}else{
		_patternId = SymbolConstants::nullSymbol;
	}
	if (eventMentionElem.hasAttribute(X_indicator)){
		_indicator = eventMentionElem.getAttribute<Symbol>(X_indicator);
	}else{
		_indicator = SymbolConstants::nullSymbol;
	}
	if (eventMentionElem.hasAttribute(X_gainLoss)){
		_gainLoss = eventMentionElem.getAttribute<Symbol>(X_gainLoss);
	}else{
		_gainLoss = SymbolConstants::nullSymbol;
	}

	_genericity = GenericityAttribute::getFromString(eventMentionElem.getAttribute<std::wstring>(X_genericity).c_str());
	_polarity = PolarityAttribute::getFromString(eventMentionElem.getAttribute<std::wstring>(X_polarity).c_str());
	_tense = TenseAttribute::getFromString(eventMentionElem.getAttribute<std::wstring>(X_tense).c_str());
	_modality = ModalityAttribute::getFromString(eventMentionElem.getAttribute<std::wstring>(X_modality).c_str());

	// Anchor proposition
	_anchorProp = eventMentionElem.loadOptionalTheoryPointer<Proposition>(X_anchor_prop_id);
	// Anchor node
	_anchorNode = eventMentionElem.loadTheoryPointer<SynNode>(X_anchor_node_id);

	// Arguments.
	XMLTheoryElementList argElems = eventMentionElem.getChildElementsByTagName(X_EventMentionArg);

	// backwards compatibility: we formerly used <EventArg>
	XMLTheoryElementList argElems2 = eventMentionElem.getChildElementsByTagName(X_EventArg, false);
	argElems.insert(argElems.end(), argElems2.begin(), argElems2.end());

    	size_t i = 0;
    	EventArgument empty_EventArgument;
    	EventValueArgument empty_EventValueArgument;
		EventSpanArgument empty_EventSpanArgument;
	BOOST_FOREACH(XMLTheoryElement argElem, argElems) {
		if (argElem.hasAttribute(X_mention_id)) {
            		i = _arguments.size();
            		_arguments.push_back(empty_EventArgument);
			_arguments[i].mention = argElem.loadTheoryPointer<Mention>(X_mention_id);
			_arguments[i].role = argElem.getAttribute<Symbol>(X_role);
			_arguments[i].score = argElem.getAttribute<float>(X_score, 0);
		} else if (argElem.hasAttribute(X_value_mention_id)) {
			i = _valueArguments.size();
			_valueArguments.push_back(empty_EventValueArgument);
			_valueArguments[i].valueMention = 0; // Filled in by resolvePointers(XMLTheoryElement)
			_valueArguments[i].role = argElem.getAttribute<Symbol>(X_role);
			_valueArguments[i].score = argElem.getAttribute<float>(X_score, 0);
		} else if (argElem.hasAttribute(X_start_token)) {
			i = _spanArguments.size();
			_spanArguments.push_back(empty_EventSpanArgument);
			_spanArguments[i].role = argElem.getAttribute<Symbol>(X_role);
			//static_cast<int>(xmldoc->lookupTokenIndex(vmElem.loadTheoryPointer<Token>(X_start_token)));
			
			//Start token and end token handled in resolvePointers
			_spanArguments[i].score = argElem.getAttribute<float>(X_score, 0);
		}else {
			argElem.reportLoadError("Expected mention_id or value_mention_id or start_token");
		}
	}
}

void EventMention::resolvePointers(SerifXML::XMLTheoryElement eventMentionElem, const Parse* parse) {
	using namespace SerifXML;
	XMLTheoryElementList argElems = eventMentionElem.getChildElementsByTagName(X_EventMentionArg);

	// backwards compatibility: we formerly used <EventArg>
	XMLTheoryElementList argElems2 = eventMentionElem.getChildElementsByTagName(X_EventArg, false);
	argElems.insert(argElems.end(), argElems2.begin(), argElems2.end());

	int n_value_args = 0;
	int n_span_args = 0;
	BOOST_FOREACH(XMLTheoryElement argElem, argElems) {
		if (argElem.hasAttribute(X_value_mention_id)) {
			_valueArguments[n_value_args].valueMention = argElem.loadTheoryPointer<ValueMention>(X_value_mention_id);
			++n_value_args;
		}else if (argElem.hasAttribute(X_start_token)) {
			const Token* startToken = argElem.loadTheoryPointer<Token>(X_start_token);
			const Token* endToken = argElem.loadTheoryPointer<Token>(X_end_token);
			for (int i=0; i<parse->getTokenSequence()->getNTokens(); i++){
				if (startToken == parse->getTokenSequence()->getToken(i)){
					_spanArguments[n_span_args].start_token = i;
				}
				if (endToken == parse->getTokenSequence()->getToken(i)){
					_spanArguments[n_span_args].end_token = i;
				}
			}

			++n_span_args;
		}

	}
}
