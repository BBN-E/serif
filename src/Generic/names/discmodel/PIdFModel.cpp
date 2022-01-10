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
#include "Generic/discTagger/BlockFeatureTable.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/names/discmodel/PIdFFeatureTypes.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <boost/algorithm/string.hpp>

//need to order sentences in constraint decoding...
//just use types in PIdFSimActiveLearning
#include "Generic/names/discmodel/PIdFModel.h"
#include <boost/scoped_ptr.hpp>

using namespace std;


Symbol PIdFModel::_NONE_ST = Symbol(L"NONE-ST");
Symbol PIdFModel::_NONE_CO = Symbol(L"NONE-CO");

Token PIdFModel::_blankToken(Symbol(L"NULL"));
Symbol PIdFModel::_blankLCSymbol = Symbol(L"NULL");
Symbol PIdFModel::_blankWordFeatures = Symbol(L"NULL");
WordClusterClass PIdFModel::_blankWordClass = WordClusterClass::nullCluster();

PIdFModel::PIdFModel(model_mode_e mode, const char* tag_set_file,
		const char* features_file, const char* model_file, const char* vocab_file,
		bool learn_transitions, bool use_clusters)
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _vocab(0), _defaultDecoder(0), _lowerCaseDecoder(0),
	  _defaultWeights(0), _lcWeights(0), _defaultVocab(0), _lcVocab(0),
	  _defaultBlockWeights(0), _lcBlockWeights(0),
	  _firstSentBlock(0), _lastSentBlock(0), _curSentBlock(0), _cur_sent_no(0),
	  _min_tot(1), _min_change(0),
	  _firstALSentBlock(0), _lastALSentBlock(0),
	  _curALSentBlock(0), _n_ALSentences(0),
	  _tagSpecificScoredSentences(0), _bigramFrequency(0), _mult(0),
	  _writeHistory(false), _print_after_every_epoch(false),
	  _use_stored_vocabulary(false), _print_sentence_selection_info(false),
	  _use_fast_training(false), _use_clusters(use_clusters)
{
	// NOTE: non-param-file mode always has _use_stored_vocabulary = false
	// This could be fixed by passing in a parameter to initialize the vocab table,
	// if someone cares to do so.
		
	_model_mode = mode;
	if (_model_mode != PIdFModel::DECODE) {
		throw UnrecoverableException("PIdFModel::PIdFModel()",
			"Non param-file PIdFModel can only be called in Decode mode");
	}
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();
	_interleave_tags = false;

	_tagSet = _new DTTagSet(tag_set_file, true, true,  _interleave_tags);
	_learn_transitions_from_training = learn_transitions;

	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	_wordFeatures = IdFWordFeatures::build();
	if (_use_clusters)
		WordClusterTable::ensureInitializedFromParamFile();

	if (_learn_transitions_from_training) {
		std::string transition_file = _model_file + "-transitions";
		_tagSet->readTransitions(transition_file.c_str());
	}

	_defaultBlockWeights = _new BlockFeatureTable(_tagSet);
	DTFeature::readWeights(*_defaultBlockWeights, model_file, PIdFFeatureType::modeltype);
	_defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultBlockWeights);

	_decoder = _defaultDecoder;
	_vocab = _defaultVocab;
	_secondaryDecoders = 0;
	_secondaryDecoders = _new PIdFSecondaryDecoders();
}

PIdFModel::PIdFModel(model_mode_e mode, const char* tag_set_file,
		const char* features_file, const char* model_file, const char* vocab_file, const char* lc_model_file,
		const char* lc_vocab_file, bool learn_transitions, bool use_clusters)
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _vocab(0), _defaultDecoder(0), _lowerCaseDecoder(0),
	  _defaultWeights(0), _lcWeights(0), _defaultVocab(0), _lcVocab(0),
	  _defaultBlockWeights(0), _lcBlockWeights(0),
	  _firstSentBlock(0), _lastSentBlock(0), _curSentBlock(0), _cur_sent_no(0),
	  _min_tot(1), _min_change(0),
	  _firstALSentBlock(0), _lastALSentBlock(0),
	  _curALSentBlock(0), _n_ALSentences(0),
	  _tagSpecificScoredSentences(0), _bigramFrequency(0), _mult(0),
	  _writeHistory(false), _print_after_every_epoch(false),
	  _use_stored_vocabulary(false), _print_sentence_selection_info(false),
	  _use_fast_training(false), _use_clusters(use_clusters)
{
	// NOTE: non-param-file mode always has _use_stored_vocabulary = false
	// This could be fixed by passing in a parameter to initialize the vocab table,
	// if someone cares to do so.
		_model_mode = mode;
	if (_model_mode != PIdFModel::DECODE) {
		throw UnrecoverableException("PIdFModel::PIdFModel()",
			"Non param-file PIdFModel can only be called in Decode mode");
	}
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();
	_interleave_tags = false;

	_tagSet = _new DTTagSet(tag_set_file, true, true,  _interleave_tags);
	_learn_transitions_from_training = learn_transitions;

	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	_wordFeatures = IdFWordFeatures::build();
	if (_use_clusters)
		WordClusterTable::ensureInitializedFromParamFile();

	if (_learn_transitions_from_training) {
		std::string transition_file = _model_file + "-transitions";
		_tagSet->readTransitions(transition_file.c_str());
	}

	_defaultBlockWeights = _new BlockFeatureTable(_tagSet);
	DTFeature::readWeights(*_defaultBlockWeights, model_file, PIdFFeatureType::modeltype);
	_defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultBlockWeights);

	_lcBlockWeights = _new BlockFeatureTable(_tagSet);
	DTFeature::readWeights(*_lcBlockWeights, lc_model_file, PIdFFeatureType::modeltype);
	_lowerCaseDecoder = _new PDecoder(_tagSet, _featureTypes, _lcBlockWeights);

	_decoder = _defaultDecoder;
	_vocab = _defaultVocab;
	_secondaryDecoders = 0;
	_secondaryDecoders = _new PIdFSecondaryDecoders();
}

