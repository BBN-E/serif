// Copyright (c) 2013 by Raytheon BBN Technologies             
// All Rights Reserved.       
#include "Generic/common/leak_detection.h"

#include "Urdu/partOfSpeech/ur_PartOfSpeechRecognizer.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/discTagger/DTTagSet.h"

using namespace std;


UrduPartOfSpeechRecognizer::UrduPartOfSpeechRecognizer() 
	: _debug_flag(false), _pidfDecoder(0), _tagSet(0), _sent_no(0),
	  _tokenSequence(0), _docTheory(0)	
{
	if (ParamReader::isParamTrue("debug_pos_recognizer"))
	{
		_debug_flag = true;
	}
	std::string pos_model_file = ParamReader::getRequiredParam("ppos_model_file");
	std::string pos_features_file = ParamReader::getRequiredParam("ppos_features_file");
	std::string pos_tag_file = ParamReader::getRequiredParam("ppos_tag_set_file");
	std::string pos_vocab_file = ParamReader::getParam("ppos_vocab_file");
	
	bool pos_learn_transitions = ParamReader::getRequiredTrueFalseParam("ppos_learn_transitions");
	bool pos_use_clusters = ParamReader::getRequiredTrueFalseParam("ppos_use_clusters");
	

	/* PIdFModel(model_mode_e mode, const char* tag_set_file, 
		const char* features_file, const char* model_file, const char* vocab_file,
		bool learn_transitions = false, bool use_clusters=true); */
	_pidfDecoder = _new PIdFModel(PIdFModel::DECODE, pos_tag_file.c_str(), 
		pos_features_file.c_str(), pos_model_file.c_str(), pos_vocab_file.c_str(), 
		pos_learn_transitions, pos_use_clusters);
	_tagSet = _pidfDecoder->getTagSet();

}

UrduPartOfSpeechRecognizer::~UrduPartOfSpeechRecognizer() {
	delete _pidfDecoder;
}


int UrduPartOfSpeechRecognizer::getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
														TokenSequence *tokenSequence)
{
	PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence);
	_pidfDecoder->decode(sentence);
	results[0] = _new PartOfSpeechSequence(tokenSequence);
	for (int num = 0; num < results[0]->getNTokens(); ++num) {
		results[0]->addPOS(_tagSet->getTagSymbol(sentence.getTag(num)), static_cast<float> (sentence.marginScore), num);
	}
	int n_results = 1;
	return n_results;
}

	

