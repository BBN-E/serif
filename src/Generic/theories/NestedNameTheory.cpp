// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/NestedNameSpan.h"
#include "Generic/theories/TokenSequence.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp> 

NestedNameTheory::NestedNameTheory(const TokenSequence *tokSequence, const NameTheory *nameTheory, std::vector<NestedNameSpan*> nestedNameSpans) 
		: NameTheory(tokSequence), _nameTheory(nameTheory) 
{
	BOOST_FOREACH(NestedNameSpan* span, nestedNameSpans) {
		takeNameSpan((NameSpan*)span);
	}
}

void NestedNameTheory::setNameTheory(const NameTheory* nameTheory) {
	if (_nameTheory != 0)
		throw InternalInconsistencyException("NestedNameTheory::setNameTheory",
			"Name theory is already set!");
	_nameTheory = nameTheory;
}

void NestedNameTheory::updateObjectIDTable() const {
	NameTheory::updateObjectIDTable();
}

void NestedNameTheory::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"NestedNameTheory", this);

	stateSaver->saveReal(score);

	stateSaver->saveInteger(static_cast<int>(nameSpans.size()));
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_tokenSequence);
	stateSaver->savePointer(_nameTheory);
	stateSaver->beginList(L"NestedNameTheory::nameSpans");
	BOOST_FOREACH(NameSpan *span, nameSpans)
		span->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

NestedNameTheory::NestedNameTheory(StateLoader *stateLoader)
: NameTheory(0, 0, 0), _nameTheory(0) 
{
	int id = stateLoader->beginList(L"NestedNameTheory");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	score = stateLoader->loadReal();
	int n_name_spans = stateLoader->loadInteger();
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());
	_nameTheory = static_cast<NameTheory*>(stateLoader->loadPointer());
	stateLoader->beginList(L"NestedNameTheory::nameSpans");
	for (int i = 0; i < n_name_spans; i++)
		nameSpans.push_back(_new NestedNameSpan(stateLoader));
	stateLoader->endList();

	stateLoader->endList();
}

void NestedNameTheory::resolvePointers(StateLoader * stateLoader) {
	_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
	_nameTheory = static_cast<NameTheory*>(stateLoader->getObjectPointerTable().getPointer(_nameTheory));
	BOOST_FOREACH(NameSpan *span, nameSpans) 
		span->resolvePointers(stateLoader);
}


const wchar_t* NestedNameTheory::XMLIdentifierPrefix() const {
	return L"nestednametheory";
}

void NestedNameTheory::saveXML(SerifXML::XMLTheoryElement nestedNameTheoryElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("NestedNameTheory::saveXML", "Expected context to be NULL");
	nestedNameTheoryElem.setAttribute(X_score, score);
	nestedNameTheoryElem.saveTheoryPointer(X_token_sequence_id, getTokenSequence());
	BOOST_FOREACH(NameSpan *span, nameSpans)
		nestedNameTheoryElem.saveChildTheory(X_NestedName, span, getTokenSequence());
	nestedNameTheoryElem.saveTheoryPointer(X_name_theory_id, getNameTheory());
}

NestedNameTheory::NestedNameTheory(SerifXML::XMLTheoryElement nestedNameTheoryElem) 
	: NameTheory(nestedNameTheoryElem)
{
	using namespace SerifXML;
	XMLTheoryElementList nameElems = nestedNameTheoryElem.getChildElementsByTagName(X_NestedName);
	for (size_t i=0; i<nameElems.size(); ++i)
		nameSpans.push_back(_new NestedNameSpan(nameElems[i]));
	_nameTheory = nestedNameTheoryElem.loadTheoryPointer<NameTheory>(X_name_theory_id);
	if (_nameTheory == 0)
		nestedNameTheoryElem.reportLoadError("Expected a name_theory_id");
}
