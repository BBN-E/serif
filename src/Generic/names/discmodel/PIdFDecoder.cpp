// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/limits.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "common/InternalInconsistencyException.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8OutputStream.h"
#include "common/UTF8Token.h"
#include "common/SessionLogger.h"
#include "theories/Token.h"
#include "theories/NameTheory.h"
#include "theories/NameSpan.h"
#include "theories/EntityType.h"
#include "theories/DocTheory.h"
#include "WordClustering/WordClusterTable.h"
#include "WordClustering/WordClusterClass.h"
#include "names/IdFWordFeatures.h"
#include "discTagger/DTTagSet.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "names/discmodel/PIdFFeatureType.h"
#include "discTagger/PDecoder.h"
#include "names/discmodel/TokenObservation.h"
#include "names/discmodel/PIdFFeatureTypes.h"
#include "names/discmodel/PIdFSentence.h"
#include "names/discmodel/PIdFDecoder.h"

#include <iostream>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;


Symbol PIdFDecoder::_NONE_ST = Symbol(L"NONE-ST");
Symbol PIdFDecoder::_NONE_CO = Symbol(L"NONE-CO");


PIdFDecoder::PIdFDecoder()
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _defaultDecoder(0), _lowerCaseDecoder(0),
	  _upperCaseDecoder(0), _defaultWeights(0), _ucWeights(0), _lcWeights(0)
{
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();

	// Read parameters
	char interleave_tags[500];
	if (!ParamReader::getParam("pidf_interleave_tags",interleave_tags,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFDecoder()",
			"Parameter 'pidf_interleave_tags' not specified");
	}
	if(strcmp(interleave_tags, "true") == 0){
		_interleave_tags = true;
	}
	else{
		_interleave_tags = false;
	}

	char tag_set_file[500];
	if (!ParamReader::getParam("pidf_tag_set_file",tag_set_file,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFDecoder()",
			"Parameter 'pidf_tag_set_file' not specified");
	}
	_tagSet = _new DTTagSet(tag_set_file, true, true,  _interleave_tags);

	char model_file[500];
	if (!ParamReader::getParam("pidf_model_file",model_file,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFDecoder()",
			"Parameter 'pidf_model_file' not specified");
	}

	char learn_transitions[500];
	if (!ParamReader::getParam("pidf_learn_transitions",learn_transitions,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_learn_transitions' not specified");
	}
	if(strcmp(learn_transitions, "true") == 0){
		_learn_transitions_from_training = true;
	}
	else{
		_learn_transitions_from_training = false;
	}
	if(_learn_transitions_from_training){
		char transition_file[520];
		strcpy(transition_file, model_file);
		strncat(transition_file, "-transitions", 520);
		_tagSet->readTransitions(transition_file);
	}
	char features_file[500];
	if (!ParamReader::getParam("pidf_features_file",features_file,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFDecoder()",
			"Parameter 'pidf_features_file' not specified");
	}
	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);



	_wordFeatures = IdFWordFeatures::build();

	WordClusterTable::ensureInitializedFromParamFile();

	_defaultWeights = _new DTFeature::FeatureWeightMap(500009);
	DTFeature::readWeights(*_defaultWeights, model_file, PIdFFeatureType::modeltype);




	_defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultWeights);

	if (ParamReader::getParam("lowercase_pidf_model_file",model_file,									 500))	{
		_lcWeights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_lcWeights, model_file,PIdFFeatureType::modeltype);
		_lowerCaseDecoder = _new PDecoder(_tagSet, _featureTypes, _lcWeights);
	} else {
		_lowerCaseDecoder = 0;
		_lcWeights = 0;
	}

	if (ParamReader::getParam("uppercase_pidf_model_file",model_file,									 500))	{
		_ucWeights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_ucWeights, model_file, PIdFFeatureType::modeltype);
		_upperCaseDecoder = _new PDecoder(_tagSet, _featureTypes, _ucWeights);
	} else {
		_upperCaseDecoder = 0;
		_ucWeights = 0;
	}

	_decoder = _defaultDecoder;

}

