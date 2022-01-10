// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/discTagger/DTTagSet.h"  
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/discTagger/BlockFeatureTable.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/names/discmodel/PIdFFeatureTypes.h"
#include "Chinese/names/discmodel/ch_PIdFCharSentence.h"
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <boost/algorithm/string.hpp>

#include "Chinese/names/discmodel/ch_PIdFCharModel.h"

#include "Generic/discTagger/DTMaxMatchingListFeatureType.h"

using namespace std;


Symbol PIdFCharModel::_NONE_ST = Symbol(L"NONE-ST");
Symbol PIdFCharModel::_NONE_CO = Symbol(L"NONE-CO");

Token PIdFCharModel::_blankToken(Symbol(L"NULL"));
Symbol PIdFCharModel::_blankLCSymbol = Symbol(L"NULL");
Symbol PIdFCharModel::_blankWordFeatures = Symbol(L"NULL");
WordClusterClass PIdFCharModel::_blankWordClass = WordClusterClass::nullCluster();

PIdFCharModel::PIdFCharModel()
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0), _secondaryDecoders(0),
	  _print_sentence_selection_info(false)
{

	PIdFFeatureTypes::ensureFeatureTypesInstantiated();
	_wordFeatures = IdFWordFeatures::build();
	WordClusterTable::ensureInitializedFromParamFile();

	// Read parameters
	if (ParamReader::getRequiredTrueFalseParam("pidf_interleave_tags"))
	{
		_interleave_tags = true;
	}
	else{
		_interleave_tags = false;
	}

	// This should only be turned on if sentence selection is the only thing you are doing!
	if (ParamReader::isParamTrue("print_sentence_selection_info"))		
		_print_sentence_selection_info = true;
	else _print_sentence_selection_info = false;

	std::string tag_set_file = ParamReader::getRequiredParam("pidf_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), true, true,  _interleave_tags);
	
	_model_file = ParamReader::getRequiredParam("pidf_model_file");

	if (ParamReader::getRequiredTrueFalseParam("pidf_learn_transitions")) {
		_learn_transitions_from_training = true;
	}
	else {
		_learn_transitions_from_training = false;
	}

	std::string features_file = ParamReader::getRequiredParam("pidf_features_file");

	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), PIdFFeatureType::modeltype);
	_weights = _new BlockFeatureTable(_tagSet); 

	if(_learn_transitions_from_training ){
		std::string transition_file = _model_file + "-transitions";
		_tagSet->readTransitions(transition_file.c_str());
	}
	DTFeature::readWeights(*_weights, _model_file.c_str(), PIdFFeatureType::modeltype);
	_decoder = _new PDecoder(_tagSet, _featureTypes, _weights);
	_secondaryDecoders = _new PIdFSecondaryDecoders();
}

PIdFCharModel::PIdFCharModel(const char* tag_set_file, const char* features_file, 
							 const char* model_file, bool learn_transitions)
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0), _secondaryDecoders(0),
	  _print_sentence_selection_info(false)
{
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();

	_interleave_tags = false;

	_tagSet = _new DTTagSet(tag_set_file, true, true,  _interleave_tags);
	_learn_transitions_from_training = learn_transitions;

	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	_wordFeatures = IdFWordFeatures::build();
	WordClusterTable::ensureInitializedFromParamFile();

	// This should only be turned on if sentence selection is the only thing you are doing!
	if (ParamReader::isParamTrue("print_sentence_selection_info"))		
		_print_sentence_selection_info = true;
	else _print_sentence_selection_info = false;

	_weights = _new BlockFeatureTable(_tagSet); 

	if(_learn_transitions_from_training) {
		std::string model_file_str(model_file);
		std::string transition_file = model_file_str + "-transitions";
		_tagSet->readTransitions(transition_file.c_str());
	}
	DTFeature::readWeights(*_weights, model_file, PIdFFeatureType::modeltype);
	_decoder = _new PDecoder(_tagSet, _featureTypes, _weights);
	_secondaryDecoders = _new PIdFSecondaryDecoders();	
}

PIdFCharModel::~PIdFCharModel() {
	delete _decoder;
	delete _weights;
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;
	delete _secondaryDecoders;	
}

void PIdFCharModel::resetForNewDocument(DocTheory *docTheory) {}

void PIdFCharModel::decode(PIdFCharSentence &sentence) {
	int tags[MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL+2];
	
	int sent_length = std::min(sentence.getLength(), MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL);
	//clear out the old observations array - it is populated with the members from the previous decoding
	for(vector<DTObservation*>::iterator i = _observations.begin(); i != _observations.end(); ++i) {
		delete *i;
	}
	_observations.clear();

	populateSentence(_observations, &sentence, sent_length);

	_secondaryDecoders->AddDecoderResultsToObservation(_observations);

	_decoder->decode(_observations, tags);

	for (int k = 0; k < sent_length; k++)
		sentence.setTag(k, tags[k+1]);
}

