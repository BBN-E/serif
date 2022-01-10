// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "theories/KoreanFeatureValueStructure.h"
#include <iostream>

KoreanFeatureValueStructure::KoreanFeatureValueStructure() {
	Symbol null(L":NULL");
	_partOfSpeech = null;
	_analyzed = false;

	char message[1000];
	sprintf(message, "%s", "Warning: Instantiating a KoreanFeatureValueStructure with null arguments!");  
	std::cerr << message << std::endl;
}

KoreanFeatureValueStructure::KoreanFeatureValueStructure(Symbol partOfSpeech, bool analyzed):
			_partOfSpeech(partOfSpeech), _analyzed(analyzed) {}

bool KoreanFeatureValueStructure::operator==(const FeatureValueStructure &other) const {
	const KoreanFeatureValueStructure *local = static_cast<const KoreanFeatureValueStructure*>(&other);
	if (_partOfSpeech == local->_partOfSpeech && _analyzed == local->_analyzed)
		return true;
	return false;
}

bool KoreanFeatureValueStructure::operator!=(const FeatureValueStructure &other) const {
	const KoreanFeatureValueStructure *local = static_cast<const KoreanFeatureValueStructure*>(&other);
	if (_partOfSpeech == local->_partOfSpeech && _analyzed == local->_analyzed)
		return false;
	return true;
}

void KoreanFeatureValueStructure::dump(UTF8OutputStream &uos) {
	uos << " " << _partOfSpeech.to_string() << " "; 
	uos.flush();
}




void KoreanFeatureValueStructure::saveXML(SerifXML::XMLElement fvsElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("KoreanFeatureValueStructure::saveXML", "Expected context to be NULL");
	fvsElem.setAttribute(X_pos, _partOfSpeech);
	fvsElem.setAttribute(X_is_analyzed, _analyzed);
}

KoreanFeatureValueStructure::KoreanFeatureValueStructure(SerifXML::XMLElement fvsElem) {
	using namespace SerifXML;
	_partOfSpeech = fvsElem.getAttribute<Symbol>(X_pos);
	_analyzed = fvsElem.getAttribute<bool>(X_is_analyzed, false);
}
