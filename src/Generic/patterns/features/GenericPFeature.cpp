// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/GenericPFeature.h"

int GenericPFeature::getSentenceNumber() const { return _sent_no; }

int GenericPFeature::getStartToken() const { return -1; }

int GenericPFeature::getEndToken() const { return -1; }

bool GenericPFeature::equals(PatternFeature_ptr other) { 
	if (boost::shared_ptr<GenericPFeature> f = 
			boost::dynamic_pointer_cast<GenericPFeature>(other)) {
		return f->getSentenceNumber() == getSentenceNumber() && f->getPattern() == getPattern();
	}
	return false;
}

void GenericPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const { /* do nothing */ }

void GenericPFeature::setCoverage(const DocTheory * docTheory) { /* do nothing */ }

void GenericPFeature::setCoverage(const PatternMatcher_ptr patternMatcher) { /* do nothing */ }

void GenericPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);
	
	elem.setAttribute(X_sentence_number, _sent_no);
}

GenericPFeature::GenericPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
}
