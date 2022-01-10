// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/NameTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp> 

#include <iostream>
#include <vector>

NameTheory::NameTheory(const TokenSequence *tokSequence, int n_name_spans_, NameSpan** nameSpans_)
: _tokenSequence(tokSequence), score(0)
{
	if (nameSpans_ != 0) {
		for (int i = 0; i < n_name_spans_; i++) 
			nameSpans.push_back(nameSpans_[i]);
	}
}

NameTheory::NameTheory(const TokenSequence *tokSequence, std::vector<NameSpan*> nameSpanVector)
: _tokenSequence(tokSequence), nameSpans(nameSpanVector), score(0)
{}

NameTheory::NameTheory(const NameTheory &other, const TokenSequence *tokSequence)
: _tokenSequence(tokSequence), score(other.score)
{
	BOOST_FOREACH(NameSpan* otherSpan, other.nameSpans) {
		nameSpans.push_back(_new NameSpan(*otherSpan));
	}
}

NameTheory::~NameTheory() {
	while (!nameSpans.empty()) {
		NameSpan *s = nameSpans.back();
		nameSpans.pop_back();
		delete s;
	}
}

void NameTheory::setTokenSequence(const TokenSequence* tokenSequence) {
	if (_tokenSequence != 0)
		throw InternalInconsistencyException("NameTheory::setTokenSequence",
			"Token sequence is already set!");
	_tokenSequence = tokenSequence;
}

void NameTheory::takeNameSpan(NameSpan *nameSpan) {
	nameSpans.push_back(nameSpan);
}

void NameTheory::removeNameSpan(int i) {
	if (i < static_cast<int>(nameSpans.size()) && i >= 0) {
		NameSpan *s = nameSpans.at(i);
		nameSpans.erase(nameSpans.begin() + i);
		delete s;
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"NameTheory::removeNameSpan()", static_cast<int>(nameSpans.size()), i);
	}

}

NameSpan* NameTheory::getNameSpan(int i) const {
	if (i < static_cast<int>(nameSpans.size()) && i >= 0) {
		return nameSpans.at(i);
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"NameTheory::getNameSpan()", static_cast<int>(nameSpans.size()), i);
	}
}

std::wstring NameTheory::getNameString(int i) const {
	if (i < static_cast<int>(nameSpans.size()) && i >= 0) {
		std::wstring nameString;
		NameSpan *span = nameSpans.at(i);
		for (int j = span->start; j <= span->end; j++) {
			Symbol word = _tokenSequence->getToken(j)->getSymbol();
			nameString += word.to_string();
			if (j < span->end)
				nameString += L" ";
		}
		// transform to uppercase
		//for (i = 0; i < offset; i++)
		//	nameStr[i] = towupper(nameStr[i]);
		return nameString;
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"NameTheory::getNameString()", static_cast<int>(nameSpans.size()), i);
	}
}


void NameTheory::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Name Theory (score: " << score << "):";

	if (nameSpans.empty()) {
		out << " (no names)";
	}
	else {
		BOOST_FOREACH(NameSpan *span, nameSpans) {
			out << newline << "- " << *(span);
		}
	}

	delete[] newline;
}


void NameTheory::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	BOOST_FOREACH(NameSpan *span, nameSpans) 
		span->updateObjectIDTable();
}

void NameTheory::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"NameTheory", this);

	stateSaver->saveReal(score);

	stateSaver->saveInteger(static_cast<int>(nameSpans.size()));
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_tokenSequence);
	stateSaver->beginList(L"NameTheory::nameSpans");
	BOOST_FOREACH(NameSpan *span, nameSpans)
		span->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

NameTheory::NameTheory(StateLoader *stateLoader)
: _tokenSequence(0) 
{
	int id = stateLoader->beginList(L"NameTheory");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	score = stateLoader->loadReal();
	int n_name_spans = stateLoader->loadInteger();
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());
	stateLoader->beginList(L"NameTheory::nameSpans");
	for (int i = 0; i < n_name_spans; i++)
		nameSpans.push_back(_new NameSpan(stateLoader));
	stateLoader->endList();

	stateLoader->endList();
}

void NameTheory::resolvePointers(StateLoader * stateLoader) {
	_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
}

const wchar_t* NameTheory::XMLIdentifierPrefix() const {
	return L"nametheory";
}

void NameTheory::saveXML(SerifXML::XMLTheoryElement nametheoryElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("NameTheory::saveXML", "Expected context to be NULL");
	nametheoryElem.setAttribute(X_score, score);
	nametheoryElem.saveTheoryPointer(X_token_sequence_id, getTokenSequence());
	BOOST_FOREACH(NameSpan *span, nameSpans)
		nametheoryElem.saveChildTheory(X_Name, span, getTokenSequence());
}

NameTheory::NameTheory(SerifXML::XMLTheoryElement nameTheoryElem) {
	using namespace SerifXML;
	nameTheoryElem.loadId(this);
	score = nameTheoryElem.getAttribute<float>(X_score, 0);
	_tokenSequence = nameTheoryElem.loadTheoryPointer<TokenSequence>(X_token_sequence_id);
	if (_tokenSequence == 0)
		nameTheoryElem.reportLoadError("Expected a token_sequence_id");
	XMLTheoryElementList nameElems = nameTheoryElem.getChildElementsByTagName(X_Name, false);
	for (size_t i=0; i<nameElems.size(); ++i)
		nameSpans.push_back(_new NameSpan(nameElems[i]));


}
