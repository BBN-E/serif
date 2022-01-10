// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SPECIAL_CASE_MERGER_H
#define EN_SPECIAL_CASE_MERGER_H

#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"

/**
  *  Groups together several mergers that cover special cases for English coref.
  */
class EnglishSpecialCaseMerger : public CompositeMentionGroupMerger {
public:
	EnglishSpecialCaseMerger(MentionGroupConstraint_ptr constraints);
};

#endif
