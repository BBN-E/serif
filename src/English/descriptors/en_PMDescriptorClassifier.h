#ifndef EN_PMDESCRIPTOR_CLASSIFIER_H
#define EN_PMDESCRIPTOR_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"

class ProbModel;
class SynNode;
class DebugStream;
class MentionSet;
class Mention;


class EnglishPMDescriptorClassifier {
public:
	EnglishPMDescriptorClassifier();
	~EnglishPMDescriptorClassifier();

	// the actual probability work do-er
	// returns the number of possibilities selected
	int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults);

private:
	static const int _VOCAB_SIZE;
	ProbModel* _headModel;
	ProbModel* _priorModel;
	ProbModel* _premodModel;
	ProbModel* _premodBackoffModel;
	ProbModel* _functionalParentModel;

	DebugStream _debug;

	// the functional parent is the closest ancestor of node 
	// whose head is not node
	const SynNode* getFunctionalParentNode(const SynNode* node);

	void initialize(const char* model_prefix);
	void verifyEntityTypes(char *file_name);
};



#endif
