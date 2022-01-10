// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_SIMACTIVELEARNING_TRAINER_H
#define P_IDF_SIMACTIVELEARNING_TRAINER_H

#include "common/Symbol.h"
#include "discTagger/DTFeature.h"
#include "names/discmodel/PIdFSentence.h"

#include "common/UTF8OutputStream.h"
//use STL implementations, maybe I should implement these myself.....
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>


class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class IdFWordFeatures;
class PDecoder;
//using namespace std;


	
class SentAndScore{
public:
	double margin;
	PIdFSentence* sent;
	int id;
	SentAndScore();
	SentAndScore(double m, PIdFSentence* s, int id);

	bool operator<(SentAndScore& other)
	{
		return margin < other.margin;
	}
};

class PIdFSimActiveLearningTrainer {
private:
	UTF8OutputStream _historyStream;	
	// Define a template class vector of int
	typedef std::vector<SentAndScore> ScoredSentenceVector;

    //Define an scoredSentenceVector for template class vector of strings
	typedef ScoredSentenceVector::iterator ScoredSentenceVectorIt;

	ScoredSentenceVector _scoredSentences;
	int _n_ALSentences;
	int _nToAdd;
	int _nActiveLearningIter;
	bool _allowSentenceRepeats;
	bool _interleave_tags;
public:
	enum output_mode_e {TACITURN_OUTPUT, VERBOSE_OUTPUT};

	/** The default constructor gets all the information it needs from
	  * ParamReader. */
	PIdFSimActiveLearningTrainer();

	/** This constructor creates a PIdFSimActiveLearningTrainer that does not need
	  * ParamReader.
	  *
	  * Here are some recommended default argument values:
	  *   seed_features: true
	  *   add_hyp_features: true
	  *   weightsum_granularity: 20
	  *   epochs: 5
	  *
	  * NOTE: The IdFWordFeatures object you pass in will be deleted when
	  * the PIdFSimActiveLearningTrainer is destroyed.
	  
	PIdFSimActiveLearningTrainer(enum output_mode_e output_mode, bool seed_features,
				bool add_hyp_features, int weightsum_granularity,
				int epochs, const char *features_file,
				const char *tag_set_file, const char *word_clusters_file,
				IdFWordFeatures *wordFeatures, const char *model_file);
	*/
	~PIdFSimActiveLearningTrainer();

	DTTagSet *getTagSet() { return _tagSet; }

	/** Add given sentence to list of sentences to train on.
	  * You do not need to leave your PIdFSentence objects in memory -- 
	  * PIdFSimActiveLearningTrainer makes its own copy. In fact, you should probably keep
	  * a single PIdFSentence object for yourself and recycle it (using
	  * its clear() method). */
	void addTrainingSentence(PIdFSentence &sentence);

	/** Load all sentences from given training file.
	  * The file argument may only be omitted if
	  * you used the default constructor, which gets the training file
	  * name from ParamReader */
	void addTrainingSentencesFromTrainingFile(const char *file = 0);
	
	/** Add given sentence to list of active learning sentences to choose from.
	  * You do not need to leave your PIdFSentence objects in memory -- 
	  * PIdFSimActiveLearningTrainer makes its own copy. In fact, you should probably keep
	  * a single PIdFSentence object for yourself and recycle it (using
	  * its clear() method). */
	void addActiveLearningSentence(PIdFSentence &sentence);
	/** Load all active learning sentences from given file.
	  * The file argument may only be omitted if
	  * you used the default constructor, which gets the training file
	  * name from ParamReader */
	void addActiveLearningSentencesFromTrainingFileList(const char *file = 0);

	/** This is the method that will take a long time to return.
	  * It will write the model file to the path specified to the
	  * constructor. */
	void train();
	/** 
	  *	 This method calls train, to train a current model from 
	  *  sentences in the training block and then selects a new 
	  *  set of sentences from the Active Learning block to 
	  *  add to the sentence block.  
	  */
	void trainAndSelect(bool print_models, int count, int n_to_select = -1 );
	void doActiveLearning( );

private:
	void addTrainingFeatures();
	double trainEpoch();
	void readInitialWeights();
	void writeWeights(int epoch = -1);
	void writeTransitions(int epoch = -1);
	void writeTrainingSentences(int epoch = -1);

	bool populateTrainingSentence(PIdFSentence &sentence);

	// These are for looping through the sentences in the SentenceBlocks
	void seekToFirstSentence();
	bool moreSentences();
	PIdFSentence *getNextSentence();


	// These are for looping through the active learning sentences
	void seekToFirstALSentence();
	bool moreALSentences();
	PIdFSentence *getNextALSentence();

	enum output_mode_e _output_mode;
	bool _seed_features;
	bool _add_hyp_features;
	//alternative ways of determining how many epochs to run for
	//train for at most _epochs, but stop when either you've 
	//guessed correctly about _min_tot% of the sentences
	//or the model didn't change by at least _min_change between the 
	//previous epoch and this epoch, 
	//if _min_tot = 1 and _min_change = 0, you will train for _epochs epochs

	int _epochs;
	double _min_tot;
	double _min_change;
	int _weightsum_granularity;
	bool _learn_transitions_from_training;
	char _training_file[500]; // only used if default constructor called
	char _active_learning_file[500]; 

	char _model_file[500];
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;

	PDecoder *_decoder;
	DTFeature::FeatureWeightMap *_weights;

	DTFeature::FeatureWeightMap *_currentWeights;

	class SentenceBlock {
	public:
		const static int BLOCK_SIZE = 10000;

		PIdFSentence sentences[BLOCK_SIZE];
		int n_sentences;
		SentenceBlock *next;

		SentenceBlock() : n_sentences(0), next(0) {}
	};

	SentenceBlock *_firstSentBlock;
	SentenceBlock *_lastSentBlock;
	SentenceBlock *_curSentBlock;
	int _cur_sent_no;

	//keep a separate block of active learning sentences
	SentenceBlock *_firstALSentBlock;
	SentenceBlock *_lastALSentBlock;
	SentenceBlock *_curALSentBlock;
	int _curAL_sent_no;

};

#endif
