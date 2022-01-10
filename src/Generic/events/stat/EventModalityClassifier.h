// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_MODALITY_CLASSIFIER_H
#define EVENT_MODALITY_CLASSIFIER_H

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
class EventModalityObservation;
class DTTagSet;
class DTFeatureTypeSet;
class EventMentionSet;
class P1Decoder;
class ETProbModelSet;


class EventModalityClassifier {
public:
	EventModalityClassifier(int mode_);
	~EventModalityClassifier();

	int processSentence(const TokenSequence *tokens, const Parse *parse, MentionSet *mentionSet, 
		const PropositionSet *propSet, EventMention **potentials, int max);
	void train(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences);
	void train_2layer(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences);
	void replaceModel(char *model_file);
	
	void decode(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences, UTF8OutputStream& htmlStream);


	//void decode(TokenSequence **tokenSequences, Parse **parses, 
	//	ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
	//	EventMentionSet **esets, int nsentences,
	//							UTF8OutputStream& resultStream,
	//							UTF8OutputStream& keyStream,
	//							UTF8OutputStream& htmlStream);

	//void selectAnnotation(Symbol *docIds, Symbol *docTopics, TokenSequence **tokenSequences, 
	//	Parse **parses, ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
	//	PropositionSet **propSets, EventMentionSet **esets, int nsentences, 
	//	UTF8OutputStream& annotationStream);

	void resetRoundRobinStatistics();
	void printRoundRobinStatistics(UTF8OutputStream &out);
	//void setDocumentTopic(Symbol topic) { _documentTopic = topic; }

	void devTest(TokenSequence **tokenSequences, Parse **parses, 
				ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
				PropositionSet **propSets, EventMentionSet **esets, 
				int nsentences); 

	void setModality(const TokenSequence *tseq, Parse *parse, 
				ValueMentionSet *valueMentionSet, MentionSet *mentionSet, 
				PropositionSet *propSet, EventMention *em);

	enum { TRAIN, DECODE, ADD_FEATURES, DEVTEST, TRAIN_TWO_LAYER, DEVTEST_TWO_LAYER, DECODE_TWO_LAYER};
private:
	int MODE;
	
	bool DEBUG;
	UTF8OutputStream _debugStream;
	
	// add for devTest 2008-01-22
	UTF8OutputStream _devTestStream, _keyStream, _htmlStream;

	EventModalityObservation *_observation;
	MaxEntModel *_maxentModel;
	MaxEntModel *_maxentModel_layer1;
	MaxEntModel *_maxentModel_layer2;

	P1Decoder *_p1Model;
	P1Decoder *_p1Model_layer1;
	P1Decoder *_p1Model_layer2;

	DTFeature::FeatureWeightMap *_p1Weights;
	DTFeature::FeatureWeightMap *_p1Weights_layer1;
	DTFeature::FeatureWeightMap *_p1Weights_layer2;

	DTFeature::FeatureWeightMap *_maxentWeights;
	DTFeature::FeatureWeightMap *_maxentWeights_layer1;
	DTFeature::FeatureWeightMap *_maxentWeights_layer2;

	double _recall_threshold;
	float _p1_overgen_percentage;
	bool _seed_features;

	ETProbModelSet *_probModelSet;
	
	int processToken(int tok, const TokenSequence *tokens, const Parse *parse, MentionSet *mentionSet, 
					const PropositionSet *propSet, EventMention **potentials, int max);

	int processToken_2layer(int tok, const TokenSequence *tokens, const Parse *parse, MentionSet *mentionSet, 
					const PropositionSet *propSet, EventMention **potentials, int max);

	float setScore(EventMention *eventMention);

	void decodeToP1Distribution();
	void decodeToP1Distribution_2layer();
	int decodeToMaxentDistribution();
	int decodeToMaxentDistribution_2layer();
	void printP1DebugScores(UTF8OutputStream& out);
	void printMaxentDebugScores(UTF8OutputStream& out);

	//bool isHighValueAnnotation(int tok, const TokenSequence *tokens, const Parse *parse, 
	//						MentionSet *mentionSet, const PropositionSet *propSet);
	void printAnnotationSentence(UTF8OutputStream& out, Symbol *docIds,
								 TokenSequence **tokenSequences, int index, int n_sentences);
	
	bool _use_maxent_model;
	bool _use_p1_model;

	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	DTFeatureTypeSet *_featureTypes_layer1;
	DTFeatureTypeSet *_featureTypes_layer2;

	double *_tagScores;
	bool *_tagFound;

	Symbol _documentTopic;

	void walkThroughTrainingSentence(int mode, int epoch, 
		const TokenSequence *tokenSequence, const Parse *parse, 
		const ValueMentionSet *valueMentionSet, MentionSet *mentionSet, 
		const PropositionSet *propSet, const EventMentionSet *eset);

	void walkThroughTrainingSentence_twoLayer(int mode, int epoch, 
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

	static const int MAX_MODALITIES = 10;
};


#endif
