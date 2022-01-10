// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_TIMEX_ARG_FINDER_H
#define RELATION_TIMEX_ARG_FINDER_H

#include "theories/RelMention.h"
class SynNode;
class TokenSequence;
class Parse;
class MentionSet;
class PropositionSet;
class RelMentionSet;
class RelationTimexArgObservation;
class MaxEntModel;
class DTTagSet;
class DTFeatureTypeSet;
#include "discTagger/DTFeature.h"

class RelationTimexArgFinder {
public:
	RelationTimexArgFinder(int mode_);
	~RelationTimexArgFinder();
	void cleanup();

	void train();
	void roundRobin();
	void devtest();

	void attachTimeArguments(RelMentionSet *relMentionSet, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet);
	void attachTimeArgument(RelMention *relMention, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet);
	void train(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		RelMentionSet **relsets, int nsentences);
	void decode(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		RelMentionSet **relsets, int nsentences, UTF8OutputStream& resultStream);
	void replaceModel(char *model_file);
	void resetRoundRobinStatistics();
	void printRoundRobinStatistics(UTF8OutputStream &out);

	enum { TRAIN, TRAIN_SECOND_PASS, DECODE };
private:
	int MODE;
	UTF8OutputStream _debugStream;
	int DEBUG;

	const MentionSet *_mentionSet;

	void attachTimeArgument(MaxEntModel *model, RelMention *relMention, 
		const TokenSequence *tokens, ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet);
	
	MaxEntModel *_model;
	DTFeature::FeatureWeightMap *_weights;
	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	double *_tagScores;
	double _recall_threshold;

	MaxEntModel *_secondPassModel;
	DTFeature::FeatureWeightMap *_secondPassWeights;
	DTFeatureTypeSet *_secondPassFeatureTypes;
	bool _use_two_pass_model;

	struct PotentialTimeArgument {
		Symbol role;
		const ValueMention *valueMention;
		float score;
        PotentialTimeArgument* next;			
		PotentialTimeArgument(Symbol role_, const ValueMention *valueMention_, float score_)
			: role(role_), valueMention(valueMention_), score(score_), next(0) {}
		~PotentialTimeArgument() { 
			delete next; 
		}
    };

	PotentialTimeArgument *_potentialTimeArgument;
	int addTagsForCandidateValue(MaxEntModel *model, const ValueMention *value);
	void addPotentialArgument(PotentialTimeArgument *newArgument);

	RelationTimexArgObservation *_observation;

	void dumpTrainingParameters(UTF8OutputStream &out);

	// ROUND ROBIN DECODING
	int _correct_args;
	int _correct_non_args;
	int _wrong_type;
	int _missed;
	int _spurious;
	void printDebugScores(RelMention *rm, const ValueMention *arg, 
										  UTF8OutputStream& stream);


	// LOADING TRAINING
	class TokenSequence **_tokenSequences;
	class Parse **_parses;
	class Parse **_secondaryParses;
	class NPChunkTheory** _npChunks;
	class ValueMentionSet ** _valueMentionSets;
	class MentionSet ** _mentionSets;
	class PropositionSet ** _propSets;
	class RelMentionSet ** _relationMentionSets;

	class MorphologicalAnalyzer *_morphAnalysis;

	int loadTrainingData(class TrainingLoader *trainingLoader);	

};


#endif