PIdFModel::PIdFModel(model_mode_e mode)
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _vocab(0), _defaultDecoder(0), _lowerCaseDecoder(0),
	  _defaultWeights(0), _lcWeights(0), _defaultVocab(0), _lcVocab(0),
	  _defaultBlockWeights(0), _lcBlockWeights(0),
	  _firstSentBlock(0), _lastSentBlock(0), _curSentBlock(0), _cur_sent_no(0),
	  _min_tot(1), _min_change(0),
	  _firstALSentBlock(0), _lastALSentBlock(0),
	  _curALSentBlock(0), _n_ALSentences(0),
	  _tagSpecificScoredSentences(0), _bigramFrequency(0), _mult(0),
	  _writeHistory(false), _print_after_every_epoch(false),
	  _use_stored_vocabulary(false), _print_sentence_selection_info(false),
	  _defaultBigramVocab(0), _use_fast_training(false)	  
{
	_model_mode = mode;

	PIdFFeatureTypes::ensureFeatureTypesInstantiated();
	_wordFeatures = IdFWordFeatures::build();

	_use_clusters = ParamReader::getOptionalTrueFalseParamWithDefaultVal("pidf_use_clusters", true);
	if (_use_clusters)
		WordClusterTable::ensureInitializedFromParamFile();

	_interleave_tags = ParamReader::getRequiredTrueFalseParam("pidf_interleave_tags");

	std::string param_file = ParamReader::getRequiredParam("pidf_tag_set_file");
	_tagSet = _new DTTagSet(param_file.c_str(), true, true,  _interleave_tags);

	_model_file = ParamReader::getRequiredParam("pidf_model_file");
	
	_vocab_file = ParamReader::getParam("pidf_vocab_file");
	if (!_vocab_file.empty()) {
		_use_stored_vocabulary = true;
	} else _use_stored_vocabulary = false;

	_bigram_file = ParamReader::getParam("pidf_bigram_vocab_file");
	if (!_bigram_file.empty()) {
		_use_stored_bigram = true;
	} else _use_stored_bigram = false;

	_learn_transitions_from_training = ParamReader::getRequiredTrueFalseParam("pidf_learn_transitions");
	
	param_file = ParamReader::getRequiredParam("pidf_features_file");
	_featureTypes = _new DTFeatureTypeSet(param_file.c_str(), PIdFFeatureType::modeltype);

	if (_model_mode == DECODE) {

		if (ParamReader::isParamTrue("restrict_features_with_vocab")) {
			if(!_use_stored_vocabulary) {
				throw InternalInconsistencyException(
					"PIdFModel::PIdFModel()",
					"Trying to restrict features by vocab, but no pidf_vocab_file parameter specified!");
			}
			PIdFFeatureType::setUnigramVocab(_vocab_file.c_str());
		}
		if (ParamReader::isParamTrue("restrict_features_with_bigram_vocab")) {
			if(!_use_stored_bigram) {
				throw InternalInconsistencyException(
					"PIdFModel::PIdFModel()",
					"Trying to restrict features by bigram vocab, but no pidf_bigram_vocab_file parameter specified!");
			}
			PIdFFeatureType::setBigramVocab(_bigram_file.c_str());
		}
		 
		if (_learn_transitions_from_training && (_model_mode == DECODE)) {
			std::string transition_file = _model_file + "-transitions";
			_tagSet->readTransitions(transition_file.c_str());
		}
		
		if (true) {
			_defaultBlockWeights = _new BlockFeatureTable(_tagSet);
			DTFeature::readWeights(*_defaultBlockWeights, _model_file.c_str(), PIdFFeatureType::modeltype);
			_defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultBlockWeights);
		}
		else { // non-optimized feature weights data structure - can be helpful for debugging
			_defaultWeights = _new DTFeature::FeatureWeightMap(500009);
			DTFeature::readWeights(*_defaultWeights, _model_file.c_str(), PIdFFeatureType::modeltype);
			_defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultWeights);
		}

		if (_use_stored_vocabulary) {
			boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(_vocab_file.c_str()));
			UTF8InputStream& in(*in_scoped_ptr);
			_defaultVocab = _new NgramScoreTable(1, in);
			in.close();
		}
		else _defaultVocab = 0;

		if (_use_stored_bigram) {
			boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(_bigram_file.c_str()));
			UTF8InputStream& in(*in_scoped_ptr);
			_defaultBigramVocab = _new NgramScoreTable(2, in);
			in.close();
		}
		else _defaultBigramVocab = 0;

		// This should only be turned on if sentence selection is the only thing you are doing!
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("print_sentence_selection_info", false);

		SessionLogger &logger = *SessionLogger::logger;
		std::string lc_model_file = ParamReader::getParam("lowercase_pidf_model_file");
		if (!lc_model_file.empty()) {
			_lcBlockWeights = _new BlockFeatureTable(_tagSet);
			DTFeature::readWeights(*_lcBlockWeights, lc_model_file.c_str(), PIdFFeatureType::modeltype);
			_lowerCaseDecoder = _new PDecoder(_tagSet, _featureTypes, _lcBlockWeights);

			std::string lc_vocab_file = ParamReader::getParam("lowercase_pidf_vocab_file");
			if (_use_stored_vocabulary && !lc_vocab_file.empty()) {
				boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(lc_vocab_file.c_str()));
				UTF8InputStream& in(*in_scoped_ptr);
				_lcVocab = _new NgramScoreTable(1, in);
				in.close();
			} else _lcVocab = 0;
		}

    }
    if (_model_mode == DECODE_AND_CHOOSE) {
        // use FeatureWeightMap instead of BlockFeatureTable
        // also add some variables to do with choosing new sentences
        // otherwise the same initialization as DECODE
		_defaultWeights = _new DTFeature::FeatureWeightMap(500009);
        DTFeature::readWeights(*_defaultWeights, _model_file.c_str(), PIdFFeatureType::modeltype);
        _defaultDecoder = _new PDecoder(_tagSet, _featureTypes, _defaultWeights);
		if (_use_stored_vocabulary) {
			boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(_vocab_file.c_str()));
			UTF8InputStream& in(*in_scoped_ptr);
			_defaultVocab = _new NgramScoreTable(1, in);
			in.close();
		}
		else _defaultVocab = 0;

		if (_use_stored_bigram) {
			boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(_bigram_file.c_str()));
			UTF8InputStream& in(*in_scoped_ptr);
			_defaultBigramVocab = _new NgramScoreTable(2, in);
			in.close();
		}
		else _defaultBigramVocab = 0;

		// This should only be turned on if sentence selection is the only thing you are doing!
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("print_sentence_selection_info", false);

		SessionLogger &logger = *SessionLogger::logger;
		std::string lc_model_file = ParamReader::getParam("lowercase_pidf_model_file");
		if (!lc_model_file.empty()) {
			_lcBlockWeights = _new BlockFeatureTable(_tagSet);
			DTFeature::readWeights(*_lcBlockWeights, lc_model_file.c_str(), PIdFFeatureType::modeltype);
			_lowerCaseDecoder = _new PDecoder(_tagSet, _featureTypes, _lcBlockWeights);

			std::string lc_vocab_file = ParamReader::getParam("lowercase_pidf_vocab_file");
			if (_use_stored_vocabulary && !lc_vocab_file.empty()) {
				boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(lc_vocab_file.c_str()));
				UTF8InputStream& in(*in_scoped_ptr);
				_lcVocab = _new NgramScoreTable(1, in);
				in.close();
			} else _lcVocab = 0;
		}

        _nToAdd = ParamReader::getRequiredIntParam("pidf_active_learning_sent_to_add");
        _allowSentenceRepeats = ParamReader::isParamTrue("pidf_active_learning_allow_repeats");
        if (ParamReader::hasParam("percent_specific_tags")) {
          _percentTagSpecific = ParamReader::getRequiredFloatParam("percent_specific_tags");
          _nStoredTags = (_tagSet->getNRegularTags()-2)/2;
          _tagSpecificScoredSentences = _new ScoredSentenceVector[_nStoredTags];
        }
        else {
          _percentTagSpecific = 0;
          _nStoredTags = 0;
        }
        
    }
	
	else if ((_model_mode == TRAIN) ||
		     (_model_mode == SIM_AL) || 
			 (_model_mode == UNSUP) ||
			 (_model_mode == UNSUP_CHILD) || 
			 (_model_mode == TRAIN_AND_DECODE))
	{
		_defaultWeights = _new DTFeature::FeatureWeightMap(500009);

		Symbol outputMode = ParamReader::getParam(L"pidf_trainer_output_mode");
		if (outputMode == Symbol(L"taciturn"))		_output_mode = TACITURN_OUTPUT;
		else if (outputMode == Symbol(L"verbose"))  _output_mode = VERBOSE_OUTPUT;
		else {
			throw UnexpectedInputException("PIdFModel::PIdFModel()",
				"Parameter 'pidf_trainer_output_mode' not specified");
		}

		_seed_features = ParamReader::getRequiredTrueFalseParam("pidf_trainer_seed_features");
		_add_hyp_features = ParamReader::getRequiredTrueFalseParam("pidf_trainer_add_hyp_features");
		_epochs = ParamReader::getRequiredIntParam("pidf_trainer_epochs");
		_use_fast_training = ParamReader::getOptionalTrueFalseParamWithDefaultVal("pidf_trainer_use_fast_training", false);

		_required_margin = ParamReader::getOptionalFloatParamWithDefaultValue("pidf_trainer_required_margin", 1.0);
		if (_required_margin <= 0.0) {
			throw UnexpectedInputException("PIdFModel::PIdFModel()",
				"Invalid parameter value for 'pidf_trainer_required_margin'. The value has to be > 0.0.");
		}

		_print_after_every_epoch = ParamReader::isParamTrue("pidf_print_model_every_epoch");
	

		_min_tot = ParamReader::getOptionalFloatParamWithDefaultValue("pidf_trainer_min_tot", 1);
		_min_change = ParamReader::getOptionalFloatParamWithDefaultValue("pidf_trainer_min_change", 0);
		_min_change = _min_change/100;
		_weightsum_granularity = ParamReader::getRequiredIntParam("pidf_trainer_weightsum_granularity");
			
		if ((_model_mode == TRAIN) || (_model_mode == TRAIN_AND_DECODE)) {
			_nTrainSent = ParamReader::getOptionalIntParamWithDefaultValue("pidf_ntrainsent", 0);
		}
		else if (_model_mode == SIM_AL) {
			
			_nToAdd = ParamReader::getRequiredIntParam("pidf_active_learning_sent_to_add");
			_nActiveLearningIter = ParamReader::getRequiredIntParam("pidf_active_learning_epochs");
			_allowSentenceRepeats = ParamReader::isParamTrue("pidf_active_learning_allow_repeats");

			_nTrainSent = 0;
			if (ParamReader::getParam("pidf_training_file") == "") {			
				// if there is no training file, pidf_ntrainset is required
				_nTrainSent = ParamReader::getOptionalIntParamWithDefaultValue("pidf_ntrainsent", 0);
				if (_nTrainSent == 0)
					throw UnexpectedInputException("PIdFModel::PIdFModel()",
						"Neither Parameter 'pidf_training_file' or 'pidf_ntrainsent' specified");
			}

			_nStoredTags = 0;
			_percentTagSpecific = 0;
			if (ParamReader::hasParam("pidf_sim_al_percent_tag")) {
				_percentTagSpecific = ParamReader::getRequiredFloatParam("pidf_sim_al_percent_tag");
				_nStoredTags = (_tagSet->getNRegularTags()-2)/2;
				_tagSpecificScoredSentences = _new ScoredSentenceVector[_nStoredTags];
			}

			std::string filename = ParamReader::getParam("bigram_freq");
			if (!filename.empty()) {
				boost::scoped_ptr<UTF8InputStream> bistream_scoped_ptr(UTF8InputStream::build());
				UTF8InputStream& bistream(*bistream_scoped_ptr);
				bistream.open(filename.c_str());
				_bigramFrequency = _new NgramScoreTable(2, bistream);
				int count;
				bistream >> count;
				bistream.close();
				NgramScoreTable::Table::iterator start = _bigramFrequency->get_start();
				NgramScoreTable::Table::iterator end = _bigramFrequency->get_end();
				NgramScoreTable::Table::iterator curr = start;
				while (curr != end) {
					float f = (*curr).second;
					double d = static_cast<double>(f);
					(*curr).second = (float)log(1- d/ (double)count);
					++curr;
				}
				_mult = ParamReader::getRequiredFloatParam("bigram_mult");
			}
		}
		else if (_model_mode == UNSUP) {
			_nToAdd = ParamReader::getRequiredIntParam("pidf_unsup_sent_to_add");
			_nActiveLearningIter = ParamReader::getRequiredIntParam("pidf_unsup_epochs");
			_allowSentenceRepeats = false;
			_nTrainSent = ParamReader::getRequiredIntParam("pdif_ntrainsent");
			_n_child_models = ParamReader::getRequiredIntParam("n_bagged_models");
			_child_percent_training = ParamReader::getRequiredFloatParam("percent_sent_per_model");
		}

		if ((_model_mode == TRAIN) || 
			(_model_mode == TRAIN_AND_DECODE) ||
			(_model_mode == SIM_AL))
		{
			std::string history_buffer = _model_file + ".hist.txt";
			_historyStream.open(history_buffer.c_str());
			_writeHistory = true;

			if (_historyStream.fail()) {
				throw UnexpectedInputException("PIdFModel::PIdFModel()",
					"Cant open history stream");
			}
		}
	}

	_decoder = _defaultDecoder;
	_vocab = _defaultVocab;
	_secondaryDecoders = 0;

	_secondaryDecoders = _new PIdFSecondaryDecoders();
}

PIdFModel::~PIdFModel() {
	delete _defaultDecoder;
	delete _defaultWeights;
	delete _defaultBlockWeights;
	delete _lowerCaseDecoder;
	delete _lcWeights;
	delete _lcBlockWeights;
	delete _defaultVocab;
	delete _lcVocab;
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;
	delete _secondaryDecoders;

	//delete the sentence blocks
	SentenceBlock* first = _firstSentBlock;
	while(first != 0){
		SentenceBlock* next = first->next;
		delete first;
		first = next;;
	}
	first = _firstALSentBlock;
	while(first != 0){
		SentenceBlock* next = first->next;
		delete first;
		first = next;;
	}
	for (vector<DTObservation*>::iterator i = _observations.begin(); i != _observations.end(); ++i) {
		delete *i;
	}
}

void PIdFModel::resetForNewDocument(DocTheory *docTheory) {
	_decoder = _defaultDecoder;
	_vocab = _defaultVocab;
	if (docTheory != 0) {
		int doc_case = docTheory->getDocumentCase();
		if (doc_case == DocTheory::LOWER && _lowerCaseDecoder != 0) {
			SessionLogger &logger = *SessionLogger::logger;
			logger.reportDebugMessage() << "Using lowercase pIdF decoder\n";
			_decoder = _lowerCaseDecoder;
			_vocab = _lcVocab;
		}
	}
}

/*
decoding methods
*/

void PIdFModel::decode() {

	std::string input_file = ParamReader::getRequiredParam("pidf_input_file");
	std::string output_file = ParamReader::getRequiredParam("pidf_output_file");
	
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(input_file.c_str());
	if (in.fail()) {
		throw UnexpectedInputException("PIdFModel::decode()",
			"Could not open input file for reading");
	}

	UTF8OutputStream out;
	out.open(output_file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFModel::decode()",
			"Could not create output file");
	}

	decode(in, out);
}

void PIdFModel::decode(UTF8InputStream &in, UTF8OutputStream &out) {
	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);
	int sentence_n = 0;

	if (ParamReader::isParamTrue("pidf_do_constrained_decode")) {
		constrainedDecode(in, out);
	}
	else {
		while (idfSentence.readSexpSentence(in)) {
			decode(idfSentence);
			idfSentence.writeSexp(out);
			cout << sentence_n << "\n";
			sentence_n++;
		}
		cout << "\n";
	}
}

