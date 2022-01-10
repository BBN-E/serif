// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_COMPOUND_MENTION_FINDER_H
#define KR_COMPOUND_MENTION_FINDER_H

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/CompoundMentionFinder.h"
class Mention;
class MentionSet;

class KoreanCompoundMentionFinder : public CompoundMentionFinder {
private:
	// This class should only be instantiated via CompoundMentionFinder::getInstance()
	KoreanCompoundMentionFinder();
	friend class CompoundMentionFinder::FactoryFor<KoreanCompoundMentionFinder>;
	// This class should only be deleted via CompoundMentionFinder::deleteInstance()
	~KoreanCompoundMentionFinder();
public:
	virtual Mention *findPartitiveWholeMention(MentionSet *mentionSet,
		Mention *baseMention){ 
			return NULL; 
	}
	virtual Mention **findAppositiveMemberMentions(MentionSet *mentionSet,
		Mention *baseMention) { 
			return NULL; 
	}
	virtual Mention **findListMemberMentions(MentionSet *mentionSet,
		Mention *baseMention) { 
			return NULL; 
	}
	virtual Mention *findNestedMention(MentionSet *mentionSet,
		Mention *baseMention) { 
			return NULL; 
	}
	virtual void coerceAppositiveMemberMentions(Mention** mentions){}
	virtual void setCorrectAnswers(class CorrectAnswers *correctAnswers) {}
	virtual void setCorrectDocument(class CorrectDocument *correctDocument) {}
	virtual void setSentenceNumber(int sentno) {}

};

#endif
