// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/XMLStrings.h"
#include "Generic/theories/Fact.h"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

Fact::Fact(Symbol factType, Symbol patternID, double score, int score_group, int start_sentence, int end_sentence, int start_token, int end_token)
: _factType(factType), _patternID(patternID), _score(score), _score_group(score_group), _start_sentence(start_sentence), 
  _end_sentence(end_sentence), _start_token(start_token), _end_token(end_token)
{ }

Fact::~Fact() { }

void Fact::updateObjectIDTable() const {
	throw InternalInconsistencyException("Fact::updateObjectIDTable",
		"Fact does not currently have state file support");
}
void Fact::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Fact::saveState",
		"Fact does not currently have state file support");
}

void Fact::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Fact::resolvePointers",
		"Fact does not currently have state file support");
}

void Fact::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	elem.setAttribute(X_fact_type, _factType);
	if (!_patternID.is_null())
		elem.setAttribute(X_pattern_id, _patternID);
	elem.setAttribute(X_score, _score);
	if (_score_group != -1)
		elem.setAttribute(X_score_group, _score_group);
	elem.setAttribute(X_start_sentence, _start_sentence);
	elem.setAttribute(X_end_sentence, _end_sentence);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);

	BOOST_FOREACH(FactArgument_ptr arg, _arguments) {
		if (boost::dynamic_pointer_cast<MentionFactArgument>(arg)) {
			elem.saveChildTheory(X_MentionFactArgument, arg.get(), context);
		} else if (boost::dynamic_pointer_cast<ValueMentionFactArgument>(arg)) {
			elem.saveChildTheory(X_ValueMentionFactArgument, arg.get(), context);
		} else if (boost::dynamic_pointer_cast<TextSpanFactArgument>(arg)) {
			elem.saveChildTheory(X_TextSpanFactArgument, arg.get(), context);
		} else if (boost::dynamic_pointer_cast<StringFactArgument>(arg)) {
			elem.saveChildTheory(X_StringFactArgument, arg.get(), context);
		} else {
			throw UnexpectedInputException("Fact::saveXML", "Unknown fact argument type");
		}
	}
}

Fact::Fact(SerifXML::XMLTheoryElement elem, const DocTheory* theory) {
	using namespace SerifXML;

	elem.loadId(this);

	_factType = elem.getAttribute<Symbol>(X_fact_type);
	if (elem.hasAttribute(X_pattern_id))
		_patternID = elem.getAttribute<Symbol>(X_pattern_id);
	else _patternID = Symbol();
	_score = elem.getAttribute<double>(X_score);
	if (elem.hasAttribute(X_score_group))
		_score_group = elem.getAttribute<int>(X_score_group);
	else
		_score_group = -1;
	_start_sentence = elem.getAttribute<int>(X_start_sentence);
	_end_sentence = elem.getAttribute<int>(X_end_sentence);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token); 

	XMLTheoryElementList argElems = elem.getChildElements();
	size_t n_args = argElems.size();
	for (size_t i = 0; i < n_args; ++i) {
		XMLTheoryElement argElem = argElems[i];
		if (argElem.hasTag(X_MentionFactArgument))
			addFactArgument(boost::make_shared<MentionFactArgument>(argElem, theory));
		else if (argElem.hasTag(X_ValueMentionFactArgument))
			addFactArgument(boost::make_shared<ValueMentionFactArgument>(argElem, theory));
		else if (argElem.hasTag(X_TextSpanFactArgument))
			addFactArgument(boost::make_shared<TextSpanFactArgument>(argElem, theory));
		else if (argElem.hasTag(X_StringFactArgument))
			addFactArgument(boost::make_shared<StringFactArgument>(argElem, theory));
		else 
			throw UnexpectedInputException("Fact::Fact", "Unknown fact argument type");
	}
}

void Fact::addFactArgument(FactArgument_ptr argument) {
	// Makes copies of arguments to make sure we don't share arguments between facts
	if (MentionFactArgument_ptr mfa = boost::dynamic_pointer_cast<MentionFactArgument>(argument))
		_arguments.push_back(boost::make_shared<MentionFactArgument>(*mfa));
	else if (ValueMentionFactArgument_ptr vmfa = boost::dynamic_pointer_cast<ValueMentionFactArgument>(argument))
		_arguments.push_back(boost::make_shared<ValueMentionFactArgument>(*vmfa));
	else if (TextSpanFactArgument_ptr tsfa = boost::dynamic_pointer_cast<TextSpanFactArgument>(argument))
		_arguments.push_back(boost::make_shared<TextSpanFactArgument>(*tsfa));
	else if (StringFactArgument_ptr sfa = boost::dynamic_pointer_cast<StringFactArgument>(argument))
		_arguments.push_back(boost::make_shared<StringFactArgument>(*sfa));
	else
		throw UnexpectedInputException("Fact::addFactArgument", "Unknown argument type");
}
