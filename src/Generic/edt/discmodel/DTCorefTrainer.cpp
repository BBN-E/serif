// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/WordConstants.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefTrainer.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/PreLinker.h"
#include "Generic/edt/discmodel/CorefUtils.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/metonymy/MetonymyAdder.h"
#include "Generic/propositions/PropositionFinder.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/trainers/CorefDocument.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/state/StateLoader.h"
#include "Generic/wordClustering/WordClusterTable.h"

#include <iostream>
#include <stdio.h>
#include <cctype>
#include <limits>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

#define MIN_VAL -10000000
#define NO_LINK_OBS_INDEX	0
#define LINK_OBS_INDEX		1

const int DTCorefTrainer::ENTITY_LINK_MAX = 500;
const int DTCorefTrainer::MAX_CANDIDATES = 100;

// this is intended for development mode, when one might want to save
// the model after each iteration

const float targetLoadingFactor = static_cast<float>(0.7);
DTCorefTrainer::DTCorefTrainer()
	: MODE(TRAIN), SEARCH_STRATEGY(CLOSEST), MODEL_TYPE(MAX_ENT),
	TRAIN_SOURCE(STATE_FILES), TARGET_MENTION_TYPE(Mention::DESC),
	  _filter_by_entity_type(true), _filter_by_entity_subtype(false),
	  _train_from_left_to_right_on_sentence(false),
	  _list_mode(false),
	  _featureTypes(0), _noneFeatureTypes(0), _featureTypesArr(0),
	  _tagSet(0), _tagScores(0),
	  _n_observations(0), _observations(0), _infoMap(0),
	  _use_metonymy(false), _metonymyAdder(0),
	  _maxEntDecoder(0), _maxEntWeights(0),
	  _pruning(0), _percent_held_out(0),
	  _mode(MaxEntModel::SCGIS), _max_iterations(0),
	  _variance(0), _likelihood_delta(0.0001),
	  _stop_check_freq(1), _link_threshold(0.5),
	  _p1Decoder(0), _p1Weights(0),  
	  _epochs(0), _n_instances_seen(0),
	  _n_correct(0), _total_n_correct(0),
	  _use_non_ace_entities_as_no_links(false),
	  _max_non_ace_candidates(0), _use_no_link_examples(false),
	  _do_names_first(false), _limit_to_names(false), 
	  _use_p1_averaging(true), _p1_required_margin(0),
	  _print_model_every_epoch(false),
	  _propositionFinder(0),
	  _alreadySeenDocumentEntityIDtoTrainingID(0),
	  _correct_link(0), _correct_nolink(0), _missed(0), _wrong_link(0),
	  _spurious(0), _correct_ACE_nolink(0), _ace_spurious(0),
	  _correct_nonACE_nolink(0), _nonACE_spurious(0),
	  _considerWithinSentenceLinksOnly(false), _considerOutsideSentenceLinksOnly(false),
	  _correct_nolink_within_document(0), _correct_nolink_within_sentence(0), _correct_nolink_new_entity(0),
	  _spurious_nolink_within_sentence(0), _spurious_nolink_within_document(0), _spurious_nolink_new_entity(0),
	  _correct_within_sentence(0), _correct_outside_sentence(0),
	  _missed_within_sentence(0), _missed_outside_sentence(0),
	  _wrong_link_within_sentence(0), _wrong_link_outside_sentence(0),
	  DEBUG(false)
	  // these structures used only by AUG_PARSES methods, which are now deprecated
	  //,_pronounClassifier(PronounClassifier::build()), _searcher(false)  
{

	std::string buffer;

	WordClusterTable::ensureInitializedFromParamFile();
	PreLinker::setSpecialCaseLinkingSwitch(true);
	PreLinker::setEntitySubtypeFilteringSwitch(false);


	// COREF MODEL TYPE -- MaxEnt, P1 or P1_RANKING
	buffer = ParamReader::getRequiredParam("dt_coref_model_type");
	if (boost::iequals(buffer, "maxent")){
		MODEL_TYPE = MAX_ENT;
	}else if (boost::iequals(buffer, "p1")){
		MODEL_TYPE = P1;
	}else if (boost::iequals(buffer, "both")){
	}else if (boost::iequals(buffer, "P1_RANKING")){
		MODEL_TYPE = P1_RANKING;
	}else {
		throw UnrecoverableException("DTCorefTrainer::DTCorefTrainer()",
			"Parameter 'dt_coref_model_type' not recognized");
	}


	// P1_RANKING
	// We use many observations for P1_RANKING
	// for the other training schemes we will only use _observations[1];
	_n_observations = (MODEL_TYPE == P1_RANKING) ? ENTITY_LINK_MAX+1 : 2;
	// DTNoneCorefObservation can only have features that are solely dependent on the mention
	// not on any entity.
	_observations.push_back(_new DTNoneCorefObservation());
	for(int i=1; i<_n_observations; i++){
		_observations.push_back(_new DTCorefObservation());
	}

	// DEBUGGING
	buffer = ParamReader::getParam("dt_coref_train_debug");
	DEBUG = false;
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}
	// FILTERING BY ENTITY TYPE AND SUBTYPE	
	buffer = ParamReader::getParam("do_coref_entity_type_filtering");
	if (buffer == "false")
		_filter_by_entity_type = false;
	_filter_by_entity_subtype = ParamReader::isParamTrue("do_coref_entity_subtype_filtering");

	if (_filter_by_entity_subtype)
		PreLinker::setEntitySubtypeFilteringSwitch(true);


	// TARGET MENTION TYPE -- DESC or PRON
	buffer = ParamReader::getRequiredParam("dt_coref_target_mention_type");
	if (boost::iequals(buffer, "desc"))
		TARGET_MENTION_TYPE = Mention::DESC;
	else if (boost::iequals(buffer, "pron"))
		TARGET_MENTION_TYPE = Mention::PRON;
	else if (boost::iequals(buffer, "name"))
		TARGET_MENTION_TYPE = Mention::NAME;
	else {
		throw UnrecoverableException("DTCorefTrainer::DTCorefTrainer()",
			"Parameter 'dt_coref_target_mention_type' must be 'DESC' or 'PRON'");
	}


	// SEARCH TYPE -- closest or best
	buffer = ParamReader::getRequiredParam("dt_coref_search_strategy");
	if (boost::iequals(buffer, "closest")) {
		SEARCH_STRATEGY = CLOSEST;
	} else if (boost::iequals(buffer, "best")) {
		SEARCH_STRATEGY = BEST;
	} else {
		throw UnrecoverableException("DTCorefTrainer::DTCorefTrainer()",
			"Parameter 'dt_coref_search_strategy' not recognized");
	}

	_use_non_ace_entities_as_no_links = ParamReader::isParamTrue("dt_coref_use_non_ACE_ents_as_no_links");
	if (_use_non_ace_entities_as_no_links) {
		if (_filter_by_entity_type || _filter_by_entity_subtype)
			throw UnrecoverableException("DTCorefTrainer::sortEntires()",
				"'filter-by-type' or 'filter-by-type' should not be used with 'use-non-ACE-entities'");
		_max_non_ace_candidates = ParamReader::getRequiredIntParam("dt_coref_use_non_ACE_ents_as_no_links_max_candidates");
		if (_max_non_ace_candidates == 0)
			throw UnrecoverableException("DTCorefTrainer::DTCorefTrainer()",
			"Parameter 'dt_coref_use_non_ACE_ents_as_no_links_max_candidates' not recognized");
	}
	
	
	_use_no_link_examples = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_no_links_examples", false);

	// MODEL OUTSIDE AND WITHIN SENTENCE LINKS SEPARATELY
	bool model_outside_and_within_sentence_links_separately = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_pron_model_outside_and_within_sentence_links_separately", false);
	if (model_outside_and_within_sentence_links_separately) {
		_considerWithinSentenceLinksOnly = ParamReader::getOptionalTrueFalseParamWithDefaultVal("consider_within_sentence_links_only", false);
		_considerOutsideSentenceLinksOnly = ParamReader::getOptionalTrueFalseParamWithDefaultVal("consider_outside_sentence_links_only", false);

		if (TARGET_MENTION_TYPE != Mention::PRON) {
			throw UnexpectedInputException("DTCorefTrainer::DTCorefTrainer()",
				"Parameter 'dt_pron_model_outside_and_within_sentence_separately' only valid when 'dt_coref_target_mention_type' is PRON.");
		}
		if ((_considerWithinSentenceLinksOnly && _considerOutsideSentenceLinksOnly) ||
			(!_considerWithinSentenceLinksOnly && !_considerOutsideSentenceLinksOnly))
		{
			throw UnexpectedInputException("DTCorefTrainer::DTCorefTrainer()",
				"Parameters 'consider_within_sentence_links_only' and 'consider_outside_sentence_links_only' cannot have matching values.");
		}
	}

	// SOURCE OF TRAINING DATA -- state-files or aug-parses
	buffer = ParamReader::getRequiredParam("dt_coref_train_source");
	if (boost::iequals(buffer, "state_files")) {
		TRAIN_SOURCE = STATE_FILES;
	}
	else if (boost::iequals(buffer, "aug_parses")) {
		TRAIN_SOURCE = AUG_PARSES;
		throw UnexpectedInputException("DTCorefTrainer::DTCorefTrainer()",
                                       "The aug_parses train source mode is deprecated.");
	}
	else {
		throw UnexpectedInputException("DTCorefTrainer::DTCorefTrainer()", 
			"Invalid parameter value for 'dt_coref_train_source'.  Must be 'state_files'.");
	}

	_train_from_left_to_right_on_sentence = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_coref_train_from_left_to_right_on_sentence", false);

	// DO NAMES FIRST (deals with appositives (for Arabic))
	_do_names_first = ParamReader::isParamTrue("do_coref_link_names_first");
	if (_do_names_first) {
		if (!_train_from_left_to_right_on_sentence) {
			throw UnrecoverableException("DTCorefTrainer::DTCorefTrainer()",
				"Invalid value for parameters 'do_coref_link_names_first'=true when the usual training is names then nominals than pronouns.\n This parameter should only be defined true if 'dt_coref_train_from_left_to_right_on_sentence' is true");
		}
		if (TARGET_MENTION_TYPE == Mention::NAME){
			throw UnrecoverableException("DTCorefTrainer::DTCorefTrainer()",
				"Invalid value for parameters 'do_coref_link_names_first=true && dt_coref_target_mention_type=name'");
		}
	} 

	_print_model_every_epoch = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_coref_print_model_every_epoch", false);

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("dt_coref_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	// Initialize the AbbrevTable and the mention information mapper. (This should be done before initializing the features)
	if(TARGET_MENTION_TYPE == Mention::NAME) {
		_infoMap = _new DocumentMentionInformationMapper();
	}else {
		_infoMap = NULL;
	}

	// FEATURES
	DTCorefFeatureTypes::ensureFeatureTypesInstantiated();
	std::string features_file = ParamReader::getRequiredParam("dt_coref_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), DTCorefFeatureType::modeltype);

	if (MODEL_TYPE == P1_RANKING){
		_noneFeatureTypes = DTCorefFeatureTypes::makeNoneFeatureTypeSet(TARGET_MENTION_TYPE);
		_featureTypesArr = _new DTFeatureTypeSet*[_tagSet->getNTags()];
		_featureTypesArr[_tagSet->getNoneTagIndex()] = _noneFeatureTypes;
		_featureTypesArr[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _featureTypes;
	}

	// MODEL FILE NAME
	_model_file = ParamReader::getRequiredParam("dt_coref_model_file");

	// TRAINING DATA FILE
	_training_file = ParamReader::getRequiredParam("dt_coref_training_file");
	_list_mode = ParamReader::isParamTrue("dt_coref_training_list_mode");

	// PROP FINDER
	_propositionFinder = _new PropositionFinder();

	// LIMIT LINKING JUST TO NAMES
	_limit_to_names = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_coref_link_only_to_names", false);

	// in order for metonymy to take effect "use-metonymy" parameter should be set to 'true'
	_use_metonymy = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_coref_use_metonymy", false);
	if (_use_metonymy)
		_metonymyAdder = MetonymyAdder::build();

}

DTCorefTrainer::~DTCorefTrainer() {
	
	delete _featureTypes;
	if(MODEL_TYPE == P1_RANKING){
		delete _noneFeatureTypes;
		delete [] _featureTypesArr;
	}

	delete _tagSet;
	delete _tagScores;

	for(std::vector<DTNoneCorefObservation*>::iterator i = _observations.begin(); i != _observations.end(); ++i) {
		delete *i;
	}
	_observations.clear();
	
	
	delete _propositionFinder;
}


void DTCorefTrainer::train() {

	_p1Weights = 0;
	_p1Decoder = 0;
	_maxEntWeights = 0;
	_maxEntDecoder = 0;
	_epochs = 1;

	if (MODEL_TYPE == BOTH) {
		throw UnexpectedInputException("DTCorefTrainer::train()",
									 "You can't train both types of models at once.");
	} else if (MODEL_TYPE == MAX_ENT) {

		// TRAIN MODE
		std::string param_mode = ParamReader::getRequiredParam("maxent_trainer_mode");
		if (param_mode == "GIS")
			_mode = MaxEntModel::GIS;
		else if (param_mode == "SCGIS")
			_mode = MaxEntModel::SCGIS;
		else
			throw UnexpectedInputException("DTCorefTrainer::train()",
							"Invalid setting for parameter 'maxent_trainer_mode'");

		// PRUNING
		_pruning = ParamReader::getRequiredIntParam("maxent_trainer_pruning_cutoff");

		// PERCENT HELD OUT
		_percent_held_out = ParamReader::getRequiredIntParam("maxent_trainer_percent_held_out");
		if (_percent_held_out < 0 || _percent_held_out > 50)
			throw UnexpectedInputException("DTCorefTrainer::train()",
				"Parameter 'maxent_trainer_percent_held_out' must be between 0 and 50");

		// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
		_max_iterations = ParamReader::getRequiredIntParam("maxent_trainer_max_iterations");

		// GAUSSIAN PRIOR VARIANCE
		_variance = ParamReader::getRequiredFloatParam("maxent_trainer_gaussian_variance");

		// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
		_likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("maxent_min_likelihood_delta", .0001);

		// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
		_stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("maxent_stop_check_frequency", 1);

		_train_vector_file = ParamReader::getParam("maxent_train_vector_file");
		_test_vector_file = ParamReader::getParam("maxent_test_vector_file");

		const char* train_vf = 0;
		if (!_train_vector_file.empty())
			train_vf = _train_vector_file.c_str();
		const char* test_vf = 0;
		if (!_test_vector_file.empty())
			test_vf = _test_vector_file.c_str();	

		_maxEntWeights = _new DTFeature::FeatureWeightMap();
		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxEntWeights,
								   _mode, _percent_held_out, _max_iterations, _variance,
								   _likelihood_delta, _stop_check_freq,
								   train_vf, test_vf);


	} else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		_epochs = ParamReader::getRequiredIntParam("dt_coref_trainer_epochs");
		if(_epochs<1)
			throw UnexpectedInputException("DTCorefTrainer::train()",
				"Parameter 'dt_coref_trainer_epochs' must be defined and lager than 0");
		_p1Weights = _new DTFeature::FeatureWeightMap();
		if(MODEL_TYPE == P1){
			_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights);
		} else {// RANKING
			_p1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _p1Weights);
		}


		_use_p1_averaging = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_coref_use_p1_averaging",true);

		_p1_required_margin = ParamReader::getOptionalIntParamWithDefaultValue("dt_coref_p1_required_margin", 0);
	}

	if (MODEL_TYPE == MAX_ENT) {
		if (TRAIN_SOURCE == STATE_FILES)
			trainMaxEntStateFiles();
		//else
		//	trainMaxEntAugParses();
	} else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		if (TRAIN_SOURCE == STATE_FILES)
			trainP1StateFiles();
		//else
		//	trainP1AugParses();
	}

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	if (_p1Weights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights->begin();
			iter != _p1Weights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}

	if (_maxEntWeights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _maxEntWeights->begin();
			iter != _maxEntWeights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}
}


