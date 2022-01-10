// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/DescriptorClassifier.h"
#include "Generic/descriptors/discmodel/P1DescriptorClassifier.h"

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"

#include "Generic/propositions/PropositionFinder.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/theories/NodeInfo.h"

#include "Generic/theories/Parse.h"
#include "time.h"

#include "Generic/maxent/MaxEntModel.h"

P1DescriptorClassifier::P1DescriptorClassifier(Task task)
	: _task(task), _propFinder(0), _propSet(0), _currRoot(0),
	_maxEntDecoder(0)
{
	P1DescFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();
	
	if (ParamReader::isParamTrue("p1_desc_use_propositions")) {
		_propFinder = _new PropositionFinder();
	}

	std::string features_file;
	std::string model_file;
	std::string tag_set_file;
	double undergen = 0;
	if (_task == DESC_CLASSIFY) {
		features_file = ParamReader::getRequiredParam("p1_desc_features_file");
		model_file = ParamReader::getRequiredParam("p1_desc_model_file");
		tag_set_file = ParamReader::getRequiredParam("p1_desc_tag_set_file");		
		undergen = ParamReader::getOptionalFloatParamWithDefaultValue("p1_desc_undergen_percentage", 0);
	}
	else if (_task == PREMOD_CLASSIFY) { 		
		features_file = ParamReader::getRequiredParam("p1_nom_premod_features_file");
		model_file = ParamReader::getRequiredParam("p1_nom_premod_model_file");
		tag_set_file = ParamReader::getRequiredParam("p1_desc_tag_set_file");		
	}
	else { // _task == PRONOUN_CLASSIFY		
		features_file = ParamReader::getRequiredParam("p1_pronoun_type_features_file");
		model_file = ParamReader::getRequiredParam("p1_pronoun_type_model_file");
		tag_set_file = ParamReader::getRequiredParam("p1_pronoun_type_tag_set_file");	
	}
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1DescFeatureType::modeltype);

	_weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*_weights, model_file.c_str(), P1DescFeatureType::modeltype);

	_decoder = _new P1Decoder(_tagSet, _featureTypes, _weights);
	if (undergen != 0)
		_decoder->setUndergenPercentage(undergen);
	std::string maxent_model_file = ParamReader::getParam("maxent_desc_model_file");
	if (!maxent_model_file.empty()) {
		_maxEntWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_maxEntWeights, maxent_model_file.c_str(), P1DescFeatureType::modeltype);
		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxEntWeights);
	}




	// turn on wordnet only if we're doing descriptors, not nom-premods:
	//_observation = _new DescriptorObservation(_task != PREMOD_CLASSIFY);
	//turn of wordnet for speed! mrf 8-9-05
	_observation = _new DescriptorObservation(ParamReader::isParamTrue("p1_desc_use_wordnet"));	
}

P1DescriptorClassifier::~P1DescriptorClassifier() {
	delete _observation;
	delete _decoder;
	delete _weights;
	delete _featureTypes;
	delete _tagSet;
	delete _propFinder;
	delete _propSet;
}

void P1DescriptorClassifier::cleanup() {
	if (_propSet) {
		delete _propSet;
		_propSet = 0;
	}
}

int P1DescriptorClassifier::classifyDescriptor(MentionSet *currSolution, const SynNode* node,
											 EntityType types[], double scores[], int max_results)
{
	int num_results = 0;
	double total_score = 1;

	updatePropositionSet(currSolution);
	_observation->populate(currSolution, node, _propSet);

	/*
	if(_propFinder != 0){
		updatePropositionSet(currSolution);
		//std::cout<<"add prop set to  observation: "<<std::endl;
		_observation->addPropositionSet(_propSet);
	}*/
	
	EntityType proposedType = EntityType::getOtherType();
	//std::cout<<"decode"<<std::endl;
	//_decoder->setOvergenPercentage(.1);
	Symbol answer = _decoder->decodeToSymbol(_observation);
	//std::cout<<"done decoding"<<std::endl;
	//overgenerate with confident max ent
	if(_maxEntDecoder != 0){
		double max_ent_scores[500];
		int best_tag = 0;
		int ntags = _maxEntDecoder->decodeToDistribution(_observation, 
			max_ent_scores, 500, &best_tag);
		if(max_ent_scores[best_tag] > .7){
			answer = _tagSet->getTagSymbol(best_tag);
		}
	}
	if (answer != _tagSet->getNoneTag()) {
		proposedType = 	EntityType(answer);
	}

	num_results = DescriptorClassifier::insertScore(total_score, proposedType,
				scores, types, num_results, max_results);

	// this would be odd
	if (num_results == 0)
		throw InternalInconsistencyException("DescriptorClassifier::_classifyDescriptor",
		"Couldn't find a decent score");

	return num_results;
}

EntityType P1DescriptorClassifier::classifyDescriptor(
	MentionSet *currSolution, const SynNode *node)
{
	_observation->populate(currSolution, node);
	Symbol answer = _decoder->decodeToSymbol(_observation);
	if (answer == _tagSet->getNoneTag())
		return EntityType::getOtherType();
	else
		return EntityType(answer);
}
void P1DescriptorClassifier::updatePropositionSet(MentionSet* mentSet) {

	if (_propFinder == 0)
		return;

	const SynNode* thisroot = mentSet->getParse()->getRoot();
	while(thisroot->getParent() != 0){
		thisroot = thisroot->getParent();
	}
	if((_propSet !=0) &&
		(_currRoot != 0) && 
		(thisroot == _currRoot) &&
		(thisroot->getID() == _currRoot->getID())){
	}
	else{
		delete _propSet;
		_propSet = 0;
		_propFinder->resetForNewSentence(mentSet->getSentenceNumber());
		//make a copy of the thisroot
		_currRoot = thisroot;
		_propSet = _propFinder->getPropositionTheory(thisroot, mentSet);
		if(_propSet->getNPropositions() > 0){
		}
		_currRoot = 0;
	}
}
