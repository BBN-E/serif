// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_MODEL_H
#define P_IDF_MODEL_H

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/names/discmodel/PIdFSecondaryDecoders.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/common/limits.h"
#include "Generic/names/IdFWordFeatures.h"
#include <vector>

#include "Generic/discTagger/DTMaxMatchingListFeatureType.h"
#include "Generic/discTagger/DTObservation.h"

class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class PDecoder;
class BlockFeatureTable;

class SentAndScore {
public:
	double margin;
	double realmargin;
	PIdFSentence* sent;
	int id;
	int pos;
	int unk;
	double rare_tag_val;
    int n_name_spans;
	//SentAndScore();
	//SentAndScore(double m, PIdFSentence* s, int id);

	bool operator<(const SentAndScore& other) const {
		return margin < other.margin;
	}
	SentAndScore(): margin(0), sent(0) {};
	SentAndScore(double m, PIdFSentence* s, int num): margin(m), sent(s), id(num), pos(0), rare_tag_val(0) {};
    SentAndScore(double m, PIdFSentence* s, int num, int name_spans): margin(m), sent(s), id(num), pos(0), rare_tag_val(0), n_name_spans(name_spans) {};
};

/**
  * PIdFModel is a merge of what used to be PIdFTrainer, PIdFDecoder, 
  * and PIdFSimActiveLearningTrainer ---mrf
  */

class PIdFModel {
		
public:
	
	enum output_mode_e {TACITURN_OUTPUT, VERBOSE_OUTPUT};
	enum model_mode_e {TRAIN, TRAIN_AND_DECODE, DECODE, SIM_AL, UNSUP, UNSUP_CHILD, DECODE_AND_CHOOSE};


	/** 
	  * The default constructor gets all the information it needs from
	  * ParamReader. 
	  */
	PIdFModel(model_mode_e mode);

	/** 
	  * -WARNING- This constructor may only be used in decode mode,
	  *	It was written to allow the PIdF to be used for nominal hw recognition
	  *	This constructor uses the input files specified in 
	  *	the parameter list
	  */
	PIdFModel(model_mode_e mode, const char* tag_set_file, 
		const char* features_file, const char* model_file, const char* vocab_file,
		bool learn_transitions = false, bool use_clusters=true);
	PIdFModel(model_mode_e mode, const char* tag_set_file, 
		const char* features_file, const char* model_file, const char* vocab_file, 
		const char* lc_model_file, const char* lc_vocab_file,
		bool learn_transitions = false, bool use_clusters=true);

	void setWordFeaturesMode(int mode) { _wordFeatures->setMode(mode); }

	~PIdFModel();

	DTTagSet *getTagSet() { return _tagSet; }

	/** 
	  * Add given sentence to list of sentences to train on.
	  * You do not need to leave your PIdFSentence objects in memory -- 
	  * PIdFTrainer makes its own copy. In fact, you should probably keep
	  * a single PIdFSentence object for yourself and recycle it (using
	  * its clear() method). 
	  */
	void addTrainingSentence(PIdFSentence &sentence);
	
	/** 
	  * Load all sentences from given training file.
	  * The file argument may only be omitted if
	  * you used the default constructor, which gets the training file
	  * name from ParamReader 
	  */
	void addTrainingSentencesFromTrainingFile(const char *file, bool is_encrypted);

	/** 
	  * Load training sentences from a list of files (similar to IdF)
	  * instead of a single file
	  */
	void addTrainingSentencesFromTrainingFileList(const char *file, bool files_are_encrypted);

	/** 
	  * Add given sentence to list of active learning sentences to choose from.
	  * You do not need to leave your PIdFSentence objects in memory -- 
	  * PIdFSimActiveLearningTrainer makes its own copy. In fact, you should probably keep
	  * a single PIdFSentence object for yourself and recycle it (using
	  * its clear() method). 
	  * --- this method is mostly copied from addTrainingSentence()
	  */
	void addActiveLearningSentence(PIdFSentence &sentence);

	/** 
	  * Load all active learning sentences from given file.
	  * The file argument may only be omitted if
	  * you used the default constructor, which gets the training file
	  * name from ParamReader 
	  * ---- this method is mostly copied from addTrainingSentencesFromTrainingFile
	  */
	void addActiveLearningSentencesFromTrainingFile(const char *file = 0);

	/** 
	  * Load active learning sentences from a list of files (similar to IdF)
	  * instead of a single file
	  */
	void addActiveLearningSentencesFromTrainingFileList(const char *file = 0);

	/** 
	  * Load the first n sentences from active learning corpus to training 
	  * (to be used as seed training).
	  *  must have already called addActiveLearningSentencesFromTrainingFile
	  */
	 void addTrainingSentencesFromALCorpus(int nsent = 200);

