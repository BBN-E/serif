// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/descriptors/DescriptorClassifierTrainer/es_DescriptorClassifierTrainer.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/Mention.h"

SpanishDescriptorClassifierTrainer::SpanishDescriptorClassifierTrainer() : _priorWriter(2,100),
															 _headWriter(2, 100),
															 _postmodWriter(3, 100),
															 _postmodBackoffWriter(2, 100),
															 _parentWriter(2, 100)
{
	// open output files
	std::string prefix = ParamReader::getRequiredParam("desc_classify_model");
	openOutputFiles(prefix.c_str());

}

SpanishDescriptorClassifierTrainer::~SpanishDescriptorClassifierTrainer() {
	closeOutputFiles();
}

void SpanishDescriptorClassifierTrainer::openOutputFiles(const char* model_prefix)
{
	std::string model_prefix_str(model_prefix);

	std::string file = model_prefix_str + ".mnp1";
	_priorStream.open(file.c_str());

	file = model_prefix_str + ".mnp2";
	_headStream.open(file.c_str());

	file = model_prefix_str + ".mnp3";
	_postmodStream.open(file.c_str());

	file = model_prefix_str + ".mnp4";
	_postmodBackoffStream.open(file.c_str());

	file = model_prefix_str + ".mnp5";
	_parentStream.open(file.c_str());
}

void SpanishDescriptorClassifierTrainer::closeOutputFiles() {
	_priorStream.close();
	_headStream.close();
	_postmodStream.close();
	_postmodBackoffStream.close();
	_parentStream.close();
}

void SpanishDescriptorClassifierTrainer::addMention(MentionSet *mentionSet, Mention *mention, Symbol type) {
	const SynNode *node = mention->getNode();

	Symbol priorArr[2];
	Symbol headArr[2];
	Symbol postmodArr[3];
	Symbol postmodBackArr[2];
	Symbol parentArr[2];
	// Gather EDT type, head word, postmods, and functional parent
	priorArr[0] = SymbolConstants::nullSymbol;
	priorArr[1] = headArr[0]  = postmodArr[0] = postmodBackArr[0] = parentArr[0] = type;
	// the postmods and the head word
	int headIndex = node->getHeadIndex();
	Symbol termToHead[100];
	int termSize = node->getTerminalSymbols(termToHead, 100);
	headArr[1] = postmodArr[1] = node->getHeadWord();

	_priorWriter.registerTransition(priorArr);
	_headWriter.registerTransition(headArr);

	bool found_head = false;
	for (int j = 0; j < termSize; j++) {
		if(termToHead[j] == postmodArr[1]) {
 			found_head = true;
			continue;
		}
		if (!found_head)
			continue;
		postmodArr[2] = postmodBackArr[1] = termToHead[j];
		_postmodWriter.registerTransition(postmodArr);
		_postmodBackoffWriter.registerTransition(postmodBackArr);
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

// this was borrowed from SpanishDescriptorClassifier.
// TODO: have some utility functions area?
const SynNode* SpanishDescriptorClassifierTrainer::getFunctionalParentNode(const SynNode* node) {
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

void SpanishDescriptorClassifierTrainer::writeModels() {
	_priorWriter.writeModel(_priorStream);
	_headWriter.writeModel(_headStream);
	_postmodWriter.writeModel(_postmodStream);
	_postmodBackoffWriter.writeModel(_postmodBackoffStream);
	_parentWriter.writeModel(_parentStream);
}
