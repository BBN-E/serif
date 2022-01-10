// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRONOUN_LINKER_TRAINER_H
#define PRONOUN_LINKER_TRAINER_H

#include "Generic/edt/PronounLinkerTrainer/PronounLinkerTrainer.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/DebugStream.h"
#include "Generic/trainers/ProbModelWriter.h"

class Mention;
class EntitySet;
class Entity;
class SynNode;

class PronounLinkerTrainer {
public:
	PronounLinkerTrainer(int mode_);
	
	void train();

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

	void registerPronounLink(Mention *antecedent, const SynNode *antecedentNode, 
							 Mention *pronoun, Symbol hobbsDistance);
	void writeModels(const char* model_prefix);

	bool antecedentAlreadySeen(EntitySet *entitySet, Entity *entity, Mention *ment);

	ProbModelWriter _antPriorWriter, _hobbsDistWriter, _pronHeadWordWriter, _pronParWordWriter;

	DebugStream _debugOut;
}; 

#endif