void DTCorefTrainer::devTest() {
	MODE = DEVTEST;

	_p1Weights = 0;
	_p1Decoder = 0;
	_maxEntWeights = 0;
	_maxEntDecoder = 0;
	_epochs = 1;

	std::string param = ParamReader::getRequiredParam("dt_coref_devtest_out");
	_devTestStream.open(param.c_str());

	if (MODEL_TYPE == P1_RANKING){

		_p1Weights = _new DTFeature::FeatureWeightMap();
		std::string file = _model_file + "-rank";
		DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _p1Weights, false);
		
	}
	if (MODEL_TYPE == P1 || MODEL_TYPE == BOTH) {

		_p1Weights = _new DTFeature::FeatureWeightMap();
		std::string file = _model_file + "-p1";
		DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);
		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, 0, false);

	}

	if (MODEL_TYPE == MAX_ENT || MODEL_TYPE == BOTH) {

		_maxEntWeights = _new DTFeature::FeatureWeightMap();
		std::string file = _model_file + "-maxent";
		DTFeature::readWeights(*_maxEntWeights, file.c_str(), DTCorefFeatureType::modeltype);
		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxEntWeights);

		_link_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_coref_maxent_link_threshold", 0.5);

	}

	_correct_link = 0;
	_correct_nolink = 0;
	_missed = 0;
	_spurious = 0;
	_wrong_link = 0;
	_correct_ACE_nolink = 0;
	_ace_spurious = 0;
	_correct_nonACE_nolink = 0;
	_nonACE_spurious = 0;

	_correct_nolink_within_document = 0;
	_correct_nolink_within_sentence = 0;
	_correct_nolink_new_entity = 0;
	_spurious_nolink_within_sentence = 0;
	_spurious_nolink_within_document = 0;
	_spurious_nolink_new_entity = 0;
	_correct_within_sentence = 0;
	_correct_outside_sentence = 0;
	_missed_within_sentence = 0;
	_missed_outside_sentence = 0;
	_wrong_link_within_sentence = 0;
	_wrong_link_outside_sentence = 0;


	if (TRAIN_SOURCE == STATE_FILES)
		devTestStateFiles();
	//else // TRAIN_SOURCE == AUG_PARSES
	//	devTestAugParses();

	_devTestStream << "<U><B>RESULTS ON ONLY ACE MENTIONS </B></U><br>\n";
	_devTestStream << "CORRECT: " << _correct_link + _correct_ACE_nolink;
	_devTestStream << " (" << _correct_link << " LINK, " << _correct_ACE_nolink << " NOLINK)<br>\n";
	_devTestStream << "MISSED: " << _missed << "<br>\n";
	_devTestStream << "SPURIOUS(forward links): " << _ace_spurious << "<br>\n";
	_devTestStream << "WRONG LINK: " << _wrong_link << "<br>\n";
	int total_ACE_links = _missed + _ace_spurious + _wrong_link + _correct_link + _correct_ACE_nolink;
	_devTestStream << "TOTAL(ACE): " << total_ACE_links << "<br>\n";
	_devTestStream << "<br>\n";

	int within_sentence_total = _missed_within_sentence + _spurious + _wrong_link_within_sentence + _correct_within_sentence + _correct_nolink;
	_devTestStream << "<U><B>RESULTS INCLUDING ONLY MENTIONS THAT LINK WITHIN THE SAME SENTENCE</B></U><br>\n";
	_devTestStream << "CORRECT: " << _correct_within_sentence + _correct_nolink;
	_devTestStream << " (";
	_devTestStream << _correct_within_sentence << " LINK, ";
	_devTestStream << _correct_nolink_within_document << " NOLINK-DOCUMENT, ";
	_devTestStream << _correct_nolink_within_sentence << " NOLINK-SENTENCE, ";
	_devTestStream << _correct_nolink_new_entity << " NOLINK-NEWENT";
	_devTestStream << ")<br>\n";
	_devTestStream << "MISSED: " << _missed_within_sentence << "<br>\n";
	_devTestStream << "SPURIOUS: " << _spurious;
	_devTestStream << " (";
	_devTestStream << _spurious_nolink_within_document << " NOLINK-DOCUMENT, ";
	_devTestStream << _spurious_nolink_within_sentence << " NOLINK-SENTENCE, ";
	_devTestStream << _spurious_nolink_new_entity << " NOLINK-NEWENT";
	_devTestStream << ")<br>\n";
	_devTestStream << "WRONG LINK: " << _wrong_link_within_sentence << "<br>\n";
	_devTestStream << "TOTAL: " << within_sentence_total << "<br>\n";
	_devTestStream << "<br>\n";

	int outside_sentence_total = _missed_outside_sentence + _spurious + _wrong_link_outside_sentence + _correct_outside_sentence + _correct_nolink;
	_devTestStream << "<U><B>RESULTS INCLUDING ONLY MENTIONS THAT LINK OUTSIDE THE SAME SENTENCE</B></U><br>\n";
	_devTestStream << "CORRECT: " << _correct_outside_sentence + _correct_nolink;
	_devTestStream << " (";
	_devTestStream << _correct_outside_sentence << " LINK, ";
	_devTestStream << _correct_nolink_within_document << " NOLINK-DOCUMENT, ";
	_devTestStream << _correct_nolink_within_sentence << " NOLINK-SENTENCE, ";
	_devTestStream << _correct_nolink_new_entity << " NOLINK-NEWENT";
	_devTestStream << ")<br>\n";
	_devTestStream << "MISSED: " << _missed_outside_sentence << "<br>\n";
	_devTestStream << "SPURIOUS: " << _spurious;
	_devTestStream << " (";
	_devTestStream << _spurious_nolink_within_document << " NOLINK-DOCUMENT, ";
	_devTestStream << _spurious_nolink_within_sentence << " NOLINK-SENTENCE, ";
	_devTestStream << _spurious_nolink_new_entity << " NOLINK-NEWENT";
	_devTestStream << ")<br>\n";
	_devTestStream << "WRONG LINK: " << _wrong_link_outside_sentence << "<br>\n";
	_devTestStream << "TOTAL: " << outside_sentence_total << "<br>\n";
	_devTestStream << "<br>\n";

	bool _show_non_ACE = true; //  FOR DEVTEST
	int total_links = _missed + _spurious + _wrong_link + _correct_link + _correct_nolink;
	if (_show_non_ACE) {
		_devTestStream << "<U><B>RESULTS INCLUDING NON-ACE MENTIONS </B></U><br>\n";
		_devTestStream << "CORRECT: " << _correct_link + _correct_nolink;
		_devTestStream << " (" << _correct_link << " LINK, " << _correct_nolink << " NOLINK)<br>\n";
		_devTestStream << "SPURIOUS: " << _spurious << "<br>\n";
		_devTestStream << "TOTAL: " << total_links << "<br>\n";
		_devTestStream << "<br>\n";
	}

	if (_show_non_ACE) {
		_devTestStream << "<U><B>RESULTS FOR NO-LINK MENTIONS </B></U><br>\n";
		_devTestStream << "FORWARD-COREF(ACE) CORRECT NO-LINK: " << _correct_ACE_nolink << "<br>\n";
		_devTestStream << "FORWARD-COREF(ACE) SPURIOUS: " << _ace_spurious << "<br>\n";
		_devTestStream << "<br>\n";

		_devTestStream << "NON-ACE CORRECT NO-LINK: " << _correct_nonACE_nolink << "<br>\n";
		_devTestStream << "NON-ACE SPURIOUS: " << _nonACE_spurious << "<br>\n";
		_devTestStream << "<br>\n";
		float correct_percent = (float) (_correct_link + _correct_nolink) / total_links;
		_devTestStream << "\n% CORRECT(All): " << correct_percent << "<br>\n";
	}
	float correct_percent_ACE = (float) (_correct_link + _correct_ACE_nolink) / total_ACE_links;
	_devTestStream << "\n<b>% CORRECT(ACE): " << correct_percent_ACE << "</b><br>\n";

	float correct_percent_within_sentence = (float) (_correct_within_sentence + _correct_nolink) / within_sentence_total;
	float correct_percent_outside_sentence = (float) (_correct_outside_sentence + _correct_nolink) / outside_sentence_total;
	_devTestStream << "\n<b>% CORRECT(WITHIN SENTENCE): " << correct_percent_within_sentence << "</b><br>\n";
	_devTestStream << "\n<b>% CORRECT(OUTSIDE SENTENCE): " << correct_percent_outside_sentence << "</b><br>\n";
	
	_devTestStream.flush();


	delete _p1Weights;
	delete _maxEntWeights;
	delete _p1Decoder;
	delete _maxEntDecoder;
}

void DTCorefTrainer::trainMaxEntStateFiles() {
	loadTrainingDataFromStateFiles();

	for (std::vector<DocTheory *>::reverse_iterator iter = _docTheories.rbegin(); iter != _docTheories.rend(); ++iter) {
		processDocument(*iter);
	}

	SessionLogger::info("SERIF") << "Deriving model...\n";
	_maxEntDecoder->deriveModel(_pruning);
	
	writeWeights();

	BOOST_FOREACH(DocTheory *docTheory, _docTheories) 
		delete docTheory;
	_docTheories.clear();
}

void DTCorefTrainer::trainP1StateFiles() {
	loadTrainingDataFromStateFiles();
	
	SessionLogger::info("SERIF") << "\n";
	for (int epoch = 0; epoch < _epochs; epoch++) {
		SessionLogger::info("SERIF") << "Epoch " << epoch << "...\n";
		_n_instances_seen = 0;
		_n_correct = 0;
		_total_n_correct = 0;
		for (std::vector<DocTheory *>::reverse_iterator iter = _docTheories.rbegin(); iter != _docTheories.rend(); ++iter) {
			processDocument(*iter);
		}	
		SessionLogger::info("SERIF") << "FINAL: (" << _n_instances_seen << " seen): " << _total_n_correct
		      << " (" << 100.0*_total_n_correct/_n_instances_seen << "%)" << "\n";
		writeWeights(epoch);
	}

	writeWeights();

	BOOST_FOREACH(DocTheory *docTheory, _docTheories)
		delete docTheory;
	_docTheories.clear();
}

void DTCorefTrainer::loadTrainingDataFromStateFiles() {
	if (_list_mode) {

		boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build(_training_file.c_str()));
		UTF8InputStream& filelist(*filelist_scoped_ptr);
		UTF8Token filename;
		while (!filelist.eof()) {
			filelist >> filename;
			if (wcscmp(L"", filename.chars()) == 0) 
				continue;
			else {
				loadTrainingDataFromStateFile(filename.chars());
			}
		}
	}
	else {
		loadTrainingDataFromStateFile(_training_file.c_str());
	}
}

void DTCorefTrainer::loadTrainingDataFromStateFile(const wchar_t *filename) {
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, filename, 500);
	loadTrainingDataFromStateFile(state_file_name_str);
}

