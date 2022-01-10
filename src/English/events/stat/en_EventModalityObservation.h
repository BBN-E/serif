// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_EVENT_MODALITY_OBSERVATION_H
#define EN_EVENT_MODALITY_OBSERVATION_H

#include "Generic/events/stat/EventModalityObservation.h"

class EnglishEventModalityObservation : public EventModalityObservation {
public:
	EnglishEventModalityObservation() : EventModalityObservation() {}

    virtual void populate(int token_index, const TokenSequence *tokens, const Parse *parse,
		MentionSet *mentionSet, const PropositionSet *propSet, bool use_wordnet);

 private:
    virtual bool isLikelyNonAsserted (Proposition *prop) const;
	virtual std::vector<bool> identifyNonAssertedProps(const PropositionSet *propSet,
													   const MentionSet *mentionSet) const;

    virtual bool isLedbyAllegedAdverb(Proposition * prop) const;
    virtual bool isLedbyModalWord(Proposition * prop) const;
    virtual bool isFollowedbyIFWord(Proposition * prop) const;
    virtual bool isLedbyIFWord(Proposition * prop) const;
    virtual bool parentIsLikelyNonAsserted(Proposition * prop, const PropositionSet *propSet, const MentionSet *mentionSet) const;
	virtual void findIndicators(int token_index, const TokenSequence *tokens,
								const Parse *parse, MentionSet *mentionSet);
};

#endif
