// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef APPOSLINKER_H
#define APPOSLINKER_H

#include "edt/MentionLinker.h"

class ApposLinker: public MentionLinker {
public:
	ApposLinker();
	virtual int linkMention (LexEntitySet * currSolution, int currMentionUID, Entity::Type linkType, LexEntitySet *results[], int max_results);
	static bool isIndependentlyLinkable(Mention *mention);
};

#endif