void DTCorefTrainer::loadTrainingDataFromStateFile(const char *filename) {
	SessionLogger::info("SERIF") << "Loading data from " << filename << "\n";
	
	StateLoader *stateLoader = _new StateLoader(filename);
	int num_docs = TrainingLoader::countDocumentsInFile(filename);

	wchar_t state_tree_name[100];
	wcscpy(state_tree_name, L"DocTheory following stage: doc-relations-events");

	for (int i = 0; i < num_docs; i++) {
		DocTheory *docTheory = _new DocTheory(static_cast<Document*>(0));
		docTheory->loadFakedDocTheory(stateLoader, state_tree_name);
		docTheory->resolvePointers(stateLoader);
		_docTheories.push_back(docTheory);	
	}

	delete stateLoader;
}


void DTCorefTrainer::processDocument(DocTheory *docTheory) {
	EntitySet *entitySet = _new EntitySet(docTheory->getNSentences());
	EntitySet *corrEntitySet = docTheory->getEntitySet();

	for(int i=0; i< _n_observations; i++){
		if(_infoMap != NULL)
			_observations[i]->resetForNewDocument(entitySet, _infoMap);
		else
			_observations[i]->resetForNewDocument(entitySet);
	}

	// initialize coref ID hash
	int numBuckets = static_cast<int>(docTheory->getEntitySet()->getNEntities() / targetLoadingFactor);
	numBuckets = (numBuckets >= 2 ? numBuckets : 5);
	_alreadySeenDocumentEntityIDtoTrainingID = _new IntegerMap(numBuckets);


	_previousParses.clear();
	_entityLinks.clear();
	

	for (int i = 0; i < docTheory->getNSentences(); i++) {
		if (MODE == DEVTEST)
			_devTestStream<<L"Processing sentence #"<<i<<L"<br>\n";
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(i);
		MentionSet *mentionSet = sentTheory->getMentionSet(); 
		PropositionSet *propSet = sentTheory->getPropositionSet();
		propSet->fillDefinitionsArray();

		entitySet->loadMentionSet(mentionSet);

		processSentenceEntitiesFromStateFile(sentTheory, mentionSet, entitySet, corrEntitySet, propSet);

		if (MODE == TRAIN && _use_p1_averaging && (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING)) {
			// For the P1 and RANKING models, we add to the running sums every sentence;
			//  this is a nice balance between every instance (too slow)
			//  and every document (potentially too infrequent)
			for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights->begin();
				iter != _p1Weights->end(); ++iter)
			{
				(*iter).second.addToSum();
			}
		}

		_previousParses.push_back(sentTheory->getFullParse());

		// find propositional links between entities
		for (int j = 0; j < propSet->getNPropositions(); j++) {
			const Proposition* prop = propSet->getProposition(j);
			if (prop->getNArgs() < 2 || prop->getArg(0)->getRoleSym() != Argument::REF_ROLE)
				continue;
			MentionUID ref = prop->getArg(0)->getMention(mentionSet)->getUID();
			Entity *refent = entitySet->getEntityByMention(ref);
			if (refent == 0)
				continue;
			for (int k = 1; k < prop->getNArgs(); k++) {
				if (prop->getArg(k)->getType() != Argument::MENTION_ARG)
					continue;
				MentionUID arg = prop->getArg(k)->getMention(mentionSet)->getUID();
				Entity *argent = entitySet->getEntityByMention(arg);
				if (argent != 0) {
					_entityLinks.push_back(EntityIDPair(refent->getID(), argent->getID(), DTCorefObservation::_linkSym));
				}
			}
		}

	}
	// clean up data structures -- we can do this in P1, but not in MAX_ENT
	// -- at the moment we just leak them in MAX_ENT
	if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		delete entitySet;
	}

	delete _alreadySeenDocumentEntityIDtoTrainingID;
	if(_infoMap!=NULL)
		_infoMap->cleanUpAfterDocument();
}// processDocument()

void DTCorefTrainer::processSentenceEntitiesFromStateFile(SentenceTheory *sentTheory,
											  MentionSet *mentionSet,
										      EntitySet *entitySet,
											  EntitySet *corrEntitySet,
											  PropositionSet *propSet)
{
	// FOR RANKING
	for(int i=0; i< _n_observations; i++){
        _observations[i]->resetForNewSentence(mentionSet);
	}

	// set up pre-link mention arrays
	_copulaLinks.clear();
	_appositiveLinks.clear();
	_specialLinks.clear();

	// this will find the prelinks and store them in these arrays
	PreLinker::preLinkAppositives(_appositiveLinks, mentionSet);
	PreLinker::preLinkSpecialCases(_specialLinks, mentionSet, propSet);
	PreLinker::preLinkCopulas(_copulaLinks, mentionSet, propSet);

	// we copy the mentionSet before running metonymy over it
	const MentionSet origMentionSet(*mentionSet);
	if (_use_metonymy) {
		// This actually changes the MentionSet and its mentions
		// A mention entityType might be changed when it is a metonymic mention
		_metonymyAdder->addMetonymyTheory(mentionSet, propSet);
	}


	int nMentions = mentionSet->getNMentions();
	GrowableArray <MentionUID> mentions(nMentions);
	if(_train_from_left_to_right_on_sentence) {
		// add the Names fist as to enable forward coref within a sigle sentence
		if (_do_names_first)
			correctCorefAllNameMentions(entitySet, corrEntitySet, mentionSet);
		for (int i = 0; i < mentionSet->getNMentions(); i++) {
			Mention *mention = mentionSet->getMention(i);
			// already added names earlier
			if (_do_names_first && mention->mentionType == Mention::NAME)
				continue;
			mentions.add(mention->getUID());
		}
	}else { // train on sentence the same as Serif names first, then nominals and pronouns last
		//now split the mentions
		GrowableArray <MentionUID> names(nMentions);
		GrowableArray <MentionUID> descriptors(nMentions);
		GrowableArray <MentionUID> pronouns(nMentions);
		GrowableArray <MentionUID> others(nMentions);

		for (int j = 0; j < nMentions; j++) {
			Mention *thisMention = mentionSet->getMention(j);
			if (thisMention->mentionType == Mention::NAME)
				names.add(thisMention->getUID());
			else if (thisMention->mentionType == Mention::DESC &&
					 thisMention->getEntityType().isRecognized())
				descriptors.add(thisMention->getUID());
			else if (thisMention->mentionType == Mention::PRON &&
				WordConstants::isLinkingPronoun(
						thisMention->getNode()->getHeadWord()))
				pronouns.add(thisMention->getUID());
			else
				others.add(thisMention->getUID());
		}
		// add the name mentions first followed by nominal and pronoun last
		for (int i=0; i<names.length(); i++)
			mentions.add(names[i]);
		for (int i=0; i<others.length(); i++)
			mentions.add(others[i]);
		for (int i=0; i<descriptors.length(); i++)
			mentions.add(descriptors[i]);
		for (int i=0; i<pronouns.length(); i++)
			mentions.add(pronouns[i]);
	}


	for (int i = 0; i < mentions.length(); i++) {
		Mention *mention = mentionSet->getMention(mentions[i]);
		const Mention *origMention = origMentionSet.getMention(mentions[i]);
		const EntityType mentionOrigEntityType = origMention->getEntityType();
//		EntitySubtype mentionOrigEntityType = mention->getEntitySubtype();
		MentionUID id = mention->getUID();

		// add some mention information that is used during the feature extraction
		if(_infoMap!=NULL)
			_infoMap->addMentionInformation(mention, sentTheory->getTokenSequence());

//		mention->dump(std::cerr);		cerr<<endl;

		// we are devTesting accuracy also on unrecognized types
		// while for P1 and MaxEnt training only on recognized types
		if (!mention->isOfRecognizedType() && MODE == TRAIN && MODEL_TYPE != P1_RANKING)
			continue;
		

		if (mention->mentionType == Mention::NAME ||
			mention->mentionType == Mention::PRON ||
			mention->mentionType == Mention::DESC ||
			mention->mentionType == Mention::PART)
		{
			Entity *corefEnt = corrEntitySet->getEntityByMention(mention->getUID(), mentionOrigEntityType);

			// for train continue if no entity is found for the mention
			// the mention is included in training if using P1_RANKING and _use_no_link_examples is true
			if (corefEnt == 0 && MODE == TRAIN 
				&& (MODEL_TYPE != P1_RANKING || !_use_no_link_examples))
				continue;

			int coref_id;
			if (corefEnt == 0)
				//	-1	no entity
				coref_id =  -1; 
			else if (_alreadySeenDocumentEntityIDtoTrainingID->get(corefEnt->getID()) == 0)
				//	a first mention of an entity. it will be added to the entitySet
				coref_id =  entitySet->getNEntities();
			else //	a mention of an already seen entity. Holds the position of the entity in entitySet
				coref_id = (*_alreadySeenDocumentEntityIDtoTrainingID)[corefEnt->getID()];

			// exclude copula-linked and specially-linked instances from training
			if (mention->getMentionType() == TARGET_MENTION_TYPE &&
				_copulaLinks[i] == 0 &&
				_specialLinks[i] == 0 &&
				!NodeInfo::isNominalPremod(mention->getNode()) &&
				!(TARGET_MENTION_TYPE == Mention::PRON && !WordConstants::isLinkingPronoun(mention->getNode()->getHeadWord())))
			{
				_mentionLinks.clear();
				const Proposition* prop = propSet->getDefinition(i);
				if (prop != 0) {
					for (int k = 1; k < prop->getNArgs(); k++) {
						Argument *arg = prop->getArg(k);
						if (arg->getType() != Argument::MENTION_ARG)
							continue;
						const Mention *argMent = arg->getMention(mentionSet);
						// we need to have already seen this mention in our path, or
						// it needs to be a name
						if (argMent->getUID() > mention->getUID() &&
							argMent->getMentionType() != Mention::NAME)
						{
							continue;
						}
						Entity *argEnt = corrEntitySet->getEntityByMention(argMent->getUID(), argMent->getEntityType());
						if (argEnt == 0) {
							Mention *child = arg->getMention(mentionSet)->getChild();
							while (child != NULL) {
								if (child->getMentionType() == Mention::NONE)
									argEnt = corrEntitySet->getEntityByMention(child->getUID(), child->getEntityType());
								child = child->getChild();
							}
						}
						if (argEnt == 0)
							continue;
						_mentionLinks.push_back(EntityIDPair(argEnt->getID(), -1, DTCorefObservation::_linkSym));
					}//for
				}
				trainMention(mention, mentionOrigEntityType, coref_id, entitySet, sentTheory);
			}//if

			// updating the entitySet with the new mention
			if(coref_id==-1) {
				// not an ACE mention
				if (MODEL_TYPE == P1_RANKING && _use_non_ace_entities_as_no_links){
					// add a non-ACE mention as entity candidate - later mapped into a no-link decision
					entitySet->addNew(mention->getUID(), mention->getEntityType());
				}else { // just for devTest - add nothing
					continue;
				}
			}else {
				// an ACE mention, part of corefEnt
				if (_alreadySeenDocumentEntityIDtoTrainingID->get(corefEnt->getID()) != 0) {
					entitySet->add(mention->getUID(), coref_id);
				}else {
					// map the entity id into the position in entitySet
					(*_alreadySeenDocumentEntityIDtoTrainingID)[corefEnt->getID()] = coref_id; // = entitySet->getNEntities()
					// add a new ACE entity
//					entitySet->addNew(mention->getUID(), mention->getEntityType());
					// we add with the original EntityType so that not to screw the devtest
					entitySet->addNew(mention->getUID(), mentionOrigEntityType);
				}
			}


		}
	}
}

void DTCorefTrainer::devTestStateFiles() {

	loadTrainingDataFromStateFiles();

	_n_instances_seen = 0;
	_n_correct = 0;
	for (std::vector<DocTheory *>::reverse_iterator iter = _docTheories.rbegin(); iter != _docTheories.rend(); ++iter) {
		if (MODE == DEVTEST){
			//_devTestStream<<L"<br>Working on input file: "<<(*iter)->getDocument()->getName()<<L"<br>\n";
			if((*iter)->getNSentences()>0){
				_devTestStream<<L"from original input file:"<<(*iter)->getSentenceTheory(0)->getDocID().to_string()<<"<br>\n";
//				_devTestStream<<L"from original input file:"<<(*iter)->getDocument()->getName().to_string()<<"<br>\n";
			}
		}
		processDocument(*iter);
	}

	BOOST_FOREACH(DocTheory *docTheory, _docTheories) 
		delete docTheory;
	_docTheories.clear();
}

void DTCorefTrainer::decodeToP1Distribution(DTObservation *observation) {
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		_tagScores[i] = _p1Decoder->getScore(observation, i);
	}
}


