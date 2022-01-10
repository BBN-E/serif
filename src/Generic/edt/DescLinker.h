// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCLINKER_H
#define DESCLINKER_H

#include "edt/MentionLinker.h"
#include "edt/LexEntitySet.h"
#include "theories/Mention.h"

// Note: The name linker, which parallels this conceptually, is language-independent
// There is no hope that this rule-based descriptor linker will be likewise

class DescLinker: public MentionLinker {
public:
	/** Create and return a new DescLinker. */
	static DescLinker *build() { return _factory()->build(); }
	/** Hook for registering new DescLinker factories. */
	struct Factory { virtual DescLinker *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~DescLinker() {}
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) = 0;
protected:
	DescLinker() {}
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/edt/en_DescLinker.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/edt/ar_DescLinker.h"
//
//#else/
//	#include "Generic/edt/xx_DescLinker.h"
//#endif

#endif
