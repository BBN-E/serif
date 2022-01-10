// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Value.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueType.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolConstants.h"

#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"

#include "boost/regex.hpp"

Value::Value(ValueMention *ment, int value_id) 
			: _ID(value_id), _valMention(ment) 
{
	_type = ValueType(_valMention->getFullType().getNameSymbol());
	ment->setDocValue(this);
}

Value::Value(Value &other, int set_offset, int sent_offset, std::vector<ValueMentionSet*> mergedValueMentionSets, const ValueMentionSet* documentValueMentionSet, ValueMentionSet::ValueMentionMap &documentValueMentionMap)
: _ID(other._ID + set_offset), _type(other._type),
_timexVal(other._timexVal), _timexAnchorVal(other._timexAnchorVal), _timexAnchorDir(other._timexAnchorDir),
_timexSet(other._timexSet), _timexMod(other._timexMod), _timexNonSpecific(other._timexNonSpecific)
{
	if (documentValueMentionMap.find(other._valMention->getUID().toInt()) != documentValueMentionMap.end()) {
		// This ValueMention is a document-level ValueMention, look it up in the map we created when merging the document ValueMentionSets.
		_valMention = documentValueMentionSet->getValueMention(documentValueMentionMap[other._valMention->getUID().toInt()]);
	} else {
		_valMention = mergedValueMentionSets[other._valMention->getUID().sentno() + sent_offset]->getValueMention(other._valMention->getUID().index());
	}
	_valMention->setDocValue(this);
}

bool Value::isSpecificDate() const {
	// YYYY exact
	static const boost::wregex timex_regex_y_exact(L"^([12][0-9][0-9][0-9])$");
	// YYYY-
	static const boost::wregex timex_regex_y_hyphen(L"^([12][0-9][0-9][0-9])-.*");

	if (_timexVal.is_null())
		return false;
	std::wstring valString = _timexVal.to_string();

	boost::wcmatch matchResult;

	return (boost::regex_match(valString.c_str(), matchResult, timex_regex_y_exact) ||
		boost::regex_match(valString.c_str(), matchResult, timex_regex_y_hyphen));
}

void Value::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void Value::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Value", this);

	stateSaver->saveInteger(_ID);
	stateSaver->saveSymbol(_type.getNameSymbol());

	// Added 6/12/06
	if (getTimexVal().is_null())
		stateSaver->saveSymbol(SymbolConstants::nullSymbol);
	else stateSaver->saveSymbol(_timexVal);
	if (getTimexAnchorVal().is_null())
		stateSaver->saveSymbol(SymbolConstants::nullSymbol); 
	else stateSaver->saveSymbol(_timexAnchorVal);
	if (getTimexAnchorDir().is_null()) 
		stateSaver->saveSymbol(SymbolConstants::nullSymbol); 
	else stateSaver->saveSymbol(_timexAnchorDir);
	if (getTimexSet().is_null()) 
		stateSaver->saveSymbol(SymbolConstants::nullSymbol); 
	else stateSaver->saveSymbol(_timexSet);
	if (getTimexMod().is_null()) 
		stateSaver->saveSymbol(SymbolConstants::nullSymbol); 
	else stateSaver->saveSymbol(_timexMod);
	if (getTimexNonSpecific().is_null()) 
		stateSaver->saveSymbol(SymbolConstants::nullSymbol); 
	else stateSaver->saveSymbol(_timexNonSpecific);
	//////////////////////////////////////////////////////

	stateSaver->savePointer(_valMention);

	stateSaver->endList();
}

Value::Value(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"Value");
	stateLoader->getObjectPointerTable().addPointer(id, this);

    _ID = stateLoader->loadInteger();
	_type = ValueType(stateLoader->loadSymbol());

	_timexVal = stateLoader->loadSymbol();
	_timexAnchorVal = stateLoader->loadSymbol();
	_timexAnchorDir = stateLoader->loadSymbol();
	_timexSet = stateLoader->loadSymbol();
	_timexMod = stateLoader->loadSymbol();
	_timexNonSpecific = stateLoader->loadSymbol();

	// Added 6/12/06
	if (_timexVal == SymbolConstants::nullSymbol)
		_timexVal = Symbol();
	if (_timexAnchorVal == SymbolConstants::nullSymbol)
		_timexAnchorVal = Symbol();
	if (_timexAnchorDir == SymbolConstants::nullSymbol)
		_timexAnchorDir = Symbol();
	if (_timexSet == SymbolConstants::nullSymbol)
		_timexSet = Symbol();
	if (_timexMod == SymbolConstants::nullSymbol)
		_timexMod = Symbol();
	if (_timexNonSpecific == SymbolConstants::nullSymbol)
		_timexNonSpecific = Symbol();
	//////////////////////////////////////////////////////

	_valMention = (ValueMention *) stateLoader->loadPointer();

	stateLoader->endList();
}

void Value::resolvePointers(StateLoader * stateLoader) {
	_valMention = (ValueMention *) stateLoader->getObjectPointerTable().getPointer(_valMention);
	_valMention->setDocValue(this);
}

const wchar_t* Value::XMLIdentifierPrefix() const {
	return L"value";
}

void Value::saveXML(SerifXML::XMLTheoryElement valueElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Value::saveXML", "Expected context to be NULL");
	valueElem.setAttribute(X_type, _type.getNameSymbol());
	valueElem.saveTheoryPointer(X_value_mention_ref, _valMention);
	if (!_timexVal.is_null())
		valueElem.setAttribute(X_timex_val, _timexVal);
	if (!_timexAnchorVal.is_null())
		valueElem.setAttribute(X_timex_anchor_val, _timexAnchorVal);
	if (!_timexAnchorDir.is_null())
		valueElem.setAttribute(X_timex_anchor_dir, _timexAnchorDir);
	if (!_timexSet.is_null())
		valueElem.setAttribute(X_timex_set, _timexSet);
	if (!_timexMod.is_null())
		valueElem.setAttribute(X_timex_mod, _timexMod);
	if (!_timexNonSpecific.is_null())
		valueElem.setAttribute(X_timex_non_specific, _timexNonSpecific);
	// Don't record _ID - we can reconstruct it at load time
}

Value::Value(SerifXML::XMLTheoryElement valueElem, int value_id)
: _ID(value_id)
{
	using namespace SerifXML;
	valueElem.loadId(this);
	_type = ValueType(valueElem.getAttribute<Symbol>(X_type));
	_valMention = const_cast<ValueMention*>(valueElem.loadTheoryPointer<ValueMention>(X_value_mention_ref));
	// Also set the ValueMention's pointer back to this Value
	_valMention->setDocValue(this);
	_timexVal = valueElem.getAttribute<Symbol>(X_timex_val, Symbol());
	_timexAnchorVal = valueElem.getAttribute<Symbol>(X_timex_anchor_val, Symbol());
	_timexAnchorDir = valueElem.getAttribute<Symbol>(X_timex_anchor_dir, Symbol());
	_timexSet = valueElem.getAttribute<Symbol>(X_timex_set, Symbol());
	_timexMod = valueElem.getAttribute<Symbol>(X_timex_mod, Symbol());
	_timexNonSpecific = valueElem.getAttribute<Symbol>(X_timex_non_specific, Symbol());
}
