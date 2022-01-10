// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_TRIGGER_FINDER_H
#define EVENT_TRIGGER_FINDER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/common/UTF8OutputStream.h"

class TokenSequence;
class Parse;
class ValueMentionSet;
class MentionSet;
class PropositionSet;
class EventMention;
class MaxEntModel;
class EventTriggerObservation;
class DTTagSet;
class DTFeatureTypeSet;
class EventMentionSet;
class P1Decoder;
class ETProbModelSet;
class SynNode;


class EventTriggerFinder {
public:
	EventTriggerFinder(int mode_);
	~EventTriggerFinder();

	int processSentence(const TokenSequence *tokens, const Parse *parse, MentionSet *mentionSet, 
		const PropositionSet *propSet, EventMention **potentials, int max);
	void train(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, Symbol *docTopics, int nsentences);
	void replaceModel(char *model_file);
	void roundRobinDecode(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences,
								UTF8OutputStream& resultStream,
								UTF8OutputStream& HRresultStream,
								UTF8OutputStream& keyStream,
								UTF8OutputStream& htmlStream);

	EventMentionSet* devTestDecode(const TokenSequence *tseq, Parse *parse, ValueMentionSet *valueMentionSet, MentionSet *mentionSet,
		PropositionSet *propSet, EventMentionSet *eset, int nsentences);

	void selectAnnotation(Symbol *docIds, Symbol *docTopics, TokenSequence **tokenSequences, 
		Parse **parses, ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
		PropositionSet **propSets, EventMentionSet **esets, int nsentences, 
		UTF8OutputStream& annotationStream);

	void resetRoundRobinStatistics();
	void printRoundRobinStatistics(UTF8OutputStream &out);
	void printRecPrecF1(UTF8OutputStream& out);

	void setDocumentTopic(Symbol topic) { _documentTopic = topic; }

	const DTTagSet* getTagSet() { return _tagSet; }

	enum { TRAIN, DECODE, ADD_FEATURES };
private:
	int MODE;
	
	bool DEBUG;
	UTF8OutputStream _debugStream;

	EventTriggerObservation *_observation;
	MaxEntModel *_maxentModel;
	P1Decoder *_p1Model;
	DTFeature::FeatureWeightMap *_p1Weights;
	DTFeature::FeatureWeightMap *_maxentWeights;
	double _recall_threshold;
	float _p1_overgen_percentage;
	bool _seed_features;

	ETProbModelSet *_probModelSet;
	
	void processToken(int tok, const TokenSequence *tokens, const Parse *parse, MentionSet *mentionSet, 
					const PropositionSet *propSet, EventMention **potentials, int& n_potential_events, int max_events);
	float setScore(EventMention *eventMention, int modelnum);

	void decodeToP1Distribution();
	int decodeToMaxentDistribution();
	void printP1DebugScores(UTF8OutputStream& out);
	void printMaxentDebugScores(UTF8OutputStream& out);

	bool isHighValueAnnotation(int tok, const TokenSequence *tokens, const Parse *parse, 
							MentionSet *mentionSet, const PropositionSet *propSet);
	void printAnnotationSentence(UTF8OutputStream& out, Symbol *docIds,
								 TokenSequence **tokenSequences, int index, int n_sentences);
	
	bool _use_maxent_model;
	bool _use_p1_model;

	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	double *_tagScores;
	bool *_tagFound;

	Symbol _documentTopic;

	void walkThroughTrainingSentence(int mode, int epoch, 
		const TokenSequence *tokenSequence, const Parse *parse, 
		const ValueMentionSet *valueMentionSet, MentionSet *mentionSet, 
		const PropositionSet *propSet, const EventMentionSet *eset);

	// ROUND ROBIN DECODING
	int _correct;
	int _wrong_type;
	int _missed;
	int _spurious;
	int _HR_correct;
	int _HR_wrong_type;
	int _HR_missed;
	int _HR_spurious;

	void printHTMLSentence(UTF8OutputStream &out, const TokenSequence *tseq, int index, 
		int correct_answer, int system_answer, float system_score = 0);

	void dumpP1TrainingParameters(UTF8OutputStream &out, int epoch);
	void dumpMaxentTrainingParameters(UTF8OutputStream &out);

	static const int MAX_TOKEN_TRIGGERS = 10;
/*
	//frequency counts
	int _n_nhead_trigger;
	int _n_vhead_trigger;
	int _n_npremod_trigger;
	int _n_nother_trigger;
	int _n_tokens;

	int _n_nhead;
	int _n_vhead;
	int _n_npremod;
	int _n_nother;
	typedef Symbol::HashMap<int> SymbolToIntMap;

	SymbolToIntMap _other_triggers;
	SymbolToIntMap _head_triggers;
*/	
	bool isLikelyTrigger(const SynNode* aNode);
	bool isConfidentP1Tag(int tag);
	void selectModels(bool& use_sub, bool& use_obj, bool& use_only_confident_P1);
	enum { COUNT_ALL, COUNT_BY_ARG, COUNT_PM, COUNT_KNOWN, COUNT_KNOWN_CORRECT };
	enum {CORRECT, MISSING, SPURIOUS, WRONG_TYPE};
	int _scoringCounts[4][5];
	void updateRRCounts(int correct, int ans, int nEventArgs);
	
};


#endif
