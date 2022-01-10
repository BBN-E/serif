// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/descriptors/DescriptorClassifierTrainer/en_DescriptorClassifierTrainer.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/Mention.h"

EnglishDescriptorClassifierTrainer::EnglishDescriptorClassifierTrainer() : _priorWriter(2,100),
															 _headWriter(2, 100),
															 _premodWriter(3, 100),
															 _premodBackoffWriter(2, 100),
															 _parentWriter(2, 100)
{
	// open output files
	std::string prefix = ParamReader::getRequiredParam("desc_classify_model");
	openOutputFiles(prefix.c_str());
}

EnglishDescriptorClassifierTrainer::~EnglishDescriptorClassifierTrainer() {
	closeOutputFiles();
}

void EnglishDescriptorClassifierTrainer::openOutputFiles(const char* model_prefix)
{
	char file[501];

	sprintf(file,"%s.mnp1", model_prefix);
	_priorStream.open(file);

	sprintf(file,"%s.mnp2", model_prefix);
	_headStream.open(file);

	sprintf(file,"%s.mnp3", model_prefix);
	_premodStream.open(file);

	sprintf(file,"%s.mnp4", model_prefix);
	_premodBackoffStream.open(file);

	sprintf(file,"%s.mnp5", model_prefix);
	_parentStream.open(file);
}

void EnglishDescriptorClassifierTrainer::closeOutputFiles() {
	_priorStream.close();
	_headStream.close();
	_premodStream.close();
	_premodBackoffStream.close();
	_parentStream.close();
}

void EnglishDescriptorClassifierTrainer::addMention(MentionSet *mentionSet, Mention *mention, Symbol type) {
	const SynNode *node = mention->getNode();

	Symbol priorArr[2];
	Symbol headArr[2];
	Symbol premodArr[3];
	Symbol premodBackArr[2];
	Symbol parentArr[2];
	// Gather EDT type, head word, premods, and functional parent
	priorArr[0] = SymbolConstants::nullSymbol;
	priorArr[1] = headArr[0]  = premodArr[0] = premodBackArr[0] = parentArr[0] = type;
	// the premods and the head word
	int headIndex = node->getHeadIndex();
	Symbol termToHead[100];
	int termSize = node->getTerminalSymbols(termToHead, 100);
	headArr[1] = premodArr[1] = node->getHeadWord();

	_priorWriter.registerTransition(priorArr);
	_headWriter.registerTransition(headArr);

	for (int j = 0; j < termSize; j++) {
		if(termToHead[j]==premodArr[1])
			break;
		premodArr[2] = premodBackArr[1] = termToHead[j];
		_premodWriter.registerTransition(premodArr);
		_premodBackoffWriter.registerTransition(premodBackArr);
	}

	// if the current node is the head of a partitive, get the functional parent
	// of the entire partitive. Otherwise, just get the functional parent of the
	// node.
	const Mention* parentMent = mention->getParent();
	const SynNode* funcParent = 0;
	if (parentMent != 0 && parentMent->mentionType == Mention::PART)
		funcParent = getFunctionalParentNode(parentMent->getNode());
	else
		funcParent = getFunctionalParentNode(node);

	parentArr[1] = funcParent ? funcParent->getHeadWord() : SymbolConstants::nullSymbol;
	_parentWriter.registerTransition(parentArr);

}

// this was borrowed from EnglishDescriptorClassifier.
// TODO: have some utility functions area?
const SynNode* EnglishDescriptorClassifierTrainer::getFunctionalParentNode(const SynNode* node) {
	if( node == 0 )
		return 0;
	const SynNode* nd = node;
	while( nd != 0 &&
		nd->getParent() != 0 &&
		nd->getParent()->getHead() == nd )
		nd = nd->getParent();
	if( nd != 0 && nd->getParent() != 0 )
		return nd->getParent();
	return 0; // otherwise we can't use it anyway
}

void EnglishDescriptorClassifierTrainer::writeModels() {
	_priorWriter.writeModel(_priorStream);
	_headWriter.writeModel(_headStream);
	_premodWriter.writeModel(_premodStream);
	_premodBackoffWriter.writeModel(_premodBackoffStream);
	_parentWriter.writeModel(_parentStream);
}
