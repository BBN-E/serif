// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RULENAMELINKER_H
#define RULENAMELINKER_H

#include <boost/shared_ptr.hpp>

#include "Generic/edt/MentionLinker.h"
#include "Generic/edt/LexEntitySet.h"
#include "Generic/theories/Mention.h"

// Note: This is a language-specific, rule-based namelinker

class RuleNameLinker: public MentionLinker {
public:
	/** Create and return a new RuleNameLinker. */
	static RuleNameLinker *build() { return _factory()->build(); }
	/** Hook for registering new RuleNameLinker factories */
	struct Factory { virtual RuleNameLinker *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

public:
	virtual ~RuleNameLinker() {}

	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) = 0;

protected:
	RuleNameLinker() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/edt/en_RuleNameLinker.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/edt/kr_RuleNameLinker.h"
//#else
//	#include "Generic/edt/xx_RuleNameLinker.h"
//#endif

#endif
