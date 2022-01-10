// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/limits.h"
#include "common/UTF8InputStream.h"
#include "common/ParamReader.h"
#include "discTagger/DTTagSet.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "discTagger/P1Decoder.h"
#include "maxent/MaxEntModel.h"
#include "relations/discmodel/RelationObservation.h"
#include "relations/discmodel/DTRelationSet.h"
#include "discTagger/DTTagSet.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "relations/discmodel/P1RelationAltModel.h"
#include "relations/discmodel/P1RelationFeatureType.h"
#include "relations/discmodel/RelationObservation.h"
#include <boost/scoped_ptr.hpp>


P1RelationAltModel::P1RelationAltModel() : _nDecoders(0), _is_initialized(false) {}

void P1RelationAltModel::initialize() {
	// already been initialized, so just return
	if (_is_initialized)
		return;
	
	_is_initialized = true;

	char buffer[500];
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
	ParamReader::getRequiredParam("alternative_relation_model_file", buffer, 500);
	uis.open(buffer);
	if(uis.fail()){
		sprintf(msg, "Error opening alternative-relation-model-file: %s", buffer);
		throw UnrecoverableException("P1RelationAltModel:P1RelationAltModel()", msg);
	}
	uis >> tok;
	_nDecoders = _wtoi(tok.chars());
	//std::cout<<"tok: "<<tok.symValue().to_debug_string()<< " _ndecoders: "<<_nDecoders<<std::endl;
	if(_nDecoders >= MAX_ALT_P1_DECODERS){
		sprintf(msg, 
			"Number of decoders in alternative-relation-model-file is greater than MAX_ALT_P1_DECODERS: %f", 
			MAX_ALT_P1_DECODERS);
		throw UnrecoverableException("P1RelationAltModel:P1RelationAltModel()", msg);
	}
	for(int i =0; i< _nDecoders; i++){
		//std::cout<<"reading decoder: "<<i<<std::endl;
		uis >>tok;
		_altDecoders[i]._name = tok.symValue();
		
		uis >>tok;
		if (tok.symValue() == Symbol(L"MAXENT"))
			_altDecoders[i]._type = MAXENT;
		else if (tok.symValue() == Symbol(L"P1"))
			_altDecoders[i]._type = P1;
		else
			throw UnexpectedInputException("P1RelationAltModel::P1RelationAltModel()",
				"Found unexpected model type in alternative-relation-model-file, should be 'P1' or 'MAXENT'.");
		
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		_altDecoders[i]._tagSet = _new DTTagSet(buffer, false, false);
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		_altDecoders[i]._features = _new DTFeatureTypeSet(buffer, P1RelationFeatureType::modeltype);
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		_altDecoders[i]._weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_altDecoders[i]._weights, buffer,  P1RelationFeatureType::modeltype);
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

P1RelationAltModel::~P1RelationAltModel(){
	for (int i =0; i< _nDecoders; i++){
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
int P1RelationAltModel::getNAltDecoders(){
	return _nDecoders;
}
Symbol P1RelationAltModel::getDecoderName(int i){
	char msg[1000];
	if(i >= _nDecoders){
		sprintf(msg, 
			"getDecoderName i is greater than n_decoders: %f", 
			_nDecoders);
		throw UnrecoverableException("P1RelationAltModel:getDecoderName()", msg);
	}
	return _altDecoders[i]._name;
}
Symbol P1RelationAltModel::getDecoderPrediction(int i, RelationObservation *obs){
	char msg[1000];
	if(i >= _nDecoders){
		sprintf(msg, 
			"getDecoderPrediction i is greater than n_decoders: %f", 
			_nDecoders);
		throw UnrecoverableException("P1RelationAltModel:getDecoderPrediction()", msg);
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
Symbol P1RelationAltModel::getDecoderPrediction(Symbol name, RelationObservation *obs){
	char msg[1000];
	int decnum = -1;
	for(int i =0; i<_nDecoders; i++){
		if(_altDecoders[i]._name == name){
			decnum = i;
			break;
		}
	}
	if(decnum == -1){
		sprintf(msg, 
			"No Decoder defined with name: %s", 
			name.to_debug_string());
		throw UnrecoverableException("P1RelationAltModel:getDecoderPrediction()", msg);
	}
	return getDecoderPrediction(decnum, obs);
}	
int P1RelationAltModel::addDecoderPredictionsToObservation(RelationObservation *obs){
	for(int i = 0; i<_nDecoders; i++){
		Symbol result;
		if (_altDecoders[i]._type == P1)
			result = _altDecoders[i]._p1Decoder->decodeToSymbol(obs);
		else // _altDecoders[i]._type == MAXENT
			result = _altDecoders[i]._maxentDecoder->decodeToSymbol(obs);
		obs->setAltDecoderPrediction(i, result);
	}
	return i;
}



