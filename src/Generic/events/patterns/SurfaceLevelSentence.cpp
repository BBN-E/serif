// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/patterns/SurfaceLevelSentence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/MentionSet.h"

SurfaceLevelSentence::SurfaceLevelSentence(const SynNode *node,
										   MentionSet *mentionSet) 
{
	_length = 0;
	_mentionSet = mentionSet;
	addToSentence(node);
}

void SurfaceLevelSentence::addToSentence(const SynNode *node) {
	Mention *mention = _mentionSet->getMentionByNode(node);
	if (mention != 0 &&
		mention->getEntityType().isRecognized())
	{
		// preterm or one above preterm w/ no postmods
		if (node->isPreterminal() ||
			(node->getHead()->isPreterminal() &&
			 node->getHeadIndex() == node->getNChildren() - 1) ||
			(node->getHead()->isPreterminal() &&
			 node->getChild(node->getNChildren() - 1)->getTag() == Symbol(L"CD")))
		{
			_surfaceSentence[_length].node = node;
			_surfaceSentence[_length].mention = mention;
			_surfaceSentence[_length].token = Symbol();
			_surfaceSentence[_length].token_index = -1;
			_length++;
			return;
		}
	}
	if (node->isPreterminal()) {
		if (node->getTag() == Symbol(L",") ||
			node->getTag() == Symbol(L"-") ||
			node->getTag() == Symbol(L"--") ||
			node->getTag() == Symbol(L"''") ||
			node->getTag() == Symbol(L"``"))
			return;
		if (node->getHeadWord() == Symbol(L"the"))
			return;
		_surfaceSentence[_length].node = node;
		_surfaceSentence[_length].mention = 0;
		_surfaceSentence[_length].token = node->getHeadWord();
		_surfaceSentence[_length].token_index = node->getStartToken();
		_length++;
		return;
	} 	
	for (int i = 0; i < node->getNChildren(); i++) {
		addToSentence(node->getChild(i));
	}
}

std::string SurfaceLevelSentence::toDebugString() const {
	std::string str = "";
	for (int i = 0; i < _length; i++) {
		str += "[";
		if (!_surfaceSentence[i].token.is_null()) {
			str += _surfaceSentence[i].token.to_debug_string();
		} else {
			str += _surfaceSentence[i].mention->getNode()->toDebugTextString();
		}
		str += "] ";
	}
	return str;
}

std::wstring SurfaceLevelSentence::toString() const {
	std::wstring str = L"";
	for (int i = 0; i < _length; i++) {
		if (!_surfaceSentence[i].token.is_null()) {
			str += _surfaceSentence[i].token.to_string();
		} else {
			str += L"[";
			str += _surfaceSentence[i].mention->getNode()->toTextString();
			str += L"]";
		}
		str += L" ";
	}
	return str;
}
