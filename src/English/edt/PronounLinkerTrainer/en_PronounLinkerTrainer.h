// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_PRONOUNLINKERTRAINER_H
#define en_PRONOUNLINKERTRAINER_H

#include "Generic/edt/PronounLinkerTrainer/PronounLinkerTrainer.h"
#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/Symbol.h"
#include "Generic/trainers/CorefDocument.h"

class EnglishPronounLinkerTrainer : public PronounLinkerTrainer {
public:
	EnglishPronounLinkerTrainer();
	~EnglishPronounLinkerTrainer();

	virtual void openInputFile(char *trainingFile);
	virtual void closeInputFile();

	virtual void trainModels();
	virtual void writeModels();

private:
	void openOutputFiles(const char* model_prefix);
	void closeOutputFiles();

	void trainDocument(CorefDocument *document);
	void registerPronounLink(Mention *antecedent, const SynNode *antecedentNode, Mention *pronoun, Symbol hobbsDistance);

	UTF8OutputStream _antPriorStream, _hobbsDistStream, _pronHeadWordStream, _pronParWordStream;
	AnnotatedParseReader _trainingSet;
	ProbModelWriter _antPriorWriter, _hobbsDistWriter, _pronHeadWordWriter, _pronParWordWriter;

	DebugStream _debugOut;
}; 

#endif
