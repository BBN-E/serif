// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/PartOfSpeech.h"
#include "Generic/theories/Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include "Generic/common/ParamReader.h"

Symbol PartOfSpeech::UnlabeledPOS = Symbol(L"-UNLABELED_POS-");

int PartOfSpeech::getNPOS() const {
	return static_cast<int>(_pos_probs.size());
}

int PartOfSpeech::addPOS(Symbol pos, float prob) {
	_pos_probs.push_back(std::make_pair(pos, prob));
	return static_cast<int>(_pos_probs.size());
}

Symbol PartOfSpeech::getLabel(int i) const {
	if (i >= (int) _pos_probs.size()) {
		throw InternalInconsistencyException::arrayIndexException("PartOfSpeech::getLabel()", _pos_probs.size(), i);
	}
	return _pos_probs[i].first;
}
float PartOfSpeech::getProb(int i) const {
	if (i >= (int) _pos_probs.size()) { 
		throw InternalInconsistencyException::arrayIndexException("PartOfSpeech::getProb()", _pos_probs.size(), i);
	}
	return _pos_probs[i].second;
}

void PartOfSpeech::dump(std::ostream &out, int indent) const {
	out << "[";
	for (size_t i = 0; i < _pos_probs.size(); i++) {
		if (i > 0) { out << " "; }
		out <<"("<<_pos_probs[i].first.to_debug_string()<<" "<<_pos_probs[i].second<<")";
	}
	out <<"]";
}


void PartOfSpeech::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

#define BEGIN_PARTOFSPEECH (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::PartOfSpeechOffset))

void PartOfSpeech::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_PARTOFSPEECH, this);
	} else {
		stateSaver->beginList(L"PartOfSpeech", this);
	}
	stateSaver->saveInteger(_pos_probs.size());
	for(size_t i = 0; i<_pos_probs.size(); i++){
        stateSaver->saveSymbol(_pos_probs[i].first);
		stateSaver->saveReal(_pos_probs[i].second);
	}
	stateSaver->endList();
}

PartOfSpeech::PartOfSpeech(StateLoader *stateLoader) {
	//Use the integer replacement for "PartOfSpeech" if the state file was compressed
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_PARTOFSPEECH : L"PartOfSpeech");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	int pos_probs_size = stateLoader->loadInteger();
	for(int i = 0; i< pos_probs_size; i++){
        Symbol s = stateLoader->loadSymbol();
		float f = stateLoader->loadReal();
		_pos_probs.push_back(std::make_pair(s, f));
	}
	stateLoader->endList();
}

void PartOfSpeech::resolvePointers(StateLoader * stateLoader) {}

const wchar_t* PartOfSpeech::XMLIdentifierPrefix() const {
	return L"pos";
}

void PartOfSpeech::saveXML(SerifXML::XMLTheoryElement partofspeechElem, const Theory *context) const {
	using namespace SerifXML;
	const Token *token = dynamic_cast<const Token*>(context);
	if (context == 0)
		throw InternalInconsistencyException("PartOfSpeech::saveXML", "Expected context to be a Token");
	partofspeechElem.saveTheoryPointer(X_token_id, token);
	if (_pos_probs.size() == 0)
		return; // Nothing more to do.

	// Record the most likely part of speech tag in the <POS> element itself, and record
	// other possible tags in <ALTERNATE_POS> children.
	size_t best_pos_tag = 0;
	for (size_t i=1; i<_pos_probs.size(); ++i)
		if (_pos_probs[i].second > _pos_probs[best_pos_tag].second)
			best_pos_tag = i;
	partofspeechElem.setAttribute(X_tag, _pos_probs[best_pos_tag].first);
	if (_pos_probs[0].second < 1.0)
		partofspeechElem.setAttribute(X_prob, _pos_probs[best_pos_tag].second);
	for (size_t i=0; i<_pos_probs.size(); ++i) {
		if (i != best_pos_tag) {
			XMLTheoryElement candidateElem = partofspeechElem.addChild(X_AlternatePOS);
			candidateElem.setAttribute(X_tag, _pos_probs[i].first);
			candidateElem.setAttribute(X_prob, _pos_probs[i].second);
		}
	}
}

PartOfSpeech::PartOfSpeech(SerifXML::XMLTheoryElement partOfSpeechElem) {
	using namespace SerifXML;
	partOfSpeechElem.loadId(this);
	XMLTheoryElementList candidateElems = partOfSpeechElem.getChildElementsByTagName(X_AlternatePOS);
	if (partOfSpeechElem.hasAttribute(X_tag)) {
		
		_pos_probs.push_back(std::make_pair(partOfSpeechElem.getAttribute<Symbol>(X_tag),
			                                partOfSpeechElem.getAttribute<float>(X_prob, 1.0)));
		for (size_t i=0; i<candidateElems.size(); ++i) {
			_pos_probs.push_back(std::make_pair(candidateElems[i].getAttribute<Symbol>(X_tag),
				                                candidateElems[i].getAttribute<float>(X_prob)));
		}
	}
}