void PIdFModel::constrainedDecode(UTF8InputStream &in, UTF8OutputStream &out) {

	DTTagSet *constraintTags = _tagSet;

	// If constraint tag set differs from model tags, create a separate DTTagSet
	std::string constraint_tags_file = ParamReader::getParam("pidf_constraint_tags");
	if (!constraint_tags_file.empty()) {
		constraintTags = _new DTTagSet(constraint_tags_file.c_str(), true, true, _interleave_tags);
	} 

	int sentence_n = 0;
	PIdFSentence constraintSentence(constraintTags, MAX_SENTENCE_TOKENS);
	while (constraintSentence.readTrainingSentence(in)) {
		std::wstring sexp_string = constrainedDecode(constraintSentence, constraintTags);
		out << sexp_string;
		cout << sentence_n << "\n";
		sentence_n++;
	}
	cout << "\n";

	if (!constraint_tags_file.empty()) {
		delete constraintTags;
	}
	/*
	int nskipped = 0;
	ScoredSentenceVector _scoredSentences;
	ScoredSentenceVector _unsortedSentences;
	int skipmargin = ParamReader::getOptionalIntParamWithDefaultValue("pidf_constrained_decode_minmargin",-1);
	double percent_to_add = -1;
	if (ParamReader::hasParam("pidf_constrained_decode_percent_to_add")) {
		percent_to_add = ParamReader::getRequiredFloatParam("pidf_constrained_decode_percent_to_add");
		PIdFSentence* sent;
		while (idfSentence.readTrainingSentence(in)) {
			if (idfSentence.getLength() == 0) {
				if (_output_mode == VERBOSE_OUTPUT) 
					cout << "Ignoring empty training sentence\n";
				continue;
			}
			constrainedDecode(idfSentence);
			sent =_new PIdFSentence();
			sent->populate(idfSentence);
			SentAndScore thisSent(idfSentence.marginScore, sent, sentence_n);
			_scoredSentences.push_back(thisSent);
			_unsortedSentences.push_back(thisSent);
			cout << sentence_n << "\r";
			sentence_n++;
		}


		sort(_scoredSentences.begin(), 	_scoredSentences.end());
		int ntoadd = (int)(percent_to_add * sentence_n);
		ScoredSentenceVectorIt begin = _scoredSentences.begin();
		ScoredSentenceVectorIt end = _scoredSentences.end();
		ScoredSentenceVectorIt curr = end;
		std::cout << "\nwrite " << ntoadd << " sentences" << std::endl;
		double firstmargin = 0;
		firstmargin = (curr-1)->margin;
		double lastmargin;
		for (int i = 0; i < ntoadd; i++) {
			curr--;
			if (curr == begin) {
				break;
			}
			lastmargin = curr->margin;
			curr->sent->marginScore = -1.1;
		}
		std::cout << "Margins from: " << lastmargin << " to " << firstmargin << std::endl;

		begin = _unsortedSentences.begin();
		end = _unsortedSentences.end();

		for (curr = begin; curr != end; curr++) {
			if (curr->sent->marginScore == -1.1) {
				curr->sent->writeSexp(out);
			}
		}
		//need to clean up
		for (curr = begin; curr != end; curr++) {
			delete curr->sent;
			curr->sent = 0;
		}
		_scoredSentences.erase(_scoredSentences.begin(), _scoredSentences.end());
		_unsortedSentences.erase(_unsortedSentences.begin(), _unsortedSentences.end());
	}
	else {
		while (idfSentence.readTrainingSentence(in)) {
			if (idfSentence.getLength() == 0) {
				if (_output_mode == VERBOSE_OUTPUT) 
					cout << "Ignoring empty training sentence\n";
				continue;
			}

			constrainedDecode(idfSentence);

			if (idfSentence.marginScore > skipmargin) {
				idfSentence.writeSexp(out);
			}
			else {
				nskipped++;
				cout << "skipping: " << sentence_n << "with margin" << idfSentence.marginScore << "\n";
			}
			cout << sentence_n << "\r";
		}
        sentence_n++;
		std::cout << "\nskipped: " << nskipped << " out of " << sentence_n << std::endl;

	}
	std::cout << "\n";
	*/
}

std::wstring PIdFModel::constrainedDecode(PIdFSentence &constraintSentence, DTTagSet *constraintTags) {
	int constraints[MAX_SENTENCE_TOKENS+2];
	int tags[MAX_SENTENCE_TOKENS+2];
	
	// populate constraints and _observations arrays 
	if (_tagSet == constraintTags) {
		constraints[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < constraintSentence.getLength(); j++) {
			int tag = constraintSentence.getTag(j);
			if (constraintTags->isNoneTag(tag)) {
				constraints[j+1] = -1;
			}
			else {
				constraints[j+1] = tag;
			}
		}
		constraints[constraintSentence.getLength()+1] = _tagSet->getEndTagIndex();
		populateSentence(_observations, &constraintSentence, _decoder == _lowerCaseDecoder);
	}
	else {
		PIdFSentence decodeSentence(_tagSet, MAX_SENTENCE_TOKENS);

		constraints[0] = _tagSet->getStartTagIndex();	
		for (int j = 0; j < constraintSentence.getLength(); j++) {
			decodeSentence.addWord(constraintSentence.getWord(j));

			// map constraintTags value to _tagSet value
			Symbol constraint_sym = constraintTags->getTagSymbol(constraintSentence.getTag(j));
			int tag = _tagSet->getTagIndex(constraint_sym);
			if (tag == -1) {  
				// this tag must not be in the standard _tagSet, map to NONE
				constraints[j+1] = _tagSet->getNoneTagIndex();
			}
			else if (_tagSet->isNoneTag(tag)) {
				// no constraints for things tagged as NONE
				constraints[j+1] = -1;
			}
			else {
				constraints[j+1] = tag;
			}
		}

		constraints[constraintSentence.getLength()+1] = _tagSet->getEndTagIndex();
		populateSentence(_observations, &decodeSentence, _decoder == _lowerCaseDecoder);
	}

	_secondaryDecoders->AddDecoderResultsToObservation(_observations);
	constraintSentence.marginScore = _decoder->constrainedDecode(_observations, constraints, tags);

	// Constrained tags won't change when updating the input/output sentence
	for (int k = 0; k < constraintSentence.getLength(); k++)
		constraintSentence.setTag(k, tags[k+1]);

	// construct tagged sentence string
	bool prev_tag_was_none = false;
	std::wstring result = L"(";
	for (int k = 0; k < constraintSentence.getLength(); k++) {
		if (k > 0)
			result.append(L" ");
		result.append(L"(");
		result.append(constraintSentence.getWord(k).to_string());
		result.append(L" ");
		if (constraints[k+1] == -1) {
			if (_tagSet->isNoneTag(tags[k+1])) {
				if (!prev_tag_was_none)
					result.append(_tagSet->getNoneSTTag().to_string());
				else
					result.append(_tagSet->getNoneCOTag().to_string());
				prev_tag_was_none = true;
			} else {
				result.append(_tagSet->getTagSymbol(tags[k+1]).to_string());
				prev_tag_was_none = false;
			}
		} else {
			result.append(constraintTags->getTagSymbol(constraintSentence.getTag(k)).to_string());
			prev_tag_was_none = constraintTags->isNoneTag(constraintSentence.getTag(k));
		}
		result.append(L")");
	}
	result.append(L")\n");
	
	return result;
}

void PIdFModel::decode(PIdFSentence &sentence) {
	int tags[MAX_SENTENCE_TOKENS+2];

	populateSentence(_observations, &sentence, _decoder == _lowerCaseDecoder);
	_secondaryDecoders->AddDecoderResultsToObservation(_observations);
	_decoder->decode(_observations, tags);

	for (int k = 0; k < sentence.getLength(); k++)
		sentence.setTag(k, tags[k+1]);
}

void PIdFModel::decode(PIdFSentence &sentence, double &margin) {
	int tags[MAX_SENTENCE_TOKENS+2];

	populateSentence(_observations, &sentence, _decoder == _lowerCaseDecoder);
	_secondaryDecoders->AddDecoderResultsToObservation(_observations);
	_decoder->decode(_observations, tags);

	margin = _decoder->decodeAllTags(_observations);

	for (int k = 0; k < sentence.getLength(); k++)
		sentence.setTag(k, tags[k+1]);
	
}

int PIdFModel::getNameTheories(NameTheory **results, int max_theories,
								 TokenSequence *tokenSequence)
{

	PIdFSentence sentence(_tagSet, *tokenSequence);

	double margin = 0;
	if (_print_sentence_selection_info)
		decode(sentence, margin);
	else decode(sentence);

	results[0] = makeNameTheory(sentence);

	if (_print_sentence_selection_info)
		results[0]->setScore(static_cast<float>(margin / 1000.0));
	return 1;
}


