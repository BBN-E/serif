// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Arabic/descriptors/ar_PronounClassifier.h"
#include "Generic/common/WordConstants.h"
#include "Generic/descriptors/discmodel/P1DescriptorClassifier.h"
#include "Generic/common/ParamReader.h"

#include "Arabic/parse/ar_STags.h"

ArabicPronounClassifier::ArabicPronounClassifier() 	:  DEBUG(false),
_p1DescriptorClassifier(0){
	if (ParamReader::isParamTrue("classify_pronoun_type")) {
		_p1DescriptorClassifier = 
			_new P1DescriptorClassifier(P1DescriptorClassifier::PRONOUN_CLASSIFY);
	}
	std::string debugStreamParam = ParamReader::getParam("pronoun_classifier_debug_file");
	if (!debugStreamParam.empty()) {
		_debugStream.open(debugStreamParam.c_str());
		if (_debugStream.fail()) {
			std::cerr << "could not open pronoun_classifier_debug_file\n";
		}
		else {
			DEBUG = true;
		}
	}


}

ArabicPronounClassifier::~ArabicPronounClassifier() {}

int ArabicPronounClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching	
	if (isBranching)
		results[0] = _new MentionSet(*currSolution);
	else
		results[0] = currSolution;

	// can't classify if the mention was made partitive or appositive or list or nested
	// we identify nested by looking for a mention type of NAME or APPOS
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
			throw InternalInconsistencyException("ArabicPronounClassifier::classifyMention()",
				"Unexpected mention type seen");
	}

	const SynNode* node = currMention->node;
	const SynNode* preterm = node;
	if(!preterm->isPreterminal()){
		node->getHeadPreterm();	
	}
	if (!ArabicSTags::isPronoun(preterm->getTag()))
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
	newMention->setEntityType(EntityType::getUndetType());
	Symbol hw = newMention->getHead()->getHeadWord();
	if(WordConstants::is1pPronoun(hw) || WordConstants::is2pPronoun(hw)){
		newMention->setEntityType(EntityType::getPERType());
	}
	else{
		if (_p1DescriptorClassifier != 0) {
			EntityType newType = classifyPronounType(currSolution, node);
			if (newType == EntityType::getOtherType())
				newMention->setEntityType(EntityType::getUndetType());
			else
				newMention->setEntityType(newType);
		}
		else {
			newMention->setEntityType(EntityType::getUndetType());
		}
	}
/*	//this hack relies on non-UPENN POS tags
if (ArabicSTags::getPronPOSPerson(preterm->getTag()) == 3)
		newMention->setEntityType(EntityType::getUndetType();
	else
		newMention->setEntityType(EntityType::getPERType();
*/
	return 1;
}

EntityType ArabicPronounClassifier::classifyPronounType(MentionSet *currSolution, const SynNode* node)
												 //EntityType types[], double scores[], int max_results)
{	
	//int num_results = 0;
	
	if (DEBUG) {
		std::wstring nodeStr = node->getParent()->toString(0);
		_debugStream << "--- Classifying " << node->getHeadWord().to_string() << "\n"
					 << nodeStr << "\n";
	}


	EntityType answer = _p1DescriptorClassifier->classifyDescriptor(currSolution, node);
	//num_results = insertScore(1.0, answer, scores, types, num_results, max_results);

	if (DEBUG) {
		_debugStream << "Classified as: "
					 << answer.getName().to_string() << "\n";
		_debugStream << "\n";
		_debugStream.flush();
	}

	/*if (num_results == 0) {
		throw InternalInconsistencyException(
			"ArabicPronounClassifier::classifyPronounType", "Couldn't find a decent score");
	}
	return num_results;*/

	return answer;
}
