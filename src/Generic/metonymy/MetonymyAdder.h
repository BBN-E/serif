// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef METONYMY_ADDER_H
#define METONYMY_ADDER_H

#include <boost/shared_ptr.hpp>

class MentionSet;
class PropositionSet;
class DocTheory;

class MetonymyAdder {
public:
	/** Create and return a new MetonymyAdder. */
	static MetonymyAdder *build() { return _factory()->build(); }
	/** Hook for registering new MetonymyAdder factories */
	struct Factory { virtual MetonymyAdder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

public:

	virtual ~MetonymyAdder() {}

	virtual void resetForNewSentence() = 0;
	virtual void resetForNewDocument(DocTheory *docTheory) = 0;

	virtual void addMetonymyTheory(const MentionSet *mentionSet,
			                       const PropositionSet *propSet) = 0;

protected:
	MetonymyAdder() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ENGLISH_LANGUAGE)
//	#include "English/metonymy/en_MetonymyAdder.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/metonymy/ch_MetonymyAdder.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/metonymy/ar_MetonymyAdder.h"
//#else
//	#include "Generic/metonymy/xx_MetonymyAdder.h"
//#endif

#endif