void DTCorefTrainer::trainMention(const Mention *ment, EntityType mentionType, int entityID, 
								  EntitySet *entitySet, SentenceTheory *sentTheory)
{		
	HobbsDistance::SearchResult hobbsCandidates[MAX_CANDIDATES];
	int nHobbsCandidates, nCandidates;

	std::vector<LinkRecord> rankingRecords;

	bool is_ACE_ment = (entityID != -1);
	bool withinSameSentence = false;

	if (entityID != -1 && entityID < entitySet->getNEntities()) {
		Entity *antecedent_entity = entitySet->getEntity(entityID);
		MentionUID correct_antecedent_id = CorefUtils::getLatestMention(entitySet, antecedent_entity, ment);	
		if (correct_antecedent_id.isValid()) {
			Mention *antecedentMention = entitySet->getMention(correct_antecedent_id);
			if (ment->getSentenceNumber() == antecedentMention->getSentenceNumber())
				withinSameSentence = true;
			if (_considerOutsideSentenceLinksOnly && withinSameSentence)
				return;
		}
	}

	if (DEBUG) {
		_debugStream << "***************************\n";
		_debugStream << "Training on mention from entity" << entityID << " (";
		_debugStream << ment->getEntityType().getName().to_debug_string() << ".";
		_debugStream << ment->getEntitySubtype().getName().to_debug_string() << ") ";
		_debugStream << ment->getNode()->toTextString() << "\n\n";
		_debugStream.flush();
	}

	// this is a bit of a hack to get both of these, but because there is no
	// assignment operation on GrowableArrays, we'll just do it this way
	const GrowableArray <Entity *> &entitiesByType = entitySet->getEntitiesByType(mentionType);
	const GrowableArray <Entity *> &allEntities = entitySet->getEntities();

	if (TARGET_MENTION_TYPE == Mention::PRON) {
		nHobbsCandidates = HobbsDistance::getCandidates(ment->getNode(), _previousParses,
			hobbsCandidates, MAX_CANDIDATES);
		nCandidates = allEntities.length();
	} else {
		nHobbsCandidates = 0;
		if (_filter_by_entity_type || _filter_by_entity_subtype) {
			nCandidates = entitiesByType.length();
		} else {
			nCandidates = allEntities.length();
		}
	}

	// break if there are no candidates on train
	// on devTest we will count this example because we don't want to change the test results
	// depending on the byType or the bySubtype options.
	if (nCandidates == 0){
//		if ((_filter_by_entity_type || _filter_by_entity_subtype) && is_nonACE_ment) {
//			_correct_nonACE_nolink++;
//			return;
//		}
		if (MODE == TRAIN)
			return;
	}

	int apposID = getPreLinkID(entitySet, ment, _appositiveLinks[ment->getIndex()]);
	int copulaID = getPreLinkID(entitySet, ment, _copulaLinks[ment->getIndex()]);

	// this means we haven't yet seen the mention the candidate is prelinked to--
	// so probably this is then a bad training instance!
	if (MODE == TRAIN &&
		((_appositiveLinks[ment->getIndex()] != 0 && apposID == -1) ||
		 (_copulaLinks[ment->getIndex()] != 0 && copulaID == -1)))
		return;


	// Sort entities by the last mention id
	GrowableArray <Entity *> sortedEnts;
	sortEntities(allEntities, entitiesByType, ment, entityID, entitySet, nCandidates, sortedEnts, _max_non_ace_candidates);

	Entity *correct_link = NULL;
	Entity *hypothesized_link = NULL;
	Entity *sec_best_link = NULL;
	
	int hypothesized_hobbs_distance = -1;
	int correct_hobbs_distance = -1;

	double correct_link_score = 0;
	double hypothesized_score = MIN_VAL;
	double sec_best_score = MIN_VAL;

	int correct_link_index = 0;
	int hypothesized_index = 0;
	int sec_best_index = 0;
	
	int correct_tag_index = _tagSet->getNoneTagIndex();
	int hypothesized_tag_index = _tagSet->getNoneTagIndex();
	int sec_best_tag_index = _tagSet->getNoneTagIndex();
	int link_tag_index = _tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol());
	int no_link_tag_index = _tagSet->getNoneTagIndex();
	
	double best_conf = -1;
	double sec_best_conf = -1;
	double correct_conf = -1;
	
	if ((MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) && MODE == DEVTEST && _p1Decoder->DEBUG) {
		_p1Decoder->_debugStream << L"\nCONSIDERING: " << ment->getNode()->toTextString() << "\n\n";
	}

	// FOR RANKING
	// Deal with the NO-LINK option
	if (MODEL_TYPE == P1_RANKING) {
		_observations[NO_LINK_OBS_INDEX]->populate(ment->getUID());
 		double score = _p1Decoder->getScore(_observations[NO_LINK_OBS_INDEX], no_link_tag_index);
		LinkRecord record(NO_LINK_OBS_INDEX, no_link_tag_index, 0, score);
		correct_link_score = score;
		rankingRecords.push_back(record);
		
		if (DEBUG) {
			_debugStream << "CANDIDATE: NO LINK\n";
			_p1Decoder->printDebugInfo(_observations[NO_LINK_OBS_INDEX], no_link_tag_index, _debugStream);
			_debugStream << "\n";
		}
	}

	// Score the linking options
	for (int e=sortedEnts.length(); e>0 ; e--) {
		// limit the number of possible false link candidates
		if (SEARCH_STRATEGY == CLOSEST && MODE == TRAIN && correct_link != 0)
			break;

		Entity* prevEntity = sortedEnts[e-1];

		int hobbs_distance = getHobbsDistance(entitySet, prevEntity,
			hobbsCandidates, nHobbsCandidates);
		// deal with preLinks
		Symbol preLinkSym = DTCorefObservation::_noLinkSym;
		if (apposID == prevEntity->getID()) {
			preLinkSym = DTCorefObservation::_apposSym;
		} else if (copulaID == prevEntity->getID()) {
			preLinkSym = DTCorefObservation::_copulaSym;
		}

		Mention::Type entityMentionLevel = CorefUtils::getEntityMentionLevel(entitySet,prevEntity);
		Symbol relInCommon = DTCorefObservation::_noLinkSym;
		for (size_t i = 0; i < _mentionLinks.size(); i++) {
			EntityIDPair pair1 = _mentionLinks[i];
			for (size_t j = 0; j < _entityLinks.size(); j++) {
				EntityIDPair pair2 = _entityLinks[j];
				if ((pair2.entity1 == pair1.entity1 &&
					 pair2.entity2 == prevEntity->getID()) ||
					(pair2.entity2 == pair1.entity1 &&
					 pair2.entity1 == prevEntity->getID()))
				{
					if (MODE == DEVTEST) {
						_devTestStream << L"ENTITY LINK IN COMMON: " << ment->getNode()->toTextString() << L"<br>";
						_devTestStream << L"ENTITY LINK IN COMMON: ";
						printEntity(_devTestStream, entitySet, prevEntity);
						_devTestStream << "<br>";
						_devTestStream << L"COMMON ENTITY = ";
						printEntity(_devTestStream, entitySet, entitySet->getEntity(pair1.entity1));
						_devTestStream << "<br>";
					}
					relInCommon = DTCorefObservation::_linkSym;
					break;
				}
			}
			if (relInCommon == DTCorefObservation::_linkSym)
				break;
		}

		DTCorefObservation *observation =  
			static_cast<DTCorefObservation*>((MODEL_TYPE == P1_RANKING) ? _observations[e] : _observations[LINK_OBS_INDEX]) ;
		observation->populate(ment->getUID(), prevEntity->getID(), hobbs_distance, preLinkSym, relInCommon);

		int answer = _tagSet->getNoneTagIndex();
		// Here is where the correct entity get marked.
		if ((TRAIN_SOURCE == STATE_FILES && prevEntity->getID() == entityID)) //||
			//(TRAIN_SOURCE == AUG_PARSES && _alreadySeenDocumentEntityIDtoTrainingID->get(entityID) != NULL && prevEntity->getID() == *(_alreadySeenDocumentEntityIDtoTrainingID->get(entityID)))) 
		{
			// when we we train for name linking and constrain to limit only to name
			// when training we want to map non-NAME-level entities into a decision not to link
			// but instead start a new entity.
			if (MODE == TRAIN && TARGET_MENTION_TYPE==Mention::NAME && _limit_to_names && entityMentionLevel!=Mention::NAME) {
				// do nothing...
			}else {
				answer = link_tag_index;
				correct_link = prevEntity;
				correct_hobbs_distance = hobbs_distance;
				// FOR RANKING
				correct_link_index = e;
				correct_tag_index = link_tag_index;
			}
		}

		if (MODE == TRAIN) {
			if (DEBUG) {
				_debugStream << "-------\n";
				_debugStream << "CANDIDATE: entity" << prevEntity->getID() << " [";
				for (int i = 0; i < prevEntity->getNMentions(); i++) {
					int sentno = Mention::getSentenceNumberFromUID(prevEntity->getMention(i));
					int id = Mention::getIndexFromUID(prevEntity->getMention(i));
					Mention *m = entitySet->getMentionSet(sentno)->getMention(id);
					_debugStream << m->getNode()->toTextString();
					if (i != prevEntity->getNMentions() - 1)
						_debugStream << L"; ";
				}
				_debugStream << L"] (" << prevEntity->getType().getName().to_debug_string() << ")\n";
			}

			if (MODEL_TYPE == P1) {
				_n_instances_seen++;
				bool correct = _p1Decoder->train(observation, answer);
				if (correct) {
					_n_correct++;
					_total_n_correct++;
				}
				if (_n_instances_seen % 1000 == 0) {
					SessionLogger::info("SERIF") << _n_instances_seen << ": " << _n_correct
					          << " (" << 100.0*_n_correct/1000 << "%)" << "\n";
					_n_correct = 0;
				}
				if (DEBUG) {
					_debugStream << "\n";
					_p1Decoder->printDebugInfo(observation, link_tag_index, _debugStream);
					_debugStream << "\n";
					_p1Decoder->printDebugInfo(observation, no_link_tag_index, _debugStream);
					_debugStream << "\n";
					if (!correct) {
						if (answer == link_tag_index) {
							_debugStream << "Updated " << _tagSet->getTagSymbol(link_tag_index).to_debug_string() << " by 1.\n";
							_debugStream << "Updated " << _tagSet->getTagSymbol(no_link_tag_index).to_debug_string() << " by -1.\n";
						}
						else {
							_debugStream << "Updated " << _tagSet->getTagSymbol(no_link_tag_index).to_debug_string() << " by 1.\n";
							_debugStream << "Updated " << _tagSet->getTagSymbol(link_tag_index).to_debug_string() << " by -1.\n";
						}
					}
				}
			} else if (MODEL_TYPE == MAX_ENT) {
				_maxEntDecoder->addToTraining(observation, answer);
				if (DEBUG)
					_maxEntDecoder->printDebugInfo(observation, answer, _debugStream);
			} else if (MODEL_TYPE == P1_RANKING) {
				double score = _p1Decoder->getScore(observation, link_tag_index);
				LinkRecord record(e, link_tag_index, prevEntity, score);
				rankingRecords.push_back(record);
				if (correct_link_index == e) {
					correct_link_score = score;
				}
				if (DEBUG)
					_p1Decoder->printDebugInfo(observation, link_tag_index, _debugStream);
			}
			if (DEBUG)
				_debugStream << "\n";

		} else if (MODE == DEVTEST) {
			//_devTestStream << "ENTITY: ";
			//printEntity(_devTestStream, entitySet, prevEntity);
			//_devTestStream << "<br>";

			if ((MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) && _p1Decoder->DEBUG) {
				_p1Decoder->_debugStream << L"POSSIBLE LINK: [";
				for (int i = 0; i < prevEntity->getNMentions(); i++) {
					int sentno = Mention::getSentenceNumberFromUID(prevEntity->getMention(i));
					int id = Mention::getIndexFromUID(prevEntity->getMention(i));
					Mention *m = entitySet->getMentionSet(sentno)->getMention(id);
					_p1Decoder->_debugStream << m->getNode()->toTextString();
					if (i != prevEntity->getNMentions() - 1)
						_p1Decoder->_debugStream << L"; ";
				}
				_p1Decoder->_debugStream << L"]\n";
			}

			if (MODEL_TYPE == P1_RANKING) { // RANKING
				double score;
				// for name linking with _limit_to_names although the correct entity is present 
				// we force the model not to choose it by setting its score to lowest score.
				if (TARGET_MENTION_TYPE==Mention::NAME && _limit_to_names && entityMentionLevel != Mention::NAME
					&& ((TRAIN_SOURCE == STATE_FILES && prevEntity->getID() == entityID)))// ||
					    //(TRAIN_SOURCE == AUG_PARSES && _alreadySeenDocumentEntityIDtoTrainingID->get(entityID) != NULL && prevEntity->getID() == *(_alreadySeenDocumentEntityIDtoTrainingID->get(entityID))))) 
				{
					score = -std::numeric_limits<double>::max();
				} else { // otherwise we score it
					score = _p1Decoder->getScore(observation, link_tag_index);
				}
				LinkRecord record(e, link_tag_index, prevEntity, score);
				rankingRecords.push_back(record);
				if (correct_link_index == e) {
					correct_link_score = score;
				}
			}else { // P1 & MAXENT
				if (hypothesized_link == 0 || SEARCH_STRATEGY == BEST) {
					int hypothesis;
					double this_score = 0;
					if (MODEL_TYPE == P1) {
						hypothesis = _p1Decoder->decodeToInt(observation);
						this_score = _p1Decoder->getScore(observation, hypothesis);
					}
					else if (MODEL_TYPE == MAX_ENT) {
						_maxEntDecoder->decodeToDistribution(observation, _tagScores, 
															 _tagSet->getNTags(), &hypothesis);
						this_score = _tagScores[hypothesis];
						if (hypothesis == _tagSet->getNoneTagIndex() &&
							_tagScores[link_tag_index] > _link_threshold)
						{
							hypothesis = link_tag_index;
							this_score = _tagScores[link_tag_index];
						}
					} else if (MODEL_TYPE == BOTH) {
						int p1_hypothesis = _p1Decoder->decodeToInt(observation);
						int maxent_hypothesis = _maxEntDecoder->decodeToInt(observation);
						if (p1_hypothesis == _tagSet->getNoneTagIndex() &&
							maxent_hypothesis != _tagSet->getNoneTagIndex())
						{
							// and
							hypothesis = maxent_hypothesis; // None
							// or
							//hypothesis = _tagSet->getNoneTagIndex();
						} else if (p1_hypothesis != _tagSet->getNoneTagIndex() &&
							maxent_hypothesis == _tagSet->getNoneTagIndex())
						{
							// and
							hypothesis = p1_hypothesis; // Link
							// or
							//hypothesis = _tagSet->getNoneTagIndex();
						} else {
							// same answer
							hypothesis = p1_hypothesis;
						}
					}

					if (hypothesis != _tagSet->getNoneTagIndex()) {
//						if (SEARCH_STRATEGY == CLOSEST || this_score > hypothesized_score)
						if (this_score > hypothesized_score)
						{
							if (hypothesized_score != MIN_VAL) {
								_devTestStream << "ENTITY OVERRIDE: ";
								_devTestStream << hypothesized_score << " -- [";
								printEntity(_devTestStream, entitySet, hypothesized_link);
								_devTestStream << "] LOSES TO ";
								_devTestStream << this_score << " -- [";
								printEntity(_devTestStream, entitySet, prevEntity);
								_devTestStream << "]<br>\n";
							}
							hypothesized_index = e;
							hypothesized_link = prevEntity;
							hypothesized_hobbs_distance = hobbs_distance;
							hypothesized_score = this_score;
							hypothesized_tag_index = link_tag_index;
						}
					}
				}
			}//! RANKING
		}// MODE
	}// for

	if (MODEL_TYPE == P1_RANKING) {
		       
		std::sort(rankingRecords.begin(), rankingRecords.end());

		hypothesized_link = rankingRecords.front().entity;
		hypothesized_index = rankingRecords.front().link_index;
		hypothesized_tag_index = rankingRecords.front().tag_index;
		hypothesized_score = rankingRecords.front().score;

		if (rankingRecords.size() > 1) {
			sec_best_link = rankingRecords.at(1).entity;
			sec_best_index = rankingRecords.at(1).link_index;
			sec_best_tag_index = rankingRecords.at(1).tag_index;
			sec_best_score = rankingRecords.at(1).score;
		}
		
		computeConfidence(rankingRecords, correct_link_score, best_conf, sec_best_conf, correct_conf);

		if (DEBUG) {
			// print top three candidates
			_debugStream << "WINNING CANDIDATES:\n";			
			for (int i = 0; i < 3; i++) {
				_debugStream << i+1 << ". ";
				if (i >= (int)rankingRecords.size()) {
					_debugStream << "NO LINK" << " (" << -std::numeric_limits<double>::max() << ")\n";
				}
				else {
					LinkRecord record = rankingRecords.at(i);
					if (record.entity == 0)
						_debugStream << "NO LINK";
					else
						_debugStream << "entity" << record.entity->getID();
					_debugStream << " (" << record.score << ")\n";
				}
			}
			_debugStream << "\n";
		}

		_n_instances_seen++;
		bool correct = false;
		if (hypothesized_index == correct_link_index || // true link or true no-link
			// deals with the case of non-ACE entities and with new entities
			(correct_link_index == 0  && hypothesized_index!=0 && !hypothesized_link->getType().isRecognized())) {
			correct= true;
			_n_correct++;
			_total_n_correct++;
		}

		// SHOW PROGRESS
		if (_n_instances_seen % 300 == 0) {
			SessionLogger::info("SERIF") << _n_instances_seen << ": " << _n_correct
						<< " (" << 100.0*_n_correct/300 << "%)" << "\n";
			_n_correct = 0;
		}

		// UPDATING THE RANKING WEIGHTS
		if (MODE == TRAIN) {
			bool correct_by_margin = (correct && (rankingRecords.size() < 2 || rankingRecords.at(0).score > rankingRecords.at(1).score +_p1_required_margin)) ? true : false;
			if (!correct_by_margin) {
					
				LinkRecord recordToDecrement = rankingRecords.at(0);
				
				// If we made the correct prediction, then the second prediction must be within the margin,
				// so decrement it instead
				if (correct) {
					recordToDecrement = rankingRecords.at(1);
				}

				// takes care of non-ACE links: adjust the hyp observation (link to ACE entity) by -1
				// and adjust each of the non-ACE observations by +(1/number of non-ACE observations), since we
				// don't know which is the "true" observation.      
				if (!is_ACE_ment && _use_non_ace_entities_as_no_links) {
					_p1Decoder->adjustWeights(_observations[recordToDecrement.link_index], recordToDecrement.tag_index, -1.0, true);
					// first, count the total number of non-ACE links
					int n_other_cand = 0;
					for (int j = sortedEnts.length(); j > 0; j--) {
						Entity *ent = sortedEnts[j-1]; // -1 because of the no_link option
						if (!ent->getType().isRecognized())
							n_other_cand++;
					}
					// now, increment each non-ACE link by its share of the positive weight
					for (int i = sortedEnts.length(); i > 0; i--) {
						Entity *ent = sortedEnts[i-1]; // -1 because of the no_link option
						if (!ent->getType().isRecognized())
							_p1Decoder->adjustWeights(_observations[i], link_tag_index, +(1.0/n_other_cand), true);
						if (SEARCH_STRATEGY == CLOSEST && ent->getID() == entityID)
							break;
					}
				} else {
					// takes care of ACE linked mentions, mentions of new ACE entities
					// and non-ACE mentions when we are not using non-ACE entities.
					// In the case of non-ACE mentions, the no-link observation is updated by +1
					_p1Decoder->adjustErrorWeights(_observations[recordToDecrement.link_index], recordToDecrement.tag_index,
									               _observations[correct_link_index], correct_tag_index, 1, true);
					if (DEBUG) {
						if (correct_link_index != 0) {
							Entity *correctEntity = sortedEnts[correct_link_index-1];
							_debugStream << "Updating weights for entity" << correctEntity->getID();
							_debugStream << " by 1.\n";
						}
						else {
							_debugStream << "Updating weights for NO_LINK by 1.\n";
						}
						if (recordToDecrement.link_index != 0) {
							Entity *hypothesizedEntity = recordToDecrement.entity;
							_debugStream << "Updating weights for entity" << hypothesizedEntity->getID();
							_debugStream << " by -1.\n";
						}
						else {
							_debugStream << "Updating weights for NO_LINK by -1.\n";
						}
						_debugStream << "\n";
					}

				}
			}
		}// TRAIN

	}// RANKING


	if (MODE == DEVTEST) {
		// even if we include appositives or copulas in training/devtest,
		// we don't want to score them! that makes scores artifically high.
		if (_appositiveLinks[ment->getIndex()] != 0 || _copulaLinks[ment->getIndex()] != 0) 
			return;

		Symbol correct_decision = (correct_link_index == 0) ? DTCorefObservation::_noLinkSym : DTCorefObservation::_linkSym;

		// MAP THE SELECTION INTO LINK/NO-LINK DECISION
		Symbol hypothesized_decision = (hypothesized_index == 0 || !hypothesized_link->getType().isRecognized())
			? DTCorefObservation::_noLinkSym : DTCorefObservation::_linkSym;
		Symbol sec_best_decision = (sec_best_index == 0 || !sec_best_link->getType().isRecognized())
			? DTCorefObservation::_noLinkSym : DTCorefObservation::_linkSym;

		_devTestStream  << "(" << (is_ACE_ment ? "ACE" : "NON-ACE") << ") ";
		_devTestStream << L"<b> "<< ment->getNode()->toTextString() << L" --> ";
		if(_infoMap!=NULL) {
			printHeadWords(_infoMap->getHWMentionMapper()->get(ment->getUID()), _devTestStream);
		}else {
			_devTestStream << ment->getNode()->toTextString();
		}
		_devTestStream << L"</b>";
		_devTestStream << L"<br>\n";
		if(TRAIN_SOURCE == STATE_FILES) _devTestStream << getCurrentDebugSentence(sentTheory, ment) << "<br>\n";
		_devTestStream << L"Number of candidate entities: " << sortedEnts.length() << L"<br>\n";
		
		// SHOULD BE NO LINK
		if (correct_link_index == 0) {
			
			// GUESSED NO LINK OR GUESSED LINK TO NON-ACE ENTITY  => CORRECT NO LINK
			if ((hypothesized_index == 0) || 
				(hypothesized_index != 0 && !hypothesized_link->getType().isRecognized()))
			{
				_correct_nolink++;
				if (is_ACE_ment)
					_correct_ACE_nolink++;
				else
					_correct_nonACE_nolink++;

				if (MODEL_TYPE == P1_RANKING) {
					_devTestStream << L"<font color=orange>CORRECT NO_LINK";
					if (entityID != -1) {
						if (entityID == entitySet->getNEntities()) {
							_correct_nolink_new_entity++;
							_devTestStream << L" NEW ENTITY";
						}
						else {
							_correct_nolink_within_sentence++;
							_devTestStream << L" WITHIN SENTENCE";
						}
					}
					else {
						_correct_nolink_within_document++;
						_devTestStream << L" WITHIN DOCUMENT";
					}
					_devTestStream << L"</font> <br>\n";
					if(sortedEnts.length()==0) _devTestStream << L"DUE TO 0 CANDIDATES <br>\n";
					printDebugScores(_observations[hypothesized_index], hypothesized_tag_index, hypothesized_score, best_conf, hypothesized_decision, hypothesized_link,
								     _observations[sec_best_index], sec_best_tag_index, sec_best_score, sec_best_conf, sec_best_decision, sec_best_link, _devTestStream, entitySet); 
				}
			}
			
			// GUESSED LINK TO ACE ENTITY  => SPURIOUS
			else if (hypothesized_link != 0 && hypothesized_link->getType().isRecognized()) {
				_spurious++;
				
				_devTestStream << L"<font color=blue>SPURIOUS";
				if (MODEL_TYPE == P1_RANKING) {
					if (entityID != -1) {
						if (entityID == entitySet->getNEntities()) {
							_spurious_nolink_new_entity++;
							_devTestStream << L" NEW ENTITY";
						}
						else {
							_spurious_nolink_within_sentence++;
							_devTestStream << L" WITHIN SENTENCE";
						}
					}
					else {
						_spurious_nolink_within_document++;
						_devTestStream << L" WITHIN DOCUMENT";
					}
				}
				
				if (is_ACE_ment) {
					_ace_spurious++;
				} else {
					_nonACE_spurious++;
				}
				_devTestStream << L"</font>: <cite>link to</cite> ";
				if (MODEL_TYPE == P1_RANKING) {
					printDebugScores(_observations[hypothesized_index], hypothesized_tag_index, hypothesized_score, best_conf, hypothesized_decision, hypothesized_link,
						             _observations[correct_link_index], correct_tag_index, correct_link_score, correct_conf, correct_decision, correct_link, _devTestStream, entitySet);
				}else {
					_devTestStream << L"[";
					printEntity(_devTestStream, entitySet, hypothesized_link);
					_devTestStream << L"]<br>";
					_devTestStream << "<br>\n";
					printDebugScores(ment->getUID(), hypothesized_link->getID(), hypothesized_hobbs_distance, _devTestStream);			
				}
			}
		}
		// SHOULD BE LINK (correct_link != 0)
		else {
			
			// GUESSED NO LINK OR GUESSED LINK TO NON-ACE ENTITY  => MISSING
			if ((hypothesized_index == 0) || 
				(hypothesized_index != 0 && !hypothesized_link->getType().isRecognized()))
			{
				_missed++;
				if (withinSameSentence)
					 _missed_within_sentence++;
				else
					 _missed_outside_sentence++;
				_devTestStream << L"<font color=purple>MISSING</font>: <cite>link to</cite> ";
				if (MODEL_TYPE == P1_RANKING) {
					printDebugScores(_observations[correct_link_index], correct_tag_index, correct_link_score, correct_conf, correct_decision, correct_link,
									 _observations[hypothesized_index], hypothesized_tag_index, hypothesized_score, best_conf, hypothesized_decision, hypothesized_link, _devTestStream, entitySet); 
				 }else {
					_devTestStream << L"[";
					printEntity(_devTestStream, entitySet, correct_link);
					_devTestStream << L"]<br>";
					_devTestStream << "<br>\n";
					printDebugScores(ment->getUID(), correct_link->getID(), correct_hobbs_distance, _devTestStream);
				}
			}

			// GUESSED LINK TO WRONG ENTITY => WRONG LINK
			else if (hypothesized_link != correct_link) {
				_wrong_link++;
				if (withinSameSentence)
					_wrong_link_within_sentence++;
				else
					_wrong_link_outside_sentence++;
				_devTestStream << L"<font color=green>WRONG</font>: <cite>link to</cite>";
				if (MODEL_TYPE == P1_RANKING) {
					printDebugScores(_observations[hypothesized_index], hypothesized_tag_index, hypothesized_score, best_conf, hypothesized_decision, hypothesized_link,
						             _observations[correct_link_index], correct_tag_index, correct_link_score, correct_conf, correct_decision, correct_link, _devTestStream, entitySet); 
				} else {
					_devTestStream << L" [";
					printEntity(_devTestStream, entitySet, hypothesized_link);
					_devTestStream << L"]<br>";
					printDebugScores(ment->getUID(), hypothesized_link->getID(), hypothesized_hobbs_distance, _devTestStream);		
					_devTestStream << "<br>\n";
					_devTestStream << L"\n<font color=green>SHOULD BE</font>: <cite>link to</cite> [";
					printEntity(_devTestStream, entitySet, correct_link);
					_devTestStream << L"]<br>";
					_devTestStream << "<br>\n";
					printDebugScores(ment->getUID(), correct_link->getID(), correct_hobbs_distance, _devTestStream);
				}
			}

			// GUESSED CORRECT  => CORRECT
			else {
				_correct_link++;
				if (withinSameSentence)
					_correct_within_sentence++;
				else
					_correct_outside_sentence++;
				_devTestStream << L"<font color=red>CORRECT</font>: <cite>link to</cite>";
				if (MODEL_TYPE == P1_RANKING) {
					printDebugScores(_observations[hypothesized_index], hypothesized_tag_index, hypothesized_score, best_conf, hypothesized_decision, hypothesized_link,
						             _observations[sec_best_index], sec_best_tag_index, sec_best_score, sec_best_conf, sec_best_decision, sec_best_link, _devTestStream, entitySet); 
				} else {
					_devTestStream << L" [";
					printEntity(_devTestStream, entitySet, correct_link);
					_devTestStream << L"]<br>";
					_devTestStream << "<br>\n";
					printDebugScores(ment->getUID(), hypothesized_link->getID(), hypothesized_hobbs_distance, _devTestStream);
				}
			}
		}
		_devTestStream << "<br>\n";
	}

	if (MODE == TRAIN && DEBUG) {
		_debugStream << "***************************\n";
	}
}




