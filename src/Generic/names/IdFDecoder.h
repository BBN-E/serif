// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_DECODER_H
#define IDF_DECODER_H

#include "Generic/names/IdFModel.h"
#include "Generic/names/IdFWordFeatures.h"

class UTF8OutputStream;
class NameClassTags;
class IdFSentenceTokens;
class IdFSentenceTheory;
class IdFListSet;

/**
 * Basic IdF decoder. 
 * 
 * This is a basically a simple tagger, finding the best path through a sentence, where
 * at each word, it's generating a tag (with P(tag | prev_tag, prev_word)) and the 
 * word itself (with P(word | tag, prev_tag, prev_word)). 
 * 
 * The tag set is composed of a start-sentence tag, an end-sentence tag, and, for each 
 * possible name class, a start-that-name-class tag, and a continue-that-name-class tag. 
 * One name class, of course, is NONE. Name classes are specified either by an outside
 * file, or, in the case of SERIF, by using the classes in EntityConstants.h. The 
 * full tag Symbols in training and model files look like PERSON-ST and PERSON-CO. 
 * (I didn't use -START and -CONTINUE because it makes the files so big!)
 *
 */
class IdFDecoder {
public:

	/*This is a struct for holding word probability 
	  and tag probability scores for debugging purposes. 
	  As we compute the trellis we will fill the two
	  dimensional matrix and print it when it is full.
	
	typedef struct {
		double wordProb;
		double tagProb;
	} probBreakdown;

	probBreakdown** _breakdownMatrix;
	*/	

	IdFDecoder(const char *model_file_prefix, const NameClassTags *nameClassTags, 
		const char *list_file_name = 0, int word_features_mode = IdFWordFeatures::DEFAULT);
	IdFDecoder(const char *model_file_prefix, 
		const NameClassTags *nameClassTags,
		const IdFWordFeatures *wordFeatures);
	~IdFDecoder();
	bool checkNameClassTagConsistency() { return _model->checkNameClassTagConsistency();}
	void setForwardPruningThreshold(float threshold);
	void setForwardBeamWidth(int beamwidth);

	// not used in SERIF (used in stand-alone decoder to wrap decodeSentence)
	void decode(const char *input_file, const char *output_file);
	void decodeNBest(const char *input_file, const char *output_file);

	// decodes over sentenceTokens, fills in sentenceTheory
	int decodeSentence(IdFSentenceTokens* sentenceTokens, 
		IdFSentenceTheory* sentenceTheory);

	// decodes over sentenceTokens, fills in theories with pointers to
	// IdFSentenceTheory objects that the consumer is responsible for deleting
	int decodeSentenceNBest(IdFSentenceTokens* sentenceTokens, 
		IdFSentenceTheory **theories, int max_theories);

	IdFModel *_model;
private:
	const NameClassTags *_nameClassTags;

	// 1-best
	double **_alphaProbs;
	double _best_alpha_prob;
	int **_backPointers;
	float FORWARD_PRUNING_THRESHOLD;
	int FORWARD_BEAM_WIDTH;

	int decodeSentenceAsGiven(IdFSentenceTokens* sentenceTokens, 
		IdFSentenceTheory* sentenceTheory);

	// n-best
	int BACKWARD_BEAM_WIDTH;
	float BACKWARD_PRUNING_THRESHOLD;
	IdFSentenceTheory **_currentTheories;
	int _numCurrentTheories;
	IdFSentenceTheory **_previousTheories;
	int _numPreviousTheories;
	IdFSentenceTheory **_translatedNBestTheories;

	int decodeSentenceNBestAsGiven(IdFSentenceTokens* sentenceTokens, 
		IdFSentenceTheory **theories, 
		int max_theories);

	// token translation (for lists)
	int *_tokenTranslationArray;
	void translateTokens(IdFSentenceTokens *original, IdFSentenceTokens *modified);
	void unTranslateTheory(IdFSentenceTheory *modified, IdFSentenceTheory *original,
		int original_length);

	// for debugging
	void printTrellis(IdFSentenceTokens* sentenceTokens, UTF8OutputStream &stream);

	// core functions
	int walkForwardThroughSentence(IdFSentenceTokens* sentenceTokens);
	void walkBackwardThroughSentence(IdFSentenceTokens* sentenceTokens);
	void addCurrentTheory(IdFSentenceTheory *prevTheory, int sentence_index, 
						  int tag, double alpha_score, double beta_score);
	void transferCurrentTheoriesToPrevious();

};





#endif
