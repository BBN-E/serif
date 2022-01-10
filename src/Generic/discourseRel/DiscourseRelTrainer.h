// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef Discourse_Rel_TRAINER_H
#define Discourse_Rel_TRAINER_H

#include <iostream>
#include <stdio.h>

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"
#include <vector>
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/common/SymbolUtilities.h"

class DTFeatureTypeSet;
class DTTagSet;
class P1Decoder;
class MaxEntModel;

class Entity;
class EntitySet;
class TokenSequence;
class Parse;
class MentionSet;
class Mention;
class SynNode;
class DocTheory;


class DTObservation;
class DiscourseRelObservation;
class PennDiscourseTreebank;
class CrossSentRelation;


class DiscourseRelTrainer {
public:
	DiscourseRelTrainer();
	~DiscourseRelTrainer();

	void train();
	void devTest();

private:
	int MODEL_TYPE;
	enum {P1, MAX_ENT, BOTH, P1_RANKING};

	int MODE;
	enum {TRAIN, DEVTEST, CV, ANALY};

	int TRAIN_SOURCE;
	enum {PTB_PARSES, AUTO_PARSES};

	int TASK;
	enum {EXPLICIT_ALL, EXPLICIT_SELECT, CROSS_SENT_ALL, CROSS_SENT_SELECT, CROSS_SENT_IMPLICIT};
	std::string _training_file_list;
	std::string _pdtb_file_list;
	std::string _connectives_file;
	std::string _stopword_file;

	std::string _model_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	double *_tagScores;

	const static int MAX_CANDIDATES;
	//DiscourseRelObservation *_observation;

	// these two data members are used for cross-validation or train/eval of the model
	int _cross_validation_fold;   //default value = 5
	std::vector<DTObservation*> _observations;
	std::vector<int> _keys;
	//vector<CrossSentRelation *> _keysDetails;
	vector<string> _relTypes;

	// MAX_ENT
	MaxEntModel *_maxEntDecoder;
	DTFeature::FeatureWeightMap *_maxEntWeights;
	int _pruning;
	int _percent_held_out;
	int _mode;
	int _max_iterations;
	double _variance;
	double _likelihood_delta;
	int _stop_check_freq;
	std::string _train_vector_file;
	std::string _test_vector_file;
	double _link_threshold;
	
	void trainMaxEntAutoParses();
	void trainMaxEntPTBParses(); 

	// P1
	P1Decoder *_p1Decoder;
	DTFeature::FeatureWeightMap *_p1Weights;
	int _epochs;
	int _n_instances_seen;
	int _n_correct;
	int _total_n_correct;
	
	// RANKING
	double *_rankingScores;

	void trainP1AutoParses();
	void trainP1PTBParses(); 
	void trainEpochAutoParses();
	
	// DEVTEST
	UTF8OutputStream _devTestStream;
	void devTestMaxEntPTBParses();
	void devTestMaxEntAutoParses();
	
	// CROSS VALIDATION
	// specific to cross-sentence relation
	// statistics for different relation types: explicit, implicit, altLex, entRel
	struct IntIntIntInt {
		int correctPos;
		int correctNeg;
		int falsePos;
		int falseNeg;
	};

	UTF8OutputStream _cvStream;
	void cvMaxEntPTBParses();
	void cvMaxEntAutoParses();
	
	UTF8OutputStream _featDebugStream;
	
	// structure to hold information from brandy internal state files
	// theories such as POS, Parsing, Entity, Relation etc.
	int _num_documents;
	DocTheory **_docTheories;
	//class Symbol *_docIds;
	//class Symbol *_documentTopics;
	class TokenSequence **_tokenSequences;
	const class Parse **_parses;
	//class Parse **_secondaryParses;
	//class NPChunkTheory** _npChunks;
	//class ValueMentionSet ** _valueMentionSets;
	class MentionSet ** _mentionSets;
	const class PropositionSet ** _propSets;
	class EntitySet ** _entitySets;
	//class EventMentionSet ** _eventMentionSets;

	vector <std::string> _stateFileNameList;

	// structure to hold information from PDTB
	PennDiscourseTreebank *_pdtb;

	void loadTrainingDataFromList(const char *listfile);
	void loadTrainingData(const wchar_t *filename, int& index);	
	

	void processSentence (string docName, string sentIndex, const TokenSequence *tokens, Parse *parse);
	void processSentence_crossSent (string docName, string sentIndex, int sent);
	
	void processDocument(DocTheory *docTheory, string docName);
	void processDocument_crossSentRel(DocTheory *docTheory, string docName);
	//void printDebugScores(int mentionID, int entityID, 
	//	                 int hobbs_distance,UTF8OutputStream& stream);
									  
	//void decodeToP1Distribution(DiscourseRelObservation *observation);								  
	void writeWeights(int epoch = -1);
	void dumpTrainingParameters(UTF8OutputStream &out);
	int countDocumentsInFileList(const char *filename);
	void printFeatures(UTF8OutputStream &out, DTObservation *observation, int key);
	void dataAnalysis();

	bool DEBUG;
	UTF8OutputStream _debugStream;

};

#endif
