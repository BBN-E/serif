// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTIONLINKER_H
#define MENTIONLINKER_H

#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/LexEntitySet.h"

class DocTheory;

class MentionLinker {
	public:
		virtual ~MentionLinker() {}
		virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) = 0;
		virtual void cleanUpAfterDocument() {}
		virtual void resetForNewSentence() {}
		virtual void resetForNewDocument(Symbol docName) {}
		virtual void resetForNewDocument(DocTheory *docTheory) {}
		virtual void correctAnswersLinkMention(EntitySet *currSolution, MentionUID currMentionUID, EntityType linkType) {}
		virtual void printCorrectAnswer(int id) {}
};

#endif
