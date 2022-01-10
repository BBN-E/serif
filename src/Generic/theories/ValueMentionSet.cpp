// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include <boost/format.hpp>

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/DocTheory.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp>

ValueMentionSet::ValueMentionSet(const TokenSequence *tokenSequence, int n_values) 
: _n_values(n_values), _score(0.0f), _tokenSequence(tokenSequence)
{
	if (n_values == 0) {
		_values = 0;
	} else {
		_values = _new ValueMention*[n_values];
		for (int i = 0; i < n_values; i++)
			_values[i] = 0;
	}
}

ValueMentionSet::ValueMentionSet(const ValueMentionSet &other, const TokenSequence *tokenSequence)
: _n_values(other._n_values), _score(other._score)
{
	if (tokenSequence == NULL)
		_tokenSequence = other._tokenSequence;
	else
		_tokenSequence = tokenSequence;
	_values = _new ValueMention*[_n_values];
	for (int i = 0; i < _n_values; i++) {
		if (tokenSequence != NULL)
			// Create an equivalent value mention with the appropriate sentence reassignment
			_values[i] = _new ValueMention(tokenSequence->getSentenceNumber(), ValueMentionUID(tokenSequence->getSentenceNumber(), other._values[i]->getUID().index()), other._values[i]->getStartToken(), other._values[i]->getEndToken(), other._values[i]->getFullType().getNameSymbol());
		else
			_values[i] = _new ValueMention(*(other._values[i]));
	}
}

ValueMentionSet::ValueMentionSet(std::vector<ValueMentionSet*> splitValueMentionSets, std::vector<int> sentenceOffsets, std::vector<ValueMentionMap> &valueMentionMaps, int total_sentences)
{
	_n_values = 0;
	BOOST_FOREACH(ValueMentionSet* splitValueMentionSet, splitValueMentionSets) {
		if (splitValueMentionSet != NULL)
			_n_values += splitValueMentionSet->getNValueMentions();
		valueMentionMaps.push_back(ValueMentionMap());
	}
	if (_n_values > 0)
		_values = _new ValueMention*[_n_values];
	else
		_values = NULL;

	// For document-level value mentions, no tokens
	_tokenSequence = NULL;

	// Take the highest set score when merging
	_score = 0.0;
	int value_mention_set_offset = 0;
	for (size_t vms = 0; vms < splitValueMentionSets.size(); vms++) {
		ValueMentionSet* splitValueMentionSet = splitValueMentionSets[vms];
		if (splitValueMentionSet != NULL) {
			if (splitValueMentionSet->_score > _score)
				_score = splitValueMentionSet->_score;

			// Renumber the document ValueMentions here instead of implementing a different deep copier
			for (int vm = 0; vm < splitValueMentionSet->getNValueMentions(); vm++) {
				ValueMention* splitValueMention = splitValueMentionSet->getValueMention(vm);
				_values[value_mention_set_offset + vm] = _new ValueMention(sentenceOffsets[vms] + splitValueMention->getSentenceNumber(), ValueMentionUID(total_sentences, value_mention_set_offset + vm), splitValueMention->getStartToken(), splitValueMention->getEndToken(), splitValueMention->getFullType().getNameSymbol());
				valueMentionMaps[vms][splitValueMention->getUID().toInt()] = _values[value_mention_set_offset + vm]->getUID();
			}
			value_mention_set_offset += splitValueMentionSet->getNValueMentions();
		}
	}
}

ValueMentionSet::~ValueMentionSet() {
	for (int i = 0; i < _n_values; i++) {
		delete _values[i];
	}
	delete [] _values;
}

void ValueMentionSet::setTokenSequence(const TokenSequence* tokenSequence) {
	if (_tokenSequence != 0)
		throw InternalInconsistencyException("ValueMentionSet::setTokenSequence",
			"Token sequence is already set!");
	_tokenSequence = tokenSequence;
}

void ValueMentionSet::takeValueMention(int i, ValueMention *vmention) {
	if ((unsigned) i < (unsigned) _n_values) {
		delete _values[i];
		_values[i] = vmention;

	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"ValueMentionSet::takeValueMention()", _n_values, i);
	}
}

