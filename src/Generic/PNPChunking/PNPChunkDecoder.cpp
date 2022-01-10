// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"

#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"
#include "Generic/PNPChunking/PNPChunkFeatureTypes.h"
#include "Generic/PNPChunking/PNPChunkSentence.h"
#include "Generic/PNPChunking/PNPChunkDecoder.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"

#include "Generic/theories/NPChunkTheory.h"

#include <iostream>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>

using namespace std;


PNPChunkDecoder::PNPChunkDecoder()
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0)
{
	PNPChunkFeatureTypes::ensureFeatureTypesInstantiated();

	// Read parameters
	std::string tag_set_file = ParamReader::getRequiredParam("pnpchunk_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), true, true);

	std::string features_file = ParamReader::getRequiredParam("pnpchunk_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), PNPChunkFeatureType::modeltype);
	
	std::string model_file = ParamReader::getRequiredParam("pnpchunk_model_file");

	_wordFeatures = IdFWordFeatures::build();

	WordClusterTable::ensureInitializedFromParamFile();

	_weights = _new DTFeature::FeatureWeightMap(500009);
	_decoder = _new PDecoder(_tagSet, _featureTypes, _weights);
	DTFeature::readWeights(*_weights, model_file.c_str(), PNPChunkFeatureType::modeltype);

	/*_obsArray = _new TokenPOSObservation[MAX_SENTENCE_TOKENS+4];
	for (int i = 0; i < MAX_SENTENCE_TOKENS + 4; i++)
		_observations[i] = &_obsArray[i];
		*/
}

PNPChunkDecoder::~PNPChunkDecoder() {
	delete _decoder;
	delete _weights;
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;

	for(vector<DTObservation*>::iterator i = _observations.begin(); i != _observations.end(); ++i) {
		delete *i;
	}

	_observations.clear();
}


void PNPChunkDecoder::decode() {
	std::string input_file = ParamReader::getRequiredParam("pnpchunk_input_file");
	std::string output_file = ParamReader::getRequiredParam("pnpchunk_output_file");

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(input_file.c_str());
	if (in.fail()) {
		throw UnexpectedInputException("PNPChunkDecoder::decode()",
			"Could not open input file for reading");
	}

	UTF8OutputStream out;
	out.open(output_file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PNPChunkDecoder::decode()",
			"Could not create output file");
	}

	decode(in, out);
}

void PNPChunkDecoder::decode(UTF8InputStream &in, UTF8OutputStream &out) {
	PNPChunkSentence npchunkSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int tags[MAX_SENTENCE_TOKENS+4];

	Token blankToken(Symbol(L"NULL"));
	Symbol blankPOSSymbol = Symbol(L"NULL");
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	int sentence_n = 0;
	while (npchunkSentence.readPOSTestSentence(in)) {
		int n_observations = npchunkSentence.getLength() + 4;

		static_cast<TokenPOSObservation*>(_observations[0])->populate(
			blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures, blankWordClass);
		static_cast<TokenPOSObservation*>(_observations[1])->populate(
			blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures, blankWordClass);

		for (int i = 0; i < npchunkSentence.getLength(); i++) {
			PNPChunkDecoder::populateObservation(
				static_cast<TokenPOSObservation*>(_observations[i+2]),
				_wordFeatures, npchunkSentence.getWord(i), npchunkSentence.getPOS(i),
				i == 0);
		}
		static_cast<TokenPOSObservation*>(_observations[n_observations - 1])
			->populate(blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures,
					   blankWordClass);
		static_cast<TokenPOSObservation*>(_observations[n_observations - 2])
			->populate(blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures,
					   blankWordClass);


		_decoder->decode(_observations, tags);

		for (int j = 0; j < npchunkSentence.getLength(); j++)
			npchunkSentence.setTag(j, tags[j+2]);

		npchunkSentence.writeSexp(out);

		cout << sentence_n << "\r";

		sentence_n++;
	}

	cout << "\n";
}

void PNPChunkDecoder::decode(const TokenSequence* tokens, const Symbol *pos, PNPChunkSentence*& sentence) {
	Token blankToken(Symbol(L"NULL"));
	Symbol blankPOSSymbol = Symbol(L"NULL");
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	int tags[MAX_SENTENCE_TOKENS+4];

	//clear the observations vector, so we don't have anything from previous sentences
	for(vector<DTObservation*>::iterator i = _observations.begin(); i != _observations.end(); ++i) {
		delete *i;
	}
	_observations.clear();

	_observations.push_back(new TokenPOSObservation());
	static_cast<TokenPOSObservation*>(_observations[0])->populate(
		blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures, blankWordClass);
	_observations.push_back(new TokenPOSObservation());
	static_cast<TokenPOSObservation*>(_observations[1])->populate(
		blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures, blankWordClass);

	for (int j = 0; j < tokens->getNTokens(); j++) {
		_observations.push_back(new TokenPOSObservation());
		PNPChunkDecoder::populateObservation(
			static_cast<TokenPOSObservation*>(_observations[j+2]),
			_wordFeatures, tokens->getToken(j), pos[j],
			j == 0);
	}
	_observations.push_back(new TokenPOSObservation());
	static_cast<TokenPOSObservation*>(_observations.back())
		->populate(blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures,
					blankWordClass);
	_observations.push_back(new TokenPOSObservation());
	static_cast<TokenPOSObservation*>(_observations.back())
		->populate(blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures,
					blankWordClass);

	_decoder->decode(_observations, tags);

	sentence = _new PNPChunkSentence(_tagSet, MAX_SENTENCE_TOKENS, tokens, pos);

	for (int k = 2; k < (int)(_observations.size()) - 2; k++) {
		sentence->setTag(k-2, tags[k]);
	}
}

void PNPChunkDecoder::devTest() {
	std::string input_file = ParamReader::getRequiredParam("pnpchunk_training_file");
	std::string output_file = ParamReader::getRequiredParam("pnpchunk_devtest_output_file");

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(input_file.c_str());
	if (in.fail()) {
		throw UnexpectedInputException("PNPChunkDecoder::devTest()",
			"Could not open input file for reading");
	}

	UTF8OutputStream out;
	out.open(output_file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PNPChunkDecoder::devTest()",
			"Could not create output file");
	}

	devTest(in, out);
}

