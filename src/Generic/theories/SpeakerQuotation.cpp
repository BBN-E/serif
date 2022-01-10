// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/theories/SpeakerQuotation.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"

#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"


SpeakerQuotation::SpeakerQuotation(const Mention* speakerMention, int quote_start_sentno, int quote_start_tokno,
								   int quote_end_sentno, int quote_end_tokno)
: _speakerMention(speakerMention), _quote_start_sentno(quote_start_sentno), _quote_start_tokno(quote_start_tokno),
  _quote_end_sentno(quote_end_sentno), _quote_end_tokno(quote_end_tokno)
{
}

void SpeakerQuotation::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void SpeakerQuotation::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"SpeakerQuotation", this);

	stateSaver->savePointer(_speakerMention);
	stateSaver->saveInteger(_quote_start_sentno);
	stateSaver->saveInteger(_quote_start_tokno);
	stateSaver->saveInteger(_quote_end_sentno);
	stateSaver->saveInteger(_quote_end_tokno);

	stateSaver->endList();
}

SpeakerQuotation::SpeakerQuotation(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"SpeakerQuotation");
	stateLoader->getObjectPointerTable().addPointer(id, this);

    _speakerMention = (const Mention *) stateLoader->loadPointer();
	_quote_start_sentno = stateLoader->loadInteger();
	_quote_start_tokno = stateLoader->loadInteger();
	_quote_end_sentno = stateLoader->loadInteger();
	_quote_end_tokno = stateLoader->loadInteger();

	stateLoader->endList();
}

void SpeakerQuotation::resolvePointers(StateLoader *stateLoader) {
	_speakerMention = (const Mention *) stateLoader->getObjectPointerTable().getPointer(_speakerMention);	
}

const wchar_t* SpeakerQuotation::XMLIdentifierPrefix() const {
	return L"quote";
}

void SpeakerQuotation::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;
	const DocTheory* docTheory = dynamic_cast<const DocTheory*>(context);
	if (docTheory == 0)
		throw InternalInconsistencyException("SpeakerQuotation::saveXML", "Expected context to be a DocTheory");

	elem.saveTheoryPointer(X_speaker, _speakerMention);

	const TokenSequence* startSentTokSeq = docTheory->getSentenceTheory(_quote_start_sentno)->getTokenSequence();
	const TokenSequence* endSentTokSeq = docTheory->getSentenceTheory(_quote_end_sentno)->getTokenSequence();
	const Token *startToken = startSentTokSeq->getToken(_quote_start_tokno);
	const Token *endToken = endSentTokSeq->getToken(_quote_end_tokno);
	elem.saveTheoryPointer(X_start_token, startToken);
	elem.saveTheoryPointer(X_end_token, endToken);

}

SpeakerQuotation::SpeakerQuotation(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;
	elem.loadId(this);

	_speakerMention = elem.loadTheoryPointer<Mention>(X_speaker);

	XMLSerializedDocTheory *xmldoc = elem.getXMLSerializedDocTheory();
	const Token* startTok = elem.loadTheoryPointer<Token>(X_start_token);
	const Token* endTok = elem.loadTheoryPointer<Token>(X_end_token);
	_quote_start_sentno = static_cast<int>(xmldoc->lookupTokenSentNo(startTok));
	_quote_start_tokno = static_cast<int>(xmldoc->lookupTokenIndex(startTok));
	_quote_end_sentno = static_cast<int>(xmldoc->lookupTokenSentNo(startTok));
	_quote_end_tokno = static_cast<int>(xmldoc->lookupTokenIndex(endTok));
}
