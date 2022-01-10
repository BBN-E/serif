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
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"
#include "Generic/PNPChunking/PNPChunkFeatureTypes.h"
#include "Generic/PNPChunking/PNPChunkSentence.h"
#include "Generic/PNPChunking/PNPChunkDecoder.h"
#include "Generic/PNPChunking/PNPChunkTrainer.h"

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <boost/scoped_ptr.hpp>

using namespace std;


PNPChunkTrainer::PNPChunkTrainer()
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0), //_weightSums(0),
	  _print_last_weights_after_every_epoch(false), _print_sum_weights_after_every_epoch(false)
{
	PNPChunkFeatureTypes::ensureFeatureTypesInstantiated();

	// Read parameters

	_epochs = ParamReader::getRequiredIntParam("pnpchunk_trainer_epochs");

	std::string training_file = ParamReader::getRequiredParam("pnpchunk_training_file");

	if (ParamReader::isParamTrue("pnpchunk_trainingfile_is_list")) {
		readTrainingFileList(training_file.c_str());
	} else {
		_training_files.push_back(training_file);
	}

	_print_last_weights_after_every_epoch = ParamReader::isParamTrue("print_last_weights_every_epoch");
	_print_sum_weights_after_every_epoch = ParamReader::isParamTrue("print_sum_weights_every_epoch");

	_model_file = ParamReader::getRequiredParam("pnpchunk_model_file");

	std::string features_file = ParamReader::getRequiredParam("pnpchunk_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), PNPChunkFeatureType::modeltype);

	std::string tag_set_file = ParamReader::getRequiredParam("pnpchunk_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), true, true);

	_wordFeatures = IdFWordFeatures::build();

	WordClusterTable::ensureInitializedFromParamFile();
}

PNPChunkTrainer::~PNPChunkTrainer() {
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;
}


void PNPChunkTrainer::train() {
	_weights = _new DTFeature::FeatureWeightMap(500009);
	//_weightSums = _new DTFeature::FeatureWeightMap(500009);
	_decoder = _new PDecoder(_tagSet, _featureTypes, _weights, true, true);
	for (int epoch = 0; epoch < _epochs; epoch++) {
		trainEpoch();
		if (_print_last_weights_after_every_epoch) {
			SessionLogger::info("SERIF") << "Writing last weights: " << epoch + 1 << std::endl;
			writeWeights(epoch + 1);
		}
		if (_print_sum_weights_after_every_epoch) {
			SessionLogger::info("SERIF") << "Writing sum weights: " << epoch + 1 << std::endl;
			writeLazySumWeights(_decoder->getNHypotheses(), epoch + 1);
		}
	}

	writeLazySumWeights(_decoder->getNHypotheses());


//	cerr << "Press enter to free weight tables and decoder...\n";
//	getchar();

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		(*iter).first->deallocate();
	}
	delete _decoder;
	delete _weights;
	//delete _weightSums;

//	cerr << "Press enter to continue...\n";
//	getchar();
}

void PNPChunkTrainer::trainEpoch() {
	int sentence_n = 0;
	for (size_t i = 0; i < _training_files.size(); i++) {
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		SessionLogger::info("SERIF")<<"read: "<<_training_files.at(i)<<std::endl;
		in.open(_training_files.at(i).c_str());
		if (in.fail()) {
			throw UnexpectedInputException("PNPChunkTrainer::train()",
				"Unable to open training file.");
		}

		PNPChunkSentence npchunkSentence(_tagSet, MAX_SENTENCE_TOKENS);

		std::vector<DTObservation *> observations;
		int tags[MAX_SENTENCE_TOKENS+4];

		Token blankToken(Symbol(L"NULL"));
		Symbol blankLCSymbol = Symbol(L"NULL");
		Symbol blankWordFeatures = Symbol(L"NULL");
		Symbol blankPOS = Symbol(L"NULL");
		WordClusterClass blankWordClass = WordClusterClass::nullCluster();

		while (npchunkSentence.readPOSTrainingSentence(in)) {
			int n_observations = npchunkSentence.getLength() + 4;

			static_cast<TokenPOSObservation*>(observations[0])->populate(
				blankToken, blankPOS, blankLCSymbol, blankWordFeatures, blankWordClass);
			static_cast<TokenPOSObservation*>(observations[1])->populate(
				blankToken, blankPOS, blankLCSymbol, blankWordFeatures, blankWordClass);
			for (int i = 0; i < npchunkSentence.getLength(); i++) {
				PNPChunkDecoder::populateObservation(
					static_cast<TokenPOSObservation*>(observations[i+2]),
					_wordFeatures, npchunkSentence.getWord(i), npchunkSentence.getPOS(i),
					i == 0);
			}
			static_cast<TokenPOSObservation*>(observations[n_observations - 1])
				->populate(blankToken, blankPOS, blankLCSymbol, blankWordFeatures,
						   blankWordClass);
			static_cast<TokenPOSObservation*>(observations[n_observations - 2])
				->populate(blankToken, blankPOS, blankLCSymbol, blankWordFeatures,
						   blankWordClass);

			tags[0] = _tagSet->getStartTagIndex();
			tags[1] = _tagSet->getStartTagIndex();
			for (int j = 0; j < npchunkSentence.getLength(); j++)
				tags[j+2] = npchunkSentence.getTag(j);
			tags[n_observations-1] = _tagSet->getEndTagIndex();
			tags[n_observations-2] = _tagSet->getEndTagIndex();

			_decoder->train(observations, tags, 2);
			//DTFeature::addWeightsToSum(_weights);
			/*
			for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
				 iter != _weights->end(); ++iter)
			{
				*(*_weightSums)[(*iter).first] += *(*iter).second;
			}
			*/
			if (sentence_n % 1000 == 0) {
				cout << sentence_n << ": " << (int) _weights->size() << "\n";
			/*		<< (int)_weights->get_path_length() << "\r";
					<< (int)_weights->get_num_lookup_eqs() << "/"
					<< (int)_weights->get_num_lookups() << "\r"; */
				cout.flush();
			}


			sentence_n++;
		}

		cout << "\n";
		in.close();
	}
}

void PNPChunkTrainer::writeWeights(int epoch) {
	if(epoch == -1){
		writeWeights("");
	}
	else{
		char buffer[50];
		sprintf(buffer, "last-epoch-%d", epoch);
		writeWeights(buffer);
	}
}

void PNPChunkTrainer::writeWeights(const char* str) {
	UTF8OutputStream out;

	std::string file;
	if(strcmp("", str)== 0 ){
		file = _model_file;
	}else{
		std::string new_str(str);
		file = _model_file + "-" + new_str;
	}
	out.open(file.c_str());

	if (out.fail()) {
		throw UnexpectedInputException("PNPChunkTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	DTFeature::writeWeights(*_weights, out, false);

	out.close();
}


void PNPChunkTrainer::writeLazySumWeights(long n_hypotheses, int epoch) {
	if(epoch == -1){
		writeLazySumWeights("", n_hypotheses);
	}
	else{
		char buffer[50];
		sprintf(buffer, "epoch-%d", epoch);
		writeLazySumWeights(buffer, n_hypotheses);
	}
}

void PNPChunkTrainer::writeLazySumWeights(const char* str, long n_hypotheses) {
	UTF8OutputStream out;

	std::string file;
	if(strcmp("", str)== 0 ){
		file = _model_file;
	}else{
		std::string new_str(str);
		file = _model_file + "-" + new_str;
	}
	out.open(file.c_str());

	if (out.fail()) {
		throw UnexpectedInputException("PNPChunkTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	DTFeature::writeLazySumWeights(*_weights, out, n_hypotheses, false);

	out.close();

/*
	for (DTFeature::FeatureWeightMap::iterator iter = _weightSums->begin();
		 iter != _weightSums->end(); ++iter)
	{
		DTFeature *feature = (*iter).first;

		out << L"((" << feature->getFeatureType()->getName().to_string()
			<< L" ";
		feature->write(out);
		out << L") " << *(*iter).second << L")\n";
	}

	out.close();
	*/

}

void PNPChunkTrainer::readTrainingFileList(const char *training_list_file) {
	ifstream in;
	in.open(training_list_file);
	if (in.fail()) {
		throw UnexpectedInputException("PNPChunkTrainer::readTrainingFileList()",
			"Unable to open training list file.");
	}

	int num_training_files;
	in >> num_training_files;

	string line;
	for (int i = 0; i < num_training_files; i++) {
		in >> line;
		if (in.fail())
			throw UnexpectedInputException("PNPChunkTrainer::readTrainingFileList()",
				"Unable to read training list file.");

		if (line[line.length() - 1] == '\r')
			line.resize(line.length() - 1);

		_training_files.push_back(line);
	}
	in.close();
}