void PNPChunkDecoder::devTest(UTF8InputStream &in, UTF8OutputStream &out) {
	PNPChunkSentence npchunkSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int tags[MAX_SENTENCE_TOKENS+4];

	Token blankToken(Symbol(L"NULL"));
	Symbol blankPOSSymbol = Symbol(L"NULL");
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	int sentence_n = 0;
	int n_words = 0;
	int n_tags_correct = 0;
	while (npchunkSentence.readPOSTrainingSentence(in)) {
		int n_observations = npchunkSentence.getLength() + 4;

		static_cast<TokenPOSObservation*>(_observations[0])->populate(
			blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures, blankWordClass);
		static_cast<TokenPOSObservation*>(_observations[1])->populate(
			blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures, blankWordClass);

		for (int i = 0; i < npchunkSentence.getLength(); i++) {
			PNPChunkDecoder::populateObservation(
				static_cast<TokenPOSObservation*>(_observations[i+2]),
				_wordFeatures, npchunkSentence.getWord(i), npchunkSentence.getPOS(i),
				i == 0);
		}
		static_cast<TokenPOSObservation*>(_observations[n_observations - 1])
			->populate(blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures,
					   blankWordClass);
		static_cast<TokenPOSObservation*>(_observations[n_observations - 2])
			->populate(blankToken, blankPOSSymbol, blankLCSymbol, blankWordFeatures,
					   blankWordClass);


		_decoder->decode(_observations, tags);

		for (int j = 0; j < npchunkSentence.getLength(); j++) {
			if (npchunkSentence.getTag(j) == tags[j+2]) {
				n_tags_correct++;
			}
			npchunkSentence.setTag(j, tags[j+2]);
			n_words++;
		}
		npchunkSentence.writeSexp(out);

		cout << sentence_n << "\r";

		sentence_n++;
	}

	double percent = ((double) n_tags_correct / n_words) * 100;
	out << percent << "% of tags correct\n";
	cout << "\n";
}


void PNPChunkDecoder::populateObservation(TokenPOSObservation *observation,
									  IdFWordFeatures *wordFeatures,
									  const Token* wordtoken, Symbol pos, bool first_word)
{
	populateObservation(observation, wordFeatures, wordtoken->getSymbol(), pos, first_word);
}

void PNPChunkDecoder::populateObservation(TokenPOSObservation *observation,
									  IdFWordFeatures *wordFeatures,
									  Symbol word, Symbol pos, bool first_word)
{
	Token token(word);

	std::wstring buf(word.to_string());
	std::transform(buf.begin(), buf.end(), buf.begin(), towlower);
	Symbol lcSymbol(buf.c_str());

	Symbol idfWordFeature = wordFeatures->features(word, first_word, false);

	WordClusterClass wordClass(word);

	observation->populate(token, pos, lcSymbol, idfWordFeature, wordClass);
}