void PIdFModel::populateObservation(TokenObservation *observation,
									  IdFWordFeatures *wordFeatures,
									  Symbol word, bool first_word,
									  int wordCount,
									  bool lowercased_word_is_in_vocab,
									  bool use_lowercase_clusters,
     								  NgramScoreTable* bigram_counts)
{
	Token token(word);

	std::wstring buf(word.to_string());
	std::transform(buf.begin(), buf.end(), buf.begin(), towlower);
	Symbol lcSymbol(buf.c_str());

	Symbol idfWordFeature = wordFeatures->features(word, first_word, lowercased_word_is_in_vocab);
	WordClusterClass wordClass = WordClusterTable::isInitialized() ? WordClusterClass(word, use_lowercase_clusters) : _blankWordClass;

	Symbol allFeatures[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	int n_word_features = wordFeatures->getAllFeatures(word, first_word, lowercased_word_is_in_vocab,
		allFeatures, DTFeatureType::MAX_FEATURES_PER_EXTRACTION);

	observation->populate(token, lcSymbol, idfWordFeature, wordClass, allFeatures
		, n_word_features, wordCount, bigram_counts);
}


void PIdFModel::populateSentence(std::vector<DTObservation *> & observations,
								 PIdFSentence* sentence, bool use_lowercase_clusters) {
	for (vector<DTObservation*>::iterator i = _observations.begin(); i != _observations.end(); ++i) {
		delete *i;
	}
	observations.clear(); // Remove previous sentence's stuff (however, clear() does NOT call delete)
	// Match labels to sentence by token/character
	bool lowercased_word_is_in_vocab = false;
	observations.push_back(new TokenObservation());
	static_cast<TokenObservation*>(observations[0])->populate(
		_blankToken, _blankLCSymbol, _blankWordFeatures, 
		_blankWordClass, 0, 0, 1000, _defaultBigramVocab);
	for (int i = 0; i < sentence->getLength(); i++) {
		Symbol word = sentence->getWord(i);
		int wordCount = 1000;
		if (_vocab != 0) {
			wordCount = (int)_vocab->lookup(&word);
			Symbol lcWord = SymbolUtilities::lowercaseSymbol(word);
			lowercased_word_is_in_vocab = (_vocab->lookup(&lcWord) > 0);
		}
		observations.push_back(new TokenObservation());
		populateObservation(
			static_cast<TokenObservation*>(observations[i+1]),
			_wordFeatures, word, i == 0, /*word_is_in_vocab*/wordCount ,
			lowercased_word_is_in_vocab, use_lowercase_clusters, _defaultBigramVocab);
		//add appropriate labels from above to observation
	}
	observations.push_back(new TokenObservation());
	static_cast<TokenObservation*>(observations.back())
		->populate(_blankToken, _blankLCSymbol, _blankWordFeatures,
			_blankWordClass, 0, 0, 1000, _defaultBigramVocab);

	
	// update matching cache for list-based features
	int n_feature_types = _featureTypes->getNFeaturesTypes();

	for (int i = 0; i < n_feature_types; i++) {
		const DTFeatureType *featureType = _featureTypes->getFeatureType(i);
		if(const DTMaxMatchingListFeatureType* dtMaxMatchingListFeature = dynamic_cast<const DTMaxMatchingListFeatureType*>(featureType)) {
			updateDictionaryMatchingCache(dtMaxMatchingListFeature, observations);
		}
	}
	//
}

void PIdFModel::updateDictionaryMatchingCache(const DTMaxMatchingListFeatureType* dtMaxMatchingListFeature, 
	std::vector<DTObservation *> &observations) {

	Symbol featureName = dtMaxMatchingListFeature->getFeatureName();

	std::vector<bool> dictionaryMatchingCache;
	int MAX_TOKENS_LOOK_AHEAD = 6;

	std::vector<std::wstring> tokens;
	for(size_t i=0; i<observations.size(); i++) {
		TokenObservation *o = static_cast<TokenObservation*>(observations[i]);
		tokens.push_back(o->getSymbol().to_string());
		dictionaryMatchingCache.push_back(false); // set all to be NOT_IN_LIST
	}

	for(int i=0; i<(int)tokens.size(); i++) {
		// longest first
		int j=i+MAX_TOKENS_LOOK_AHEAD<=(int)tokens.size()-1?i+MAX_TOKENS_LOOK_AHEAD:(int)tokens.size()-1;
		for(int end_idx=j; end_idx>=i && end_idx>=0; end_idx--) {
//		for(; j>=i && j>=0; j--) {
			std::wostringstream wordsstream;
			for(int idx=i; idx<=end_idx; idx++)
				wordsstream << tokens[idx];
			
			Symbol wordString(wordsstream.str().c_str());
			bool isMatch = dtMaxMatchingListFeature->isMatch(wordString);
			if (isMatch) { // found match
				for(int idx=i; idx<=end_idx; idx++) {
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


NameTheory *PIdFModel::makeNameTheory(PIdFSentence &sentence) {
	int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);

	int n_name_spans = 0;
	for (int j = 0; j < sentence.getLength(); j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			_tagSet->isSTTag(sentence.getTag(j)))
		{
			n_name_spans++;
		}
	}

	const TokenSequence* tokenSequence = sentence.getTokenSequence();
	NameTheory *nameTheory = _new NameTheory(tokenSequence);

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

		nameTheory->takeNameSpan(_new NameSpan(tok_index, end_index,
			EntityType(_tagSet->getReducedTagSymbol(tag))));

		tok_index = end_index + 1;
	}

	return nameTheory;
}


void PIdFModel::addTrainingSentence(PIdFSentence &sentence) {
	if (_lastSentBlock == 0) {
		_firstSentBlock = _new SentenceBlock();
		_lastSentBlock = _firstSentBlock;
	}
	else if (_lastSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastSentBlock->next = _new SentenceBlock();
		_lastSentBlock = _lastSentBlock->next;
	}

	_lastSentBlock->sentences[_lastSentBlock->n_sentences++].populate(sentence);
}

void PIdFModel::train() {
	if (_use_stored_vocabulary) {
		_defaultVocab = _new NgramScoreTable(1,10000);
		_vocab = _defaultVocab;
		fillVocabTable(_vocab);
		_vocab->print(_vocab_file.c_str());
	}
	if (_use_stored_bigram)	{
		_defaultBigramVocab = _new NgramScoreTable(2,100000);
		fillBigramVocabTable(_defaultBigramVocab);
		_defaultBigramVocab->print(_bigram_file.c_str());
	}

	if (ParamReader::isParamTrue("restrict_features_with_vocab")) {
		if(!_use_stored_vocabulary) {
			throw InternalInconsistencyException(
				"PIdFModel::PIdFModel()",
				"Trying to restrict features by vocab, but no vocab file specified!");
		}
		PIdFFeatureType::setUnigramVocab(_vocab_file.c_str());
	}
	if (ParamReader::isParamTrue("restrict_features_with_bigram_vocab")) {
		if(!_use_stored_bigram) {
			throw InternalInconsistencyException(
				"PIdFModel::PIdFModel()",
				"Trying to restrict features by bigram vocab, but no bigram vocab file specified!");
		}
		PIdFFeatureType::setBigramVocab(_bigram_file.c_str());
	}

	// collect acceptable tag transitions from the training sentences
	if (_learn_transitions_from_training) {
		_tagSet->resetSuccessorTags();
		_tagSet->resetPredecessorTags();
		seekToFirstSentence();
		while (moreSentences()) {
			PIdFSentence *sentence = getNextSentence();
			Symbol prevTag = _tagSet->getStartTag();
			for (int i = 0; i< sentence->getLength(); i++) {
				Symbol nextTag = _tagSet->getTagSymbol(sentence->getTag(i));
				_tagSet->addTransition(prevTag, nextTag);
				prevTag = nextTag;
			}
			_tagSet->addTransition(prevTag, _tagSet->getEndTag());
		}
	}

	_decoder = _new PDecoder(_tagSet, _featureTypes, _defaultWeights, _add_hyp_features);

	if (_seed_features) {
		cout << "Seeding weight table with all features from training set...\n";
		cout.flush();
		addTrainingFeatures();
	}

	if (_use_fast_training) {
		fastTrain();
	} else {
		normalTrain();
	}
}

void PIdFModel::normalTrain() {
	bool randomize = ParamReader::isParamTrue("pidf_randomize_training");
	
	double prevtot = 0;
	double tot = 0;
	cout << "Starting epoch   1 ...\n";
	if (_writeHistory)
		outputDate(_historyStream);

	tot = trainEpoch(0, randomize);
	if (_print_after_every_epoch) {
		std::cerr << "Writing weights: 1" << std::endl;
		DTFeature::addWeightsToSum(_defaultWeights); // make sure there is a sum
		writeWeights(1);
	}
	for (int epoch = 1; epoch < _epochs; epoch++) {
		cout << "Starting epoch " << epoch + 1 << "...\n";

		if (_writeHistory) _historyStream << "Starting epoch " << epoch + 1 << "...\n";
		cout.flush();
		prevtot = tot;
			
		tot = trainEpoch(epoch, randomize);
		if (tot >= _min_tot) {
			break;
		}
		if ((tot-prevtot) < _min_change) {
			break;
		}
		if (_print_after_every_epoch) {
			std::cerr << "Writing weights: " << epoch + 1 << std::endl;
			DTFeature::addWeightsToSum(_defaultWeights); // make sure there is a sum
			writeWeights(epoch + 1);
		}
		if (_writeHistory) {
			outputDate(_historyStream);
			_historyStream.flush();
		}
	}
}


void PIdFModel::fastTrain() {
	bool randomize = ParamReader::isParamTrue("pidf_randomize_training");

	double prev_first_tot = 0;
	double first_tot = 0;
	double est_tot = 0;
	cout << "Starting epoch   1 ...\n";
	if (_writeHistory)
		outputDate(_historyStream);

	trainEpochFocusOnErrors(0, est_tot, first_tot);

	if (_print_after_every_epoch) {
		cout << "Writing weights: 1" << std::endl;
		DTFeature::addWeightsToSum(_defaultWeights); // make sure there is a sum
		writeWeights(1);
	}
	for (int epoch = 1; epoch < _epochs; epoch++) {
		cout << "Starting epoch " << epoch + 1 << "...\n";

		if (_writeHistory) _historyStream << "Starting epoch " << epoch + 1 << "...\n";
		cout.flush();
		prev_first_tot = first_tot;
			
		trainEpochFocusOnErrors(epoch, est_tot, first_tot);
		if (first_tot >= _min_tot) {
			break;
		}
		if ((first_tot - prev_first_tot) < _min_change) {
			break;
		}
		if (_print_after_every_epoch) {
			cout << "Writing weights: " << epoch + 1 << std::endl;
			DTFeature::addWeightsToSum(_defaultWeights); // make sure there is a sum
			writeWeights(epoch + 1);
		}
		if (_writeHistory) {
			outputDate(_historyStream);
			_historyStream.flush();
		}
	}
}


void PIdFModel::writeModel(char* str) {
	writeWeights(str);
	writeTrainingSentences(str);
	if (_learn_transitions_from_training) {
		writeTransitions(str);
	}
}

void PIdFModel::writeSentences(char* str) {
  writeTrainingSentences(str);
}

void PIdFModel::writeWeights(int epoch) {
	if (epoch == -1) {
		writeWeights("");
	}
	else {
		char buffer[50];
		sprintf(buffer, "epoch-%d", epoch);
		writeWeights(buffer);
	}
}

void PIdFModel::writeWeights(const char* str) {
	UTF8OutputStream out;

	std::string file;
	if (strcmp("", str)== 0) {
		file = _model_file;
	}
	else {
		std::string new_str(str);
		file = _model_file + "-" + new_str;
	}
	out.open(file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFModel::writeWeights()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	DTFeature::writeSumWeights(*_defaultWeights, out);
	out.close();
}

void PIdFModel::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForReference(Symbol(L"pidf_training_file"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_tag_set_file"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_features_file"), out);
	DTFeature::recordParamForReference(Symbol(L"word_cluster_bits_file"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_vocab_file"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_interleave_tags"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_learn_transitions"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_epochs"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_seed_features"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_add_hyp_features"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_weightsum_granularity"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_min_tot"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_min_change"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_use_fast_training"), out);
	DTFeature::recordParamForReference(Symbol(L"pidf_trainer_required_margin"), out);
	out << L"\n";
}

void PIdFModel::writeTransitions(char* str) {
	UTF8OutputStream out;

	std::string file;
	if (strcmp("", str)== 0) {
		file = _model_file + "-transitions";
	}
	else {
		std::string new_str(str);
		file = _model_file + "-" + new_str + "-transitions";
	}
	out.open(file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFModel::writeTransitions()",
			"Could not open model file for writing");
	}

	_tagSet->writeTransitions(file.c_str());
}



void PIdFModel::writeTrainingSentences(char* str) {
	UTF8OutputStream out;
	std::string file;
	if (strcmp("", str)== 0) {
		file = _model_file + "-sent";
	}
	else {
		std::string new_str(str);
		file = _model_file + "-" + new_str + "-sent";
	}
	out.open(file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFModel::writeTrainingSentences()",
			"Could not open model training sentence file for writing");
	}
	seekToFirstSentence();
	PIdFSentence* sent;

	while (moreSentences()) {
		sent = getNextSentence();
		sent->writeSexp(out);
	}
	out.close();
}

void PIdFModel::freeWeights(){
	//	cerr << "Press enter to free weight tables and decoder...\n";
	//	getchar();

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	if (_defaultWeights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _defaultWeights->begin();
			 iter != _defaultWeights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}
	/*if (_defaultBlockWeights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _defaultBlockWeights->begin();
				 iter != _defaultBlockWeights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}*/
	if (_lcWeights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _lcWeights->begin();
			 iter != _lcWeights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}
	/*if (_lcBlockWeights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _lcBlockWeights->begin();
				 iter != _lcBlockWeights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}*/

	delete _decoder;
	delete _defaultWeights;
	delete _defaultBlockWeights;
	delete _lcWeights;
	delete _lcBlockWeights;
	_decoder = 0;
	_defaultWeights = 0;
	_defaultBlockWeights = 0;
	_lcWeights = 0;
	_lcBlockWeights = 0;

	//	cerr << "Press enter to continue...\n";
	//	getchar();
}



void PIdFModel::addTrainingFeatures() {
	int tags[MAX_SENTENCE_TOKENS+2];
	seekToFirstSentence();
	int sentence_n = 0;
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();
		int n_observations = sentence->getLength() + 2;

		populateSentence(_observations, sentence);

		tags[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < sentence->getLength(); j++)
			tags[j+1] = sentence->getTag(j);
		tags[n_observations-1] = _tagSet->getEndTagIndex();

		_decoder->addFeatures(_observations, tags, 1);

		sentence_n++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << ": " << (int) _defaultWeights->size() << "\n";
/*				<< (int)_weights->get_path_length() << "\r";
				<< (int)_weights->get_num_lookup_eqs() << "/"
				<< (int)_weights->get_num_lookups() << "\r"; */
			cout.flush();
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << "              \r";
		}
	}

	cout << "\n";
}

void PIdFModel::fillVocabTable(NgramScoreTable *table) {
	seekToFirstSentence();
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();
		for (int i = 0; i < sentence->getLength(); i++) {
			Symbol word = sentence->getWord(i);
			table->add(&word);
		}
	}
}

void PIdFModel::fillBigramVocabTable(NgramScoreTable *table) {
	seekToFirstSentence();
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();
		Symbol* bigram = _new Symbol[2];
		bigram[0] = sentence->getWord(0);
		for (int i = 1; i < sentence->getLength(); i++) {
			bigram[1] = sentence->getWord(i);
			table->add(bigram);
			bigram[0] = bigram[1];
		}
	}
}


