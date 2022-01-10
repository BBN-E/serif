// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_DESCRIPTORCLASSIFIERTRAINER_H
#define ar_DESCRIPTORCLASSIFIERTRAINER_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/DescriptorClassifierTrainer.h"


class ArabicDescriptorClassifierTrainer : public DescriptorClassifierTrainer {
private:
	friend class ArabicDescriptorClassifierTrainerFactory;

public:
	
	// a separate call to this function is required for writing purposes. This separation allows us
	// to train on multiple files before writing the models to disk.
	virtual void writeModels();

	~ArabicDescriptorClassifierTrainer();

private:
	ArabicDescriptorClassifierTrainer();

	virtual void addMention(MentionSet *mentionSet, Mention *mention, Symbol type);
	
	void openOutputFiles(const char* model_prefix);
	void closeOutputFiles();

	// the functional parent is the closest ancestor of node 
	// whose head is not node. This was stolen from ArabicDescriptorClassifier
	const SynNode* getFunctionalParentNode(const SynNode* node);

	UTF8OutputStream _priorStream, _headStream, _postmodStream, _postmodBackoffStream, _parentStream;
	ProbModelWriter _priorWriter, _headWriter, _postmodWriter, _postmodBackoffWriter, _parentWriter;

}; 

class ArabicDescriptorClassifierTrainerFactory: public DescriptorClassifierTrainer::Factory {
	virtual DescriptorClassifierTrainer *build() { return _new ArabicDescriptorClassifierTrainer(); }
};


#endif