/** This function now automatically adds suffixes to models based on the model type (e.g. maxent).
  * This is so that one could run a combination model where multiple types of models are loaded
  * simultaneously without having separate parameters for each.
  */
void DTCorefTrainer::writeWeights(int epoch) {
	if (epoch != -1 && _print_model_every_epoch == false)
		return;

	UTF8OutputStream out;
	std::string modeltype;
	char file[600];
	if (MODEL_TYPE == MAX_ENT) {
		if (epoch == -1)
			sprintf(file, "%s-maxent", _model_file.c_str());
		else
			sprintf(file, "%s-epoch-%d-maxent", _model_file.c_str(), epoch);
	} else if (MODEL_TYPE == P1) {
		if (epoch == -1)
			sprintf(file, "%s-p1", _model_file.c_str());
		else
			sprintf(file, "%s-epoch-%d-p1", _model_file.c_str(), epoch);
	} else if (MODEL_TYPE == P1_RANKING) {
		if (epoch == -1)
			sprintf(file, "%s-rank", _model_file.c_str());
		else
			sprintf(file, "%s-epoch-%d-rank", _model_file.c_str(), epoch);
	}

	out.open(file);

	if (out.fail()) {
		throw UnexpectedInputException("DTCorefTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	if (MODEL_TYPE == MAX_ENT)
		DTFeature::writeWeights(*_maxEntWeights, out);
	else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING)
		DTFeature::writeSumWeights(*_p1Weights, out);
	out.close();

}

void DTCorefTrainer::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	
	if (_list_mode)
		DTFeature::recordFileListForReference(Symbol(L"dt_coref_training_file"), out);
	else DTFeature::recordParamForReference(Symbol(L"dt_coref_training_file"), out);
	DTFeature::recordParamForReference(Symbol(L"dt_coref_training_list_mode"), out);

	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"lc_word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"dt_coref_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"dt_coref_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"do_coref_entity_type_filtering"), out);
	DTFeature::recordParamForConsistency(Symbol(L"do_coref_entity_subtype_filtering"), out);
	DTFeature::recordParamForConsistency(Symbol(L"dt_coref_model_type"), out);

	DTFeature::recordParamForReference(Symbol(L"dt_coref_target_mention_type"), out);
	DTFeature::recordParamForReference(Symbol(L"dt_coref_search_strategy"), out);
	DTFeature::recordParamForReference(Symbol(L"dt_coref_train_source"), out);
	DTFeature::recordParamForReference(Symbol(L"dt_coref_train_from_left_to_right_on_sentence"), out);
	DTFeature::recordParamForReference(Symbol(L"do_coref_link_names_first"), out);
	DTFeature::recordParamForReference(Symbol(L"dt_coref_link_only_to_names"), out);
	DTFeature::recordParamForReference(Symbol(L"dt_coref_use_metonymy"), out);

	DTFeature::recordParamForReference(Symbol(L"entity_type_set"), out);
	DTFeature::recordParamForReference(Symbol(L"value_type_set"), out);
	DTFeature::recordParamForReference(Symbol(L"entity_subtype_set"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_subtypes"), out);
	DTFeature::recordParamForReference(Symbol(L"desc_head_subtypes"), out);
	DTFeature::recordParamForReference(Symbol(L"name_word_subtypes"), out);
	DTFeature::recordParamForReference(Symbol(L"full_name_subtypes"), out);
	DTFeature::recordParamForReference(Symbol(L"guesser_female_names"), out);
	DTFeature::recordParamForReference(Symbol(L"guesser_female_descriptors"), out);
	DTFeature::recordParamForReference(Symbol(L"guesser_male_names"), out);
	DTFeature::recordParamForReference(Symbol(L"guesser_male_descriptors"), out);
	DTFeature::recordParamForReference(Symbol(L"use_nominal_premods"), out);
	DTFeature::recordParamForReference(Symbol(L"use_whq_mentions"), out);
	DTFeature::recordParamForReference(Symbol(L"partitive_headword_list"), out);
	DTFeature::recordParamForReference(Symbol(L"desc_types"), out);
	DTFeature::recordParamForReference(Symbol(L"word_net_dictionary_path"), out);
	DTFeature::recordParamForReference(Symbol(L"hobbs_do_step_8"), out);
	DTFeature::recordParamForReference(Symbol(L"temporal_headword_list"), out);
	DTFeature::recordParamForReference(Symbol(L"unify_appositives"), out);
	DTFeature::recordParamForReference(Symbol(L"make_partitive_props"), out);
	DTFeature::recordParamForReference(Symbol(L"do_coref_link_names_first"), out);

	if (MODEL_TYPE == MAX_ENT) {
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_percent_held_out"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_max_iterations"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_gaussian_variance"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_min_likelihood_delta"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_stop_check_frequency"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_pruning_cutoff"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_mode"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_train_vector_file"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_test_vector_file"), out);
	} else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		DTFeature::recordParamForReference(Symbol(L"dt_coref_trainer_epochs"), out);
		DTFeature::recordParamForReference(Symbol(L"use_no_links_examples"), out);
		DTFeature::recordParamForReference(Symbol(L"dt_coref_use_p1_averaging"), out);
		DTFeature::recordParamForReference(Symbol(L"dt_coref_use_non_ACE_ents_as_no_links"), out);
		DTFeature::recordParamForReference(Symbol(L"dt_coref_use_non_ACE_ents_as_no_links_max_candidates"), out);
		DTFeature::recordParamForReference(Symbol(L"dt_coref_p1_required_margin"), out);
		DTFeature::recordParamForReference(Symbol(L"dt_pron_model_outside_and_within_sentence_links_separately"), out);
		DTFeature::recordParamForReference(Symbol(L"consider_within_sentence_links_only"), out);
		DTFeature::recordParamForReference(Symbol(L"consider_outside_sentence_links_only"), out);
	}
	if (TARGET_MENTION_TYPE == Mention::NAME) {
		DTFeature::recordParamForReference(Symbol(L"abbrev_maker_file"), out);
		DTFeature::recordParamForConsistency(Symbol(L"edt_capital_to_country_file"), out);
		DTFeature::recordParamForConsistency(Symbol(L"dtnamelink_per_wk_file"), out);
		DTFeature::recordParamForConsistency(Symbol(L"dtnamelink_org_wk_file"), out);
	}

	out << L"\n";

}

int DTCorefTrainer::getHobbsDistance(EntitySet *entitySet, Entity *entity,
					 HobbsDistance::SearchResult *hobbsCandidates, int nHobbsCandidates)
{
	for (int i = 0; i < nHobbsCandidates; i++) {
		const MentionSet *mset = entitySet->getMentionSet(hobbsCandidates[i].sentence_number);
		const Mention *ment = mset->getMentionByNode(hobbsCandidates[i].node);
		if (ment != 0 && entitySet->getEntityByMention(ment->getUID()) == entity)
			return i;
	}
	return -1;
}

int DTCorefTrainer::getPreLinkID(EntitySet *entitySet, const Mention *ment, const Mention *preLink) {
	if (preLink == 0)
		return -1;

	Entity *ent = entitySet->getEntityByMention(preLink->getUID(), preLink->getEntityType());
	if (ent != 0) {
		//if (MODE == DEVTEST) {
		//	_devTestStream << "CANDIDATE: <b>" << ment->getNode()->toTextString() << "</b><br>";
		//	_devTestStream << "PRELINKED TO: " << ent->getID() << " -- " << preLink->getNode()->toTextString() << "<p>";
		//}
		return ent->getID();
	} else {
		//if (MODE == DEVTEST) {
		//	_devTestStream << "CANDIDATE: <b>" << ment->getNode()->toTextString() << "</b><br>";
		//	_devTestStream << "NO ID FOUND FOR PRELINK: " << preLink->getNode()->toTextString() << "<p>";
		//}
		return -1;
	}
}

// Sort entities by the last mention id
// add several non-ACE entities if needed
// For name linking it enables filtering non name level entities.
void DTCorefTrainer::sortEntities(const GrowableArray <Entity *> &allEntities,
								  const GrowableArray <Entity *> &entitiesByType,
								  const Mention *ment,
								  int entityID,
								  const EntitySet *entitySet,
								  int nCandidates,
								  GrowableArray <Entity *>& sortedEnts,
								  int max_non_ace_candidates)
{
	// brute force method of sorting
	for (int i = 0; i < nCandidates; i++) {

		// again, this is a bit hackish due to the nature of GrowableArrays
		// note that we set nCandidates earlier in this function!
		Entity *ent;
		if (TARGET_MENTION_TYPE == Mention::PRON)
			ent = allEntities[i];
		else if (_filter_by_entity_type || _filter_by_entity_subtype) 
			ent = entitiesByType[i];
		else //not filtering by type
			ent = allEntities[i];

		// throw out ent if filtering by subtype and subtype doesn't match
		if (_filter_by_entity_subtype && !CorefUtils::subtypeMatch(entitySet, ent, ment))
			continue;

		// insert only ACE entities
		if (!ent->getType().isRecognized())
			continue;

		// check to see whether the candidate antecedent is in the same sentence 
		// or a previous sentence
		if (_considerWithinSentenceLinksOnly || _considerOutsideSentenceLinksOnly) {
			MentionUID last_id = CorefUtils::getLatestMention(entitySet, ent, ment);
			if (last_id.isValid()) {
				Mention *lastMention = entitySet->getMention(last_id);
				if (ment->getSentenceNumber() == lastMention->getSentenceNumber()) {
					if (_considerOutsideSentenceLinksOnly)
						continue;
				} 
				else {
					if (_considerWithinSentenceLinksOnly)
						continue;
				}
			}
		}

		// this is a bit biased as we are enabling to link to the correct ID in devtest
		// while in real run we will not enable it. However the bias is very small.
		// We include the correct entity in order to decide whther there was an error or not.
		if (TARGET_MENTION_TYPE==Mention::NAME && _limit_to_names 
			&& (MODE == TRAIN || ent->getID()!=entityID) 
			&& (CorefUtils::getEntityMentionLevel(entitySet,ent)!=Mention::NAME))
				continue;

		CorefUtils::insertEntityIntoSortedArray(sortedEnts, entitySet, ent, ment, ENTITY_LINK_MAX);
	}

	// sort the non-ACE entities and insert some of them
	if (_use_non_ace_entities_as_no_links) {
		GrowableArray<Entity *> non_ACE_ents;
		for (int i = 0; i < allEntities.length(); i++) {

			Entity *ent = allEntities[i];
			if (!ent->getType().isRecognized())
				CorefUtils::insertEntityIntoSortedArray(non_ACE_ents, entitySet, ent, ment, ENTITY_LINK_MAX);
		}//for

		int max_non_ACE_used = (non_ACE_ents.length() > max_non_ace_candidates) ? max_non_ace_candidates : non_ACE_ents.length();
		for (int i=non_ACE_ents.length()-1; i>=non_ACE_ents.length()-max_non_ACE_used&& i>=0; i--) {

			CorefUtils::insertEntityIntoSortedArray(sortedEnts, entitySet, non_ACE_ents[i], ment, ENTITY_LINK_MAX);
		}
	}//use non-ACE entities
}




void DTCorefTrainer::correctCorefAllNameMentions(EntitySet *entitySet, EntitySet *corrEntitySet, MentionSet *mentionSet)
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		MentionUID id = mention->getUID();

//		if (!mention->isOfRecognizedType() && MODE == TRAIN && MODEL_TYPE != P1_RANKING)
//			continue;
		if (mention->mentionType != Mention::NAME)
			continue;
		Entity *corefEnt = corrEntitySet->getEntityByMention(mention->getUID(), mention->getEntityType());
		if (corefEnt == 0)
			continue;

		if (_alreadySeenDocumentEntityIDtoTrainingID->get(corefEnt->getID()) == 0) {
			(*_alreadySeenDocumentEntityIDtoTrainingID)[corefEnt->getID()] = entitySet->getNEntities();
			entitySet->addNew(mention->getUID(), mention->getEntityType());
		}else
			entitySet->add(mention->getUID(), (*_alreadySeenDocumentEntityIDtoTrainingID)[corefEnt->getID()]);
	}	
}


