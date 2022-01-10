#ifndef AR_DESCRIPTOR_CLASSIFIER_H
#define AR_DESCRIPTOR_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/EntityType.h"
#include "Generic/descriptors/DescriptorClassifier.h"

class SynNode;
class MentionSet;
class ArabicPMDescriptorClassifier;
class P1DescriptorClassifier;

class ArabicDescriptorClassifier : public DescriptorClassifier {
private:
	friend class ArabicDescriptorClassifierFactory;

public:
	~ArabicDescriptorClassifier();

private:
	ArabicDescriptorClassifier();
	// the actual probability work do-er
	// returns the number of possibilities selected
	virtual int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults);

	ArabicPMDescriptorClassifier *_pmDescriptorClassifier;
	P1DescriptorClassifier *_p1DescriptorClassifier;
	enum { PM, P1 };
	int _classifier_type;
};

class ArabicDescriptorClassifierFactory: public DescriptorClassifier::Factory {
	virtual DescriptorClassifier *build() { return _new ArabicDescriptorClassifier(); }
};




#endif
