// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_SENTENCE_DRIVER_H
#define CA_SENTENCE_DRIVER_H

#include "Generic/driver/SentenceDriver.h"
#include "Generic/driver/Stage.h"

class DocTheory;
class Sentence;
class SentenceTheory;
class CorrectDocument;
class CorrectAnswers;

class CASentenceDriver : public SentenceDriver {
public:
	CASentenceDriver();

	void beginDocument(DocTheory *docTheory);

	SentenceTheoryBeam *run(DocTheory *docTheory, int sent_no, Stage startStage, Stage endStage);

	/** Return true if the models for the specified stage have been loaded. */
	virtual bool stageModelsAreLoaded(Stage stage);
private:

	CorrectAnswers *_correctAnswers; // we do not own this - it points to a Singleton instance

	bool _names_loaded;
	bool _nestedNames_loaded;
	bool _values_loaded;
	bool _relations_loaded;

	void loadNameModels();
	void loadNestedNameModels();
	void loadValueModels();
	void loadEventModels();
	void loadRelationModels();
};


#endif
