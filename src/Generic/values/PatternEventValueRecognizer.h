//Copyright 2011 BBN Technologies
//All rights reserved

#ifndef PATTERN_EVENT_VALUE_RECOGNIZER
#define PATTERN_EVENT_VALUE_RECOGNIZER

#include <boost/shared_ptr.hpp>


class DocTheory;

//This class uses BPL to label Event Values from already found event mentions

class PatternEventValueRecognizer {
public:
	/** Create and return a new PatternEventValueRecognizer. */
	static PatternEventValueRecognizer *build() { return _factory()->build(); }
	/** Hook for registering new PatternEventValueRecognizer factories. */
	struct Factory { virtual PatternEventValueRecognizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	/*	This method should eventually build a document level mention set based
	*	on string-fill event args and add it to the docTheory.
	*/
	virtual void createEventValues(DocTheory* docTheory) = 0;

	virtual ~PatternEventValueRecognizer() {}

protected:
	PatternEventValueRecognizer() {}
	
private:
	static boost::shared_ptr<Factory> &_factory();

};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/values/en_PatternEventValueRecognizer.h"
//#else //Eventually, there should be a ch_PatternEventValueRecognizer.h using BPL instead of the depricated event pattern language
//	#include "Generic/values/xx_PatternEventValueRecognizer.h"
//#endif


#endif