PIdFDecoder::PIdFDecoder(
	const char *tag_set_file, const char *features_file,
	const char *model_file, const char *word_clusters_file,
	IdFWordFeatures *wordFeatures)
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _defaultDecoder(0), _lowerCaseDecoder(0),
	  _upperCaseDecoder(0), _defaultWeights(0), _ucWeights(0), _lcWeights(0)
{
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();

	// Read parameters

	_tagSet = _new DTTagSet(tag_set_file, true, true);

	_featureTypes = _new DTFeatureTypeSet(features_file,PIdFFeatureType::modeltype);

	if (wordFeatures == 0)
		_wordFeatures = IdFWordFeatures::build();
	else
		_wordFeatures = wordFeatures;

	if (word_clusters_file == 0)
		WordClusterTable::ensureInitializedFromParamFile();
	else
		WordClusterTable::initTable(word_clusters_file);

	_defaultWeights = _new DTFeature::FeatureWeightMap(500009);
	DTFeature::readWeights(*_defaultWeights, model_file, PIdFFeatureType::modeltype);

	_defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultWeights);
	_decoder = _defaultDecoder;
	_lowerCaseDecoder = 0;
	_upperCaseDecoder = 0;
}


PIdFDecoder::~PIdFDecoder() {
	delete _defaultDecoder;
	delete _defaultWeights;
	delete _upperCaseDecoder;
	delete _ucWeights;
	delete _lowerCaseDecoder;
	delete _lcWeights;
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;
}

void PIdFDecoder::resetForNewDocument(DocTheory *docTheory) {
	_decoder = _defaultDecoder;
	if (docTheory != 0) {
		int doc_case = docTheory->getDocumentCase();
		if (doc_case == DocTheory::LOWER && _lowerCaseDecoder != 0) {
			SessionLogger &logger = *SessionLogger::logger;
			logger.beginWarning();
			logger << "Using lowercase pIdF decoder\n";
			_decoder = _lowerCaseDecoder;
		} else if (doc_case == DocTheory::UPPER && _upperCaseDecoder != 0) {
			SessionLogger &logger = *SessionLogger::logger;
			logger.beginWarning();
			logger << "Using uppercase pIdF decoder\n";
			_decoder = _upperCaseDecoder;
		}
	}
}

void PIdFDecoder::decode() {
	char input_file[500];
	if (!ParamReader::getParam("pidf_input_file",input_file,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::decode()",
			"Parameter 'pidf_input_file' not specified");
	}

	char output_file[500];
	if (!ParamReader::getParam("pidf_output_file",output_file,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::decode()",
			"Parameter 'pidf_output_file' not specified");
	}

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(input_file);
	if (in.fail()) {
		throw UnexpectedInputException("PIdFDecoder::decode()",
			"Could not open input file for reading");
	}

	UTF8OutputStream out;
	out.open(output_file);
	if (out.fail()) {
		throw UnexpectedInputException("PIdFDecoder::decode()",
			"Could not create output file");
	}

	decode(in, out);
}

void PIdFDecoder::decode(UTF8InputStream &in, UTF8OutputStream &out) {
	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int sentence_n = 0;
	while (idfSentence.readSexpSentence(in)) {
		decode(idfSentence);

		idfSentence.writeSexp(out);

		cout << sentence_n << "\r";

		sentence_n++;
	}

	cout << "\n";
}

