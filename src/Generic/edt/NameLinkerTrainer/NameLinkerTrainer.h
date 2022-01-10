// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAMELINKERTRAINER_H
#define NAMELINKERTRAINER_H

#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SymbolHash.h"


class NameLinkerTrainer {
public:
	NameLinkerTrainer(int mode_);

	virtual void train();

	enum { TRAIN, ROUNDROBIN, DECODE };
private:
	int MODE;

	virtual void trainOnStateFiles();
	virtual void trainOnAugParses();

	void loadTrainingDataFromStateFileList(const char *listfile);
	void loadTrainingDataFromStateFile(const wchar_t *filename);
	void loadTrainingDataFromStateFile(const char *filename);
	void trainDocument(class DocTheory *docTheory);
	
	void loadTrainingDataFromAugParseFileList(char *listfile);
	void loadTrainingDataFromAugParseFile(const wchar_t *filename);
	void loadTrainingDataFromAugParseFile(const char *filename);
	void trainDocument(class CorefDocument *document);

	void addMention(const SynNode *node, Symbol type);
	void writeModels(const char* model_prefix);

	ProbModelWriter _uniWriter, _priorWriter;
	SymbolHash _stopWords;

	void loadStopWords(const char* file);
	DebugStream _debugOut;

}; 

#endif