double PIdFModel::trainEpoch(int epoch, bool randomize_sent) {
//double PIdFModel::trainEpoch(int epoch, bool randomize_sent, bool add_average) {

	int tags[MAX_SENTENCE_TOKENS+2];

	seekToFirstSentence();
	int sentence_n = 0;
	int n_correct = 0;
	int total_ncorrect = 0;
	int count = 0;
	srand(0);

	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();
		int r = rand();
		SentAndScore thisSent(rand(), sentence, count++);
		_scoredSentences.push_back(thisSent);
	}
	
	if (randomize_sent) {
		sort(_scoredSentences.begin(), _scoredSentences.end());
	}

	ScoredSentenceVectorIt begin = _scoredSentences.begin();
	ScoredSentenceVectorIt end = _scoredSentences.end();
	ScoredSentenceVectorIt curr = begin;
	for (curr = begin; curr != end; curr++) {
		PIdFSentence* sentence = (*curr).sent;
		int n_observations = sentence->getLength() + 2;

		populateSentence(_observations, sentence);
		_secondaryDecoders->AddDecoderResultsToObservation(_observations);

		tags[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < sentence->getLength(); j++)
			tags[j+1] = sentence->getTag(j);
		tags[n_observations-1] = _tagSet->getEndTagIndex();

		double margin = _decoder->trainWithMargin(_observations, tags, 1, _required_margin);
		
		if (margin >= 0) {
			n_correct++;
			total_ncorrect++;
		}

		sentence_n++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << ": " << n_correct
				 << " (" << 100*n_correct/1000 << "%)"
				 << "; " << (int) _defaultWeights->size() << "      \n";
			if (_writeHistory) {
				_historyStream << sentence_n << ": " << n_correct
					 << " (" << 100*n_correct/1000 << "%)"
					<< "; " << (int) _defaultWeights->size() << "      \n";
			}
			cout.flush();
			n_correct = 0;
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << ": " << n_correct
				 << " (" << 100*n_correct/(sentence_n % 1000) << "%)"
				 << "              \r";
		}

		// every so-often, add weights to _weightSums
		if (sentence_n % _weightsum_granularity == 0) {
			DTFeature::addWeightsToSum(_defaultWeights);
		}
	}
	_scoredSentences.erase(_scoredSentences.begin(), _scoredSentences.end());

	cout << "final: " << sentence_n << ": " << total_ncorrect
		 << " (" << 100*((double)total_ncorrect/sentence_n) << "%)"
		 << "; " << (int) _defaultWeights->size() << "      \n";
		cout.flush();
	if (_writeHistory) {
		_historyStream << "final: " << sentence_n << ": " << total_ncorrect
			<< " (" << 100*((double)total_ncorrect/sentence_n)<< "%)"
			<< "; " << (int) _defaultWeights->size() << "      \n";
	}

	cout << "\n";
	return (double)total_ncorrect/sentence_n;
}


void PIdFModel::trainEpochFocusOnErrors(int epoch, double &estimated_acc, double &first_acc) {
	//cerr<<"in trainEpochFocusOnErrors"<<endl;
	int tags[MAX_SENTENCE_TOKENS+2];

	seekToFirstSentence();
	int accumulated_ncorrect = 0; // accumulated also over the error iteratons
	int total_margin_ncorrect = 0;
	int accumulated_sentence_n = 0;
	int accumulated_margin_ncorrect = 0; // accumulated also over the error iteratons
	std::vector<PIdFSentence*> sentencesVec;

	// add all the semtences to the training vector
	int count = 0;	
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();
		count++;
		sentencesVec.push_back(sentence);
		sentence->marginScore = 0.0;
		sentence->addToTraining();
	}

	int _sentence_updates_limit_per_epoch=3; /* need to read it from the parameter file */
	for (int i = 0; i < _sentence_updates_limit_per_epoch; i++) {
		int sentence_n = 0;
		int n_margin_correct = 0;
		int n_correct = 0;
		int total_ncorrect = 0;       // accumulated over the 1000 sent block
		typedef std::vector<PIdFSentence*>::iterator SentIterator;
		SentIterator curr = sentencesVec.begin();
		while (curr != sentencesVec.end()) {
			PIdFSentence* sentence = *curr;
			int n_observations = sentence->getLength() + 2;

			populateSentence(_observations, sentence);
			_secondaryDecoders->AddDecoderResultsToObservation(_observations);

			tags[0] = _tagSet->getStartTagIndex();
			for (int j = 0; j < sentence->getLength(); j++)
				tags[j+1] = sentence->getTag(j);
			tags[n_observations-1] = _tagSet->getEndTagIndex();

			double margin = _decoder->trainWithMargin(_observations, tags, 1, _required_margin);
			
			sentence->marginScore = margin;
			
			if (margin>=0) {
				n_correct++;
				total_ncorrect++;
				if (margin>=1) {
					n_margin_correct++;
					total_margin_ncorrect++;
				}
			}
			if (margin < 1) {
				sentencesVec.erase(curr, curr+1);
				// when erasing we don't need to advance the iterator
			}
			else {
				curr++;
			}
			sentence_n++;

			if (sentence_n % 1000 == 0) {
				cout << sentence_n << ": " << n_correct
					<< " (" << 100*n_correct/1000 << "%)"
					<< "; " << (int) _defaultWeights->size() << "      \n";
				if (_writeHistory) {
					_historyStream << sentence_n << ": " << n_correct
						<< " (" << 100*n_correct/1000 << "%)"
						<< "; " << (int) _defaultWeights->size() << "      \n";
				}

				cout.flush();
				n_margin_correct = 0;
				n_correct = 0;
			}
			else if (_output_mode == VERBOSE_OUTPUT) {
				cout << sentence_n << ": " << n_correct
					<< " (" << 100*n_correct/(sentence_n % 1000) << "%)"
					<< "              \r";
			}

			// every so-often, add weights to _weightSums
			if (sentence_n % _weightsum_granularity == 0) {
				DTFeature::addWeightsToSum(_defaultWeights);
			}
		}// while

		cout << "final: " << sentence_n << ": " << total_ncorrect
			<< " (" << 100*((double)total_ncorrect/sentence_n)<< "%)"
//					<< " (" << 100*n_correct/1000 << "%)"
			<< "; " << (int) _defaultWeights->size() << "      \n";
			cout.flush();
		if (_writeHistory) {
			_historyStream << "final: " << sentence_n << ": " << total_ncorrect
				<< " (" << 100*((double)total_ncorrect/sentence_n)<< "%)"
				<< "; " << (int) _defaultWeights->size() << "      \n";
		}

		cout << "\n";

		if (i == 0) {
			first_acc = (double)total_ncorrect/sentence_n;
		}
		accumulated_ncorrect += total_ncorrect;
		accumulated_sentence_n += sentence_n;
	}// for on errors cycles
	estimated_acc = accumulated_ncorrect/accumulated_sentence_n;
}


void PIdFModel::seekToFirstSentence() {
	_curSentBlock = _firstSentBlock;
	_cur_sent_no = 0;
}

bool PIdFModel::moreSentences() {
	return _curSentBlock != 0 &&
		   _cur_sent_no < _curSentBlock->n_sentences;
}

PIdFSentence *PIdFModel::getNextSentence() {
	if (moreSentences()) {
		PIdFSentence *result = &_curSentBlock->sentences[_cur_sent_no];

		// move to next sentence for next call
		_cur_sent_no++;
		if (_cur_sent_no == SentenceBlock::BLOCK_SIZE) {
			_curSentBlock = _curSentBlock->next;
			_cur_sent_no = 0;
		}

		return result;
	}
	else {
		return 0;
	}
}

void PIdFModel::addTrainingSentencesFromTrainingFileList(const char *file, bool files_are_encrypted) {
	if (file == 0) {
		throw UnexpectedInputException(
			"PIdFModel::addTrainingSentencesFromTrainingFileList()",
			"No training file list specified.");
	}
	boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& filelist(*filelist_scoped_ptr);
	filelist.open(file);

	if (filelist.fail()) {
		throw UnexpectedInputException(
			"PIdFModel::addTrainingSentencesFromTrainingFileList()",
			"Unable to open traing file list.");
	}
	int nfiles;
	filelist >> nfiles;
	UTF8Token token;
	cout << "Reading training sentences from disk...\n";
	int sentence_n = 0;

	// LB: I changed this on 3/3/11 so that it simply calls addTrainingSentencesFromTrainingFile()
	//     I think this is the right thing to do; the only difference is that addTrainingSentencesFromTrainingFile() 
	//     checks to see that it doesn't already have more than _nTrainSent before it loads the next sentence	
	for (int i = 0; i< nfiles; i++) {
		if (filelist.eof())
			throw UnexpectedInputException("PIdFModel::addTrainingSentencesFromTrainingFileList",
				"fewer training files than specified in file");
		filelist >> token;
		addTrainingSentencesFromTrainingFile(token.symValue().to_debug_string(), files_are_encrypted);
	}
	filelist.close();
}

void PIdFModel::addTrainingSentencesFromTrainingFile(const char *file, bool is_encrypted) {
	
	if (file == 0) {
		throw UnexpectedInputException(
			"PIdFModel::addTrainingSentencesFromTrainingFile()",
			"No training file specified.");
	}

	UTF8InputStream * in = UTF8InputStream::build(file, is_encrypted);

	if (in->fail()) {
		throw UnexpectedInputException(
			"PIdFModel::addTrainingSentencesFromTrainingFile()",
			"Unable to open training file.");
	}

	cout << "Reading training sentences from disk...\n";

	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int sentence_n = 0;
	while (true) {
		bool not_eof = true;
		try {
			not_eof = idfSentence.readTrainingSentence(*in);
		}
		catch (UnrecoverableException &e) {
			cerr << "\nskipping sentence due to error: " << e.getSource() << " " << e.getMessage() << "\n";
			wchar_t wch;
			for (wch = in->get(); wch!=L'\n' && !in->eof();) {
				if (wch == L'\r')
					break;
				wch = in->get();
			}
			continue;
		}
		if (!not_eof)
			break;
		if (idfSentence.getLength() == 0) {
			if (_output_mode == VERBOSE_OUTPUT) 
				cout << "Ignoring empty training sentence\n";
			continue;
		}

		if ((_nTrainSent <= 0)) {
			addTrainingSentence(idfSentence);
		}
		else {
			if (sentence_n < _nTrainSent) {
				addTrainingSentence(idfSentence);
			}
			else {
				break;
			}
		}

		sentence_n++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << "\n";
			cout.flush();
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << "              \r";
		}
	}
	delete in;
	cout << "Added " << sentence_n << " training sentences" << std::endl;
	cout << "\n";
}

void PIdFModel::addTrainingSentencesFromALCorpus(int n) {
	seekToFirstALSentence();

	int nadded = 0;
	while ((nadded < n) && moreALSentences()) {
		PIdFSentence* sent = getNextALSentence();
		addTrainingSentence(*sent);
		sent->addToTraining();
		nadded++;
	}
}

void PIdFModel::doActiveLearning() {
	for (int i = 0; i < _nActiveLearningIter; i++) {
		trainAndSelect(true, i, _nToAdd);
	}
	train();
	if (_writeHistory)
		_historyStream.close();
}