void PIdFCharModel::decode(PIdFCharSentence &sentence, double &margin) {
	int tags[MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL+2];

	int sent_length = std::min(sentence.getLength(), MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL);
	populateSentence(_observations, &sentence, sent_length);

	_secondaryDecoders->AddDecoderResultsToObservation(_observations);

	_decoder->decode(_observations, tags);

	margin = _decoder->decodeAllTags(_observations);

	for (int k = 0; k < sent_length; k++)
		sentence.setTag(k, tags[k+1]);
}

int PIdFCharModel::getNameTheories(NameTheory **results, int max_theories,
								 TokenSequence *tokenSequence)
{
	PIdFCharSentence sentence(_tagSet, *tokenSequence);
	double margin = 0;
	if (_print_sentence_selection_info)
		decode(sentence, margin);
	else decode(sentence);
	results[0] = makeNameTheory(sentence);
	if (_print_sentence_selection_info)
		results[0]->setScore(static_cast<float>(margin / 1000.0));
	return 1;
}


void PIdFCharModel::populateObservation(TokenObservation *observation,
									  IdFWordFeatures *wordFeatures,
									  Symbol word, bool first_word)
{
	Token token(word);

	std::wstring buf(word.to_string());
	std::transform(buf.begin(), buf.end(), buf.begin(), towlower);
	Symbol lcSymbol(buf.c_str());

	Symbol idfWordFeature = wordFeatures->features(word, first_word, false);
	WordClusterClass wordClass(word);
	
	Symbol allFeatures[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	int n_word_features = wordFeatures->getAllFeatures(word, first_word, false, 
		allFeatures, DTFeatureType::MAX_FEATURES_PER_EXTRACTION);

	observation->populate(token, lcSymbol, idfWordFeature, wordClass, allFeatures, n_word_features);
}


void PIdFCharModel::updateDictionaryMatchingCache(const DTMaxMatchingListFeatureType* dtMaxMatchingListFeature, 
	std::vector<DTObservation *> &observations) {

	Symbol featureName = dtMaxMatchingListFeature->getFeatureName();

	std::vector<bool> dictionaryMatchingCache;
	const size_t MAX_TOKENS_LOOK_AHEAD = 6;

	std::vector<std::wstring> tokens;
	for(size_t i=0; i<observations.size(); i++) {
		TokenObservation *o = static_cast<TokenObservation*>(observations[i]);
		tokens.push_back(o->getSymbol().to_string());
		dictionaryMatchingCache.push_back(false); // set all to be NOT_IN_LIST
	}

	for(size_t i=0; i<tokens.size(); i++) {
		// longest first
		size_t j=i+MAX_TOKENS_LOOK_AHEAD<=tokens.size()-1?i+MAX_TOKENS_LOOK_AHEAD:tokens.size()-1;
		for(int end_idx=static_cast<int>(j); end_idx>=static_cast<int>(i) && end_idx>=0; end_idx--) {
			std::wostringstream wordsstream;
			for(int idx=static_cast<int>(i); idx<=end_idx; idx++)
				wordsstream << tokens[idx];
			
			Symbol wordString(wordsstream.str().c_str());
			bool isMatch = dtMaxMatchingListFeature->isMatch(wordString);
			if (isMatch) { // found match
				for(int idx=static_cast<int>(i); idx<=end_idx; idx++) {
					dictionaryMatchingCache[idx]=true;
				}
				break;
			}
		}
	}

	for(size_t i=0; i<observations.size(); i++) {
		TokenObservation *o = static_cast<TokenObservation*>(observations[i]);
		o->updateCacheFeatureType2match(featureName, dictionaryMatchingCache[i]);
	}
}


void PIdFCharModel::populateSentence(std::vector<DTObservation *> & observations, 
									 PIdFCharSentence* sentence, int sent_length)
{

	observations.push_back(new TokenObservation());
	static_cast<TokenObservation*>(observations[0])->populate(_blankToken, _blankLCSymbol, _blankWordFeatures, _blankWordClass, 0, 0);
	for (int i = 0; i < sent_length; i++) {
		Symbol word = sentence->getChar(i);
		observations.push_back(new TokenObservation());
		populateObservation(
			static_cast<TokenObservation*>(observations.back()),
			_wordFeatures, word, i == 0);
	}
	observations.push_back(new TokenObservation());
	static_cast<TokenObservation*>(observations.back())
		->populate(_blankToken, _blankLCSymbol, _blankWordFeatures,
			_blankWordClass, 0, 0);

	// update matching cache for list-based features
	int n_feature_types = _featureTypes->getNFeaturesTypes();

	for (int i = 0; i < n_feature_types; i++) {
		const DTFeatureType *featureType = _featureTypes->getFeatureType(i);
		if(const DTMaxMatchingListFeatureType* dtMaxMatchingListFeature = dynamic_cast<const DTMaxMatchingListFeatureType*>(featureType)) {
			/*
			// dtMaxMatchingListFeature->updateDictionaryMatchingCache(observations);
			Symbol featureName = dtMaxMatchingListFeature->getFeatureName();
			
			for(vector<DTObservation*>::iterator it = observations.begin(); 
				it != observations.end(); ++it) {
					(*it)->dump();
			}
			*/
			updateDictionaryMatchingCache(dtMaxMatchingListFeature, observations);
		}
	}
	//
}


NameTheory *PIdFCharModel::makeNameTheory(PIdFCharSentence &sentence) {
	int sent_length = std::min(sentence.getLength(), MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL);
	int i, j;
	int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);
	float score_penalty = 0.0;
	
	int n_name_spans = 0;
	for (j = 0; j < sent_length; j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			_tagSet->isSTTag(sentence.getTag(j)))
		{
			n_name_spans++;
		}
	}
	
	int char_index = 0;
	int last_end_char = -1;
	for (i = 0; i < n_name_spans; i++) {
		while (!(sentence.getTag(char_index) != NONE_ST_tag &&
				 _tagSet->isSTTag(sentence.getTag(char_index))))
		{ char_index++; }

		int tag = sentence.getTag(char_index);

		int end_index = char_index;
		while (end_index+1 < sent_length &&
			   _tagSet->isCOTag(sentence.getTag(end_index+1)))
		{ end_index++; }

		// penalize if the name crosses a token boundary 
		if (char_index > 0 && (sentence.getTokenFromChar(char_index - 1) ==
			sentence.getTokenFromChar(char_index)))
		{
			score_penalty += 100.0;
		}
		if (end_index < sent_length - 1 &&
			(sentence.getTokenFromChar(end_index) == sentence.getTokenFromChar(end_index+1)))
		{
			score_penalty += 100.0;
		}

		// if we have overlap, figure out which name should get the token
		if (i > 0 && last_end_char != -1 && 
		   (sentence.getTokenFromChar(char_index) == sentence.getTokenFromChar(last_end_char))) 
		{
			if (firstNameHasRightsToToken(sentence, last_end_char, char_index)) {
				while (char_index < sent_length - 1 && sentence.getTokenFromChar(char_index) == sentence.getTokenFromChar(last_end_char))
					char_index++;
			} else {
				while (last_end_char > 0 && sentence.getTokenFromChar(char_index) == sentence.getTokenFromChar(last_end_char))
					last_end_char--;
				_spanBuffer[i-1]->end =  sentence.getTokenFromChar(last_end_char);
			}
		}

		
		_spanBuffer[i] = _new NameSpan(sentence.getTokenFromChar(char_index), 
							   sentence.getTokenFromChar(end_index),	
	           				   EntityType(_tagSet->getReducedTagSymbol(tag)));

		last_end_char = end_index;
		char_index = end_index + 1;
	}

	// not all these names will be good, some will now have zero length, or overlap previous name
	int bad_name_count = 0;
	int last_name_end = -1;
	for (i = 0; i < n_name_spans; i++ ) {
		if (_spanBuffer[i]->start <= last_name_end || 
			_spanBuffer[i]->start > _spanBuffer[i]->end) 
		{
			_spanBuffer[i]->start = OVERLAPPING_SPAN;
			_spanBuffer[i]->end = OVERLAPPING_SPAN;
			bad_name_count++;
		} 
		else {
			last_name_end = _spanBuffer[i]->end;
		}
	}

	NameTheory *nameTheory = _new NameTheory(sentence.getTokenSequence());
	nameTheory->setScore(-score_penalty);

	for (i = 0; i < n_name_spans; i++) {
		if (_spanBuffer[i]->start != OVERLAPPING_SPAN)
			nameTheory->takeNameSpan(_spanBuffer[i]);
		else 
 			delete _spanBuffer[i];
	}

	return nameTheory;
}

bool PIdFCharModel::firstNameHasRightsToToken(PIdFCharSentence &sentence,
											  int first_token_char_index, 
											  int second_token_char_index) 
{
	int sent_length = std::min(sentence.getLength(), MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL);
	int start_token_index = sentence.getTokenFromChar(first_token_char_index);
	for (int i = 0; i < sent_length - second_token_char_index; i++) {
		if (sentence.getTokenFromChar(second_token_char_index + i) != start_token_index) 
			return true;
		if (first_token_char_index - i < 0)
			return true;
		if (sentence.getTokenFromChar(first_token_char_index - i) != start_token_index) 
			return false;
	}
	return false;
}

	


	




