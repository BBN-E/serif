#ifndef P1_DESC_CLASSIFIER_H
#define P1_DESC_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/Symbol.h"
#include "Generic/theories/EntityType.h"

class SynNode;
class MentionSet;
class Mention;
class DTTagSet;
class DTFeatureTypeSet;
class P1Decoder;
class DescriptorObservation;
class PropositionFinder;
class PropositionSet;
class MaxEntModel;

#include "Generic/discTagger/DTFeature.h"


class P1DescriptorClassifier {
public:
	typedef enum {DESC_CLASSIFY, PREMOD_CLASSIFY, PRONOUN_CLASSIFY} Task;

	P1DescriptorClassifier(Task task = DESC_CLASSIFY);
	~P1DescriptorClassifier();

	// main function for this class
	// returns the number of possibilities selected
	int classifyDescriptor(
		MentionSet *currSolution, const SynNode* node, EntityType types[], 
		double scores[], int maxResults);

	EntityType classifyDescriptor(MentionSet *currSolution,
								  const SynNode *node);

	void cleanup();
private:
	Task _task;
	P1Decoder *_decoder;	
	DTTagSet *_tagSet;
	DTFeature::FeatureWeightMap *_weights;
	DTFeatureTypeSet *_featureTypes;
	DescriptorObservation *_observation;
	PropositionFinder *_propFinder;
	PropositionSet *_propSet;
	MaxEntModel *_maxEntDecoder;
	DTFeature::FeatureWeightMap *_maxEntWeights;
	
	//this is a hack to allow using the same proposition
	//set for different mentions in the same sentence
	//Really we should find propositions in the DescriptorRecognizer
	//and pass them through to the classifiers, but that would be a very big restructure
	const SynNode* _currRoot;
	void updatePropositionSet(MentionSet *currSolution);
	SynNode* copySynNode(const SynNode* orig);

};

#endif
