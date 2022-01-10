// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SECOND_PERSON_PRONOUN_TO_SPEAKER_MERGER_H
#define SECOND_PERSON_PRONOUN_TO_SPEAKER_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges a second person pronoun mention with either the previous speaker or the
  *  intended recipient of the document.
  */
class SecondPersonPronounToSpeakerMerger : public PairwiseMentionGroupMerger {
public:
	SecondPersonPronounToSpeakerMerger(MentionGroupConstraint_ptr constraints);
protected:
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;

	/** Returns true if m1 is the person referred to by 2nd person pronoun m2. */
	bool findSecondPersonReferentMatch(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
