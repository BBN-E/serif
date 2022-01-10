// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/MergeHistory.h"
#include "Generic/edt/MentionGroups/MentionGroupMerger.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"

MergeHistory_ptr MergeHistory::create(const Mention *mention) { 
	return boost::shared_ptr<MergeHistoryLeaf>(new MergeHistoryLeaf(mention)); 
}

MergeHistory_ptr MergeHistory::create(MergeHistory_ptr left, MergeHistory_ptr right,
									  MentionGroupMerger* merger, double score) 
{
	return boost::shared_ptr<MergeHistoryNode>(new MergeHistoryNode(left, right, merger, score));
}

std::wstring MergeHistoryNode::toString(const std::wstring prefix, 
										wchar_t left, wchar_t right) const {
	std::wstringstream result;
	result << _leftChild->toString(prefix+left+L"    ", L' ', L'|')
		   << prefix << left << L"    |\n"
		   << prefix << L"+--[" 
		   << _merger->getName().to_string() << L" " 
		   << _merge_score << L"]" << L"\n"
		   << prefix << right << L"    |\n"
		   << _rightChild->toString(prefix+right+L"    ", L'|', L' ');
	return result.str();
}

std::wstring MergeHistoryLeaf::toString(const std::wstring prefix, 
										wchar_t left, wchar_t right) const {
	std::wstringstream result;
	result << prefix << L"+-- ";

	result << "(" << _mention->getTypeString(_mention->mentionType)
		   << " mention " << _mention->getUID().toInt() << L": "
		   << _mention->getEntityType().getName() << ")";
	// Show extent, and mark head with "{{head}}"
	int ext_s = _mention->node->getStartToken();
	int ext_e = _mention->node->getEndToken();
	int head_s = _mention->getHead()->getStartToken();
	int head_e = _mention->getHead()->getEndToken();
	const MentionSet* mentionSet = _mention->getMentionSet();
	const TokenSequence* tokenSequence = mentionSet->getParse()->getTokenSequence();
	for (int i=ext_s; i<=ext_e; ++i) {
		const Token *tok = tokenSequence->getToken(i);
		result << L" ";
		if (i == head_s)
			result << L"{{"; // beginning of head
		result << tok->getSymbol();
		if (i == head_e)
			result << L"}}"; // end of head
	}
	result << "\n";
	return result.str();
}