void PIdFModel::trainAndSelect(bool print_models, int count, int nsent) {
	train();
	finalizeWeights();
	if (print_models) {
		char buff[100];
		sprintf(buff, "al-%d", count);
		writeModel(buff);
	}
	//select the sentences for the next round of active learning

	int tags[MAX_SENTENCE_TOKENS+2];
	Symbol decodedAnswer[MAX_SENTENCE_TOKENS+2];
	double score = 0;
	seekToFirstALSentence();
	int ndecoded = 0;
	int ntotal = 0;
	//since 0 is the minimum value for a margin, once we've found _nToAdd sentences
	// with a margin of 0. there's no reason to continue to decode....
	int n_margin_is_0 = 0;
	if (nsent == -1) {
		nsent = _nToAdd;
	}

	double* score_buffer = _new double[_nStoredTags];
	while ((moreALSentences()) && n_margin_is_0 < nsent) {
		PIdFSentence *sentence = getNextALSentence();
		if (_allowSentenceRepeats || !sentence->inTraining()) {
			int n_observations = sentence->getLength() + 2;
			populateSentence(_observations, sentence);

			tags[0] = _tagSet->getStartTagIndex();
			for (int j = 0; j < sentence->getLength(); j++)
				tags[j+1] = sentence->getTag(j);
			tags[n_observations-1] = _tagSet->getEndTagIndex();
			int pos = -1;
			if (_tagSpecificScoredSentences == 0) {
					score = _decoder->decodeAllTagsPos(_observations, pos);
			}
			else {
				for (int k = 0; k < _nStoredTags; k++) {
					score_buffer[k] = -1;
				}
				score = _decoder->decodeAllTags(_observations, score_buffer, _nStoredTags);
				for (int j = 0; j < _nStoredTags; j++) {
					if (score_buffer[j] > -1) {
						SentAndScore thisSent(score_buffer[j], sentence, ntotal);
						_tagSpecificScoredSentences[j].push_back(thisSent);
					}
				}
			}
			ndecoded++;
			SentAndScore thisSent(score, sentence, ntotal);
			thisSent.pos = pos - 1;
			_scoredSentences.push_back(thisSent);
			if (score == 0) {
				n_margin_is_0++;
			}
		}
		ntotal++;
	}
	delete score_buffer;

	//add the first _nToAdd sentences to the training sentences
	ScoredSentenceVectorIt begin;
	ScoredSentenceVectorIt end;
	ScoredSentenceVectorIt curr;
	int nadded = 0;
	int totals[6];
	int totalgt5 = 0;
	for (int i = 0; i < 6; i++) {
		totals[i] = 0;
	}
	if (_percentTagSpecific > 0) {
		double nnormal = (1 - _percentTagSpecific) * nsent;
		double neachtag = (_percentTagSpecific * nsent) / _nStoredTags;
		for (int i = 0; i < _nStoredTags; i++) {
			begin = _tagSpecificScoredSentences[i].begin();
			end = _tagSpecificScoredSentences[i].end();
			if (begin == end) {
				continue;
			}

			sort(begin, end);
			begin = _tagSpecificScoredSentences[i].begin();
			end = _tagSpecificScoredSentences[i].end();
			curr = begin;
			int nthistag = 0;

			while ((nthistag < neachtag) && (curr != end)) {
				if (!curr->sent->inTraining()) {
					int marginint = (int)curr->margin;
					if (marginint > 5.0) {
						totalgt5++;
					}
					else {
						totals[marginint]++;
					}
					addTrainingSentence(*curr->sent);
					curr->sent->addToTraining();
					nthistag++;
				}
				curr++;
			}
			nadded += nthistag;
			_tagSpecificScoredSentences[i].erase(begin, end);
		}
	}

	if (_bigramFrequency != 0) {
		//this is iffy, but I need a way to combine the margins with freq.
		begin = _scoredSentences.begin();
		end = _scoredSentences.end();
		curr = begin;
		double totalmargin = 0;
		while (curr != end) {
			(*curr).margin += 1;	//dont allow any margins to be 0
			totalmargin += (*curr).margin;
			curr++;
		}
		std::cout << "bigram freq pt1" << std::endl;
		curr = begin;
		while (curr != end) {
			double m = (*curr).margin / totalmargin;
			int pos = (*curr).pos;
			Symbol ngram[2];
			ngram[0] = Symbol(L"-FIRSTWORD-");
			ngram[1] = (*curr).sent->getWord(pos);
			if (pos > 0) ngram[0] = (*curr).sent->getWord(pos-1);
			double freq1 = _bigramFrequency->lookup(ngram);
			ngram[0] = ngram[1];
			ngram[1] = Symbol(L"-LASTWORD-");
			if (pos < (((*curr).sent->getLength())-1)) {
				ngram[1] = (*curr).sent->getWord(pos+1);
			}
			double freq2 = _bigramFrequency->lookup(ngram);
			double lm = log(m);
			if (freq1 == 0) {
				ngram[0] = Symbol(L"-UNK-");
				ngram[1] = Symbol(L"-UNK-");
				freq1 = _bigramFrequency->lookup(ngram);
			}
			if (freq2 == 0) {
				ngram[0] = Symbol(L"-UNK-");
				ngram[1] = Symbol(L"-UNK-");
				freq2 = _bigramFrequency->lookup(ngram);
			}

			(*curr).margin = (_mult * (1-(freq1+freq2)/2)) + ((1-_mult) *lm);
			curr++;
		}

	}

	sort(_scoredSentences.begin(), _scoredSentences.end());

	begin = _scoredSentences.begin();
	end = _scoredSentences.end();
	curr = begin;

	while ((nadded < nsent) && (curr != end)) {
		int marginint = (int)curr->margin;
		if (marginint > 5.0) {
			totalgt5++;
		}
		else {
			totals[marginint]++;
		}

		addTrainingSentence(*curr->sent);
		curr->sent->addToTraining();
		curr++;
		nadded++;
	}
	if (_writeHistory) {
		_historyStream << "Added " << nadded << " sentences: " << "\n";
		for (int i = 0; i < 6; i++) {
			_historyStream << "\t " << totals[i] << " w/margin=" << i << "\n";
		}
		_historyStream << "\t " << totalgt5 << " w/margin >= 6\n";
		_historyStream.flush();
	}
	//clean up
	_scoredSentences.erase(_scoredSentences.begin(), _scoredSentences.end());
	freeWeights();
	_defaultWeights = _new DTFeature::FeatureWeightMap(500009); //reset the default weights
}

void PIdFModel::chooseHighScoreSentences() {
	int tags[MAX_SENTENCE_TOKENS+2];
	Symbol decodedAnswer[MAX_SENTENCE_TOKENS+2];
	double score = 0;
	seekToFirstALSentence();
	int ndecoded = 0;
	int ntotal = 0;
	double* score_buffer = _new double[_nStoredTags];
    int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);
    
    // collected sentences and scores
    // EITHER all together into _scoredSentences
    // OR categorized by tag in _tagSpecificScoredSentences
    // include n_name_spans with score in either case
	while (moreALSentences()) {
		PIdFSentence *sentence = getNextALSentence();
		if (_allowSentenceRepeats || !sentence->inTraining()) {

            int n_name_spans = 0;
            for (int j = 0; j < sentence->getLength(); j++) {
              if (sentence->getTag(j) != NONE_ST_tag &&
                  _tagSet->isSTTag(sentence->getTag(j)))
                {
                  n_name_spans++;
                }
            }
	
			for (int j = 0; j < sentence->getLength(); j++)
				tags[j+1] = sentence->getTag(j);
            int n_observations = sentence->getLength() + 2;
			populateSentence(_observations, sentence);
			tags[0] = _tagSet->getStartTagIndex();
			tags[n_observations-1] = _tagSet->getEndTagIndex();
			int pos = -1;

			if (_tagSpecificScoredSentences == 0) {
					score = _decoder->decodeAllTagsPos(_observations, pos);
                    if (n_name_spans && sentence->getLength() > 1) {
                      SentAndScore thisSent(score, sentence, ntotal, n_name_spans);
                      thisSent.pos = pos - 1;
                      _scoredSentences.push_back(thisSent);
                    }
			
            } else {
				for (int k = 0; k < _nStoredTags; k++) {
					score_buffer[k] = -1;
				}

				score = _decoder->decodeAllTags(_observations, score_buffer, _nStoredTags);
				for (int j = 0; j < _nStoredTags; j++) {
					if (score_buffer[j] > -1) {
                      SentAndScore thisSent(score_buffer[j], sentence, ntotal, n_name_spans);
                      _tagSpecificScoredSentences[j].push_back(thisSent);
					}
				}
			}
			ndecoded++;
        }
		ntotal++;
	}
	delete score_buffer;

    // add sentences to training
    ScoredSentenceVectorIt begin;
    ScoredSentenceVectorIt end;
    ScoredSentenceVectorIt curr;
    int nadded = 0;

    if (_tagSpecificScoredSentences == 0) {
      reAssignScores(-1);
      sort(_scoredSentences.begin(), _scoredSentences.end());
      begin = _scoredSentences.begin();
      end = _scoredSentences.end();
      curr = end;

      for (int i = 0; i < _nToAdd; i++) {
        curr--;
        if (curr == begin) {
          break;
        }
        addTrainingSentence(*(*curr).sent);
        (*curr).sent->addToTraining();
        nadded++;
      }
      std::cout << "********* added: " << nadded << std::endl;
      //clean up
      _scoredSentences.erase(_scoredSentences.begin(), _scoredSentences.end());
      
    }

    else {
      double nnormal = (1 - _percentTagSpecific) * _nToAdd;
      double neachtag = (_percentTagSpecific * _nToAdd) / _nStoredTags;
      for (int i = 0; i < _nStoredTags; i++) {
        reAssignScores(i);
        begin = _tagSpecificScoredSentences[i].begin();
        end = _tagSpecificScoredSentences[i].end();
        if (begin == end) {
          std::cout << "no sentences with tag " << i << endl;
          continue;
        }
        sort(begin, end);
        begin = _tagSpecificScoredSentences[i].begin();
        end = _tagSpecificScoredSentences[i].end();
        curr = end;
        curr--;
        int nthistag = 0;

        while ((nthistag < neachtag) && (curr != begin)) {
          if (!curr->sent->inTraining()) {
            addTrainingSentence(*(*curr).sent);
            (*curr).sent->addToTraining();
            nthistag++;
          }
          curr--;
        }
        nadded += nthistag;
        cout << "added " << nthistag << " sentences for tag " << i << endl;
        _tagSpecificScoredSentences[i].erase(begin, end);
      }
      // if there are not enough sentences of a given tag to
      // complete the proportion, I am _not_ adding extra
      // sentences to get to _nToAdd
      std::cout << "********* added: " << nadded << std::endl;
	}

}

void PIdFModel::reAssignScores(int i) {
  // Use the margin plus information about sentence
  // length and entities to rescore each sentence
  // Do this after all sentences have been decoded so
  // that ratios can be used
  double totalmargin = 0;
  int totalnames = 0;
  int totalrare = 0;
  double totalraretag = 0;
  int totallength = 0;
  ScoredSentenceVectorIt begin;
  ScoredSentenceVectorIt end;
  ScoredSentenceVectorIt curr;
  if (i == -1) {
    begin = _scoredSentences.begin();
    end = _scoredSentences.end();
  }
  else {
    std::cout << "reassigning scores for tag " << i << endl;
    begin = _tagSpecificScoredSentences[i].begin();
    end   = _tagSpecificScoredSentences[i].end();
  }
  curr = begin;
  while (curr != end) {
    totalmargin += (*curr).margin;
    totalnames += (*curr).n_name_spans;
    totalrare += (*curr).unk;
    totalraretag += (*curr).rare_tag_val;
    totallength += (*curr).sent->getLength();
    curr++;
  }
  curr =  begin;
  while (curr != end) {
    double margin = (*curr).margin;
    double entities = static_cast<double>((*curr).n_name_spans);
    double length   = static_cast<double>((*curr).sent->getLength());
    double ent_len_ratio = entities/length;
    double margin_ratio = margin/totalmargin;
    (*curr).margin = margin_ratio + ent_len_ratio;
    curr++;
  }
}

void PIdFModel::seekToFirstALSentence() {
	_curALSentBlock = _firstALSentBlock;
	_curAL_sent_no = 0;
}

bool PIdFModel::moreALSentences() {
	return _curALSentBlock != 0 &&
		   _curAL_sent_no < _curALSentBlock->n_sentences;
}

PIdFSentence *PIdFModel::getNextALSentence() {
	if (moreALSentences()) {
		PIdFSentence *result = &_curALSentBlock->sentences[_curAL_sent_no];

		// move to next sentence for next call
		_curAL_sent_no++;
		if (_curAL_sent_no == SentenceBlock::BLOCK_SIZE) {
			_curALSentBlock = _curALSentBlock->next;
			_curAL_sent_no = 0;
		}

		return result;
	}
	else {
		return 0;
	}
}

