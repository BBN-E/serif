// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/descriptors/NomPremodClassifier.h"
#include "Generic/descriptors/xx_NomPremodClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

NomPremodClassifier::NomPremodClassifier() {}

NomPremodClassifier::~NomPremodClassifier() {}

int NomPremodClassifier::classifyMention (MentionSet *currSolution, 
										  Mention *currMention, 
										  MentionSet *results[], 
										  int max_results, 
										  bool isBranching)
{

	// EMB 6/20: 
	// ALL THIS STUFF IS COPIED AND MODIFIED FROM THE GENERIC
	// DescriptorClassifier.cpp. SEEMS REASONABLE, BUT I DON'T 
	// KNOW THE SUBTLETIES -- SO BE FOREWARNED.

	// don't bother if:
	// * it's already been given a type,
	// * it isn't a nompremod
	// * its type isn't DESC or NONE (not sure this will ever happen)
	if (currMention->getEntityType().isRecognized() || 
		!NodeInfo::isNominalPremod(currMention->getNode()) ||
		(currMention->mentionType != Mention::NONE &&
		 currMention->mentionType != Mention::DESC))
	{
		if (isBranching)
			results[0] = _new MentionSet(*currSolution);
		else
			results[0] = currSolution;
		return 1;
	}
	
	int internalBranch;
	if (isBranching)
		internalBranch = max_results > 6 ? 6 : max_results;
	else
		internalBranch = 1;

	const SynNode* node = currMention->node;
	EntityType *guessTypes = _new EntityType[internalBranch];
	double *guessScores = _new double[internalBranch];
	int nGuesses = classifyNomPremod(currSolution, node, guessTypes, guessScores, internalBranch);

	// old style: mention belongs to solution, so just change the type, move the pointer,
	// and return
	if (!isBranching) {
		currMention->setEntityType(guessTypes[0]);
		currMention->mentionType = Mention::DESC;
		results[0] = currSolution;
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
		}
	}

	delete [] guessTypes;
	delete [] guessScores;
	return nGuesses;
}

// COPIED UNCHANGED FROM DESC CLASSIFIER 6/20/04
// utility function that inserts a score and type into a list of scores and types
int NomPremodClassifier::insertScore(double score, 
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




boost::shared_ptr<NomPremodClassifier::Factory> &NomPremodClassifier::_factory() {
	static boost::shared_ptr<NomPremodClassifier::Factory> factory(new DefaultNomPremodClassifierFactory());
	return factory;
}

