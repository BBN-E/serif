#ifndef NESTED_NAME_RECOGNIZER_H
#define NESTED_NAME_RECOGNIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/TokenSequence.h"

class NestedNameRecognizer {
public:
	/** Create and return a new NestedNameRecognizer. */
	static NestedNameRecognizer *build() { return _factory()->build(); }
	/** Hook for registering new NestedNameRecognizer factories */
	struct Factory { virtual NestedNameRecognizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~NestedNameRecognizer() {}

	virtual void resetForNewSentence(const Sentence *sentence) = 0;
	virtual void resetForNewDocument(class DocTheory *docTheory = 0) = 0;

	// This does the work. It populates an array of pointers to NameTheorys
	// specified by results arg with up to max_theories NameTheory pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// NameTheorys.
	virtual int getNestedNameTheories(NestedNameTheory **results, int max_theories,
								TokenSequence *tokenSequence, NameTheory *nameTheory) = 0;

protected:
	NestedNameRecognizer() {}
private:
	static boost::shared_ptr<Factory> &_factory();
};


#endif
