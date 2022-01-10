// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DESCRIPTORCLASSIFIERTRAINER_H
#define xx_DESCRIPTORCLASSIFIERTRAINER_H

#include "Generic/descriptors/DescriptorClassifierTrainer/DescriptorClassifierTrainer.h"

class DefaultDescriptorClassifierTrainer : public DescriptorClassifierTrainer {
private:
	friend class DefaultDescriptorClassifierTrainerFactory;

	static void defaultMsg(){
		std::cerr << "<<<<<WARNING: Using unimplemented generic descriptor classifier trainer!>>>>>\n";
	};
public:

	// a separate call to this function is required for writing purposes. This separation allows us
	// to train on multiple files before writing the models to disk.
	virtual void writeModels() { }

private:
	DefaultDescriptorClassifierTrainer() {
		defaultMsg();
	}

	// This function should be implemented by a language-specific derived class, 
	// since the number and type of features and models may vary by language.
	virtual void addMention(MentionSet *mentionSet, Mention *mention, Symbol type) { }

};

class DefaultDescriptorClassifierTrainerFactory: public DescriptorClassifierTrainer::Factory {
	virtual DescriptorClassifierTrainer *build() { return _new DefaultDescriptorClassifierTrainer(); }
};


#endif
