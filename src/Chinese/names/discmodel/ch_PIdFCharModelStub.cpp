// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/limits.h"
#include "common/ParamReader.h"
#include "common/SessionLogger.h"
#include "theories/NameTheory.h"
#include "theories/NameSpan.h"
#include "discTagger/DTTagSet.h"  
#include "Chinese/names/discmodel/ch_PIdFCharSentence.h"
#include "Chinese/names/discmodel/ch_PIdFCharModel.h"

PIdFCharModel::PIdFCharModel() : _tagSet(0) {	
	char tag_set_file[500];
	ParamReader::getRequiredParam("pidf_tag_set_file", tag_set_file, 500);
	_tagSet = _new DTTagSet(tag_set_file, true, true,  false);
}

PIdFCharModel::PIdFCharModel(char* tag_set_file, char* features_file, 
							 char* model_file, bool learn_transitions) : _tagSet(0)
{
	_tagSet = _new DTTagSet(tag_set_file, true, true, false);
}

PIdFCharModel::~PIdFCharModel() {
	delete _tagSet;
}


void PIdFCharModel::decode(PIdFCharSentence &sentence) {
	for (int k = 0; k < sentence.getLength(); k++)
		sentence.setTag(k, _tagSet->getTagIndex(_tagSet->getNoneSTTag()));
}

void PIdFCharModel::decode(PIdFCharSentence &sentence, double &margin) {
	decode(sentence);
	margin = 0.0;
}

int PIdFCharModel::getNameTheories(NameTheory **results, int max_theories,
								 TokenSequence *tokenSequence)
{
	results[0] = _new NameTheory();
	results[0]->n_name_spans = 0;
	results[0]->nameSpans = 0;
	return 1;
}

NameTheory *PIdFCharModel::makeNameTheory(PIdFCharSentence &sentence) {
	NameTheory *nameTheory = _new NameTheory();
	nameTheory->n_name_spans = 0;
	nameTheory->nameSpans = 0;

	return nameTheory;
}

	


	




