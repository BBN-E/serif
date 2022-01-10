#ifndef xx_NESTED_NAME_RECOGNIZER_H
#define xx_NESTED_NAME_RECOGNIZER_H

#include "Generic/nestedNames/DefaultNestedNameRecognizer.h"

class GenericNestedNameRecognizer : public NestedNameRecognizer {
private:
	friend class GenericNestedNameRecognizerFactory;

public:
	virtual int getNestedNameTheories(NestedNameTheory **results, int max_theories,
		TokenSequence *tokenSequence, NameTheory *nameTheory) {
		// return empty theory
	//	results[0] = _new NestedNameTheory(tokenSequence, nameTheory, 0, 0);
	//	return 1;	
		return _nestedNameRecognizer.getNestedNameTheories(results, max_theories,
			tokenSequence, nameTheory);
	};

	virtual void resetForNewSentence(const Sentence *sentence) {};
	virtual void resetForNewDocument(class DocTheory *docTheory = 0) {};

protected:
	GenericNestedNameRecognizer() : _nestedNameRecognizer() {}

private:
	DefaultNestedNameRecognizer _nestedNameRecognizer;

};

class GenericNestedNameRecognizerFactory: public NestedNameRecognizer::Factory {
	virtual NestedNameRecognizer *build() { return _new GenericNestedNameRecognizer(); }
};

#endif
