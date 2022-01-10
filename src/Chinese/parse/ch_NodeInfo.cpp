// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Chinese/parse/ch_NodeInfo.h"

#include "common/Symbol.h"
#include "theories/SynNode.h"
#include "Chinese/parse/ch_STags.h"

bool ChineseNodeInfo::isReferenceCandidate(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == ChineseSTags::NP ||
			tag == ChineseSTags::NPA ||
			tag == ChineseSTags::NPP ||
			tag == ChineseSTags::DATE);
}

bool ChineseNodeInfo::isOfNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == ChineseSTags::NP ||
			tag == ChineseSTags::NPP ||
			tag == ChineseSTags::NPA);
}

bool ChineseNodeInfo::isOfHobbsNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == ChineseSTags::NP ||
			tag == ChineseSTags::NPP ||
			tag == ChineseSTags::NPA);
}

bool ChineseNodeInfo::isOfHobbsSKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == ChineseSTags::CP ||
			tag == ChineseSTags::IP);
}
