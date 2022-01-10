// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/icews/SentenceSpan.h"
#include <xercesc/util/XMLString.hpp>
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Metadata.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include <limits.h>
#include <boost/scoped_ptr.hpp>

REGISTER_SPAN_CLASS(, IcewsSentenceSpan)

void IcewsSentenceSpan::loadXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	static const XMLCh* X_original_sentence_index = xercesc::XMLString::transcode("original_sentence_index");
	Span::loadXML(spanElem);
	_original_sentence_index = spanElem.getAttribute<int>(X_original_sentence_index);
}

void IcewsSentenceSpan::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
	using namespace SerifXML;
	static const XMLCh* X_original_sentence_index = xercesc::XMLString::transcode("original_sentence_index");
	Span::saveXML(spanElem);
	spanElem.setAttribute(X_original_sentence_index, _original_sentence_index);
}

int IcewsSentenceSpan::icewsSentenceNoToSerifSentenceNo(int icews_sentence_no, const DocTheory* docTheory) {
	if (!ParamReader::getRequiredTrueFalseParam("use_icews_sentence_numbers"))
		return icews_sentence_no;

	Metadata::SpanTypeFilter filter(Symbol(L"ICEWS_Sentence"));
	const Metadata *metadata = docTheory->getMetadata();
	boost::scoped_ptr<Metadata::SpanList> spans(metadata->getSpans(&filter));
	for (int i=0; i<spans->length(); ++i) {
		IcewsSentenceSpan *span = dynamic_cast<IcewsSentenceSpan*>((*spans)[i]);
		if (static_cast<int>(span->_original_sentence_index) == icews_sentence_no) {
			EDTOffset endOffset = span->getEndOffset();
			for (int serif_sentno=0; serif_sentno<docTheory->getNSentences(); ++serif_sentno) {
				if (docTheory->getSentence(serif_sentno)->getEndEDTOffset() >= endOffset) {
					return serif_sentno+1;
				}
			}
		}
	}

	return docTheory->getNSentences();
}

int IcewsSentenceSpan::serifSentenceNoToIcewsSentenceNo(int serif_sentence_no, const DocTheory* docTheory) {
	if (ParamReader::getRequiredTrueFalseParam("use_icews_sentence_numbers")) {
		const Sentence* sent = docTheory->getSentence(serif_sentence_no);
		return edtOffsetToIcewsSentenceNo(sent->getStartEDTOffset(), docTheory);
	} else return serif_sentence_no;
}


int IcewsSentenceSpan::edtOffsetToIcewsSentenceNo(EDTOffset offset, const DocTheory* docTheory) {
	if (ParamReader::getRequiredTrueFalseParam("use_icews_sentence_numbers")) {
		const Metadata *metadata = docTheory->getMetadata();
		Metadata::SpanTypeFilter filter(Symbol(L"ICEWS_Sentence"));
		boost::scoped_ptr<Metadata::SpanList> spans(metadata->getCoveringSpans(offset, &filter));
		if (spans->length() > 0) {
			if (IcewsSentenceSpan *span = dynamic_cast<IcewsSentenceSpan*>((*spans)[0]))
				return static_cast<int>(span->_original_sentence_index);
		}
		SessionLogger::warn("ICEWS") << "Unable to find ICEWS sentence number for EDT offset "
			<< offset.value() << "; returning 0.";
		return 0;
	} else {
		for (int s = 0; s < docTheory->getNSentences(); s++) {
			if (offset >= docTheory->getSentence(s)->getStartEDTOffset() &&
				offset <= docTheory->getSentence(s)->getEndEDTOffset())
				return s;
		}
		SessionLogger::warn("ICEWS") << "Unable to find sentence number for EDT offset "
			<< offset.value() << "; returning 0.";
		return 0;
	}
}

