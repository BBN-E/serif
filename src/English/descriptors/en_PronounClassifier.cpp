// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/SynNode.h"
#include "English/parse/en_NodeInfo.h"
#include "English/common/en_WordConstants.h"
#include "English/descriptors/en_PronounClassifier.h"

EnglishPronounClassifier::EnglishPronounClassifier() {
	// figure out which type refers to people
	_personType = EntityType::getOtherType(); // just in case of no match
	for (int i = 0; i < EntityType::getNTypes(); i++) {
		EntityType type = EntityType::getType(i);
		if (type.matchesPER()) {
			_personType = type;
			break;
		}
	}
}

EnglishPronounClassifier::~EnglishPronounClassifier() {}

int EnglishPronounClassifier::classifyMention (MentionSet *currSolution,
	Mention *currMention, MentionSet *results[], int max_results,
	bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching	
	if (isBranching)
		results[0] = _new MentionSet(*currSolution);
	else
		results[0] = currSolution;

	// can't classify if the mention was already classified as another type
	switch (currMention->mentionType) {
		case Mention::PART:
		case Mention::APPO:
		case Mention::LIST:
		case Mention::NAME:
		case Mention::PRON:
		case Mention::DESC:
		case Mention::NEST:
			return 1;
		case Mention::NONE:
			break;
		default:
			throw InternalInconsistencyException("EnglishPronounClassifier::classifyMention()",
				"Unexpected mention type seen");
	}

	const SynNode* node = currMention->node;
    if (!EnglishNodeInfo::isWHQNode(node) && !WordConstants::isPronoun(node->getHeadWord()))
		return 1;
	// the forked mention
	Mention* newMention = results[0]->getMention(currMention->getIndex());
	// to be safe, if we're branching, zero out the former set and mention so
	// we don't accidentally refer to it again
	if (isBranching) {
		currSolution = 0;
		currMention = 0;
	}
	newMention->mentionType = Mention::PRON;
	if ((WordConstants::is1pPronoun(node->getHeadWord()) || WordConstants::is2pPronoun(node->getHeadWord())) &&
		!EnglishNodeInfo::isWHQNode(node))
	{
		//Given document contents, we might not always want 1p/2p pronouns to refer to people
		std::string type_for_1p_pronouns = ParamReader::getParam("entity_type_for_1p_pronouns");
		std::wstring type_for_1p = std::wstring(type_for_1p_pronouns.begin(), type_for_1p_pronouns.end());
		std::string type_for_2p_pronouns = ParamReader::getParam("entity_type_for_2p_pronouns");
		std::wstring type_for_2p = std::wstring(type_for_2p_pronouns.begin(), type_for_2p_pronouns.end());
		if (!type_for_1p.empty() && WordConstants::is1pPronoun(node->getHeadWord()) && 
			EntityType::isValidEntityType(Symbol::Symbol(type_for_1p)))
		{
			newMention->setEntityType(EntityType::EntityType(Symbol::Symbol(type_for_1p)));
		}
		else if (!type_for_2p.empty() && WordConstants::is2pPronoun(node->getHeadWord()) && 
				 EntityType::isValidEntityType(Symbol::Symbol(type_for_2p)))
		{
			newMention->setEntityType(EntityType::EntityType(Symbol::Symbol(type_for_2p)));
		}
		else
			newMention->setEntityType(_personType);
	} 
	else	
		newMention->setEntityType(EntityType::getUndetType());

	return 1;
}


