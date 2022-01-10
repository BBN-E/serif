// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/leak_detection.h"

#include "Arabic/relations/ar_RelationFinder.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/relations/discmodel/ar_P1RelationFinder.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/Parse.h"


ArabicRelationFinder::ArabicRelationFinder() : _p1RelationFinder(0) {
	std::string parameter = ParamReader::getRequiredParam("relation_model_type");
	if (parameter == "P1")
		mode = P1;
	else if (parameter == "NONE")
		mode = NONE;
	else {
		std::string error = parameter + " not a valid relation model type: use NONE or P1";
		throw UnexpectedInputException("ArabicRelationFinder::ArabicRelationFinder()", error.c_str());
	}

	if (mode == P1)
		_p1RelationFinder = _new ArabicP1RelationFinder();
}

ArabicRelationFinder::~ArabicRelationFinder() {
	delete _p1RelationFinder;
}
void ArabicRelationFinder::cleanup() {
	_p1RelationFinder->cleanup();
}

void ArabicRelationFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num) {
	if (mode == P1)
		return _p1RelationFinder->resetForNewSentence();
}

RelMentionSet *ArabicRelationFinder::getRelationTheory(EntitySet *entitySet, const Parse *parse,
		                       MentionSet *mentionSet, ValueMentionSet *valueMentionSet, 
		                       PropositionSet *propSet, const Parse *secondaryParse,
							   const PropTreeLinks* ptLinks)
{
	if (mode == P1) {
		return _p1RelationFinder->getRelationTheory(entitySet, parse, mentionSet, 
													valueMentionSet, propSet, secondaryParse);
	}
	else {
		return _new RelMentionSet();
	}
}

