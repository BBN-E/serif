// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_DESCRIPTORCLASSIFIERTRAINER_H
#define en_DESCRIPTORCLASSIFIERTRAINER_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/DescriptorClassifierTrainer.h"


class EnglishDescriptorClassifierTrainer : public DescriptorClassifierTrainer {
private:
	friend class EnglishDescriptorClassifierTrainerFactory;

public:
	~EnglishDescriptorClassifierTrainer();
	
	// a separate call to this function is required for writing purposes. This separation allows us
	// to train on multiple files before writing the models to disk.
	virtual void writeModels();

private:
	EnglishDescriptorClassifierTrainer();

	virtual void addMention(MentionSet *mentionSet, Mention *mention, Symbol type);
	
	void openOutputFiles(const char* model_prefix);
	void closeOutputFiles();

	// the functional parent is the closest ancestor of node 
	// whose head is not node. This was stolen from EnglishDescriptorClassifier
	const SynNode* getFunctionalParentNode(const SynNode* node);

	UTF8OutputStream _priorStream, _headStream, _premodStream, _premodBackoffStream, _parentStream;
	ProbModelWriter _priorWriter, _headWriter, _premodWriter, _premodBackoffWriter, _parentWriter;

}; 

class EnglishDescriptorClassifierTrainerFactory: public DescriptorClassifierTrainer::Factory {
	virtual DescriptorClassifierTrainer *build() { return _new EnglishDescriptorClassifierTrainer(); }
};


#endif
