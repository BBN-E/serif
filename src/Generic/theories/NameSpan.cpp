// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/NameSpan.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/transliterate/Transliterator.h"
#include <boost/scoped_ptr.hpp>

#include <iostream>


void NameSpan::dump(std::ostream &out, int indent) const {
	out << "Name span (" << start << " " << end << "): "
		<< type.getName().to_debug_string();
}


void NameSpan::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void NameSpan::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"NameSpan", this);

	stateSaver->saveInteger(start);
	stateSaver->saveInteger(end);
	stateSaver->saveSymbol(type.getName());

	stateSaver->endList();
}

NameSpan::NameSpan(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"NameSpan");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	start = stateLoader->loadInteger();
	end = stateLoader->loadInteger();
	type = EntityType(stateLoader->loadSymbol());

	stateLoader->endList();
}

void NameSpan::resolvePointers(StateLoader *stateLoader) {
}

const wchar_t* NameSpan::XMLIdentifierPrefix() const {
	return L"name";
}

void NameSpan::saveXML(SerifXML::XMLTheoryElement namespanElem, const Theory *context) const {
	using namespace SerifXML;
	const TokenSequence *tokSeq = dynamic_cast<const TokenSequence*>(context);
	if (context == 0)
		throw InternalInconsistencyException("NameSpan::saveXML", "Expected context to be a TokenSequence");
	// Entity type:
	namespanElem.setAttribute(X_entity_type, type.getName());
	// Pointers to start/end tokens:
	const Token *startTok = tokSeq->getToken(start);
	namespanElem.saveTheoryPointer(X_start_token, startTok);
	const Token *endTok = tokSeq->getToken(end);
	namespanElem.saveTheoryPointer(X_end_token, endTok);
	// Start/end offsets:
	const OffsetGroup &startOffset = startTok->getStartOffsetGroup();
	const OffsetGroup &endOffset = endTok->getEndOffsetGroup();
	namespanElem.saveOffsets(startOffset, endOffset);
	// Contents (optional):
	if (namespanElem.getOptions().include_spans_as_elements) 
		namespanElem.addText(namespanElem.getOriginalTextSubstring(startOffset, endOffset));
	if (namespanElem.getOptions().include_spans_as_comments)
		namespanElem.addComment(namespanElem.getOriginalTextSubstring(startOffset, endOffset));
	// Transliteration (optional)
	if (namespanElem.getOptions().include_name_transliterations) {
		boost::scoped_ptr<Transliterator> transliterator(Transliterator::build());
		namespanElem.setAttribute(X_transliteration, transliterator->getTransliteration(tokSeq, start, end));
	}
}

NameSpan::NameSpan(SerifXML::XMLTheoryElement nameSpanElem)
{
	using namespace SerifXML;
	nameSpanElem.loadId(this);
	type = EntityType(nameSpanElem.getAttribute<Symbol>(X_entity_type));
	XMLSerializedDocTheory *xmldoc = nameSpanElem.getXMLSerializedDocTheory();
	start = static_cast<int>(xmldoc->lookupTokenIndex(nameSpanElem.loadTheoryPointer<Token>(X_start_token)));
	end = static_cast<int>(xmldoc->lookupTokenIndex(nameSpanElem.loadTheoryPointer<Token>(X_end_token)));
	// Todo: allow specification by char offset rather than token pointer?
}
