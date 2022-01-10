// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/parse/es_NodeInfo.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Spanish/parse/es_STags.h"
#include "Generic/common/WordConstants.h"


//add NOUN_PROP b/c  getNPlabel() returns NOUN_PROP
bool SpanishNodeInfo::isReferenceCandidate(const SynNode *node) {
	Symbol tag = node->getTag();

	/*// NOMINAL PREMODS
	if (isNominalMod(node)) {
		return true;
		}*/	
	return (
			// nominal group:
			(tag == SpanishSTags::GRUP_NOM) 
			// pronoun (demonstrative, exclamative, indef, num, pers, rel, interrog, possessive)
			|| (tag.isInSymbolGroup(SpanishSTags::POS_P_GROUP)) 
			 // pronominal morpheme
			|| (tag == SpanishSTags::MORFEMA_PRONOMINAL)
			// possessive determiner:
			|| (tag == SpanishSTags::POS_DP)
			// Noun phrases (for extent):
			|| (tag == SpanishSTags::SN)
			|| (tag == SpanishSTags::SNP)
			|| (tag == SpanishSTags::POS_VMI)
			);
}


bool SpanishNodeInfo::isOfNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == SpanishSTags::SN ||
			tag == SpanishSTags::GRUP_NOM ||
			tag == SpanishSTags::SNP);
}

bool SpanishNodeInfo::isOfHobbsNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == SpanishSTags::SN ||
			tag == SpanishSTags::SNP ||
			tag == SpanishSTags::GRUP_NOM);
}

bool SpanishNodeInfo::isOfHobbsSKind(const SynNode *node) {
	Symbol tag = node->getTag();
	// not sure how this should be defined:
	return (tag == SpanishSTags::SENTENCE      // top-level sentence
			|| tag == SpanishSTags::S          // sentence
			|| tag == SpanishSTags::S_A        // adjectival phrase as noun complement
			|| tag == SpanishSTags::S_F_A      // adverbial comparitive clause
			|| tag == SpanishSTags::S_F_C      // completive clause
			|| tag == SpanishSTags::S_F_R      // relative clause
			|| tag == SpanishSTags::S_NF_P     // participle
			|| tag == SpanishSTags::S_DP_SBJ
			|| tag == SpanishSTags::S_F_A_DP_SBJ
			|| tag == SpanishSTags::S_F_C_DP_SBJ
			|| tag == SpanishSTags::S_F_R_DP_SBJ
			);
}

bool SpanishNodeInfo::canBeNPHeadPreterm(const SynNode *node) {
	Symbol tag = node->getTag();
	if (tag.isInSymbolGroup(SpanishSTags::POS_N_GROUP))
		return true;
	/*
	if (tag.isInSymbolGroup(SpanishSTags::POS_A_GROUP)) {
		Symbol headword = node->getHeadWord();
		return isValidAdjectiveNomPremod(node->getHeadWord());
	}
	*/
	return false;
}
