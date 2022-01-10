// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DESCRIPTORCLASSIFIER_H
#define xx_DESCRIPTORCLASSIFIER_H

#include "Generic/descriptors/DescriptorClassifier.h"

class DefaultDescriptorClassifier : public DescriptorClassifier {
private:
	friend class DefaultDescriptorClassifierFactory;
	static void defaultMsg(){
		std::cerr << "<<<<<WARNING: Using unimplemented generic descriptor classifier!>>>>>\n";
	};

public:


	virtual int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults) { return 0; }
private:
	DefaultDescriptorClassifier() {
		defaultMsg();	
	}

};

class DefaultDescriptorClassifierFactory: public DescriptorClassifier::Factory {
	virtual DescriptorClassifier *build() { return _new DefaultDescriptorClassifier(); }
};


#endif
