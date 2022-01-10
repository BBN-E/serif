// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/DTRelationSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/relations/discmodel/DTRelationAltModel.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include <boost/scoped_ptr.hpp>


DTRelationAltModel::DTRelationAltModel() : _nDecoders(0), _is_initialized(false) {}

void DTRelationAltModel::initialize() {
	// already been initialized, so just return
	if (_is_initialized)
		return;
	
	_is_initialized = true;

	char msg[1000];
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	UTF8Token tok;
	/*sub-parameter file looks like
	#ofAlternativeModels
	MODEL_NAME-1
	model-type-1
	tagset-1
	featureset-1
	modelfile-1
	MODEL_NAME-2
	model-type-2
	tagset-2
	featureset-2
	modelfile-2
	.....
	*/
	std::string model = ParamReader::getRequiredParam("alternative_relation_model_file");
	uis.open(model.c_str());
	if (uis.fail()) {
		std::string error = "Error opening alternative_relation_model_file: " + model;
		throw UnrecoverableException("DTRelationAltModel:DTRelationAltModel()", error.c_str());
	}
	uis >> tok;
	_nDecoders = _wtoi(tok.chars());
	//std::cout<<"tok: "<<tok.symValue().to_debug_string()<< " _ndecoders: "<<_nDecoders<<std::endl;
	if (_nDecoders == 0) {
		sprintf(msg, 
			"Invalid number of decoders in alternative_relation_model_file: %s", 
			Symbol(tok.chars()).to_debug_string());
		throw UnrecoverableException("DTRelationAltModel:DTRelationAltModel()", msg);
	}
	else if (_nDecoders >= MAX_ALT_DT_DECODERS) {
		sprintf(msg, 
			"Number of decoders in alternative_relation_model_file is greater than MAX_ALT_DT_DECODERS: %d", 
			MAX_ALT_DT_DECODERS);
		throw UnrecoverableException("DTRelationAltModel:DTRelationAltModel()", msg);
	}
	for (int i = 0; i < _nDecoders; i++) {
		//std::cout<<"reading decoder: "<<i<<std::endl;
		uis >> tok;
		_altDecoders[i]._name = tok.symValue();
		
		uis >> tok;
		if (tok.symValue() == Symbol(L"MAXENT"))
			_altDecoders[i]._type = MAXENT;
		else if (tok.symValue() == Symbol(L"P1"))
			_altDecoders[i]._type = P1;
		else
			throw UnexpectedInputException("DTRelationAltModel::DTRelationAltModel()",
				"Parameter 'alternative_relation_model_file' must be set to 'P1' or 'MAXENT'.");
		
		char buffer[501];
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		std::string tagSetStr = ParamReader::expand(buffer);
		_altDecoders[i]._tagSet = _new DTTagSet(tagSetStr.c_str(), false, false);
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		std::string featuresStr = ParamReader::expand(buffer);
		_altDecoders[i]._features = _new DTFeatureTypeSet(featuresStr.c_str(), P1RelationFeatureType::modeltype);
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		std::string weightsStr = ParamReader::expand(buffer);
		_altDecoders[i]._weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_altDecoders[i]._weights, weightsStr.c_str(),  P1RelationFeatureType::modeltype);
		if (_altDecoders[i]._type == P1) {
			_altDecoders[i]._p1Decoder = _new P1Decoder(_altDecoders[i]._tagSet,
													    _altDecoders[i]._features, 
													    _altDecoders[i]._weights, 0, 0);
			_altDecoders[i]._maxentDecoder = 0;
		} else  { // _altDecoders[i]._type == MAXENT
			_altDecoders[i]._maxentDecoder = _new MaxEntModel(_altDecoders[i]._tagSet,
														_altDecoders[i]._features, 
														_altDecoders[i]._weights);
			_altDecoders[i]._p1Decoder = 0;
		}
	}
}

DTRelationAltModel::~DTRelationAltModel() {
	for (int i = 0; i < _nDecoders; i++) {
		delete _altDecoders[i]._tagSet;
		_altDecoders[i]._tagSet = 0;
		delete _altDecoders[i]._features;
		_altDecoders[i]._features = 0;
		delete _altDecoders[i]._weights;
		_altDecoders[i]._weights = 0;
		delete _altDecoders[i]._p1Decoder;
		_altDecoders[i]._p1Decoder= 0;
		delete _altDecoders[i]._maxentDecoder;
		_altDecoders[i]._maxentDecoder= 0;
	}
	_nDecoders = 0;
}
int DTRelationAltModel::getNAltDecoders() {
	return _nDecoders;
}
Symbol DTRelationAltModel::getDecoderName(int i) {
	char msg[1000];
	if (i >= _nDecoders) {
		sprintf(msg, 
			"getDecoderName i is greater than n_decoders: %d", _nDecoders);
		throw UnrecoverableException("DTRelationAltModel:getDecoderName()", msg);
	}
	return _altDecoders[i]._name;
}
Symbol DTRelationAltModel::getDecoderPrediction(int i, RelationObservation *obs) {
	char msg[1000];
	if (i >= _nDecoders) {
		sprintf(msg, 
			"getDecoderPrediction i is greater than n_decoders: %d", 
			_nDecoders);
		throw UnrecoverableException("DTRelationAltModel:getDecoderPrediction()", msg);
	}
	//make sure there isn't any 2005 validation happening in the decoder
	Symbol oldvalid = obs->getValidationType();
	obs->setValidationType(Symbol(L"NONE"));
	Symbol reltype;
	if (_altDecoders[i]._type == P1)
		reltype =  _altDecoders[i]._p1Decoder->decodeToSymbol(obs);
	else // _altDecoders[i]._type == MAXENT
		reltype =  _altDecoders[i]._maxentDecoder->decodeToSymbol(obs);
	obs->setValidationType(oldvalid);
	return reltype;

}
Symbol DTRelationAltModel::getDecoderPrediction(Symbol name, RelationObservation *obs) {
	char msg[1000];
	int decnum = -1;
	for (int i = 0; i < _nDecoders; i++) {
		if (_altDecoders[i]._name == name) {
			decnum = i;
			break;
		}
	}
	if (decnum == -1) {
		sprintf(msg, 
			"No Decoder defined with name: %s", 
			name.to_debug_string());
		throw UnrecoverableException("DTRelationAltModel:getDecoderPrediction()", msg);
	}
	return getDecoderPrediction(decnum, obs);
}	
int DTRelationAltModel::addDecoderPredictionsToObservation(RelationObservation *obs) {
	int i = 0;
	for (i = 0; i < _nDecoders; i++) {
		Symbol result;
		if (_altDecoders[i]._type == P1)
			result = _altDecoders[i]._p1Decoder->decodeToSymbol(obs);
		else // _altDecoders[i]._type == MAXENT
			result = _altDecoders[i]._maxentDecoder->decodeToSymbol(obs);
		obs->setAltDecoderPrediction(i, result);
	}
	return i;
}