	 /**
	   * Add sentences to training either from AL corpus or from the 
	   * training file specified in the parameter file
	   */
	 void seedALTraining();

	/** 
	  * This is the method that will take a long time to return.
	  * It will write the model file to the path specified to the
	  * constructor. 
	  */
	void train();

	/** 
	  * This method decodes using the input and output files specified
	  * by the parameter file. 
	  */
	void decode();

	/** This method decodes using provided streams */
	void decode(UTF8InputStream &in, UTF8OutputStream &out);
	void constrainedDecode(UTF8InputStream &in, UTF8OutputStream &out);


	/** 
	  * This method decodes on a PIdFSentence by storing its answers
	  * in the PIdFSentence instance. This is the decode method to use
	  * when you do your own I/O -- that's you, Marc. 
	  */
	void decode(PIdFSentence &sentence);

	void decode(PIdFSentence &sentence, double& margin);

	/** 
	  * This method decodes on a PIdFSentence and only allows 
	  * changes to the none tags- used in semisupervised learning
	  * experiments
	  */
	std::wstring constrainedDecode(PIdFSentence &constraintSentence, DTTagSet *tagSet);
	
	/** For use as a Serif stage (as opp. to standalone) */
	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

	void resetForNewDocument(DocTheory *docTheory = 0);
	
	//void analyzeTraining(const char* testfile);
	//Symbol getClusterSymbol(const wchar_t* pref, int c);
	//void addToCountArray(float count, int offset, int* countarray);
	void writeModel(char* str);
	void writeSentences(char* str);

	/** Clean up memory assocated with weights */
	void freeWeights();
	
	/**
	  *	Important, if you are going to use the same model that you trained to decode, you must
	  * transfer the values in _sum to _value, 
	  */
	void finalizeWeights();
	
	/**
	  * Debuging method compare the weights 
	  * in this model to the weights in another model
	  */
	void compareWeights(PIdFModel& other, int n);

	/** 
	  *  Fill in observation with values of word 
	  *  this is public/static because it is used by the Active Learning (QuickLearn
	  *  version of the code as well.
	  */
	static void populateObservation(TokenObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, bool first_word, 
		//bool word_is_in_vocab,
		int wordCount,
		bool lowercased_word_is_in_vocab,
		bool use_lowercase_clusters,
		NgramScoreTable* bigram_counts
	);
	
	/** 
	  * This is just for the active learning to compile for now
	  * it should be removed and taken care of
	  * the bigram table should actually be given in the featuretype const., not here
	  */
	static void populateObservation(TokenObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, bool first_word, 
		//bool word_is_in_vocab,
		int wordCount,
		bool lowercased_word_is_in_vocab,
		bool use_lowercase_clusters
	);
	
	/** Fill in all of the observations for a sentence */
	void populateSentence(std::vector<DTObservation *> & observations, PIdFSentence* sent, 
		bool use_lowercase_clusters = false);

	/** Make a name theory (as used in Serif) from a PIdFSentence */
	NameTheory *makeNameTheory(PIdFSentence &sentence);
	
	/** Run a simulated Active Learning Experiment */
	void doActiveLearning( );

	/** Run a semisupervised experiment */
	void doUnsupervisedTrainAndSelect();

    /** Choose high-scoring sentences according to
        margin and number of named entites, print them */
    void chooseHighScoreSentences();

    void reAssignScores(int i);

	void useLowercaseDecoder() { 
		if (_lowerCaseDecoder != 0) {
			_decoder = _lowerCaseDecoder;
			_vocab = _lcVocab;
		}
	}

	void useDefaultDecoder() { _decoder = _defaultDecoder; _vocab = _defaultVocab; }

private:
	UTF8OutputStream _historyStream;
	bool _writeHistory;
	PIdFSecondaryDecoders *_secondaryDecoders;
	enum output_mode_e _output_mode;
	enum model_mode_e _model_mode;
	bool _use_clusters;
	bool _seed_features;
	bool _add_hyp_features;
	bool _print_after_every_epoch;
	bool _interleave_tags;	//this changes the order tags are processed 
	bool _learn_transitions_from_training; //learn possible features from training file, then produce model-transition file
	int _nTrainSent;
	int _weightsum_granularity;
	bool _use_fast_training; /* train by focusing on the errors */
	//bool _use_late_averaging;

	/**
	  * Alternative ways of determining how many epochs to run for
	  * train for at most _epochs, but stop when either you've 
	  * guessed correctly about _min_tot% of the sentences
	  * or the model didn't change by at least _min_change between the 
	  * previous epoch and this epoch, 
	  * if _min_tot = 1 and _min_change = -100, you will train for _epochs epochs
	  */
	int _epochs;
	double _min_tot;
	double _min_change;	

