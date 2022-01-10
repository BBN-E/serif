// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/XMLStrings.h"
#include "Generic/theories/FactArgument.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/TokenSequence.h"

FactArgument::FactArgument(Symbol role) : _role(role)
{ }

FactArgument::FactArgument(const FactArgument &other) \
	: _role(other._role) 
{ }

FactArgument::~FactArgument() { }

void FactArgument::updateObjectIDTable() const {
	throw InternalInconsistencyException("FactArgument::updateObjectIDTable",
		"FactArgument does not currently have state file support");
}
void FactArgument::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("FactArgument::saveState",
		"FactArgument does not currently have state file support");
}

void FactArgument::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("FactArgument::resolvePointers",
		"FactArgument does not currently have state file support");
}

void FactArgument::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	elem.setAttribute(X_role, _role);
}

FactArgument::FactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory) {
	using namespace SerifXML;

	_role = elem.getAttribute<Symbol>(X_role);
}

MentionFactArgument::MentionFactArgument(Symbol role, MentionUID mention_uid) 
	: FactArgument(role), _mention_uid(mention_uid)
{ }

MentionFactArgument::MentionFactArgument(const MentionFactArgument &other)
	: FactArgument(other), _mention_uid(other._mention_uid)
{ }

const Mention* MentionFactArgument::getMentionFromMentionSet(const DocTheory *docTheory) {
	return docTheory->getSentenceTheory(_mention_uid.sentno())->getMentionSet()->getMention(_mention_uid.index());			
}

void MentionFactArgument::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	const DocTheory *docTheory = dynamic_cast<const DocTheory *>(context);
	if (docTheory == 0)
		throw InternalInconsistencyException("MentionFactArgument::saveXML", "context is not DocTheory*");

	FactArgument::saveXML(elem, context);
	const Mention* mention = docTheory->getMention(_mention_uid);
	elem.saveTheoryPointer(X_mention_id, mention);
}

MentionFactArgument::MentionFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory)
	: FactArgument(elem, theory) 
{
	using namespace SerifXML;

	_mention_uid = elem.loadTheoryPointer<Mention>(X_mention_id)->getUID();
}


ValueMentionFactArgument::ValueMentionFactArgument(Symbol role, const ValueMention *valueMention, bool is_doc_date) 
	: FactArgument(role), _valueMention(valueMention), _is_doc_date(is_doc_date)
{ }

ValueMentionFactArgument::ValueMentionFactArgument(const ValueMentionFactArgument &other)
	: FactArgument(other), _valueMention(other._valueMention), _is_doc_date(other._is_doc_date)
{ }

void ValueMentionFactArgument::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	const DocTheory *docTheory = dynamic_cast<const DocTheory *>(context);
	if (docTheory == 0)
		throw InternalInconsistencyException("ValueMentionFactArgument::saveXML", "context is not DocTheory*");

	FactArgument::saveXML(elem, context);

	elem.setAttribute(X_is_doc_date, _is_doc_date);

	if (_valueMention) {
		elem.saveTheoryPointer(X_value_mention_id, _valueMention);
	}
}

ValueMentionFactArgument::ValueMentionFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory)
	: FactArgument(elem, theory) 
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_value_mention_id))
		_valueMention = elem.loadTheoryPointer<ValueMention>(X_value_mention_id);
	else
		_valueMention = 0;

	_is_doc_date = elem.getAttribute<bool>(X_is_doc_date);
}

TextSpanFactArgument::TextSpanFactArgument(Symbol role, int start_sentence, int end_sentence, int start_token, int end_token)
	: FactArgument(role), _start_sentence(start_sentence), _end_sentence(end_sentence), _start_token(start_token), _end_token(end_token)
{ }

TextSpanFactArgument::TextSpanFactArgument(const TextSpanFactArgument &other)
	: FactArgument(other), _start_sentence(other._start_sentence), _end_sentence(other._end_sentence), _start_token(other._start_token), _end_token(other._end_token)
{ }


std::wstring TextSpanFactArgument::getStringFromDocTheory(const DocTheory *docTheory) {

	std::wstringstream result;
	bool first = true;
	for (int i = getStartSentence(); i <= getEndSentence(); i++) {
		int start = 0;
		if (i == getStartSentence())
			start = getStartToken();
		const SentenceTheory *st = docTheory->getSentenceTheory(i);
		int end = st->getTokenSequence()->getNTokens() - 1;
		if (i == getEndSentence())
			end = getEndToken();
		for (int j = start; j <= end; j++) {
			if (!first)
				result << L" ";
			first = false;
			result << st->getTokenSequence()->getToken(j)->getSymbol().to_string();
		}
	}
	
	return result.str();
}


void TextSpanFactArgument::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	FactArgument::saveXML(elem, context);

	elem.setAttribute(X_start_sentence, _start_sentence);
	elem.setAttribute(X_end_sentence, _end_sentence);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);
}

TextSpanFactArgument::TextSpanFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory)
	: FactArgument(elem, theory) 
{
	using namespace SerifXML;

	_start_sentence = elem.getAttribute<int>(X_start_sentence);
	_end_sentence = elem.getAttribute<int>(X_end_sentence);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
}

StringFactArgument::StringFactArgument(Symbol role, std::wstring str) 
	: FactArgument(role), _string(str)
{ }

StringFactArgument::StringFactArgument(const StringFactArgument &other)
	: FactArgument(other), _string(other._string)
{ }

void StringFactArgument::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	FactArgument::saveXML(elem, context);

	elem.setAttribute(X_string, _string);
}

StringFactArgument::StringFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory)
	: FactArgument(elem, theory) 
{
	using namespace SerifXML;

	_string = elem.getAttribute<std::wstring>(X_string);
}
