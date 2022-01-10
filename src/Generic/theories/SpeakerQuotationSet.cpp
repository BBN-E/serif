// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/SpeakerQuotationSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/MentionSet.h"

#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"

SpeakerQuotationSet::SpeakerQuotationSet(std::vector<SpeakerQuotationSet*> splitSpeakerQuotationSets, std::vector<int> sentenceOffsets, std::vector<MentionSet*> mergedMentionSets)
{
	// Do the cross-sentence mention lookup here instead of implementing a SpeakerQuotation deep copier
	for (size_t sqs = 0; sqs < splitSpeakerQuotationSets.size(); sqs++) {
		SpeakerQuotationSet* splitSpeakerQuotationSet = splitSpeakerQuotationSets[sqs];
		if (splitSpeakerQuotationSet != NULL) {
			for (int sq = 0; sq < splitSpeakerQuotationSet->getNSpeakerQuotations(); sq++) {
				const SpeakerQuotation* splitSpeakerQuotation = splitSpeakerQuotationSet->getSpeakerQuotation(sq);
				MentionUID speakerUID = MentionUID(splitSpeakerQuotation->getSpeakerMention()->getUID().sentno() + sentenceOffsets[sqs], splitSpeakerQuotation->getSpeakerMention()->getUID().index());
				Mention* speaker = mergedMentionSets[speakerUID.sentno()]->getMention(speakerUID.index());
				_quotations.push_back(_new SpeakerQuotation(speaker, splitSpeakerQuotation->getQuoteStartSentence() + sentenceOffsets[sqs], splitSpeakerQuotation->getQuoteStartToken(), splitSpeakerQuotation->getQuoteEndSentence() + sentenceOffsets[sqs], splitSpeakerQuotation->getQuoteEndToken()));
			}
		}
	}
}

SpeakerQuotationSet::~SpeakerQuotationSet() {
	for (size_t i=0; i<_quotations.size(); ++i) {
		delete _quotations[i];
	}
}

const SpeakerQuotation *SpeakerQuotationSet::getSpeakerQuotation(int i) const {
	if (i >=0 && i < static_cast<int>(_quotations.size()))
		return _quotations[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"SpeakerQuotationSet::getSpeakerQuotation()", _quotations.size(), i);	
}

void SpeakerQuotationSet::takeQuotation(const SpeakerQuotation* speakerQuotation) {
	_quotations.push_back(speakerQuotation);
}

void SpeakerQuotationSet::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void SpeakerQuotationSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"SpeakerQuotation", this);

	stateSaver->saveInteger(_quotations.size());
	stateSaver->beginList(L"SpeakerQuotationSet::_quotations");
	for (size_t i = 0; i < _quotations.size(); i++)
		_quotations[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

SpeakerQuotationSet::SpeakerQuotationSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"SpeakerQuotation");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	int n_quotations = stateLoader->loadInteger();
	_quotations.reserve(n_quotations);
	stateLoader->beginList(L"EntitySet::_quotations");
	for (int i = 0; i < n_quotations; i++)
		_quotations.push_back(_new SpeakerQuotation(stateLoader));
	stateLoader->endList();

	stateLoader->endList();
}

void SpeakerQuotationSet::resolvePointers(StateLoader *stateLoader) {
}

const wchar_t* SpeakerQuotationSet::XMLIdentifierPrefix() const {
	return L"quoteset";
}

void SpeakerQuotationSet::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;
	const DocTheory* docTheory = dynamic_cast<const DocTheory*>(context);
	if (docTheory == 0)
		throw InternalInconsistencyException("SpeakerQuotationSet::saveXML", "Expected context to be a DocTheory");

	for (size_t i = 0; i < _quotations.size(); ++i) {
		elem.saveChildTheory(X_SpeakerQuotation, _quotations[i], context); 
	}
}

SpeakerQuotationSet::SpeakerQuotationSet(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;
	elem.loadId(this);

	XMLTheoryElementList quoteElems = elem.getChildElementsByTagName(X_SpeakerQuotation);
	for (size_t i=0; i<quoteElems.size(); ++i) {
		_quotations.push_back(_new SpeakerQuotation(elem));
	}
}
