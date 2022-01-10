// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_COMPOUND_MENTION_FINDER_H
#define XX_COMPOUND_MENTION_FINDER_H

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/CompoundMentionFinder.h"
#include <iostream>
class Mention;
class MentionSet;

class GenericCompoundMentionFinder : public CompoundMentionFinder {
private:
	// This class should only be instantiated via CompoundMentionFinder::getInstance()
	GenericCompoundMentionFinder();
	friend struct CompoundMentionFinder::FactoryFor<GenericCompoundMentionFinder>;
	// This class should only be deleted via CompoundMentionFinder::deleteInstance()
	~GenericCompoundMentionFinder() {}
public:
	virtual Mention *findPartitiveWholeMention(MentionSet *mentionSet,
											   Mention *baseMention);
	virtual Mention **findAppositiveMemberMentions(MentionSet *mentionSet,
												   Mention *baseMention);
	virtual Mention **findListMemberMentions(MentionSet *mentionSet,
											 Mention *baseMention);
	virtual Mention *findNestedMention(MentionSet *mentionSet,
									   Mention *baseMention);
	virtual void coerceAppositiveMemberMentions(Mention** mentions);
	virtual void setCorrectAnswers(class CorrectAnswers *correctAnswers);
	virtual void setCorrectDocument(class CorrectDocument *correctDocument);
	virtual void setSentenceNumber(int sentno);
};

#endif
