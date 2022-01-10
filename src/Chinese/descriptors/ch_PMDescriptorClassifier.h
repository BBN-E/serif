// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_DESCRIPTOR_CLASSIFIER_H
#define ch_DESCRIPTOR_CLASSIFIER_H

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/Entity.h"

class ProbModel;
class SynNode;
class DebugStream;
class MentionSet;
class Mention;

class ChinesePMDescriptorClassifier : public DescriptorClassifier {
public:
	ChinesePMDescriptorClassifier();
	~ChinesePMDescriptorClassifier();

	// the actual probability work do-er
	// returns the number of possibilities selected
	int classifyDescriptor(MentionSet *mentionSet, const SynNode* node, EntityType types[], double scores[], int maxResults);
private:
	static const int _VOCAB_SIZE;
	ProbModel* _headModel;
	ProbModel* _priorModel;
	ProbModel* _premodModel;
	ProbModel* _premodBackoffModel;
	ProbModel* _functionalParentModel;
	ProbModel* _lastCharModel;
	DebugStream _debug;

	// the functional parent is the closest ancestor of node 
	// whose smart head is not node
	const SynNode* _getFunctionalParentNode(const SynNode* node);
	
	// If node is a DNP with DEG head, return the main NP 
	// instead of DEG, otherwise return the head.
	const SynNode* _getSmartHead(const SynNode* node);
	
	// Given a premod node, return the relevant head word 
	Symbol _getPremodWord(MentionSet *mentionSet, const SynNode* node);

	// Returns the head word of the functional parent node
	Symbol _getFunctionalParentWord(MentionSet *mentionSet, const SynNode* node);

	void _initialize(const char* model_prefix);
	void verifyEntityTypes(const char *file_name);


};

#endif
