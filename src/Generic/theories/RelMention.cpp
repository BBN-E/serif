// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/RelMention.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

RelMention::RelMention(const Mention *lhs, const Mention *rhs,
					   Symbol type, int sentno, int relno, float score)
					   : _lhs(lhs), _rhs(rhs), _timeRole(L"NULL"), _timeArg(0),
					   _type(type), _raw_type(Symbol()),
					   _score(score), _time_arg_score(0.0),
					   _confidence(1.0),
					   _modality(Modality::ASSERTED), 
					   _tense(Tense::UNSPECIFIED),
					   _uid(sentno, relno)
{
}

RelMention::RelMention(const Mention *lhs, const Mention *rhs,
					   Symbol raw_type, int sentno, int relno)
					   : _lhs(lhs), _rhs(rhs), _timeRole(L"NULL"), 
					   _timeArg(0), _type(RelationConstants::RAW),
					   _raw_type(raw_type), _score(1.0 /* right? */),
					   _time_arg_score(0.0),
					   _confidence(1.0),
					   _modality(Modality::ASSERTED), 
					   _tense(Tense::UNSPECIFIED),
					   _uid(sentno, relno)
{
}

RelMention::RelMention(RelMention &other)
: _lhs(other.getLeftMention()), _rhs(other.getRightMention()), 
_timeRole(other.getTimeRole()), _timeArg(other.getTimeArgument()), 
_type(other.getType()), _raw_type(other.getRawType()), 
_score(other.getScore()), _time_arg_score(other.getTimeArgumentScore()),
_confidence(other.getConfidence()), _uid(other.getUID()), 
_modality(other.getModality()), _tense(other.getTense())
{}

RelMention::RelMention(RelMention &other, int sent_offset, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet)
: _timeRole(other._timeRole),
_type(other._type), _raw_type(other._raw_type), 
_score(other._score), _time_arg_score(other._time_arg_score),
_confidence(other._confidence),
_modality(other._modality), _tense(other._tense)

{
	if (mentionSet == NULL)
		throw UnrecoverableException("RelMention::RelMention", "Mention set required when copying relation set with sentence offset; did you use the wrong copy constructor?");
	_uid = RelMentionUID(other.getUID().sentno() + sent_offset, other.getUID().index());

	MentionUID lhUID = MentionUID(other.getLeftMention()->getUID().sentno() + sent_offset, other.getLeftMention()->getUID().index());
	_lhs = mentionSet->getMention(lhUID);
	MentionUID rhUID = MentionUID(other.getRightMention()->getUID().sentno() + sent_offset, other.getRightMention()->getUID().index());
	_rhs = mentionSet->getMention(rhUID);

	if (other._timeArg == NULL)
		_timeArg = NULL;
	else {
		if (valueMentionSet == NULL)
			throw InternalInconsistencyException("RelMention::RelMention", "Deep copy of RelMention time argument requires ValueMentionSet");
		ValueMentionUID timeArgUID = ValueMentionUID(other._timeArg->getUID().sentno() + sent_offset, other._timeArg->getUID().index());
		_timeArg = valueMentionSet->getValueMention(timeArgUID);
	}
}

Symbol RelMention::getRoleForMention(const Mention *mention) const {
	if (mention == _lhs) {
		return Symbol(L"ARG1");
	} else if (mention == _rhs) {
		return Symbol(L"ARG2");
	} else { throw; }
}

Symbol RelMention::getRoleForValueMention(const ValueMention *value_mention) const {
	if (value_mention == _timeArg) {
		return _timeRole;
	} else { throw; }
}


void RelMention::setTimeArgument(Symbol role, const ValueMention *time, float score) { 
	_timeRole = role; 
	_timeArg = time; 
	_time_arg_score = score;
}

void RelMention::resetTimeArgument() {
	_timeArg = 0;
	_timeRole = Symbol(L"NULL");
	_time_arg_score = 0.0;
}

std::wstring RelMention::toString() const {
	std::wstringstream wss;
	wss << _type.to_string();
	if (!_raw_type.is_null()) {
		wss << L"(";
		wss << _raw_type.to_string();
		wss << L")";
	}
	wss << L": ";
	wss << _lhs->getNode()->toTextString();
	wss << L" & ";
	wss << _rhs->getNode()->toTextString();
	if (_timeArg != 0) {
		wss << L" ";
		wss << _timeRole.to_string();
		wss << L": ";
		wss << _timeArg->getUID().toInt();
	}
	wss << L"(score: ";
	wchar_t scorestr[20];
    swprintf(scorestr, 20, L"%.4g", _score);   
	wss << scorestr;
	wss << L"; confidence: ";
	wchar_t confstr[20];
    swprintf(confstr, 20, L"%.4g", _confidence);   
	wss << confstr;
	wss << L")";
	
	return wss.str();
}
std::string RelMention::toDebugString() const {
	std::stringstream ss;
	ss << _type.to_debug_string();
	if (!_raw_type.is_null()) {
		ss << "(";
		ss << _raw_type.to_debug_string();
		ss << ")";
	}
	ss << ": ";
	ss << _lhs->getNode()->toDebugTextString();
	ss << " & ";
	ss << _rhs->getNode()->toDebugTextString();
	if (_timeArg != 0) {
		ss << " ";
		ss << _timeRole.to_debug_string();
		ss << ": ";
		ss << _timeArg->getUID().toInt();
	}
	ss << "(score: ";
	char scorestr[20];
    sprintf(scorestr, "%.4g", _score);   
	ss << scorestr;
	ss << "; confidence: ";
	char confstr[20];
    sprintf(confstr, "%.4g", _confidence);   
	ss << confstr;
	ss << ")";
	return ss.str();
}

