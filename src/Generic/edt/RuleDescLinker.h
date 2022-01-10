// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RULEDESCLINKER_H
#define RULEDESCLINKER_H

#include <boost/shared_ptr.hpp>

#include "Generic/edt/MentionLinker.h"
#include "Generic/edt/LexEntitySet.h"
#include "Generic/theories/Mention.h"

// Note: The name linker, which parallels this conceptually, is language-independent
// There is no hope that this rule-based descriptor linker will be likewise

class RuleDescLinker: public MentionLinker {
public:
	/** Create and return a new RuleDescLinker. */
	static RuleDescLinker *build() { return _factory()->build(); }
	/** Hook for registering new RuleDescLinker factories */
	struct Factory { virtual RuleDescLinker *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~RuleDescLinker() {}

protected:
	RuleDescLinker() {}

	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) = 0;

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/edt/en_RuleDescLinker.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/edt/ch_RuleDescLinker.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/edt/ar_RuleDescLinker.h"
//
//#else
//	#include "Generic/edt/xx_RuleDescLinker.h"
//#endif

#endif
