// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/sentences/en_SentenceBreaker.h"
#include "English/parse/en_NodeInfo.h"
#include "ThaiModule.h"

// Plugin setup function
extern "C" DLL_PUBLIC void setup_Thai() {

	// Thai uses the english-language definition of NodeInfo. (why?)
	NodeInfo::setImplementation<EnglishNodeInfo>();

	SentenceBreaker::setFactory(boost::shared_ptr<SentenceBreaker::Factory>
                                       (new EnglishSentenceBreakerFactory()));


}
