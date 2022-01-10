// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef POS_RECOGNIZER_H
#define POS_RECOGNIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/ValueSet.h"

class TokenSequence;
class PartOfSpeechSequence;

class PartOfSpeechRecognizer {
public:
	/** Create and return a new PartOfSpeechRecognizer. */
	static PartOfSpeechRecognizer *build() { return _factory()->build(); }
	/** Hook for registering new PartOfSpeechRecognizer factories */
	struct Factory { virtual PartOfSpeechRecognizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~PartOfSpeechRecognizer() {};

	virtual void resetForNewSentence() = 0;

	// This does the work. It populates an array of pointers to ValueSets
	// specified by results arg with up to max_theories ValueSet pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// ValueSets.
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								 TokenSequence *tokenSequence) = 0;
protected:
	PartOfSpeechRecognizer() {};

private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ARABIC_LANGUAGE
//	#include "Arabic/partOfSpeech/ar_PartOfSpeechRecognizer.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/partOfSpeech/kr_PartOfSpeechRecognizer.h"
//#elif defined(ENGLISH_LANGUAGE)
//	#include "English/partOfSpeech/en_PartOfSpeechRecognizer.h"
//#else
//	#include "Generic/partOfSpeech/xx_PartOfSpeechRecognizer.h"
//#endif


#endif
