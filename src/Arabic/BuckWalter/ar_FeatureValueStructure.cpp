// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Arabic/BuckWalter/ar_FeatureValueStructure.h"
#include "Arabic/BuckWalter/ar_BuckWalterFunctions.h"
#include "Generic/state/XMLTheoryElement.h"
#include <iostream>

ArabicFeatureValueStructure::ArabicFeatureValueStructure() {
	Symbol null(L":NULL");
	_category = null;
	_voweledString = null;
	_partOfSpeech = null;
	_gloss = null;
	_analyzed = false;

	std::cerr << "Warning: Instantiating a ArabicFeatureValueStructure with null arguments!" << std::endl;
}
	
ArabicFeatureValueStructure::ArabicFeatureValueStructure(Symbol category, Symbol voweledString, Symbol partOfSpeech, Symbol gloss, bool analyzed ) {
	
	Symbol empty;
	_category = category;
	if(_category == empty){
		throw UnrecoverableException("ArabicFeatureValueStructure::ArabicFeatureValueStructure", 
			"Attempting to instantiate a ArabicFeatureValueStructure with a NULL category");
	}
	_voweledString = voweledString;
	if(_voweledString == empty){
		throw UnrecoverableException("ArabicFeatureValueStructure::ArabicFeatureValueStructure", 
			"Attempting to instantiate a ArabicFeatureValueStructure with a NULL _voweledString");
	}
	_partOfSpeech = partOfSpeech;
	if(_partOfSpeech == empty){
		throw UnrecoverableException("ArabicFeatureValueStructure::ArabicFeatureValueStructure", 
			"Attempting to instantiate a ArabicFeatureValueStructure with a NULL _partOfSpeech");
	}
	_gloss = gloss;
	if(_gloss == empty){
		throw UnrecoverableException("ArabicFeatureValueStructure::ArabicFeatureValueStructure", 
			"Attempting to instantiate a ArabicFeatureValueStructure with a NULL _gloss");
	}
	_analyzed = analyzed;
}

bool ArabicFeatureValueStructure::operator==(const FeatureValueStructure &other) const {
	const ArabicFeatureValueStructure *local = static_cast<const ArabicFeatureValueStructure*>(&other);
	if (_category == local->_category && _voweledString == local->_voweledString &&
		_partOfSpeech == local->_partOfSpeech && _analyzed == local->_analyzed &&
		_gloss == local->_gloss)
	{
		return true;
	}
	return false;
}

bool ArabicFeatureValueStructure::operator!=(const FeatureValueStructure &other) const {
	const ArabicFeatureValueStructure *local = static_cast<const ArabicFeatureValueStructure*>(&other);
	if (_category == local->_category && _voweledString == local->_voweledString &&
		_partOfSpeech == local->_partOfSpeech && _analyzed == local->_analyzed &&
		_gloss == local->_gloss)
	{
		return false;
	}
	return true;
}

void ArabicFeatureValueStructure::dump(UTF8OutputStream &uos){
	uos << " " << _category.to_string() << " " 
		<< _voweledString.to_string() << " "
		<< _partOfSpeech.to_string() << " "
		<< "( "<<_gloss.to_string()<<" )";
	uos.flush();
}

void ArabicFeatureValueStructure::saveXML(SerifXML::XMLTheoryElement fvsElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("ArabicFeatureValueStructure::saveXML", "Expected context to be NULL");
	if (_category != BuckwalterFunctions::NULL_SYMBOL)
		fvsElem.setAttribute(X_category, _category);
	if (_voweledString != BuckwalterFunctions::NULL_SYMBOL)
		fvsElem.setAttribute(X_voweled_string, _voweledString);
	if (_partOfSpeech != BuckwalterFunctions::NULL_SYMBOL)
		fvsElem.setAttribute(X_pos, _partOfSpeech);
	if (_gloss != BuckwalterFunctions::NULL_SYMBOL)
		fvsElem.setAttribute(X_gloss, _gloss);
}

ArabicFeatureValueStructure::ArabicFeatureValueStructure(SerifXML::XMLTheoryElement fvsElem) {
	using namespace SerifXML;
	_category = fvsElem.getAttribute<Symbol>(X_category, BuckwalterFunctions::NULL_SYMBOL);
	_voweledString = fvsElem.getAttribute<Symbol>(X_voweled_string, BuckwalterFunctions::NULL_SYMBOL);
	_partOfSpeech = fvsElem.getAttribute<Symbol>(X_pos, BuckwalterFunctions::NULL_SYMBOL);
	_gloss = fvsElem.getAttribute<Symbol>(X_gloss, BuckwalterFunctions::NULL_SYMBOL);
	_analyzed = ((_category == BuckwalterFunctions::ANALYZED) ||
	             (_category == BuckwalterFunctions::RETOKENIZED) ||
	             (_category == BuckwalterFunctions::UNKNOWN));
}