void DTCorefTrainer::correctCorefAllNameMentions(EntitySet *entitySet, int ids[], MentionSet *mentionSet)
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		int coref_id = ids[i];
		if (coref_id == CorefItem::NO_ID) {
			Mention *child = mention->getChild();
			while (child != NULL) {
				if (child->getMentionType() == Mention::NONE)
					coref_id = ids[child->getIndex()];
				child = child->getChild();
			}
		}
		if (mention->mentionType != Mention::NAME)
			continue;

		if (_alreadySeenDocumentEntityIDtoTrainingID->get(coref_id) == 0) {
			entitySet->addNew(mention->getUID(), mention->getEntityType());
			if (coref_id != CorefItem::NO_ID)
				(*_alreadySeenDocumentEntityIDtoTrainingID)[coref_id] = entitySet->getEntityByMention(mention->getUID())->getID();
		}else
			entitySet->add(mention->getUID(), (*_alreadySeenDocumentEntityIDtoTrainingID)[coref_id]);

	}	
}

void DTCorefTrainer::computeConfidence(std::vector<LinkRecord> candidates, double correct_score, double &best_conf, double &sec_best_conf, double &correct_conf) {
	
	if (candidates.size() <= 1) {
		best_conf = 1, sec_best_conf = 1, correct_conf = 1;
		return;
	}	

	double best_score = candidates.at(0).score;
	double sec_best_score = candidates.at(1).score;
	double third_best_score = (candidates.size() > 2) ? candidates.at(2).score : 0;

	double normalizer = (std::abs(third_best_score)+ std::abs(sec_best_score))/2;

	best_conf = best_score / normalizer;
	sec_best_conf = sec_best_score / normalizer;
	double third_best_conf = third_best_score / normalizer;
	correct_conf =  correct_score / normalizer;
		
	if(sec_best_conf > 0) {
		best_conf -= sec_best_conf; third_best_conf -= sec_best_conf; correct_conf -= sec_best_conf; sec_best_conf = 0;
	}
		
	double  divider = (best_conf > 600) ? best_conf/600 : 1;
	best_conf /= divider;
	sec_best_conf /= divider; third_best_conf /= divider; correct_conf /= divider;

	double sum_of_confidences = exp(best_conf)+exp(sec_best_conf)+exp(third_best_conf);

	best_conf = exp(best_conf) / sum_of_confidences;
	sec_best_conf = exp(sec_best_conf) / sum_of_confidences;
	third_best_conf = exp(third_best_conf) / sum_of_confidences;
	correct_conf = exp(correct_conf) /sum_of_confidences;;
}

