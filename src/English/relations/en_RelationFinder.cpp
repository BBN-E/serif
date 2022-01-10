// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/relations/en_RelationFinder.h"
#include "Generic/relations/PatternRelationFinder.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/Parse.h"
#include "English/relations/en_ComboRelationFinder.h"


EnglishRelationFinder::EnglishRelationFinder() :
	_patternRelationFinder(0), _comboRelationFinder(0)
{

	// Backwards compatible. Please remove the first grouping when parameter files are all changed. 	
	std::string model_type = ParamReader::getParam("relation_model_type");
	if (model_type != "") {
		if (model_type == "COMBO")
			_comboRelationFinder = _new EnglishComboRelationFinder();
		else if (model_type != "NONE") {
			std::string error = "Parameter 'relation_model_type' must be set to 'COMBO' or 'NONE'";
			throw UnexpectedInputException("EnglishRelationFinder::EnglishRelationFinder()", error.c_str());
		}
	} else {
		if (ParamReader::getRequiredTrueFalseParam("use_combo_relation_model"))
			_comboRelationFinder = _new EnglishComboRelationFinder();

		if (ParamReader::getRequiredTrueFalseParam("use_relation_patterns"))
			_patternRelationFinder = _new PatternRelationFinder();
	}
}

EnglishRelationFinder::~EnglishRelationFinder() {
	delete _comboRelationFinder;
	delete _patternRelationFinder;
}

void EnglishRelationFinder::cleanup() {
	if (_comboRelationFinder)
		_comboRelationFinder->cleanup();
}

void EnglishRelationFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num) {
	if (_comboRelationFinder)
		_comboRelationFinder->resetForNewSentence();
	
	if (_patternRelationFinder)
		_patternRelationFinder->resetForNewSentence(docTheory, sentence_num);
}

void EnglishRelationFinder::allowMentionSetChanges() {
	if (_comboRelationFinder != 0)
		_comboRelationFinder->allowMentionSetChanges();
}
void EnglishRelationFinder::disallowMentionSetChanges() {
	if (_comboRelationFinder != 0)
		_comboRelationFinder->disallowMentionSetChanges();
}

RelMentionSet *EnglishRelationFinder::getRelationTheory(EntitySet *entitySet,
												 SentenceTheory *sentTheory,
												 const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
								   PropositionSet *propSet,
								   const Parse *secondaryParse,
								   const PropTreeLinks* ptLinks)

{
	// don't find relations if we didn't parse
	if (parse->isDefaultParse())
		return _new RelMentionSet();

	RelMentionSet *rmSet = _new RelMentionSet();

	if (_comboRelationFinder) {
		RelMentionSet *tempSet = _comboRelationFinder->getRelationTheory(entitySet, sentTheory, parse, 
							mentionSet, valueMentionSet, propSet, ptLinks);
		rmSet->takeRelMentions(tempSet);
		delete tempSet;
	}
	if (_patternRelationFinder) {
		// Info is stored in docTheory, passed in by resetForNewSentence(). 
		// If that's not there we're screwed anyway, since we need it for pattern matching.
		RelMentionSet *tempSet = _patternRelationFinder->getRelationTheory();
		rmSet->takeRelMentions(tempSet);
		delete tempSet;
	}
	return rmSet;
}