void PIdFModel::addActiveLearningSentence(PIdFSentence &sentence) {
	if (_lastALSentBlock == 0) {
		_firstALSentBlock = _new SentenceBlock();
		_lastALSentBlock = _firstALSentBlock;
	}
	else if (_lastALSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastALSentBlock->next = _new SentenceBlock();
		_lastALSentBlock = _lastALSentBlock->next;
	}
	_lastALSentBlock->sentences[_lastALSentBlock->n_sentences++].populate(
																sentence);
}

void PIdFModel::addActiveLearningSentencesFromTrainingFileList(const char *file) {
	_n_ALSentences = 0;
	boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& filelist(*filelist_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	if (file == 0) {
		std::string active_learning_file = ParamReader::getRequiredParam("active_learning_file");
		filelist.open(active_learning_file.c_str());
	}
	else {
		filelist.open(file);
	}

	if (filelist.fail()) {
		throw UnexpectedInputException(
			"PIdFModel::addActiveLarningSentencesFromTrainingFileList()",
			"Unable to open active learning file list.");
	}
	int nfiles;
	filelist >> nfiles;
	UTF8Token token;
	cout << "Reading active learning sentences from disk...\n";
	for (int i = 0; i< nfiles; i++) {
		if (filelist.eof())
			throw UnexpectedInputException("PIdFModel::addActiveLearningSentencesFromTrainingFileList",
				"fewer training files than specified in file");
		filelist >> token;
		in.open(token.symValue().to_string());
		if (in.fail()) {
			throw UnexpectedInputException(
				"PIdFModel::addActiveLarningSentencesFromTrainingFileList()",
				"Unable to open file from active learning file list.");
		}


		PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

		int sentence_n = 0;
		while (idfSentence.readTrainingSentence(in)) {

			if (idfSentence.getLength() == 0) {
				if (_output_mode == VERBOSE_OUTPUT) 
					cout << "Ignoring empty training sentence\n";
				continue;
			}

			idfSentence.removeFromTraining();
			addActiveLearningSentence(idfSentence);

			sentence_n++;
			_n_ALSentences++;

			if (sentence_n % 1000 == 0) {
				cout << sentence_n << "\n";
				cout.flush();
			}
			else if (_output_mode == VERBOSE_OUTPUT) {
				cout << sentence_n << "              \r";
			}
		}

		cout << "\n";
		in.close();
	}
	filelist.close();
}


void PIdFModel::addActiveLearningSentencesFromTrainingFile(const char *file) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	if (file == 0) {
		std::string active_learning_file = ParamReader::getRequiredParam("active_learning_file");
		in.open(active_learning_file.c_str());
	}
	else {
		std::cout << "active learning file: " << file << std::endl;
		in.open(file);
	}
	if (in.fail()) {
		throw UnexpectedInputException(
			"PIdFModel::addActiveLearningSentencesFromTrainingFile()",
			"Unable to open file.");
	}


	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int sentence_n = 0;
	while (idfSentence.readTrainingSentence(in)) {
		if (idfSentence.getLength() == 0) {
			if (_output_mode == VERBOSE_OUTPUT) 
				cout << "Ignoring empty training sentence\n";
			continue;
		}

		idfSentence.removeFromTraining();
		addActiveLearningSentence(idfSentence);

		sentence_n++;
		_n_ALSentences++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << "\n";
			cout.flush();
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << "              \r";
		}
	}

	cout << "\n";
	in.close();
}

void PIdFModel::seedALTraining() {
	if (_nTrainSent != 0) {
		std::cout << "Add Sentences from corpus" << std::endl;
		addTrainingSentencesFromALCorpus(_nTrainSent);
	}
	else {
		std::string training_file = ParamReader::getParam("pidf_training_file");
		if (training_file == "") {
			throw UnrecoverableException("PIdFModel::seedALTraining()",
				"neither training_file nor _nsent specified");
		}
		if (ParamReader::isParamTrue("pidf_trainingfile_is_list")) {
			std::cout << "Add Sentences from training file list" << std::endl;
			addTrainingSentencesFromTrainingFileList(training_file.c_str(), false);
		} else {
			std::cout << "Add Sentences from training file" << std::endl;
			addTrainingSentencesFromTrainingFile(training_file.c_str(), false);
		}
	}
}
/*
//for debugging output
Symbol PIdFModel::getClusterSymbol(const wchar_t *pref, int c){
	wchar_t buff[50];
	wchar_t nbuff[50];
	_itow(c, nbuff, 10);
	wcscpy(buff,pref);
	wcscat(buff,L":");
	wcscat(buff, nbuff);
	return Symbol(buff);
}

void PIdFModel::analyzeTraining(const char* testfile){
	seekToFirstSentence();
	NgramScoreTable* words = _new NgramScoreTable(1, 100000);
	NgramScoreTable* clust = _new NgramScoreTable(1, 100000);

	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();

		for (int i = 0; i < sentence->getLength(); i++) {

			WordClusterClass wordClass(sentence->getWord(i));
			Symbol c8 = getClusterSymbol(L"c8", wordClass.c8());
			clust->add(&c8);
			Symbol c12 = getClusterSymbol(L"c12", wordClass.c12());
			clust->add(&c12);
			Symbol c16 = getClusterSymbol(L"c16", wordClass.c16());
			clust->add(&c16);
			Symbol c20 = getClusterSymbol(L"c20", wordClass.c20());
			clust->add(&c20);
			words->add(&sentence->getWord(i));
		}
	}
	//count training occurrences for 0; <=2, <=4; <=8; <=16; <=32; >=64
	int zero = 0;
	int lte2 = 1;
	int lte4 = 2;
	int lte8 = 3;
	int lte16 = 4;
	int lte32 = 5;
	int gte33 = 6;
	int* clustcounts = _new int[4*7];
	int* wordcounts =  _new int[7];
	for(int i=0; i< 7; i++){
		wordcounts[i] =0;
	}
	for(int i=0; i<4*7; i++){
		 clustcounts[i] = 0;
	}



	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(testfile);
	if (in.fail()) {
		throw UnexpectedInputException(
			"PIdFModel::analyzeTraining()",
			"Unable to open test file.");
	}
	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);
	int count =0;
	while (!in.eof() && idfSentence.readSexpSentence(in)) {
		std::cout<<count<<std::endl;
		for (int i = 0; i < idfSentence.getLength(); i++) {
			float f =	words->lookup(&(idfSentence.getWord(i)));
			addToCountArray(f, 0, wordcounts);
			WordClusterClass wordClass(idfSentence.getWord(i));
			Symbol c8 = getClusterSymbol(L"c8", wordClass.c8());
			f = clust->lookup(&c8);
			addToCountArray(f, 0, clustcounts);
			Symbol c12 = getClusterSymbol(L"c12", wordClass.c12());
			f = clust->lookup(&c12);
			addToCountArray(f, 1, clustcounts);
			Symbol c16 = getClusterSymbol(L"c16", wordClass.c16());
			f = clust->lookup(&c16);
			addToCountArray(f, 2, clustcounts);
			Symbol c20 = getClusterSymbol(L"c20", wordClass.c20());
			f = clust->lookup(&c20);
			addToCountArray(f, 3, clustcounts);
		}

	}
	in.close();
	UTF8OutputStream uos;
	char buff[1000];
	if (!ParamReader::getParam("pidf_output_file",buff,									 1000))	{
		throw UnexpectedInputException("PIdFModel::analyzeTraining()",
			"Parameter 'pidf_output_file' not specified");
	}
	strcat(buff, ".analysis.txt");
	uos.open(buff);
	uos<<"# of Times words in Test File Appeared in Training\n";
	uos<<"0\t "<<wordcounts[0]<<"\n";
	uos<<"<=2\t "<<wordcounts[1]<<"\n";
	uos<<"<=4\t "<<wordcounts[2]<<"\n";
	uos<<"<=8\t "<<wordcounts[3]<<"\n";
	uos<<"<=16\t "<<wordcounts[4]<<"\n";
	uos<<"<=32\t "<<wordcounts[5]<<"\n";
	uos<<">=33\t "<<wordcounts[6]<<"\n";
	uos<<"# of Times clusters in Test File Appeared in Training\n";
	uos<<"  \t8 \t12 \t16 \t20 \n";
	uos<<"0\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[0+i*7]<<" \t";
	}
	uos<<"\n";
	uos<<"<=2\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[1+i*7]<<" \t";
	}
	uos<<"\n";
	uos<<"<=4\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[2+i*7]<<" \t";
	}
	uos<<"\n";
	uos<<"<=8\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[3+i*7]<<" \t";
	}
	uos<<"\n";
	uos<<"<=16\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[4+i*7]<<" \t";
	}
	uos<<"\n";
	uos<<"<=32\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[5+i*7]<<" \t";
	}
	uos<<"\n";
	uos<<">=33\t ";
	for(int i=0; i<=3; i++){
		uos<<clustcounts[6+i*7]<<" \t";
	}
	uos<<"\n";

	delete wordcounts;
	delete clustcounts;
}

void PIdFModel::addToCountArray(float count, int offset, int* countarray){
	int zero = 0;
	int lte2 = 1;
	int lte4 = 2;
	int lte8 = 3;
	int lte16 = 4;
	int lte32 = 5;
	int gte33 = 6;
	if(count < 1){
		countarray[zero+7*offset]++;
	}
	else if(count <= 2){
		countarray[lte2+7*offset]++;
	}
	else if(count <= 4){
		countarray[lte4+7*offset]++;
	}
	else if(count <= 8){
		countarray[lte8+7*offset]++;
	}
	else if(count <= 16){
		countarray[lte16+7*offset]++;
	}
	else if(count <= 32){
		countarray[lte32+7*offset]++;
	}
	else{
		countarray[gte33+7*offset]++;
	}
}

*/

