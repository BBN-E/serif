// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/descriptors/DescriptorClassifierTrainer/ch_DescriptorClassifierTrainer.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/Mention.h"

ChineseDescriptorClassifierTrainer::ChineseDescriptorClassifierTrainer() : _priorWriter(2,100),
															 _headWriter(2, 100),
															 _premodWriter(3, 100),
															 _premodBackoffWriter(2, 100),
															 _parentWriter(2, 100),
															 _lastCharWriter(2, 100)
{
	// open output files
	std::string prefix = ParamReader::getRequiredParam("desc_classify_model");
	openOutputFiles(prefix.c_str());

}

ChineseDescriptorClassifierTrainer::~ChineseDescriptorClassifierTrainer() {
	closeOutputFiles();
}

void ChineseDescriptorClassifierTrainer::openOutputFiles(const char* model_prefix)
{
	std::string model_prefix_str(model_prefix);

	std::string file = model_prefix_str + ".mnp1";
	_priorStream.open(file.c_str());

	file = model_prefix_str + ".mnp2";
	_headStream.open(file.c_str());

	file = model_prefix_str + ".mnp3";
	_premodStream.open(file.c_str());

	file = model_prefix_str + ".mnp4";
	_premodBackoffStream.open(file.c_str());

	file = model_prefix_str + ".mnp5";
	_parentStream.open(file.c_str());
	
	file = model_prefix_str + ".mnp6";
	_lastCharStream.open(file.c_str());
}

void ChineseDescriptorClassifierTrainer::closeOutputFiles() {
	_priorStream.close();
	_headStream.close();
	_premodStream.close();
	_premodBackoffStream.close();
	_parentStream.close();
	_lastCharStream.close();
}

void ChineseDescriptorClassifierTrainer::addMention(MentionSet *mentionSet, Mention *mention, Symbol type) {
	const SynNode *node = mention->getNode();

	Symbol priorArr[2];
	Symbol headArr[2];
	Symbol premodArr[3];
	Symbol premodBackArr[2];
	Symbol parentArr[2];
	Symbol lastCharArr[2];
	// Gather EDT type, head word, premods, and functional parent
	priorArr[0] = SymbolConstants::nullSymbol;
	priorArr[1] = headArr[0]  = premodArr[0] = premodBackArr[0] = parentArr[0] = lastCharArr[0] = type;
	// the premods and the head word
	int headIndex = node->getHeadIndex();
	headArr[1] = premodArr[1] = node->getHeadWord();

	_priorWriter.registerTransition(priorArr);
	_headWriter.registerTransition(headArr);

	for (int j = 0; j < node->getHeadIndex(); j++) {
		//premodArr[2] = premodBackArr[1] = _getSmartHead(node->getChild(j))->getHeadWord();
		premodArr[2] = premodBackArr[1] = _getPremodWord(mentionSet, node->getChild(j));
		_premodWriter.registerTransition(premodArr);
		_premodBackoffWriter.registerTransition(premodBackArr);
	}

	// if the current node is the head of a partitive, get the functional parent
	// of the entire partitive. Otherwise, just get the functional parent of the
	// node.
	const Mention* parentMent = mention->getParent();
	/*const SynNode* funcParent = 0;
	if (parentMent != 0 && parentMent->mentionType == Mention::PART)
		funcParent = _getFunctionalParentNode(parentMent->getNode());
	else
		funcParent = _getFunctionalParentNode(node); */


	//parentArr[1] = funcParent ? funcParent->getHeadWord() : SymbolConstants::nullSymbol;
	parentArr[1] = _getFunctionalParentWord(mentionSet, node);
	_parentWriter.registerTransition(parentArr);

	// find the last head word character
	const wchar_t* headString = node->getHeadWord().to_string();
	lastCharArr[1] = Symbol(headString+wcslen(headString)-1);
	_lastCharWriter.registerTransition(lastCharArr);

}

// this was borrowed from ChineseDescriptorClassifier.
// TODO: have some utility functions area?
const SynNode* ChineseDescriptorClassifierTrainer::_getFunctionalParentNode(const SynNode* node) {
	if( node == 0 )
		return 0;
	const SynNode* nd = node;
	while( nd != 0 &&
		nd->getParent() != 0 &&
		_getSmartHead(nd->getParent()) == nd )
		nd = nd->getParent();
	if( nd != 0 && nd->getParent() != 0 )
		return nd->getParent();
	return 0; // otherwise we can't use it anyway
}

const SynNode* ChineseDescriptorClassifierTrainer::_getSmartHead(const SynNode* node) {
	if (node->getTag() == ChineseSTags::DNP &&
		node->getHead()->getTag() == ChineseSTags::DEG)
	{
		if (node->getHeadIndex() > 0)
			return node->getChild(node->getHeadIndex() - 1);
	}
	return node->getHead();
}

Symbol ChineseDescriptorClassifierTrainer::_getPremodWord(MentionSet *mentionSet, const SynNode* node) {
	if (node->hasMention()) {
		Mention *ment = mentionSet->getMentionByNode(node);
		if (ment->getMentionType() == Mention::NAME)
			return ment->getEntityType().getName();
	}
	return _getSmartHead(node)->getHeadWord();
}

Symbol ChineseDescriptorClassifierTrainer::_getFunctionalParentWord(MentionSet *mentionSet, const SynNode* node) {
	const SynNode* parent = _getFunctionalParentNode(node);
	if (parent == 0)
		return SymbolConstants::nullSymbol;
	else {
		const SynNode *head = parent->getHead();
		while (!head->isPreterminal()) {
			if (head->hasMention() && mentionSet->getMentionByNode(head)->getMentionType() == Mention::NAME)
				return mentionSet->getMentionByNode(head)->getEntityType().getName();
			head = head->getHead();
		}
		return head->getHeadWord();
	}
}


void ChineseDescriptorClassifierTrainer::writeModels() {
	_priorWriter.writeModel(_priorStream);
	_headWriter.writeModel(_headStream);
	_premodWriter.writeModel(_premodStream);
	_premodBackoffWriter.writeModel(_premodBackoffStream);
	_parentWriter.writeModel(_parentStream);
	_lastCharWriter.writeModel(_lastCharStream);
}
