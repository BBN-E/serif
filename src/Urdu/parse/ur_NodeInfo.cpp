// Copyright 2013 Raytheon BBN Technologies 
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Urdu/parse/ur_NodeInfo.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Urdu/parse/ur_STags.h"
#include "Generic/common/WordConstants.h"


//add NOUN_PROP b/c  getNPlabel() returns NOUN_PROP
bool UrduNodeInfo::isReferenceCandidate(const SynNode *node) {
	Symbol tag = node->getTag();

	/*// NOMINAL PREMODS
	if (isNominalMod(node)) {
		return true;
		}*/	
	return (
			// nominal group:
			(tag == UrduSTags::GRUP_NOM) 
			// pronoun (demonstrative, exclamative, indef, num, pers, rel, interrog, possessive)
			|| (tag.isInSymbolGroup(UrduSTags::POS_P_GROUP)) 
			// possessive determiner:
			|| (tag == UrduSTags::POS_DP)
			// Noun phrases (for extent):
			|| (tag == UrduSTags::SN)
			|| (tag == UrduSTags::SNP)
			);
}


bool UrduNodeInfo::isOfNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == UrduSTags::SN ||
			tag == UrduSTags::GRUP_NOM ||
			tag == UrduSTags::SNP);
}

bool UrduNodeInfo::isOfHobbsNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == UrduSTags::SN ||
			tag == UrduSTags::SNP ||
			tag == UrduSTags::GRUP_NOM);
}

bool UrduNodeInfo::isOfHobbsSKind(const SynNode *node) {
	Symbol tag = node->getTag();
	// not sure how this should be defined:
	return (tag == UrduSTags::SENTENCE      // top-level sentence
			|| tag == UrduSTags::S          // sentence
			|| tag == UrduSTags::S_A        // adjectival phrase as noun complement
			|| tag == UrduSTags::S_F_A      // adverbial comparitive clause
			|| tag == UrduSTags::S_F_C      // completive clause
			|| tag == UrduSTags::S_F_R      // relative clause
			|| tag == UrduSTags::S_NF_P     // participle
			);
}

bool UrduNodeInfo::canBeNPHeadPreterm(const SynNode *node) {
	Symbol tag = node->getTag();
	if (tag.isInSymbolGroup(UrduSTags::POS_N_GROUP))
		return true;
	/*
	if (tag.isInSymbolGroup(UrduSTags::POS_A_GROUP)) {
		Symbol headword = node->getHeadWord();
		return isValidAdjectiveNomPremod(node->getHeadWord());
	}
	*/
	return false;
}