void DTCorefTrainer::printEntity(UTF8OutputStream &out, EntitySet *entitySet, Entity *entity) {
	for (int i = 0; i < entity->getNMentions(); i++) {
		MentionUID guid = entity->getMention(i);
		int sentno = Mention::getSentenceNumberFromUID(guid);
		Mention *m = entitySet->getMentionSet(sentno)->getMention(guid);
		if (m->getMentionType() == TARGET_MENTION_TYPE) out << L"<b>";
		out << m->getNode()->toTextString();
		if (m->getMentionType() == TARGET_MENTION_TYPE)	out << L"</b>";
		if (i != entity->getNMentions() - 1)
			out << L"; ";
	}
	out.flush();
}

void DTCorefTrainer::printDebugScores(DTCorefObservation *observation,
									  UTF8OutputStream& stream) 
{	
	if (MODEL_TYPE == MAX_ENT) {
		int best_tag;
		_maxEntDecoder->decodeToDistribution(observation, _tagScores, _tagSet->getNTags(), &best_tag);
	
		int best = 0;
		int second_best = 0;
		double best_score = MIN_VAL;
		double second_best_score = MIN_VAL;
		for (int i = 0; i < _tagSet->getNTags(); i++) {
			if (_tagScores[i] > best_score) {
				second_best = best;
				second_best_score = best_score;
				best = i;
				best_score = _tagScores[i];
			} else if (_tagScores[i] > second_best_score) {
				second_best = i;			
				second_best_score = _tagScores[i];
			}
		}
		stream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
		stream << "<font size=1>\n";
		_maxEntDecoder->printHTMLDebugInfo(observation, best, stream);
		stream << "</font>\n";
		stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
		stream << "<font size=1>\n";
		_maxEntDecoder->printHTMLDebugInfo(observation, second_best, stream);
		stream << "</font>\n";
		if (DEBUG) {
			_debugStream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
			_maxEntDecoder->printHTMLDebugInfo(observation, best, _debugStream);
			_debugStream << "\n";
			_debugStream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
			_debugStream << "\n";
			_maxEntDecoder->printHTMLDebugInfo(observation, second_best, _debugStream);
			_debugStream << "\n";
		}
	}
	else if (MODEL_TYPE == P1 ||MODEL_TYPE == P1_RANKING) {
		decodeToP1Distribution(observation);

		int best = 0;
		int second_best = 0;
		double best_score = MIN_VAL;
		double second_best_score = MIN_VAL;
		for (int i = 0; i < _tagSet->getNTags(); i++) {
			if (_tagScores[i] > best_score) {
				second_best = best;
				second_best_score = best_score;
				best = i;
				best_score = _tagScores[i];
			} else if (_tagScores[i] > second_best_score) {
				second_best = i;			
				second_best_score = _tagScores[i];
			}
		}
		stream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
		stream << "<font size=1>\n";
		_p1Decoder->printHTMLDebugInfo(observation, best, stream);
		stream << "</font>\n";
		stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
		stream << "<font size=1>\n";
		_p1Decoder->printHTMLDebugInfo(observation, second_best, stream);
		stream << "</font>\n";
		if (DEBUG) {
			_debugStream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
			_p1Decoder->printHTMLDebugInfo(observation, best, _debugStream);
			_debugStream << "\n";
			_debugStream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
			_debugStream << "\n";
			_p1Decoder->printHTMLDebugInfo(observation, second_best, _debugStream);
			_debugStream << "\n";
		}
	}
}

void DTCorefTrainer::printDebugScores(MentionUID mentionID, int entityID, 
									  int hobbs_distance,
									  UTF8OutputStream& stream) 
{
	DTCorefObservation *observation = static_cast<DTCorefObservation*>(_observations[LINK_OBS_INDEX]);
	observation->populate(mentionID, entityID, hobbs_distance);
	printDebugScores(observation, stream);
}

void DTCorefTrainer::printDebugScores(DTNoneCorefObservation *hyp_obs,
									  int hyp_tag, double hyp_score, double conf,
									  Symbol hypothesized_decision,
									  Entity *hyp_link,
									  DTNoneCorefObservation *sec_hyp_obs,
									  int sec_hyp_tag, double sec_score, double sec_conf,
									  Symbol sec_hypothesized_decision,
									  Entity *sec_hyp_link,
									  UTF8OutputStream& stream,
									  EntitySet *entitySet)
{
	if(hyp_link != NULL){
		stream << L"[";
		printEntity(stream, entitySet, hyp_link);
		stream << L"]<br>";
	}
//	stream << "<br>\n";
	stream << hypothesized_decision << L" ";
	stream << _tagSet->getTagSymbol(hyp_tag).to_string() << L": " << hyp_score  << ", "<< conf << L"<br>\n";
	stream << "<font size=1>\n";
	_p1Decoder->printHTMLDebugInfo(hyp_obs, hyp_tag, stream);
	stream << "</font>\n";
	if(sec_hyp_link != NULL){
		stream << L"[";
		printEntity(stream, entitySet, sec_hyp_link);
		stream << L"]<br>";
	}
//	stream << "<br>\n";
	stream << sec_hypothesized_decision << L" ";
	stream << _tagSet->getTagSymbol(sec_hyp_tag).to_string() << L": " << sec_score << ", "<< sec_conf <<L"<br>\n";
	stream << "<font size=1>\n";
	_p1Decoder->printHTMLDebugInfo(sec_hyp_obs, sec_hyp_tag, stream);
	stream << "</font>\n";
//	stream << L"TA " << _tagSet->getTagSymbol(correct_tag_index).to_string() << L": " << correct_score << L"<br>\n";
//	stream << "<font size=1>\n";
//	_p1Decoder->printHTMLDebugInfo(correct_obs, correct_tag_index, stream);
//	stream << "</font>\n";
}

void DTCorefTrainer::printHeadWords(SymbolArray **hwArray, UTF8OutputStream &stream){
	if (hwArray==0 || *hwArray==0)
		return;
	for (size_t i=0; i<(*hwArray)->getSizeTLength(); i++){
		stream <<(*hwArray)->getArray()[i].to_string() << L" ";
	}
}

std::wstring DTCorefTrainer::getCurrentDebugSentence(SentenceTheory *sentTheory, const Mention *ment) {
	if (sentTheory == 0)
		return std::wstring(L"");
	
	TokenSequence *ts = sentTheory->getTokenSequence();
	int start_tok = ment->getNode()->getStartToken();	
	int end_tok = ment->getNode()->getEndToken();

	std::wstring str = L"( ";
	for (int i = 0; i < ts->getNTokens(); i++) {
		if (i == start_tok)
			str += L"<b>";
		str += ts->getToken(i)->getSymbol().to_string();
		if (i == end_tok)
			str += L"</b>";
		str += L" ";
	}
	str += L")";
	return str;
}

