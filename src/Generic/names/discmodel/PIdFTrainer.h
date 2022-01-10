// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_TRAINER_H
#define P_IDF_TRAINER_H

#include "common/Symbol.h"
#include "discTagger/DTFeature.h"
#include "names/discmodel/PIdFSentence.h"

#include "common/UTF8OutputStream.h"

class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class IdFWordFeatures;
class PDecoder;


class PIdFTrainer {
		
public:
	
	enum output_mode_e {TACITURN_OUTPUT, VERBOSE_OUTPUT};

	/** The default constructor gets all the information it needs from
	  * ParamReader. */
	PIdFTrainer();

	/** This constructor creates a PIdFTrainer that does not need
	  * ParamReader.
	  *
	  * Here are some recommended default argument values:
	  *   seed_features: true
	  *   add_hyp_features: true
	  *   weightsum_granularity: 20
	  *   epochs: 5
	  *
	  * NOTE: The IdFWordFeatures object you pass in will be deleted when
	  * the PIdFTrainer is destroyed.
	  */
	PIdFTrainer(enum output_mode_e output_mode, bool seed_features,
				bool add_hyp_features, int weightsum_granularity,
				int epochs, const char *features_file,
				const char *tag_set_file, const char *word_clusters_file,
				IdFWordFeatures *wordFeatures, const char *model_file);

	~PIdFTrainer();

	DTTagSet *getTagSet() { return _tagSet; }

	/** Add given sentence to list of sentences to train on.
	  * You do not need to leave your PIdFSentence objects in memory -- 
	  * PIdFTrainer makes its own copy. In fact, you should probably keep
	  * a single PIdFSentence object for yourself and recycle it (using
	  * its clear() method). */
	void addTrainingSentence(PIdFSentence &sentence);

	/** Load all sentences from given training file.
	  * The file argument may only be omitted if
	  * you used the default constructor, which gets the training file
	  * name from ParamReader */
	void addTrainingSentencesFromTrainingFile(const char *file = 0);

	void addTrainingSentencesFromTrainingFileList(const char *file = 0);


	/** This is the method that will take a long time to return.
	  * It will write the model file to the path specified to the
	  * constructor. */
	void train();

private:
	UTF8OutputStream _historyStream;

	void addTrainingFeatures();
	double trainEpoch();
	void writeWeights(int epoch = -1);
	void writeTransitions(int epoch = -1);
	bool populateTrainingSentence(PIdFSentence &sentence);

	// These are for looping through the sentences in the SentenceBlocks
	void seekToFirstSentence();
	bool moreSentences();
	PIdFSentence *getNextSentence();


	enum output_mode_e _output_mode;
	bool _seed_features;
	bool _add_hyp_features;
	bool _interleave_tags;	//this changes the order tags are processed
	bool _learn_transitions_from_training; //learn possible features from training file, then produce model-transition file
	
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
	char _training_file[500]; // only used if default constructor called
	char _model_file[500];
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;
	bool _traininglist; 
	int _nTrainSent;

	PDecoder *_decoder;
	DTFeature::FeatureWeightMap *_weights;

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
};

#endif
