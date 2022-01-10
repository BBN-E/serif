// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_RECOGNIZER_H
#define NAME_RECOGNIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/NameTheory.h"
#include "Generic/theories/TokenSequence.h"
class Sentence;

class NameRecognizer {
public:
	/** Create and return a new NameRecognizer. */
	static NameRecognizer *build() { return _factory()->build(); }
	/** Hook for registering new NameRecognizer factories */
	struct Factory { virtual NameRecognizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~NameRecognizer() {}

	virtual void resetForNewSentence(const Sentence *sentence) = 0;

	virtual void cleanUpAfterDocument() = 0;
	virtual void resetForNewDocument(class DocTheory *docTheory = 0) = 0;

	// This does the work. It populates an array of pointers to NameTheorys
	// specified by results arg with up to max_theories NameTheory pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// NameTheorys.
	virtual int getNameTheories(NameTheory **results, int max_theories,
								TokenSequence *tokenSequence) = 0;

protected:
	NameRecognizer() {}
	virtual void postProcessNameTheory(NameTheory *theory, 
	                                   TokenSequence *tokenSequence) {}
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/names/en_NameRecognizer.h"
////	#include "names/stub_NameRecognizer.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/names/ch_NameRecognizer.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/names/ar_NameRecognizer.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/names/kr_NameRecognizer.h"
//#else
//	#include "Generic/names/xx_NameRecognizer.h"
//#endif


#endif
