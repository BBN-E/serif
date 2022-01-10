// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_ARGUMENT_FINDER_H
#define EVENT_ARGUMENT_FINDER_H

#include "Generic/theories/EventMention.h"
class SynNode;
class TokenSequence;
class Parse;
class MentionSet;
class PropositionSet;
class EventMentionSet;
class EventAAObservation;
class MaxEntModel;
class DTTagSet;
class DTFeatureTypeSet;
class P1Decoder;
#include "Generic/discTagger/DTFeature.h"

class EventArgumentFinder {
public:
	EventArgumentFinder(int mode_);
	~EventArgumentFinder();

	void attachArguments(EventMention *vMention, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet);
	void train(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences);
	void decode(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences, UTF8OutputStream& resultStream);

	void devTestDecode(TokenSequence *tokenSequence, Parse *parse, ValueMentionSet *valueMentionSet, 
				MentionSet *mentionSet, PropositionSet *propSet, EventMentionSet *eset);
	void devTestDecode(TokenSequence *tokenSequence, Parse *parse, ValueMentionSet *valueMentionSet, 
				MentionSet *mentionSet, PropositionSet *propSet, EventMentionSet *keyEset, EventMentionSet *testEset);

	void replaceModel(char *model_file);
	void resetRoundRobinStatistics();
	void printRoundRobinStatistics(UTF8OutputStream &out);

	void resetAllStatistics();
	void printRecPrecF1(UTF8OutputStream& out);

	const DTTagSet* getTagSet() { return _tagSet; }

	enum { TRAIN_MAXENT, TRAIN_P1, TRAIN_BOTH, DECODE };
private:
	int MODE;
	UTF8OutputStream _debugStream;
	int DEBUG;

	bool _attach_event_only_values;

	const MentionSet *_mentionSet;

	void attachArguments(MaxEntModel *model, const EventMention *vMention, 
		const TokenSequence *tokens, ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet);
	void collectPotentialArguments(const TokenSequence *tokens,
		const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, PropositionSet *propSet,
		const EventMention *vMention);

	void classifyObservationWithMaxEnt(EventAAObservation* observation);
	int addBasicTags(const Mention *mention, EventAAObservation* observation);
	int addBasicTags(const ValueMention *vMention, EventAAObservation* observation);
	MaxEntModel* getMaxEntModelToUse(const EventAAObservation* observation) const;
	
	int validateAndReturnRoleType(Symbol roleSym, bool isTimex=false) const;
	
	MaxEntModel *_model;
	P1Decoder *_p1Model;
	DTFeature::FeatureWeightMap *_weights;
	DTFeature::FeatureWeightMap *_p1Weights;
	double _overgen_percentage;

	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	double *_tagScores;
	double _recall_threshold;
	double _second_arg_recall_threshold;



	struct PotentialEventArgument {
		Symbol role;
		const Mention *mention;
		const ValueMention *valueMention;
		float score;
        PotentialEventArgument* next;
		PotentialEventArgument(Symbol role_, const Mention *mention_, float score_)
			: role(role_), mention(mention_), score(score_), next(0), valueMention(0) {}			
		PotentialEventArgument(Symbol role_, const ValueMention *valueMention_, float score_)
			: role(role_), valueMention(valueMention_), score(score_), next(0), mention(0) {}
		~PotentialEventArgument() { 
			delete next; 
		}
    };

	PotentialEventArgument *_potentialEventArgument;

	void addPotentialArgument(PotentialEventArgument *newArgument);

	void addFakedTags(const EventAAObservation* observation, const Mention *mention, int *choices, int nchoices);
	int _gpeFacLocChoices[3];
	int _perOrgChoices[11];


	void dumpTrainingParameters(UTF8OutputStream &out);

	// ROUND ROBIN DECODING
	int _correct_args;
	int _correct_non_args;
	int _wrong_type;
	int _missed;
	int _spurious;

	int _correct_entity_mention;
	int _wrong_entity_mention;
	int _missed_entity_mention;
	int _spurious_entity_mention;
	int _correct_value_mention;
	int _wrong_value_mention;
	int _missed_value_mention;
	int _spurious_value_mention;

	enum {CORRECT, SPURIOUS, MISSING, WRONG_TYPE};
	void printDebugResult(EventAAObservation* observation, int result, 
							UTF8OutputStream& stream,  Symbol role, Symbol other_role=Symbol(L""));
	void printDebugScores(EventAAObservation* observation, 
										   UTF8OutputStream& stream);


	// event-aa with gold entity and value mentions
	typedef std::pair<const Mention*, Symbol> MentionRolePair;
	typedef std::pair<const ValueMention*, Symbol> ValueMentionRolePair;

	std::vector<MentionRolePair> getMentionRolePairs(EventMentionSet *eset);
	std::vector<ValueMentionRolePair> getValueMentionRolePairs(EventMentionSet *eset);

	void tabulateStatistics(const std::vector<MentionRolePair>& key, const std::vector<MentionRolePair>& test);
	void tabulateStatistics(const std::vector<ValueMentionRolePair>& key, const std::vector<ValueMentionRolePair>& test);


	// split classifiers
	MaxEntModel *_directModel;
	MaxEntModel *_sharedModel;
	MaxEntModel *_unconnectedModel;
	DTFeature::FeatureWeightMap *_directWeights;
	DTFeature::FeatureWeightMap *_sharedWeights;
	DTFeature::FeatureWeightMap *_unconnectedWeights;
	DTFeatureTypeSet *_directFeatureTypes;
	DTFeatureTypeSet *_sharedFeatureTypes;
	DTFeatureTypeSet *_unconnectedFeatureTypes;
	bool _use_split_classifiers;
};


#endif
