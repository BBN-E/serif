// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COMPOUND_MENTION_FINDER_H
#define COMPOUND_MENTION_FINDER_H

class Mention;
class MentionSet;
#include <boost/shared_ptr.hpp>

class CompoundMentionFinder {
public:
	// no build method or constructor -- this singleton uses a getInstance instead.
	static CompoundMentionFinder* getInstance();
	static void deleteInstance();
	virtual ~CompoundMentionFinder() {}

	/** Set the compound mention finder implementation that should be used by
	 * getInstance() to create a new CompoundMentionFinder.  Example usage:
	 * CompoundMentionFinder::setImplementation<EnglishCompoundMentionFinder>();
	 */
    template<typename LanguageCompoundMentionFinder>
    static void setImplementation() {
        _factory() = boost::shared_ptr<Factory>
            (_new FactoryFor<LanguageCompoundMentionFinder>());
	}
	
	// These are heuristics for detecting compound mentions.
	// They return the submention(s) you ask for, or 0
	// if they find that the mention is not of the right type.

	// * CAUTION! THERE IS TRICKERY HERE: *
	// The order of these is important. The four types of mentions
	// tested for (nested, partitive, appos, list) are mutually
	// exclusive, so each of these tests is allowed to assume that
	// each previous test failed (returned 0) (or would have if they
	// haven't actually been called). For example, don't call
	// findNestedMention() on an appositive.

	virtual Mention *findPartitiveWholeMention(MentionSet *mentionSet,
											   Mention *baseMention) = 0;

	// these two return a pointer to a 0-terminated *static* array 
	// of pointers to mentions.
	virtual Mention **findAppositiveMemberMentions(MentionSet *mentionSet,
												   Mention *baseMention) = 0;

	virtual Mention **findListMemberMentions(MentionSet *mentionSet,
											 Mention *baseMention) = 0;

	/**
	 * Look for a mention in the head path of the baseMention
	 * @param mentionSet the set of all available mentions
	 * @param baseMention the mention beneath which we search
	 * @return a pointer to a nested mention, or 0 if none exists
	 */
	virtual Mention *findNestedMention(MentionSet *mentionSet,
									   Mention *baseMention) = 0;

	virtual void coerceAppositiveMemberMentions(Mention** mentions) = 0;

	// These are used by CorrectAnswerSerif.
	virtual void setCorrectAnswers(class CorrectAnswers *correctAnswers) = 0;
	virtual void setCorrectDocument(class CorrectDocument *correctDocument) = 0;
	virtual void setSentenceNumber(int sentno) = 0;

protected:
	// Factory support:
	struct Factory { virtual CompoundMentionFinder *build() = 0; };
    template<typename T>
    struct FactoryFor: public Factory {
		virtual CompoundMentionFinder *build() { return _new T(); }
	};
	static boost::shared_ptr<Factory> &_factory();

	// Singleton instance:
	static CompoundMentionFinder* _instance;
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/descriptors/en_CompoundMentionFinder.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/descriptors/ch_CompoundMentionFinder.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/descriptors/ar_CompoundMentionFinder.h"
// #elif defined(KOREAN_LANGUAGE)
// 	#include "Korean/descriptors/kr_CompoundMentionFinder.h"
// #else
// 	#include "Generic/descriptors/xx_CompoundMentionFinder.h"
// #endif


#endif