/*
void DTCorefTrainer::trainMaxEntAugParses() {
	GrowableArray<CorefDocument*> documents;
	if(_list_mode){
		char buffer[500];
		boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& filelist(*filelist_scoped_ptr);
		UTF8Token filename;
		filelist.open(_training_file.c_str());
		while(!filelist.eof()){
			filelist >> filename;
			if(wcscmp(L"", filename.chars()) == 0){
				continue;
			}
			else{
				StringTransliterator::transliterateToEnglish(buffer, filename.chars(), 500);
				_inputSet.openFile(buffer);
				_inputSet.readAllDocuments(documents);
				_inputSet.closeFile();
			}
		}
	}
	else{
	_inputSet.openFile(_training_file.c_str());
	_inputSet.readAllDocuments(documents);
	
	}

	for (int i = 0; i < documents.length(); i++) {
		CorefDocument *thisDocument = documents[i];
		SessionLogger::info("SERIF") << "Storing document: " << thisDocument->documentName.to_debug_string() << "\n";
		processDocument(thisDocument);
	}
	_inputSet.closeFile();

	SessionLogger::info("SERIF") << "Deriving model...\n";
	_maxEntDecoder->deriveModel(_pruning);

	writeWeights();

	while(documents.length() != 0) {
		CorefDocument *thisDocument = documents.removeLast();
		delete thisDocument;
	}
}

void DTCorefTrainer::trainP1AugParses() {
	SessionLogger::info("SERIF") << "\n";
	for (int epoch = 0; epoch < _epochs; epoch++) {
		SessionLogger::info("SERIF") << "Epoch " << epoch << "...\n";
		trainEpochAugParses();
		writeWeights(epoch);
	}
	writeWeights();
}

void DTCorefTrainer::trainEpochAugParses() {
	_n_instances_seen = 0;
	_n_correct = 0;
	_total_n_correct = 0;
	GrowableArray<CorefDocument*> documents;
	if(_list_mode){
		char buffer[500];
		boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& filelist(*filelist_scoped_ptr);
		UTF8Token filename;
		filelist.open(_training_file.c_str());
		while(!filelist.eof()){
			filelist >> filename;
			if(wcscmp(L"", filename.chars()) == 0){
				continue;
			}
			else{
				StringTransliterator::transliterateToEnglish(buffer, filename.chars(), 500);
				_inputSet.openFile(buffer);
				_inputSet.readAllDocuments(documents);
				_inputSet.closeFile();
			}
		}
	}
	else{
		_inputSet.openFile(_training_file.c_str());
		_inputSet.readAllDocuments(documents);
		_inputSet.closeFile();
	}
	while(documents.length() != 0) {
		CorefDocument *thisDocument = documents.removeLast();
		//std::cout << "Processing document: " << thisDocument->documentName.to_debug_string() << "\n";
		processDocument(thisDocument);
		delete thisDocument;		
	}

	SessionLogger::info("SERIF") << "FINAL: (" << _n_instances_seen << " seen): " << _total_n_correct
		      << " (" << 100.0*_total_n_correct/_n_instances_seen << "%)" << "\n";
}

void DTCorefTrainer::devTestAugParses() {
	_inputSet.openFile(_training_file.c_str());
	GrowableArray<CorefDocument*> documents;
	_inputSet.readAllDocuments(documents);
	while(documents.length() != 0) {
		CorefDocument *thisDocument = documents.removeLast();
//		std::cout << "Processing document: " << thisDocument->documentName.to_debug_string() << "\n";
		processDocument(thisDocument);
		delete thisDocument;
	}
	_inputSet.closeFile();
}
*/
/***********************************************************************************
			Mostly copied from the DescriptorLinkerTrainer
			Change CollectLinkEvents to TrainMention, this is the decode/train stage
***********************************************************************************/
/*
void DTCorefTrainer::processDocument(CorefDocument *document) {
	EntitySet *entitySet = _new EntitySet(document->parses.length());
	MentionSet *mentionSet[MAX_DOCUMENT_SENTENCES];
	CorefItemList corefItems;
	_nEntityLinks = 0;

	for(int i=0; i< _n_observations; i++){
		if(_infoMap != NULL)
			_observations[i]->resetForNewDocument(entitySet, _infoMap);
		else
			_observations[i]->resetForNewDocument(entitySet);
	}

	// initialize coref ID hash
	int numBuckets = static_cast<int>(document->corefItems.length() / targetLoadingFactor);
	numBuckets = (numBuckets >= 2 ? numBuckets : 5);
	_alreadySeenDocumentEntityIDtoTrainingID = _new IntegerMap(numBuckets);

	_previousParses.clear();

	for (int i = 0; i < document->parses.length(); i++) {

		// collect all CorefItems associated with parse before creating MentionSet,
		// because it overwrites each node's mention pointer
		findSentenceCorefItems(corefItems, document->parses[i]->getRoot(), document);
		mentionSet[i] = _new MentionSet(document->parses[i], i);

		// add annotated mention and entity types to the new mention set
		boost::scoped_array<int> coref_ids(new int[mentionSet[i]->getNMentions()]);
		addCorefItemsToMentionSet(mentionSet[i], corefItems, coref_ids.get());

		// This adds subtypes (e.g. Nation or Continent) and
		//           mention types (e.g. DESC or LIST) to every mention.
		// It assumes that NAMEs are already marked as such (done in addCorefItemsToMentionSet).
		processMentions(document->parses[i]->getRoot(), mentionSet[i]);

		PropositionSet *propSet = _propositionFinder->getPropositionTheory(
				document->parses[i], mentionSet[i]);
		propSet->fillDefinitionsArray();

		// USING RELATIONS AS FEATURES
		//_relationObservation->resetForNewSentence(document->parses[i], mentionSet[i], propSet);

		// Add mentions to entity sets and train at each decision point.
		entitySet->loadMentionSet(mentionSet[i]);

		if (_use_metonymy)
			_metonymyAdder->addMetonymyTheory(mentionSet[i], propSet);

		processSentenceEntitiesFromAugParses(mentionSet[i], entitySet, propSet, coref_ids.get());

		if (MODE == TRAIN  && _use_p1_averaging && (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING)) {
			// For the P1 model, we add to the running sums every sentence;
			//  this is a nice balance between every instance (too slow)
			//  and every document (potentially too infrequent)
			for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights->begin();
				iter != _p1Weights->end(); ++iter)
			{
				(*iter).second.addToSum();
			}
		}

		while (corefItems.length() > 0)
			CorefItem *item = corefItems.removeLast();

		_previousParses.push_back(document->parses[i]);

		// find propositional links between entities
		for (int j = 0; j < propSet->getNPropositions(); j++) {
			const Proposition* prop = propSet->getProposition(j);
			if (prop->getNArgs() < 2 || prop->getArg(0)->getRoleSym() != Argument::REF_ROLE)
				continue;
			MentionUID ref = prop->getArg(0)->getMention(mentionSet[i])->getUID();
			Entity *refent = entitySet->getEntityByMention(ref);
			if (refent == 0)
				continue;
			for (int k = 1; k < prop->getNArgs(); k++) {
				if (prop->getArg(k)->getType() != Argument::MENTION_ARG)
					continue;
				MentionUID arg = prop->getArg(k)->getMention(mentionSet[i])->getUID();
				Entity *argent = entitySet->getEntityByMention(arg);
				if (argent != 0) {
					_entityLinks[_nEntityLinks].entity1 = refent->getID();
					_entityLinks[_nEntityLinks].entity2 = argent->getID();
					_entityLinks[_nEntityLinks].linkType = DTCorefObservation::_linkSym;
					_nEntityLinks++;
				}
			}
		}

		delete propSet;
	}

	// clean up data structures -- we can do this in P1, but not in MAX_ENT
	// -- at the moment we just leak them in MAX_ENT
	if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		for (int j = 0; j < document->parses.length(); j++) {
			delete mentionSet[j];
		}
		delete entitySet;
	}

	// empty the id map for a new document
	delete _alreadySeenDocumentEntityIDtoTrainingID;
	if(_infoMap!=NULL)
		_infoMap->cleanUpAfterDocument();
}// processDocument()


void DTCorefTrainer::findSentenceCorefItems(CorefItemList &items,
											const SynNode *node,
											CorefDocument *document)
{

	if (node->hasMention()) {
		if  (document->corefItems[node->getMentionIndex()]->mention != 0)
			items.add(document->corefItems[node->getMentionIndex()]);
		// Reset the mention index to default -1, so we don't have problems later
		// with non-NPKind coref items
		document->corefItems[node->getMentionIndex()]->node->setMentionIndex(-1);
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		findSentenceCorefItems(items, node->getChild(i), document);
	}

}

void DTCorefTrainer::addCorefItemsToMentionSet(MentionSet* mentionSet,
											   CorefItemList &items,
											   int ids[])
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		ids[i] = CorefItem::NO_ID;
		const SynNode *node = mention->getNode();
		for (int j = 0; j < items.length(); j++) {
			if (items[j]->node == node) {
				mention->setEntityType(items[j]->mention->getEntityType());
				mention->setEntitySubtype(items[j]->mention->getEntitySubtype());
				ids[i] = items[j]->corefID;
				// Only add NAME and PRON mention types, bc mention
				// classifiers die on already-marked DESCs -- it wants to figure
				// that out for itself
				// TODO: should/do we really need to mark the PRONs?
				if (items[j]->mention->mentionType == Mention::NAME ||
					items[j]->mention->mentionType == Mention::PRON)
					mention->mentionType = items[j]->mention->mentionType;
				break;
			}
		}
	}
}

void DTCorefTrainer::processMentions(const SynNode* node, MentionSet *mentionSet) {

	// first time into the sentence, so initialize the searcher
	_searcher.resetSearch(mentionSet, 1);
	processNode(node);
	if (EntitySubtype::subtypesDefined())
		generateSubtypes(node, mentionSet);

}

void DTCorefTrainer::processNode(const SynNode* node) {

	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		processNode(node->getChild(i));

	// process this node only if it has a mention
	if (!node->hasMention())
		return;

	// Test 1: See if it's a partitive
	_searcher.performSearch(node, _partitiveClassifier);

	// Test 2: See if it's an appositive
	// won't do anything if it's already partitive
	_searcher.performSearch(node, _appositiveClassifier);

	// Test 3: See if it's a list
	// won't do anything if it's part or appos
	_searcher.performSearch(node, _listClassifier);

	// Test 4: See if it's a nested mention
	// won't do anything if it's part, appos, or list
	// note that being nested doesn't preclude later classification
	_searcher.performSearch(node, _nestedClassifier);

	// test 5: See if it's a pronoun (and mark it but don't classify till later)
	// won't do anything if it's part, appos, list, or nested
	_searcher.performSearch(node, *_pronounClassifier);

	// label everything else as DESC here
	_searcher.performSearch(node, _otherClassifier);
}

void DTCorefTrainer::generateSubtypes(const SynNode *node, MentionSet *mentionSet) {
	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		generateSubtypes(node->getChild(i), mentionSet);

	// process this node only if it has a mention
	if (!node->hasMention())
		return;

	// don't process this node if it already has a subtype
	Mention *ment = mentionSet->getMention(node->getMentionIndex());
	if (ment->getEntitySubtype() != EntitySubtype::getUndetType())
		return;

	_searcher.performSearch(node, _subtypeClassifier);

}

void DTCorefTrainer::processSentenceEntitiesFromAugParses(MentionSet *mentionSet,
											      EntitySet *entitySet,
												  PropositionSet *propSet,
												  int ids[])
{
	for(int i=0; i< _n_observations; i++){
        _observations[i]->resetForNewSentence(mentionSet);
	}

	// set up pre-link mention arrays
	_copulaLinks.clear();
	_appositiveLinks.clear();
	_specialLinks.clear();

	// this will find the prelinks and store them in these arrays
	PreLinker::preLinkAppositives(_appositiveLinks, mentionSet);
	PreLinker::preLinkSpecialCases(_specialLinks, mentionSet, propSet);
	PreLinker::preLinkCopulas(_copulaLinks, mentionSet, propSet);

	const MentionSet origMentionSet(*mentionSet);
	if (_use_metonymy) {
		// This actually changes the MentionSet and its mentions
		// A mention entityType might be changed when it is a metonymic mention
		_metonymyAdder->addMetonymyTheory(mentionSet, propSet);
	}

	// determine the order the mentions are added - 
	// This should be as similar as possible to the way the linker works
	int nMentions = mentionSet->getNMentions();
	GrowableArray <MentionUID> mentions(nMentions);
	if(_train_from_left_to_right_on_sentence) {
		// add the Names fist as to enable forward coref within a sigle sentence
		if (_do_names_first)
			correctCorefAllNameMentions(entitySet, ids, mentionSet);
		for (int i = 0; i < mentionSet->getNMentions(); i++) {
			Mention *mention = mentionSet->getMention(mentions[i]);
			// already added names earlier
			if (_do_names_first && mention->mentionType == Mention::NAME)
				continue;

			mentions.add(mention->getUID());
		}
	}else {
		//now split the mentions
		GrowableArray <MentionUID> names(nMentions);
		GrowableArray <MentionUID> descriptors(nMentions);
		GrowableArray <MentionUID> pronouns(nMentions);
		GrowableArray <MentionUID> others(nMentions);

		for (int j = 0; j < nMentions; j++) {
			Mention *thisMention = mentionSet->getMention(j);
			if (thisMention->mentionType == Mention::NAME)
				names.add(thisMention->getUID());
			else if (thisMention->mentionType == Mention::DESC &&
					 thisMention->getEntityType().isRecognized())
				descriptors.add(thisMention->getUID());
			else if (thisMention->mentionType == Mention::PRON &&
				WordConstants::isLinkingPronoun(
						thisMention->getNode()->getHeadWord()))
				pronouns.add(thisMention->getUID());
			else
				others.add(thisMention->getUID());
		}
		// add the name mentions first followed by nominal and pronoun last
		for (int i=0; i<names.length(); i++)
			mentions.add(names[i]);
		for (int i=0; i<others.length(); i++)
			mentions.add(others[i]);
		for (int i=0; i<descriptors.length(); i++)
			mentions.add(descriptors[i]);
		for (int i=0; i<pronouns.length(); i++)
			mentions.add(pronouns[i]);
	}

	
	for (int i = 0; i < mentions.length(); i++) {
		Mention *mention = mentionSet->getMention(mentions[i]);
		const Mention *origMention = origMentionSet.getMention(mentions[i]);
		const EntityType mentionOrigEntityType = origMention->getEntityType();

		// add some mention information that is used during the feature extraction
		// with AugParses there is no easy access to the TokenSequence so currently
		// the information from the token sequence is not supported
		if(_infoMap != NULL)
			_infoMap->addMentionInformation(mention);

		if (mention->mentionType == Mention::NAME ||
			mention->mentionType == Mention::PRON ||
			mention->mentionType == Mention::DESC)
		{
			int coref_id = ids[i];
			if (coref_id == CorefItem::NO_ID) {
				Mention *child = mention->getChild();
				while (child != NULL) {
					if (child->getMentionType() == Mention::NONE)
						coref_id = ids[child->getIndex()];
					child = child->getChild();
				}
			}

			if (coref_id == CorefItem::NO_ID)
				continue;

			// exclude copula-linked and specially-linked instances from training
			if (mention->getMentionType() == TARGET_MENTION_TYPE &&
				_copulaLinks[i] == 0 &&
				_specialLinks[i] == 0 &&
				!NodeInfo::isNominalPremod(mention->getNode()))
			{
				_nMentionLinks = 0;
				const Proposition* prop = propSet->getDefinition(i);
				if (prop != 0) {
					for (int k = 1; k < prop->getNArgs(); k++) {
						Argument *arg = prop->getArg(k);
						if (arg->getType() != Argument::MENTION_ARG)
							continue;
						const Mention *argMent = arg->getMention(mentionSet);
						Entity *argEnt = entitySet->getEntityByMention(argMent->getUID());
						// we need to have already seen this mention in our path, or
						// it needs to be a name
						if (argEnt == 0 && argMent->getMentionType() != Mention::NAME)
							continue;
						int arg_coref_id = ids[arg->getMentionIndex()];
						if (arg_coref_id == CorefItem::NO_ID) {
							Mention *child = arg->getMention(mentionSet)->getChild();
							while (child != NULL) {
								if (child->getMentionType() == Mention::NONE)
									arg_coref_id = ids[child->getIndex()];
								child = child->getChild();
							}
						}
						if (arg_coref_id == CorefItem::NO_ID || _alreadySeenDocumentEntityIDtoTrainingID->get(arg_coref_id) == 0)
							continue;
						int arg_ent_id =  (*_alreadySeenDocumentEntityIDtoTrainingID)[arg_coref_id];
						if (_nMentionLinks < MENTION_LINK_MAX) {
							_mentionLinks[_nMentionLinks].entity1 = arg_ent_id;
							_mentionLinks[_nMentionLinks].entity2 = -1;
							_mentionLinks[_nMentionLinks].linkType = DTCorefObservation::_linkSym;
							_nMentionLinks++;
						}
					}
				}
				trainMention(mention, mentionOrigEntityType, coref_id, entitySet);
			}

			// We are going to pretend this mention doesn't exist (if it is even in the training data)
			//if (DESC_LINKING && mention->mentionType == Mention::PRON) {
			//	continue;
			//}

			// once we're done training, add the mention in question to its appropriate entity (or
			// make a new one), and move on as if we'd done the right thing
			if (_alreadySeenDocumentEntityIDtoTrainingID->get(coref_id) == 0) {
				entitySet->addNew(mention->getUID(), mention->getEntityType());
				if (coref_id != CorefItem::NO_ID)
					(*_alreadySeenDocumentEntityIDtoTrainingID)[coref_id] = entitySet->getEntityByMention(mention->getUID())->getID();
			}
			else
				entitySet->add(mention->getUID(), (*_alreadySeenDocumentEntityIDtoTrainingID)[coref_id]);

		}
	}

}*/