void PIdFDecoder::decode(PIdFSentence &sentence) {
	DTObservation *observations[MAX_SENTENCE_TOKENS+2];
	TokenObservation obsArray[MAX_SENTENCE_TOKENS+2];
	for (int i = 0; i < MAX_SENTENCE_TOKENS+2; i++)
		observations[i] = &obsArray[i];
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(0, 0, Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	int n_observations = sentence.getLength() + 2;

	static_cast<TokenObservation*>(observations[0])->populate(
		blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
	for (int j = 0; j < sentence.getLength(); j++) {
		PIdFDecoder::populateObservation(
			static_cast<TokenObservation*>(observations[j+1]),
			_wordFeatures, sentence.getWord(j), j == 0,
				_decoder == _lowerCaseDecoder);
	}
	static_cast<TokenObservation*>(observations[n_observations - 1])
		->populate(blankToken, blankLCSymbol, blankWordFeatures,
					blankWordClass, 0, 0);

	_decoder->decode(n_observations, observations, tags);

	for (int k = 0; k < sentence.getLength(); k++)
		sentence.setTag(k, tags[k+1]);
}


int PIdFDecoder::getNameTheories(NameTheory **results, int max_theories,
								 TokenSequence *tokenSequence)
{
	PIdFSentence sentence(_tagSet, *tokenSequence);
	DTObservation *observations[MAX_SENTENCE_TOKENS+2];
	TokenObservation obsArray[MAX_SENTENCE_TOKENS+2];
	for (int i = 0; i < MAX_SENTENCE_TOKENS+2; i++)
		observations[i] = &obsArray[i];
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(0, 0, Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	int n_observations = sentence.getLength() + 2;

	static_cast<TokenObservation*>(observations[0])->populate(
		blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
	for (int j = 0; j < sentence.getLength(); j++) {
		PIdFDecoder::populateObservation(
			static_cast<TokenObservation*>(observations[j+1]),
			_wordFeatures, sentence.getWord(j), j == 0,
				_decoder == _lowerCaseDecoder);
	}
	static_cast<TokenObservation*>(observations[n_observations - 1])
		->populate(blankToken, blankLCSymbol, blankWordFeatures,
					blankWordClass, 0, 0);

	_decoder->decode(n_observations, observations, tags);

	for (int k = 0; k < sentence.getLength(); k++){
		sentence.setTag(k, tags[k+1]);
	}

	results[0] = makeNameTheory(sentence);
	return 1;
}


void PIdFDecoder::populateObservation(TokenObservation *observation,
									  IdFWordFeatures *wordFeatures,
									  Symbol word, bool first_word,
									  bool lowercase)
{
	Token token(0, 0, word);

	std::wstring buf(word.to_string());
	boost::to_lower(buf);
	Symbol lcSymbol(buf.c_str());

	Symbol idfWordFeature = wordFeatures->features(word, first_word, false);
	WordClusterClass wordClass(word, lowercase);


	observation->populate(token, lcSymbol, idfWordFeature, wordClass, 0, 0);
}


NameTheory *PIdFDecoder::makeNameTheory(PIdFSentence &sentence) {
	int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);

	int n_name_spans = 0;
	for (int j = 0; j < sentence.getLength(); j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			_tagSet->isSTTag(sentence.getTag(j)))
		{
			n_name_spans++;
		}
	}

	NameTheory *nameTheory = _new NameTheory();
	nameTheory->n_name_spans = n_name_spans;
	nameTheory->nameSpans = _new NameSpan*[n_name_spans];

	int tok_index = 0;
	for (int i = 0; i < n_name_spans; i++) {
		while (!(sentence.getTag(tok_index) != NONE_ST_tag &&
				 _tagSet->isSTTag(sentence.getTag(tok_index))))
		{ tok_index++; }

		int tag = sentence.getTag(tok_index);

		int end_index = tok_index;
		while (end_index+1 < sentence.getLength() &&
			   _tagSet->isCOTag(sentence.getTag(end_index+1)))
		{ end_index++; }

		nameTheory->nameSpans[i] = _new NameSpan(tok_index, end_index,
			EntityType(_tagSet->getReducedTagSymbol(tag)));

		tok_index = end_index + 1;
	}

	return nameTheory;
}


