// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Korean/parse/kr_NodeInfo.h"
#include "theories/SynNode.h"
#include "common/Symbol.h"
#include "Korean/parse/kr_STags.h"
#include "common/WordConstants.h"


bool KoreanNodeInfo::isReferenceCandidate(const SynNode *node) {
	Symbol tag = node->getTag();
	return ((tag == STags::NPP)||
			(tag == STags::NP)||
			(tag == STags::NPA));
}
