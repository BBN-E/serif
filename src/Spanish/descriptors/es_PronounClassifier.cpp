// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Spanish/descriptors/es_PronounClassifier.h"
#include "Generic/descriptors/discmodel/P1DescriptorClassifier.h"
#include "Generic/common/WordConstants.h"
#include "Spanish/parse/es_STags.h"
#include "Spanish/propositions/es_SemTreeUtils.h"

SpanishPronounClassifier::SpanishPronounClassifier() 
	:  DEBUG(false),
	   _p1DescriptorClassifier(0)
{ 

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

SpanishPronounClassifier::~SpanishPronounClassifier() {
	if (DEBUG)
		_debugStream.close();
	delete _p1DescriptorClassifier;
}

int SpanishPronounClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching)
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
			return 1;
		case Mention::NONE:
			break;
		default:
			throw InternalInconsistencyException("SpanishPronounClassifier::classifyMention()",
				"Unexpected mention type seen");
	}

	const SynNode* node = currMention->node;
	if (SpanishSemTreeUtils::isMainVerbPOS(node->getTag()) && hasDroppedPronounParent(node)){
		//std::cout << "Got an opportunity to make a VMI mention\n";
		Mention* droppedPronounMention = results[0]->getMention(currMention->getIndex());
		droppedPronounMention->mentionType = Mention::PRON;
		droppedPronounMention->setEntityType(EntityType::getUndetType());
		return 1;
	}
	if (node->getEndToken() != node->getStartToken())
		return 1; // Only consider one-word constituents 
	Symbol hwTag = node->getHeadPreterm()->getTag();
	if ((!hwTag.isInSymbolGroup(SpanishSTags::POS_P_GROUP)) &&
		(hwTag!=SpanishSTags::POS_DP))
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
	//std::cerr << "PRONOUN" << node->toFlatString() << std::endl;
	if (WordConstants::is2pPronoun(node->getHeadWord()) || WordConstants::is1pPronoun(node->getHeadWord()))
		newMention->setEntityType(EntityType::getPERType());
	else {
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
	return 1;
}


bool SpanishPronounClassifier::hasDroppedPronounParent(const SynNode* node)
{
	if (node->getTag() == SpanishSTags::S_DP_SBJ || 
		node->getTag() == SpanishSTags::S_F_A_DP_SBJ || 
		node->getTag()== SpanishSTags::S_F_A_J_DP_SBJ || 
		node->getTag() == SpanishSTags::S_F_C_P_DP_SBJ || 
		node->getTag() == SpanishSTags::S_F_C_DP_SBJ || 
		node->getTag() == SpanishSTags::S_F_R_DP_SBJ || 
		node->getTag() == SpanishSTags::S_J_DP_SBJ || 
		node->getTag() == SpanishSTags::SENTENCE_DP_SBJ){
		//std::cout << "Got an opportunity to make a VMI mention\n";
		return 1;
	}else{
		if (node->getParent()){
			return hasDroppedPronounParent(node->getParent());
		}else{
			return 0;
		}
	}

}

EntityType SpanishPronounClassifier::classifyPronounType(MentionSet *currSolution, const SynNode* node)
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
			"SpanishPronounClassifier::classifyPronounType", "Couldn't find a decent score");
	}
	return num_results;*/

	return answer;
}


// COPIED UNCHANGED FROM DESC CLASSIFIER 6/20/04
// utility function that inserts a score and type into a list of scores and types
int SpanishPronounClassifier::insertScore(double score, 
								   EntityType type, 
								   double scores[], 
								   EntityType types[], 
								   int size, int cap)
{
	bool inserted = false;
	int j, k;
	for (j=0; j<size; j++) {
		// insert and shift remaining
		if(scores[j] < score) {
			if (size < cap)
				size++;

			for (k=size-1; k>j; k--){
				scores[k] = scores[k-1];
				types[k] = types[k-1];
			}
			scores[j] = score;
			types[j] = type;
			inserted = true;
			break;
		}
	}
	// add on end or ignore
	if (!inserted) {
		if (size < cap) {
			scores[size] = score;
			types[size++] = type;
		}
	}
	return size;
}