ValueMention *ValueMentionSet::getValueMention(ValueMentionUID id) const {
	return getValueMention(id.index());
}
ValueMention *ValueMentionSet::getValueMention(int index) const {
	if (index < _n_values)
		return _values[index];
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"ValueMentionSet::getValueMention()", _n_values, index);
	}
}

void ValueMentionSet::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	// If we don't limit precision, we get a lot of instability showing up here, making diffing annoying.
	out << "Value Mention Set (" << _n_values << " values; score: " << (boost::format("%.4f") % _score) << "):";

	if (_n_values == 0) {
		out << newline << "  (no values)";
	} else {
		for (int i = 0; i < _n_values; i++) 
			out << newline << "- " << *(_values[i]);
	}

	delete[] newline;
}


void ValueMentionSet::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	for (int i = 0; i < _n_values; i++) {
		_values[i]->updateObjectIDTable();
	}
}

void ValueMentionSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"ValueMentionSet", this);

	if (stateSaver->getVersion() >= std::make_pair(1,6)) {
		stateSaver->saveReal(_score);
		stateSaver->savePointer(_tokenSequence);
	}

	stateSaver->saveInteger(_n_values);
	stateSaver->beginList(L"ValueMentionSet::_values");
	for (int i = 0; i < _n_values; i++) {
		_values[i]->saveState(stateSaver);
	}
	stateSaver->endList();

	stateSaver->endList();
}

ValueMentionSet::ValueMentionSet(StateLoader *stateLoader)
: _score(0), _tokenSequence(0)
{
	int id = stateLoader->beginList(L"ValueMentionSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	if (stateLoader->getVersion() >= std::make_pair(1,6)) {
		_score = stateLoader->loadReal();
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());
	}

	_n_values = stateLoader->loadInteger();
	_values = _new ValueMention*[_n_values];
	stateLoader->beginList(L"ValueMentionSet::_values");
	for (int i = 0; i < _n_values; i++) {
		_values[i] = _new ValueMention();
		_values[i]->loadState(stateLoader);
	}

	stateLoader->endList();

	stateLoader->endList();
}

void ValueMentionSet::resolvePointers(StateLoader * stateLoader) {
	_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
	for (int i = 0; i < _n_values; i++)
		_values[i]->resolvePointers(stateLoader);
}

const wchar_t* ValueMentionSet::XMLIdentifierPrefix() const {
	return L"vmset";
}

void ValueMentionSet::saveXML(SerifXML::XMLTheoryElement valuementionsetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context == 0) {
		for (int i = 0; i < _n_values; ++i) {
			valuementionsetElem.saveChildTheory(X_ValueMention, _values[i], getTokenSequence());
		} 
	} else { // must be a document-level ValueMentionSet
		const DocTheory *docTheory = dynamic_cast<const DocTheory*>(context);
		for (int i = 0; i < _n_values; ++i) {
			const TokenSequence *tokSeq = docTheory->getSentenceTheory(_values[i]->getSentenceNumber())->getTokenSequence();
			valuementionsetElem.saveChildTheory(X_ValueMention, _values[i], tokSeq);
		} 
	}
	valuementionsetElem.setAttribute(X_score, _score);
	if (getTokenSequence())
		valuementionsetElem.saveTheoryPointer(X_token_sequence_id, getTokenSequence());
	
}

ValueMentionSet::ValueMentionSet(SerifXML::XMLTheoryElement vmSetElem, DocTheory *docTheory)
: _values(0), _n_values(0)
{
	using namespace SerifXML;
	vmSetElem.loadId(this);
	_score = vmSetElem.getAttribute<float>(X_score, 0);
	_tokenSequence = vmSetElem.loadOptionalTheoryPointer<TokenSequence>(X_token_sequence_id);
	if (_tokenSequence == 0 && docTheory == 0)
		vmSetElem.reportLoadError("Expected either TokenSequence or DocTheory to be non-NULL");
	int sent_no = (_tokenSequence != 0) ? _tokenSequence->getSentenceNumber() : docTheory->getNSentences();
	XMLTheoryElementList vmElems = vmSetElem.getChildElementsByTagName(X_ValueMention);
	_n_values = static_cast<int>(vmElems.size());
	_values = _new ValueMention*[_n_values];
	for (int i=0; i<_n_values; ++i)
		_values[i] = _new ValueMention(vmElems[i], sent_no, i);
}
