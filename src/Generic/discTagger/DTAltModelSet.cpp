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
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/discTagger/DTAltModelSet.h"
#include <boost/scoped_ptr.hpp>


DTAltModelSet::DTAltModelSet() : _nDecoders(0), _is_initialized(false) {}

void DTAltModelSet::initialize(Symbol modeltype, const char* paramName) {
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
	std::string stream_name = ParamReader::getParam(paramName);
	uis.open(stream_name.c_str());
	if (uis.fail()) {
		sprintf(msg, "Error opening %s: %s", paramName, stream_name.c_str());
		throw UnrecoverableException("DTAltModelSet:DTAltModelSet()", msg);
	}
	uis >> tok;
	_nDecoders = _wtoi(tok.chars());
	//std::cout<<"tok: "<<tok.symValue().to_debug_string()<< " _ndecoders: "<<_nDecoders<<std::endl;
	if (_nDecoders >= MAX_ALT_DT_DECODERS) {
		sprintf(msg, 
			"Number of decoders in %s is greater than MAX_ALT_DT_DECODERS: %d",
			paramName,
			MAX_ALT_DT_DECODERS);
		throw UnrecoverableException("DTAltModelSet:DTAltModelSet()", msg);
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
		else {
			sprintf(msg, 
				"Found unexpected model type in %s, should be 'P1' or 'MAXENT'.", 
				paramName);
			throw UnexpectedInputException("DTAltModelSet::DTAltModelSet()", msg);
		}
		
		uis >> tok;
		char buffer[500];
		wcstombs(buffer, tok.chars(), 500);
		_altDecoders[i]._tagSet = _new DTTagSet(buffer, false, false);
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		_altDecoders[i]._features = _new DTFeatureTypeSet(buffer, modeltype);
		uis >> tok;
		wcstombs(buffer, tok.chars(), 500);
		_altDecoders[i]._weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_altDecoders[i]._weights, buffer, modeltype);
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

DTAltModelSet::~DTAltModelSet() {
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

int DTAltModelSet::getNAltDecoders() {
	return _nDecoders;
}

Symbol DTAltModelSet::getDecoderName(int i) {
	char msg[1000];
	if( i >= _nDecoders) {
		sprintf(msg, 
			"getDecoderName i is greater than n_decoders: %d",
			_nDecoders);
		throw UnrecoverableException("DTAltModelSet:getDecoderName()", msg);
	}
	return _altDecoders[i]._name;
}
Symbol DTAltModelSet::getDecoderPrediction(int i, DTObservation *obs) {
	char msg[1000];
	if (i >= _nDecoders) {
		sprintf(msg, 
			"getDecoderPrediction i is greater than n_decoders: %d",
			_nDecoders);
		throw UnrecoverableException("DTAltModelSet:getDecoderPrediction()", msg);
	}
	Symbol tag;
	if (_altDecoders[i]._type == P1)
		tag =  _altDecoders[i]._p1Decoder->decodeToSymbol(obs);
	else // _altDecoders[i]._type == MAXENT
		tag =  _altDecoders[i]._maxentDecoder->decodeToSymbol(obs);
	return tag;

}
Symbol DTAltModelSet::getDecoderPrediction(Symbol name, DTObservation *obs) {
	char msg[1000];
	int decnum = -1;
	for (int i = 0; i <_nDecoders; i++) {
		if (_altDecoders[i]._name == name) {
			decnum = i;
			break;
		}
	}
	if (decnum == -1) {
		sprintf(msg, 
			"No Decoder defined with name: %s", 
			name.to_debug_string());
		throw UnrecoverableException("DTAltModelSet:getDecoderPrediction()", msg);
	}
	return getDecoderPrediction(decnum, obs);
}	

int DTAltModelSet::addDecoderPredictionsToObservation(DTObservation *obs) {
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



