// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ar_NodeInfo.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/common/WordConstants.h"


//add NOUN_PROP b/c  getNPlabel() returns NOUN_PROP
bool ArabicNodeInfo::isReferenceCandidate(const SynNode *node) {
	Symbol tag = node->getTag();
	// NOMINAL PREMODS
	if (isNominalMod(node)) {
		return true;
	}	
	return ((tag == ArabicSTags::NPP)||
			(tag == ArabicSTags::NP)||
			(tag == ArabicSTags::NPA) || 
			(ArabicSTags::isPronoun(tag)) /*||
			(tag == ArabicSTags::NOUN_PROP)*/);
}
bool ArabicNodeInfo::isOfNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == ArabicSTags::NP ||
			tag == ArabicSTags::NPP ||
			tag == ArabicSTags::NPA);
}
bool ArabicNodeInfo::canBeNPHeadPreterm(const SynNode *node) {
	Symbol tag = node->getTag();
	if(node->isPreterminal()){
		if(WordConstants::isPronoun(node->getHead()->getTag())){
			return true;
		}
	}
	if(ArabicSTags::isNoun(tag)) return true;
	if(ArabicSTags::isAdjective(tag)) return true;
	return false;
}
bool ArabicNodeInfo::isNominalMod(const SynNode *node){
		return (canBeNPHeadPreterm(node) &&
		node->getParent() != 0 &&
		node->getParent()->getTag() != ArabicSTags::NPP &&
		node->getParent()->getHead() != node);
}
