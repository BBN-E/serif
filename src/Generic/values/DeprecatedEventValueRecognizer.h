// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_VALUE_RECOGNIZER_H
#define EVENT_VALUE_RECOGNIZER_H

#include <boost/shared_ptr.hpp>

class DocTheory;

class DeprecatedEventValueRecognizer {
public:
	/** Create and return a new DeprecatedEventValueRecognizer. */
	static DeprecatedEventValueRecognizer *build() { return _factory()->build(); }
	/** Hook for registering new DeprecatedEventValueRecognizer factories. */
	struct Factory { virtual DeprecatedEventValueRecognizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }
	
	// This method should eventually build a document level mention set based
	// on string-fill event args and add it to the docTheory.
	// (docTheory->takeDocumentValueMentionSet(valueMentionSet))
	virtual void createEventValues(DocTheory* docTheory) = 0;

	virtual ~DeprecatedEventValueRecognizer() {}

protected:
	DeprecatedEventValueRecognizer() {}

	
private:
	static boost::shared_ptr<Factory> &_factory();

};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/values/en_DeprecatedEventValueRecognizer.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/values/ch_DeprecatedEventValueRecognizer.h"
//#else 
//	#include "Generic/values/xx_DeprecatedEventValueRecognizer.h"
//#endif

#endif
