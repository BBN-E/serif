// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef FIRST_PERSON_PRONOUN_TO_SPEAKER_MERGER_H
#define FIRST_PERSON_PRONOUN_TO_SPEAKER_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges a first person pronoun mention with the current speaker for the document.
  */
class FirstPersonPronounToSpeakerMerger : public PairwiseMentionGroupMerger {
public:
	FirstPersonPronounToSpeakerMerger(MentionGroupConstraint_ptr constraints);
protected:
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;

	/** Returns true if m1 is the person referred to by 1st person pronoun m2. */
	bool findFirstPersonReferentMatch(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