void RelMention::dump(UTF8OutputStream &out, int indent) const {
	out << L"Relation Mention " << _uid.toInt() << L": ";
	out << this->toString();
}
void RelMention::dump(std::ostream &out, int indent) const {
	out << "Relation Mention " << _uid.toInt() << ": ";
	out << this->toDebugString();
}

// For saving state:
void RelMention::updateObjectIDTable() const { ObjectIDTable::addObject(this); }
void RelMention::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"RelMention", this);
	stateSaver->saveInteger(_uid.toInt());
	stateSaver->saveSymbol(_type);
	stateSaver->saveInteger(_modality.toInt());
	stateSaver->saveInteger(_tense.toInt());
	stateSaver->savePointer(_lhs);
	stateSaver->savePointer(_rhs);
	stateSaver->saveSymbol(_timeRole);
	stateSaver->savePointer(_timeArg);

	// Added 6/12/06
	stateSaver->saveReal(_time_arg_score);
	stateSaver->saveReal(_score);
	//////////////////////////////////////

	stateSaver->endList();
}

// For loading state:
void RelMention::loadState(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"RelMention");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	_uid = RelMentionUID(stateLoader->loadInteger());
	_type = stateLoader->loadSymbol();
	_modality = ModalityAttribute::getFromInt(stateLoader->loadInteger());
	_tense = TenseAttribute::getFromInt(stateLoader->loadInteger());
	_lhs = (Mention *) stateLoader->loadPointer();
	_rhs = (Mention *) stateLoader->loadPointer();
	_timeRole = stateLoader->loadSymbol();
	_timeArg = (ValueMention *) stateLoader->loadPointer();

	// Added 6/12/06
	_time_arg_score = stateLoader->loadReal();
	_score = stateLoader->loadReal();
	//////////////////////////////////////

	stateLoader->endList();
}
void RelMention::resolvePointers(StateLoader * stateLoader) {
	_lhs = (Mention *) stateLoader->getObjectPointerTable().getPointer(_lhs);
	_rhs = (Mention *) stateLoader->getObjectPointerTable().getPointer(_rhs);
	_timeArg = (ValueMention *) stateLoader->getObjectPointerTable().getPointer(_timeArg);

}

const wchar_t* RelMention::XMLIdentifierPrefix() const {
	return L"rm";
}

void RelMention::saveXML(SerifXML::XMLTheoryElement relmentionElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("RelMention::saveXML", "Expected context to be NULL");
	relmentionElem.setAttribute(X_score, getScore());
	if (_confidence != 1.0) 
		relmentionElem.setAttribute(X_confidence, getConfidence());
	if (!getType().is_null())
		relmentionElem.setAttribute(X_type, getType());
	if (!getRawType().is_null())
		relmentionElem.setAttribute(X_raw_type, getRawType());
	//relmentionElem.setAttribute(X_rel_mention_id, getUID());
	relmentionElem.setAttribute(X_tense, _tense.toString());
	relmentionElem.setAttribute(X_modality, _modality.toString());
	// Left and right arguments
	relmentionElem.saveTheoryPointer(X_left_mention_id, getLeftMention());
	relmentionElem.saveTheoryPointer(X_right_mention_id, getRightMention());
	// Time argument
	if (getTimeArgument()) {
		relmentionElem.saveTheoryPointer(X_time_arg_id, getTimeArgument());
		relmentionElem.setAttribute(X_time_arg_role, getTimeRole());
		relmentionElem.setAttribute(X_time_arg_score, getTimeArgumentScore());
	}
	if (relmentionElem.getOptions().include_mentions_as_comments)
		relmentionElem.addComment(toString());
}

RelMention::RelMention(SerifXML::XMLTheoryElement rmElem, int sentno, int relno)
{
	using namespace SerifXML;
	rmElem.loadId(this);
	_score = rmElem.getAttribute<float>(X_score, 1.0);
	_confidence = rmElem.getAttribute<float>(X_confidence, 1.0);
	_type = rmElem.getAttribute<Symbol>(X_type);
	_raw_type = rmElem.getAttribute<Symbol>(X_raw_type, Symbol()); // correct default value?
	_tense = TenseAttribute::getFromString(rmElem.getAttribute<std::wstring>(X_tense).c_str());
	_modality = ModalityAttribute::getFromString(rmElem.getAttribute<std::wstring>(X_modality).c_str());
	// Left and right arguments
	_lhs = rmElem.loadTheoryPointer<Mention>(X_left_mention_id);
	_rhs = rmElem.loadTheoryPointer<Mention>(X_right_mention_id);
	// Time argument
	_timeArg = rmElem.loadOptionalTheoryPointer<ValueMention>(X_time_arg_id);
	if (_timeArg) {
		_timeRole = rmElem.getAttribute<Symbol>(X_time_arg_role);
		_time_arg_score = rmElem.getAttribute<float>(X_time_arg_score, 0);
	}
	// For relMention's that appear in the documentRelMentionSet, we need to 
	// derive the sentno ourselves.
	if (sentno == -1) {
		int lhs_sentno = _lhs->getSentenceNumber();
		int rhs_sentno = _rhs->getSentenceNumber();
		sentno = (lhs_sentno<rhs_sentno)?lhs_sentno:rhs_sentno;
	}
	_uid = RelMentionUID(sentno, relno);
}
