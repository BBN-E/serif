// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/DescriptorClassifier.h"
#include "Generic/descriptors/xx_DescriptorClassifier.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"

#include <math.h>

const int DescriptorClassifier::_LOG_OF_ZERO = -10000;


int DescriptorClassifier::classifyMention (MentionSet *currSolution, 
											 	  Mention *currMention, 
												  MentionSet *results[], 
												  int max_results, 
												  bool isBranching)
{
	// don't bother if it's already been given a type OR
	// if its a nominal premod
	if (currMention->getEntityType().isRecognized() ||
		NodeInfo::isNominalPremod(currMention->getNode())) 
	{
		if (isBranching)
			results[0] = _new MentionSet(*currSolution);
		else
			results[0] = currSolution;
		return 1;
	}

	// only want to classify DESC and NONE. something else might get in via
	// nestedMentionClassifier. don't continue if it is
	switch (currMention->mentionType) {
		// bail case: reproduce the current solution and return as is
		case Mention::NAME:
		case Mention::PRON:
		case Mention::PART:
		case Mention::APPO:
		case Mention::LIST:
		case Mention::NEST:
			if (isBranching)
				results[0] = _new MentionSet(*currSolution);
			else
				results[0] = currSolution;
			return 1;
		case Mention::NONE:
		case Mention::DESC:
			break;
		default:
			throw InternalInconsistencyException(
				"DescriptorClassifier::classifyMention()",
				"Unexpected mention type seen");
	}
	const SynNode* node = currMention->node;

	int internalBranch;
	if (isBranching)
		internalBranch = max_results > 6 ? 6 : max_results;
	else
		internalBranch = 1;
	EntityType *guessTypes = _new EntityType[internalBranch];
	double *guessScores = _new double[internalBranch];
	int nGuesses = classifyDescriptor(currSolution, node, guessTypes, guessScores, internalBranch);
	// old style: mention belongs to solution, so just change the type, move the pointer,
	// and return
	if (!isBranching) {
		currMention->setEntityType(guessTypes[0]);
		currMention->mentionType = Mention::DESC;
		results[0] = currSolution;
		results[0]->setDescScore(results[0]->getDescScore() +
								 static_cast<float>(guessScores[0]));
	}
	// new style: fork the mention set, re-resolve the mention by id,
	else {
		int i;
		for (i=0; i < nGuesses; i++) {
			MentionSet* newSet = _new MentionSet(*currSolution);
			// locate the newly forked mention, since that's what we're changing
			Mention* newMent = newSet->getMention(currMention->getIndex());
			newMent->setEntityType(guessTypes[i]);
			newMent->mentionType = Mention::DESC;
			results[i] = newSet;
			results[i]->setDescScore(results[i]->getDescScore() +
									 static_cast<float>(guessScores[i]));
		}
	}

	delete [] guessTypes;
	delete [] guessScores;
	return nGuesses;

}

// utility function that inserts a score and type into a list of scores and types
int DescriptorClassifier::insertScore(double score, 
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




boost::shared_ptr<DescriptorClassifier::Factory> &DescriptorClassifier::_factory() {
	static boost::shared_ptr<DescriptorClassifier::Factory> factory(new DefaultDescriptorClassifierFactory());
	return factory;
}

