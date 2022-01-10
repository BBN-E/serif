// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_DESCRIPTORCLASSIFIERTRAINER_H
#define ch_DESCRIPTORCLASSIFIERTRAINER_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/DescriptorClassifierTrainer.h"


class ChineseDescriptorClassifierTrainer : public DescriptorClassifierTrainer {
private:
	friend class ChineseDescriptorClassifierTrainerFactory;

public:
	~ChineseDescriptorClassifierTrainer();
	
	// a separate call to this function is required for writing purposes. This separation allows us
	// to train on multiple files before writing the models to disk.
	virtual void writeModels();

private:
	ChineseDescriptorClassifierTrainer();

	virtual void addMention(MentionSet *menitonSet, Mention *mention, Symbol type);
	
	void openOutputFiles(const char* model_prefix);
	void closeOutputFiles();

	// the functional parent is the closest ancestor of node 
	// whose head is not node. This was stolen from ChineseDescriptorClassifier
	const SynNode* _getFunctionalParentNode(const SynNode* node);

	// If node is a DNP with DEG head, return the main NP 
	// instead of DEG, otherwise return the head.
	const SynNode* _getSmartHead(const SynNode* node);

	// Given a premod node, return the relevant head word 
	Symbol _getPremodWord(MentionSet *mentionSet, const SynNode* node);
	
	// Returns the head word of the functional parent node
	Symbol _getFunctionalParentWord(MentionSet *mentionSet, const SynNode* node);

	UTF8OutputStream _priorStream, _headStream, _premodStream, 
					 _premodBackoffStream, _parentStream, _lastCharStream;
	ProbModelWriter _priorWriter, _headWriter, _premodWriter, 
					_premodBackoffWriter, _parentWriter, _lastCharWriter;

}; 

class ChineseDescriptorClassifierTrainerFactory: public DescriptorClassifierTrainer::Factory {
	virtual DescriptorClassifierTrainer *build() { return _new ChineseDescriptorClassifierTrainer(); }
};


#endif
