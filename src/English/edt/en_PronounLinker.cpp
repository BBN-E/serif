// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/en_PronounLinker.h"
#include "English/edt/en_PMPronounLinker.h"
#include "Generic/edt/discmodel/DTPronounLinker.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/common/ParamReader.h"

EnglishPronounLinker::EnglishPronounLinker() : _pmPronounLinker(0), _dtPronounLinker(0) {

	std::string link_mode = ParamReader::getRequiredParam("pronoun_link_mode");
	if (link_mode == "PM" || link_mode == "pm") {
		MODEL_TYPE = PM;
		_pmPronounLinker = _new EnglishPMPronounLinker();
	}	
	else if (link_mode == "DT" || link_mode == "dt") {
		MODEL_TYPE = DT;
		_dtPronounLinker = _new DTPronounLinker();
	}
	else {
		throw UnexpectedInputException(
			"EnglishPronounLinker::EnglishPronounLinker()",
			"Parameter 'pronoun_link_mode' must be set to 'PM' or 'DT'");
	}
}

EnglishPronounLinker::~EnglishPronounLinker() {
	delete _pmPronounLinker;
	delete _dtPronounLinker;
}


void EnglishPronounLinker::resetForNewDocument(DocTheory *docTheory) {
	if (MODEL_TYPE == PM)
		_pmPronounLinker->resetForNewDocument(docTheory);
	else
		_dtPronounLinker->resetForNewDocument(docTheory);
}

void EnglishPronounLinker::addPreviousParse(const Parse *parse) {
	if (MODEL_TYPE == PM)
		_pmPronounLinker->addPreviousParse(parse);
	else
		_dtPronounLinker->addPreviousParse(parse);
}	

void EnglishPronounLinker::resetPreviousParses() {
	if (MODEL_TYPE == PM)
		_pmPronounLinker->resetPreviousParses();
	else
		_dtPronounLinker->resetPreviousParses();
}

int EnglishPronounLinker::linkMention (LexEntitySet * currSolution,
								MentionUID currMentionUID,
								EntityType linkType,
								LexEntitySet *results[],
								int max_results)
{
	if (MODEL_TYPE == PM) {
		return _pmPronounLinker->linkMention(currSolution, currMentionUID,
										linkType, results, max_results);
	} else {
		return _dtPronounLinker->linkMention(currSolution, currMentionUID,
										linkType, results, max_results);
	}
}


