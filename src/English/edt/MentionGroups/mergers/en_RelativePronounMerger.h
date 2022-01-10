// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_RELATIVE_PRONOUN_MERGER_H
#define EN_RELATIVE_PRONOUN_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges mentions with their relative pronouns identified by EnglishWHQLinkExtractor.
  */
class EnglishRelativePronounMerger : public PairwiseMentionGroupMerger {
public:
	EnglishRelativePronounMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 is a relative pronoun for m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
