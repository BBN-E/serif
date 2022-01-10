// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/ICEWSUtilities.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"

size_t ICEWSUtilities::getIcewsSentNo(ActorMention_ptr actorMention, const DocTheory* docTheory) {
	const Mention *mention = actorMention->getEntityMention();

	if (!ParamReader::getRequiredTrueFalseParam("use_icews_sentence_numbers"))
		return mention->getSentenceNumber();
		
	static Symbol SENTENCE_SPAN_SYM(L"ICEWS_Sentence");
	Metadata* metadata = docTheory->getDocument()->getMetadata();
	const TokenSequence *ts = docTheory->getSentenceTheory(mention->getSentenceNumber())->getTokenSequence();

	// Return the sentence number for the sentence containing the first
	// character of the actor mention.
	EDTOffset start = ts->getToken(mention->getNode()->getStartToken())->getStartEDTOffset();
	if (IcewsSentenceSpan *sspan = dynamic_cast<IcewsSentenceSpan*>(metadata->getCoveringSpan(start, SENTENCE_SPAN_SYM))) {
		return sspan->getSentNo();
	}

	// Fallback: return the sentence number for the sentence containing 
	// the last character of the actor mention.  (Should never be necessary.)
	EDTOffset end = ts->getToken(mention->getNode()->getEndToken())->getEndEDTOffset();
	if (IcewsSentenceSpan *sspan = dynamic_cast<IcewsSentenceSpan*>(metadata->getCoveringSpan(end, SENTENCE_SPAN_SYM))) {
		return sspan->getSentNo();
	}

	throw UnexpectedInputException("ICEWSUtilities::getIcewsSentNo",
		"No ICEWS_Sentence metadata span found covering actor mention!");
}
