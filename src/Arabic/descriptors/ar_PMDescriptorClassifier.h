// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_PMDESCRIPTOR_CLASSIFIER_H
#define ar_PMDESCRIPTOR_CLASSIFIER_H

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/Entity.h"

class ProbModel;
class SynNode;
class DebugStream;
class MentionSet;
class Mention;

class ArabicPMDescriptorClassifier  {
public:
	ArabicPMDescriptorClassifier();
	~ArabicPMDescriptorClassifier();
	// the actual probability work do-er
	// returns the number of possibilities selected
	int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], double scores[], int maxResults);

private:
	static const int _VOCAB_SIZE;
	ProbModel* _headModel;
	ProbModel* _priorModel;
	ProbModel* _postmodModel;
	ProbModel* _postmodBackoffModel;
	ProbModel* _functionalParentModel;
	DebugStream _debug;


	// the functional parent is the closest ancestor of node 
	// whose head is not node
	const SynNode* _getFunctionalParentNode(const SynNode* node);

	void _initialize(const char* model_prefix);


};

#endif
