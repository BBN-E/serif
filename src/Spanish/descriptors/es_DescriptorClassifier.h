// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_DESCRIPTOR_CLASSIFIER_H
#define ES_DESCRIPTOR_CLASSIFIER_H

#include "Generic/theories/EntityType.h"
#include "Generic/descriptors/DescriptorClassifier.h"

class SynNode;
class MentionSet;
class P1DescriptorClassifier;

// This class is really nothing but a wrapper that delegates to
// a P1DescriptorClassifier.
class SpanishDescriptorClassifier : public DescriptorClassifier {
private:
	friend class SpanishDescriptorClassifierFactory;

public:
	~SpanishDescriptorClassifier();

private:
	SpanishDescriptorClassifier();
	virtual int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults);

	P1DescriptorClassifier *_p1DescriptorClassifier;
	enum { P1 };
	int _classifier_type;
};

class SpanishDescriptorClassifierFactory: public DescriptorClassifier::Factory {
	virtual DescriptorClassifier *build() { return _new SpanishDescriptorClassifier(); }
};




#endif
