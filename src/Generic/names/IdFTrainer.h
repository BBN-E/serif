// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_TRAINER_H
#define IDF_TRAINER_H

class NameClassTags;
class IdFSentence;
class UTF8InputStream;
class IdFListSet;
#include "Generic/common/Symbol.h"
#include "Generic/names/IdFModel.h"
#include "Generic/names/IdFWordFeatures.h"

/**
 * basic IdF trainer
 */
class IdFTrainer {
public:
	IdFTrainer(const NameClassTags *nameClassTags, 
		const char *list_file_name = 0, int word_features_mode = IdFWordFeatures::DEFAULT);
	IdFTrainer(const NameClassTags *nameClassTags, const IdFWordFeatures *wordFeatures);

	void collectCountsFromFiles(const char *training_file_list);
	void deriveTables();	
	void printTables(const char *model_file_prefix);

	void collectTemporaryVocab(IdFSentenceTokens * sentence); //standalone IdF only
	void collectVocabulary(IdFSentenceTokens * sentence); //standalone IdF only
	
	void collectSentenceCounts(IdFSentence* sentence);
	void addUnknownWords();
	void estimateLambdas(const char *smoothing_file);
	void setKValues(float tag_k, float word_start_k, float word_continue_k, 
		float word_tag_k, float word_reduced_tag_k) 
	{
		_model->setKValues(tag_k, word_start_k, word_continue_k, 
			word_tag_k, word_reduced_tag_k);
	}
	
	void resetCounts();

protected:
	IdFModel *_model;
	const NameClassTags *_nameClassTags;

	NgramScoreTable* _tagEventCounts;
	NgramScoreTable* _wordEventCounts;

	NgramScoreTable* _tagEventCountsPlusUnknown;
	NgramScoreTable* _wordEventCountsPlusUnknown;

	NgramScoreTable* _temporaryVocabTable;

	IdFSentence *_sentence;

	const Symbol FIRSTWORD;
	const Symbol NOTFIRSTWORD;

	void collectTemporaryVocab(UTF8InputStream &stream);
	
	void collectCounts(UTF8InputStream &stream);

	void initializeEverything();


};

#endif