	std::string _model_file;
	bool _use_stored_vocabulary;
	bool _use_stored_bigram;
    std::string _vocab_file;
    std::string _bigram_file;

	/** DTModel classes */
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;
	PDecoder *_decoder;
	NgramScoreTable * _vocab;

	/**
      * Used when calling _decoder->decode(), since decode works on a series of observations, 
	  * not a PIdFSentence
	  */
	std::vector<DTObservation *> _observations;
	static Token _blankToken;
	static Symbol _blankLCSymbol;
	static Symbol _blankWordFeatures;
	static WordClusterClass _blankWordClass;
	static Symbol _NONE_ST;
	static Symbol _NONE_CO;
	
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

	// keep a separate block of active learning sentences
	SentenceBlock *_firstALSentBlock;
	SentenceBlock *_lastALSentBlock;
	SentenceBlock *_curALSentBlock;
	int _curAL_sent_no;

	PDecoder *_defaultDecoder;
	BlockFeatureTable *_defaultBlockWeights;
	DTFeature::FeatureWeightMap *_defaultWeights;
	NgramScoreTable *_defaultVocab;
	PDecoder *_lowerCaseDecoder;
	BlockFeatureTable *_lcBlockWeights;
	DTFeature::FeatureWeightMap *_lcWeights;
	NgramScoreTable *_lcVocab;
	NgramScoreTable *_defaultBigramVocab;
	
	/** 
	  * Vectors to all decoded sentences to be sorted, used for
	  * simulated active learning and semisupervised learning experiments
	  */
	// Define a template class vector of int
	typedef std::vector<SentAndScore> ScoredSentenceVector;
    //Define an scoredSentenceVector for template class vector of strings
	typedef ScoredSentenceVector::iterator ScoredSentenceVectorIt;
	ScoredSentenceVector* _tagSpecificScoredSentences;
	ScoredSentenceVector _scoredSentences;	

	NgramScoreTable* _bigramFrequency;
	double _mult;
	int _nStoredTags;
	double _percentTagSpecific;
	int _n_ALSentences;
	int _nToAdd;
	int _nActiveLearningIter;
	bool _allowSentenceRepeats;

	int _n_child_models;
	double _child_percent_training;
	double _required_margin; // required margin for correct classification
    bool _print_sentence_selection_info;

	void addTrainingFeatures();
	double trainEpoch(int epoch = -1, bool randomize_sent = false);
//	double trainEpoch(int epoch = -1, bool randomize_sent = false, bool add_average=true);
	
	void writeWeights(int epoch = -1);
	void writeWeights(const char* str);
	void dumpTrainingParameters(UTF8OutputStream &out);
	void writeTransitions(char* str);
	void writeTrainingSentences(char* str);
	void outputDate(UTF8OutputStream& out);

	void readInitialWeights();
	void readTransitions(char* transitionfile);

	bool populateTrainingSentence(PIdFSentence &sentence);

	/** 
	  * These are for looping through the sentences in the SentenceBlocks
	  * (used in train, sim_al, and unsup mode)
	  */
	void seekToFirstSentence();
	bool moreSentences();
	PIdFSentence *getNextSentence();

	/** 
	  * These are for looping through the active learning sentences 
	  * (used in sim_al mode and unsup mode)
	  */
	void seekToFirstALSentence();
	bool moreALSentences();
	PIdFSentence *getNextALSentence();

	/** 
	  *	 This method calls train, to train a current model from 
	  *  sentences in the training block and then selects a new 
	  *  set of sentences from the Active Learning block to 
	  *  add to the sentence block.  
	  */
	void trainAndSelect(bool print_models, int count, int n_to_select = -1 );
	
	/** 
	  *  This method creates n_models models (each with percent_of_training sentences)
	  *		1) Trains these models
	  *     2) Decodes the entire AL_corpus with each model
	  *     3) Selects the senteces with a min_margin  > 0 for all models where all models agree
	  *     4) Adds n_to_select of these decoded sentences to the training
	  */
	void unsupervisedTrainAndSelect(bool print_models,int n_models, double percent_of_training, 
		int count, int n_to_select = -1);

	/** 
	  * This method adds all the vocab in the currently stored sentences 
	  * to the specified table 
	  */
	void fillVocabTable(NgramScoreTable *table);

	/** 
	  * This method adds all the bigram vocab in the currently stored sentences 
	  * to the specified table 
	  */
	void fillBigramVocabTable(NgramScoreTable *table);

	/* faster training */
	void fastTrain(); 

	/* normal training */
	void normalTrain();

	/* faster 1 epoch training */
	void trainEpochFocusOnErrors(int epoch, double &estimated_acc, double &first_acc);

	void updateDictionaryMatchingCache(const DTMaxMatchingListFeatureType* dtMaxMatchingListFeature, 
		std::vector<DTObservation *> &observations);

};





#endif
