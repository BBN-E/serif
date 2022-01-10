#ifndef CH_DESCRIPTOR_CLASSIFIER_H
#define CH_DESCRIPTOR_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/EntityType.h"
#include "Generic/descriptors/DescriptorClassifier.h"

class SynNode;
class MentionSet;
class ChinesePMDescriptorClassifier;
class P1DescriptorClassifier;

class ChineseDescriptorClassifier : public DescriptorClassifier {
private:
	friend class ChineseDescriptorClassifierFactory;

public:
	~ChineseDescriptorClassifier();

private:
	ChineseDescriptorClassifier();

	// the actual probability work do-er
	// returns the number of possibilities selected
	virtual int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults);

	ChinesePMDescriptorClassifier *_pmDescriptorClassifier;
	P1DescriptorClassifier *_p1DescriptorClassifier;
	enum { PM, P1 };
	int _classifier_type;
};

class ChineseDescriptorClassifierFactory: public DescriptorClassifier::Factory {
	virtual DescriptorClassifier *build() { return _new ChineseDescriptorClassifier(); }
};



#endif
