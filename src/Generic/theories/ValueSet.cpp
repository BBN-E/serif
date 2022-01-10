// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Value.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp>

ValueSet::ValueSet(int n_values)
	: _n_values(n_values)
{
	if (n_values == 0) {
		_values = 0;
	}
	else {
		_values = _new Value*[n_values];
		for (int i = 0; i < n_values; i++)
			_values[i] = 0;
	}
}

ValueSet::ValueSet(std::vector<ValueSet*> splitValueSets, std::vector<int> sentenceOffsets, std::vector<ValueMentionSet*> mergedValueMentionSets, const ValueMentionSet* documentValueMentionSet, std::vector<ValueMentionSet::ValueMentionMap> &documentValueMentionMaps) {
	_n_values = 0;
	BOOST_FOREACH(ValueSet* splitValueSet, splitValueSets) {
		if (splitValueSet != NULL)
			_n_values += splitValueSet->_n_values;
	}

	if (_n_values == 0)
		_values = NULL;
	else {
		_values = _new Value*[_n_values];
		int value_set_offset = 0;
		for (size_t vs = 0; vs < splitValueSets.size(); vs++) {
			ValueSet* splitValueSet = splitValueSets[vs];
			if (splitValueSet != NULL) {
				for (int v = 0; v < splitValueSet->_n_values; v++) {
					_values[value_set_offset + v] = _new Value(*(splitValueSet->_values[v]), value_set_offset, sentenceOffsets[vs], mergedValueMentionSets, documentValueMentionSet, documentValueMentionMaps[vs]);
				}
				value_set_offset += splitValueSet->_n_values;
			}
		}
	}
}

ValueSet::~ValueSet() {
	for (int i = 0; i < _n_values; i++)
		delete _values[i];
	delete[] _values;
}

void ValueSet::takeValue(int i, Value *value) {
	if ((unsigned) i < (unsigned) _n_values) {
		delete _values[i];
		_values[i] = value;
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"ValueSet::takeValue()", _n_values, i);
	}
}

Value *ValueSet::getValue(int i) const {
	if ((unsigned) i < (unsigned) _n_values)
		return _values[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"ValueSet::getValue()", _n_values, i);
}

Value *ValueSet::getValueByValueMention(ValueMentionUID uid) const
{
	for (int i = 0; i < _n_values; i++) {
		if (_values[i]->getValMentionID() == uid)
			return _values[i];
	}
	return 0;
}

void ValueSet::updateObjectIDTable() const {

	ObjectIDTable::addObject(this);
	for (int i = 0; i < _n_values; i++)
		_values[i]->updateObjectIDTable();

}

void ValueSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"ValueSet", this);

	stateSaver->saveInteger(_n_values);
	stateSaver->beginList(L"ValueSet::_values");
	for (int i = 0; i < _n_values; i++)
		_values[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

ValueSet::ValueSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"ValueSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_n_values = stateLoader->loadInteger();
	_values = _new Value*[_n_values];
	stateLoader->beginList(L"ValueSet::_values");
	for (int i = 0; i < _n_values; i++)
		_values[i] = _new Value(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

void ValueSet::resolvePointers(StateLoader * stateLoader) {
	for (int i = 0; i < _n_values; i++)
		_values[i]->resolvePointers(stateLoader);
}

const wchar_t* ValueSet::XMLIdentifierPrefix() const {
	return L"valueset";
}

void ValueSet::saveXML(SerifXML::XMLTheoryElement valuesetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("ValueSet::saveXML", "Expected context to be NULL");
	for (int i = 0; i < _n_values; i++)
		valuesetElem.saveChildTheory(X_Value, _values[i]);
}

ValueSet::ValueSet(SerifXML::XMLTheoryElement valueSetElem)
{
	using namespace SerifXML;
	valueSetElem.loadId(this);

	XMLTheoryElementList valueElems = valueSetElem.getChildElementsByTagName(X_Value);
	_n_values = static_cast<int>(valueElems.size());
	_values = _new Value*[_n_values];
	for (int i=0; i<_n_values; ++i)
		_values[i] = _new Value(valueElems[i], i);
}