void PIdFModel::unsupervisedTrainAndSelect(bool print_models,int n_models,
										   double percent_of_training,
										   int count, int n_to_select)
{
	int i;
	
	//count the number of training sentences
	seekToFirstSentence();
	int n_train = 0;
	while (moreSentences()) {
		n_train++;
		getNextSentence();
	}
	
	PIdFSentence** train_sent = _new PIdFSentence*[n_train];
	bool* in_model = _new bool[n_train];
	NgramScoreTable* voc = _new NgramScoreTable(1, 10000);
	int ntags = _tagSet->getNTags();
	double* namevalues = _new double[ntags];
	enum selectione
	{MARGIN, NAMES, RAREWORDS, RARETYPES, NAMES_RAREWORDS, RAREWORDS_RARETYPES, NAMES_RAREWORDS_RARETYPES };
	int selection_method = MARGIN;

	std::string buffer = ParamReader::getParam("unsup_selection");
	if (!buffer.empty()) {
		if (buffer == "names") {
			selection_method = NAMES;
		}
		else if (buffer == "rare_words") {
			selection_method = RAREWORDS;
		}
		else if (buffer == "margin_only") {
			selection_method = MARGIN;
		}
		else if (buffer == "names_and_rare_words") {
			selection_method = NAMES_RAREWORDS;
		}
		else if (buffer == "rare_types") {
			selection_method = RARETYPES;
		}
		else if (buffer == "rare_words_rare_types") {
			selection_method = RAREWORDS_RARETYPES;
		}
		else if (buffer == "names_rare_words_rare_types") {
			selection_method = NAMES_RAREWORDS_RARETYPES;
		}
	}

	//put the training sentences into an array
	seekToFirstSentence();
	for (i = 0; i < n_train; i++) {
		train_sent[i] = getNextSentence();
	}

	//get training vocab for selection methods that require it
	if ((selection_method == RAREWORDS) || 
		(selection_method == NAMES_RAREWORDS) ||
		(selection_method == RAREWORDS_RARETYPES) ||
		(selection_method == NAMES_RAREWORDS_RARETYPES))
	{
		for (int i = 0; i < n_train; i++) {
			for (int j = 0; j < train_sent[i]->getLength(); j++) {
                                Symbol word = train_sent[i]->getWord(j);
				voc->add(&word);
			}
		}
	}

	for (i = 0; i < ntags; i++) {
		namevalues[i] = 0;
	}

	//get tag counts for selection methods that require it
	if ((selection_method == RARETYPES) ||
	    (selection_method == RAREWORDS_RARETYPES) ||
	    (selection_method == NAMES_RAREWORDS_RARETYPES))
	{
		for (int i = 0; i < n_train; i++) {
			for (int j = 0; j < train_sent[i]->getLength(); j++) {
				namevalues[train_sent[i]->getTag(j)]++;
			}
		}
	}

	double sumval = 0;
	for (i = 0; i < ntags; i++) {
		if (!_tagSet->isNoneTag(i)) {
			sumval += namevalues[i];
		}
		else {
			namevalues[i] = 0;
		}
	}
	double sumnamescore = 0;
	for (i = 0; i < ntags; i++) {
		if (namevalues[i] != 0) {
			namevalues[i] = 1 - (namevalues[i]/sumval);
		}
	}


	//make the n_models
	bool allow_repeats  = false;
	int nsent_per_model = static_cast<int>(ceil(percent_of_training * n_train));
	srand(count);
	PIdFModel** allModels = _new PIdFModel*[n_models];

	for (i = 0; i < n_models; i++) {
		std::cout << "--- Train model " << i << std::endl;
		if (!allow_repeats) {
			for (int j = 0; j < n_train; j++) {
				in_model[j] = false;
			}
		}
		std::cout << "\t Make model " << std::endl;
		allModels[i] = _new PIdFModel(UNSUP_CHILD);
		int nadded = 0;
		std::cout << "\t Add Training " <<std::endl;
		while (nadded < nsent_per_model) {
			//generate a sentence number
			double r = rand();
			int sent = static_cast<int>(floor((r/RAND_MAX)*(n_train -1)));
			//std::cout << "\t add sentence: " << nadded << " -"
			//	<< sent <<" out of "<<n_train<<std::endl;
			if (allow_repeats || !in_model[sent]) {
				allModels[i]->addTrainingSentence(*train_sent[sent]);
				in_model[sent] = true;
				nadded++;
			}
		}
		std::cout << "train" << std::endl;
		allModels[i]->train();
		allModels[i]->finalizeWeights();
	}
	delete in_model;
	delete train_sent;
	
	std::cout << "Finished Training all models " << std::endl;
	std::cout.flush();

	PIdFSentence* sentence_results = _new PIdFSentence[n_models];
	//int tags[MAX_SENTENCE_TOKENS+2];
	Symbol decodedAnswer[MAX_SENTENCE_TOKENS+2];
	double score = 0;
	int ndecoded = 0;
	double marginsum;
	double thismargin;
	bool start_from_begining = false;
	int poolsize = 20000;	//20000
	int nacceptable = 0;
	//seekToFirstALSentence();
	PIdFSentence* firstsentence = 0;

	while (ndecoded < poolsize) {
		marginsum = 0;
		PIdFSentence* sentence;
		if (moreALSentences()) {
			sentence = getNextALSentence();
		}
		else if (!start_from_begining) {
			seekToFirstALSentence();
			start_from_begining = true;
			sentence = getNextALSentence();
		}
		else {
			break;
		}
		if (firstsentence == 0) {
			firstsentence = sentence;
		}
		else if (sentence == firstsentence) {
			break;
		}

		if (_allowSentenceRepeats || !sentence->inTraining()) {
			int n_observations = sentence->getLength() + 2;
			populateSentence(_observations, sentence);
			bool acceptable = true;
			ndecoded++;
			for (int i = 0; i < n_models; i++) {
				sentence_results[i].populate(*sentence);
				thismargin = allModels[i]->_decoder->decodeAllTags(_observations);
				//thismargin = 1;
				if (thismargin == 0) {
					// break out of loop, since we won't allow this sentence
					acceptable = false;
					break;
				}
				marginsum += thismargin;
				//do a decode of the sentence
				allModels[i]->decode(sentence_results[i]);
				/*for (int j = 0; j < sentence_results[i].getLength(); j++) {
					sentence_results[i].setTag(j,sentence->getTag(j));
				}*/
				if (i > 0) {
					//check to see that all name tags agree with model 0's predictions
					for (int j = 0; j < sentence_results[i].getLength(); j++) {
						if (sentence_results[i].getTag(j) != sentence_results[0].getTag(j)) {
							acceptable = false;
							goto stop;
						}
					}
				}
				else {
					bool hasname = false;
					for (int j = 0; j < sentence_results[i].getLength(); j++) {
						if (!_tagSet->isNoneTag(sentence_results[i].getTag(j))) {
							hasname = true;
							break;
						}
					}
					if (!hasname) {
						acceptable = false;
						break;
					}
				}
			}
			stop: if (acceptable) {
				double avgmargin = marginsum/n_models;
				//decoded_sentences[ndecoded].populate(sentence_results[0]);
				int nname = 0;
				int nunk = 0;
				int RARE = 5;
				double namescore = 0;
				for (int j = 0; j < sentence->getLength(); j++) {
					sentence->setTag(j, sentence_results[0].getTag(j));
					if (!_tagSet->isNoneTag(sentence_results[0].getTag(j))) {
						nname++;
					}
                                        Symbol word(sentence_results[0].getWord(j));
					if (voc->lookup(&word) < RARE) {
						nunk++;
					}
					namescore += namevalues[sentence->getTag(j)];
				}
				SentAndScore thissent(avgmargin, sentence, nname);
				thissent.realmargin = avgmargin;
				thissent.unk = nunk;
				thissent.rare_tag_val = namescore;
				_scoredSentences.push_back(thissent);
				nacceptable++;
			}
		}
	}
	std::cout << "decoded: " << ndecoded << " sentences" << std::endl;
	std::cout << nacceptable << " sentences are ok" << std::endl;
	std::cout.flush();
	/*
		at this point _scoredSentences contains sentences for which
		1) All Models agree
		2) No Model has a margin of 0
		3) There is at least one name
		-----
		We want to sort sentences so that we get sentences with higher margins, and  a higher # of names

	*/
	if (nacceptable > 0) {
		ScoredSentenceVectorIt begin;
		ScoredSentenceVectorIt end;
		ScoredSentenceVectorIt curr;

		begin = _scoredSentences.begin();
		end = _scoredSentences.end();
		curr = begin;
		double totalmargin = 0;
		int totalnames = 0;
		int totalrare = 0;
		double totalraretag = 0;
		while (curr != end) {
			totalmargin += (*curr).margin;
			totalnames += (*curr).id;
			totalrare += (*curr).unk;
			totalraretag += (*curr).rare_tag_val;
			curr++;
		}
		curr =  begin;
		while (curr != end) {
			if (selection_method == NAMES ) {
				(*curr).margin = (((*curr).margin/totalmargin)+(static_cast<double>((*curr).id)/totalnames));
			}
			else if (selection_method == RAREWORDS) {
				(*curr).margin = (((*curr).margin/totalmargin)+(static_cast<double>((*curr).unk)/totalrare));
			}
			else if (selection_method == NAMES_RAREWORDS) {
				(*curr).margin = (((*curr).margin/totalmargin)
					+static_cast<double>((*curr).id)/totalnames
					+ static_cast<double>((*curr).unk)/totalrare);
			}
			else if (selection_method == NAMES_RAREWORDS_RARETYPES) {
				(*curr).margin = (((*curr).margin/totalmargin)
					+static_cast<double>((*curr).id)/totalnames
					+ static_cast<double>((*curr).unk)/totalrare
					+ static_cast<double>((*curr).rare_tag_val)/totalraretag);
			}
			else if (selection_method == RAREWORDS_RARETYPES) {
				(*curr).margin = (((*curr).margin/totalmargin)
					+ static_cast<double>((*curr).unk)/totalrare
					+ static_cast<double>((*curr).rare_tag_val)/totalraretag);
			}
			else if (selection_method == RARETYPES) {
				(*curr).margin = (((*curr).margin/totalmargin)
					+ static_cast<double>((*curr).rare_tag_val)/totalraretag);
			}
			else {
				(*curr).margin = ((*curr).margin/totalmargin);
			}
			curr++;
		}
		sort(_scoredSentences.begin(), _scoredSentences.end());
		curr = end;
		int nadded = 0;
		std::cout << "Total names: " << totalnames;
		std::cout << "\nTotalMargin: " << totalmargin << std::endl;
		for (int i = 0; i < _nToAdd; i++) {
			curr--;
			if (curr == begin) {
				break;
			}
			std::cout << "Add sentence with score: " << (*curr).margin
				<< "\t origmarg: " <<(*curr).realmargin
				<< "\t nnames: " <<(*curr).id
				<< "\t nrare: " <<(*curr).unk
				<< "\t nrare_tag: " <<(*curr).rare_tag_val<<std::endl;

			addTrainingSentence(*(*curr).sent);
			(*curr).sent->addToTraining();
			nadded++;
		}
		std::cout << "********* added: " << nadded << std::endl;
	}

	//clean up
	for (i = 0; i < n_models; i++) {
		allModels[i]->freeWeights();
		delete allModels[i];
	}
	delete namevalues;
	delete allModels;
	delete sentence_results;
}



void PIdFModel::doUnsupervisedTrainAndSelect() {
	char buffer[50];
	sprintf(buffer, "unsup-initial");
	writeTrainingSentences(buffer);
	seekToFirstALSentence();

	for (int i = 0; i < _nActiveLearningIter; i++) {
		unsupervisedTrainAndSelect(false, _n_child_models, _child_percent_training, i, _nToAdd);
		sprintf(buffer, "unsup-%d", i);
		writeTrainingSentences(buffer);
	}
}

void PIdFModel::compareWeights(PIdFModel& other, int n) {

	int i = 0;
	DTFeature::FeatureWeightMap::iterator iter = _defaultWeights->begin();
	DTFeature::FeatureWeightMap::iterator end = _defaultWeights->end();
	DTFeature::FeatureWeightMap::iterator othiter = other._defaultWeights->begin();
	DTFeature::FeatureWeightMap::iterator othend = other._defaultWeights->end();
	for (i = 0; i < n; i++) {
		++iter;
		++othiter;
		if (iter == end) {
			break;
		}
		std::wstring str = L"";
		(*iter).first->toString(str);
		Symbol mysym = Symbol(str.c_str());
		if (othiter == othend) {
			break;
		}
		str = L"";
		(*othiter).first->toString(str);
		Symbol othsym = Symbol(str.c_str());
		//if ((*iter).second.getSum() != (*othiter).second.getSum()) {
		//	std::cout << i << ": " << mysym.to_debug_string() << "\t " << (*iter).second.getSum()
		//		<< "\t " << othsym.to_debug_string() << "\t " << (*othiter).second.getSum() << std::endl;
		//	}
		//PWeight *value = _weights->get(feature);
		double othval = **(other._defaultWeights->get((*iter).first));
		if (othval != *((*iter).second)) {
			std::cout << i << ": "<<mysym.to_debug_string() << "\t " << *(*iter).second
				<< "\t " << othsym.to_debug_string() << "\t " << *(*othiter).second << std::endl;
		}
	}
}

void PIdFModel::finalizeWeights() {
	_decoder->finalizeWeights();
}

void PIdFModel::outputDate(UTF8OutputStream& out) {
	time_t currentTime;
	time( &currentTime );
	wchar_t tmpbuf[256];
	wcsftime( tmpbuf, 256,
		L"Time stamp: %B %d, %Y at %H:%M.\n", localtime( &currentTime ));
	out << tmpbuf << L"\n";
	out.flush();
}







