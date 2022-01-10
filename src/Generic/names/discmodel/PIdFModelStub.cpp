// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "theories/NameTheory.h"
#include "theories/TokenSequence.h"
#include "discTagger/DTTagSet.h"
#include "names/discmodel/PIdFModel.h"

PIdFModel::PIdFModel(model_mode_e mode) : _tagSet(0) {
	char tag_set_file[500];
	if (!ParamReader::getParam("pidf_tag_set_file", tag_set_file, 500))	{
		throw UnexpectedInputException("PIdFModel::PIdFModel()",
									   "Parameter 'pidf_tag_set_file' not specified");
	}

	_tagSet = _new DTTagSet(tag_set_file, true, true, true);
}

PIdFModel::PIdFModel(model_mode_e mode, char* tag_set_file, 
		char* features_file, char* model_file, char* vocab_file,
		bool learn_transitions)  : _tagSet(0)
{
	_tagSet = _new DTTagSet(tag_set_file, true, true, true);
}

PIdFModel::PIdFModel(model_mode_e mode, char* tag_set_file, 
		char* features_file, char* model_file, char* vocab_file, 
		char* lc_model_file, char* lc_vocab_file,
		bool learn_transitions) : _tagSet(0)
{
	_tagSet = _new DTTagSet(tag_set_file, true, true, true);
}

PIdFModel::~PIdFModel() {
	delete _tagSet;
}

void PIdFModel::decode(PIdFSentence &sentence) {
	for (int k = 0; k < sentence.getLength(); k++)
		sentence.setTag(k, _tagSet->getTagIndex(_tagSet->getNoneSTTag()));
}

void PIdFModel::decode(PIdFSentence &sentence, double &margin) {
	decode(sentence);
	margin = 0.0;
}

int PIdFModel::getNameTheories(NameTheory **results, int max_theories,
								 TokenSequence *tokenSequence)
{
	results[0] = _new NameTheory();
	results[0]->n_name_spans = 0;
	results[0]->nameSpans = 0;
	return 1;
}

NameTheory *PIdFModel::makeNameTheory(PIdFSentence &sentence) {
	NameTheory *nameTheory = _new NameTheory();
	nameTheory->n_name_spans = 0;
	nameTheory->nameSpans = 0;

	return nameTheory;
}
