// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef CONFIDENCE_PFEATURE_H
#define CONFIDENCE_PFEATURE_H

#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "common/Symbol.h"

/** A pseudo-feature used to record an overall confidence level for a 
  * feature set. */
class ConfidencePFeature : public PseudoPatternFeature {
private:
	ConfidencePFeature(float confidence): PseudoPatternFeature(confidence) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ConfidencePFeature, float);
public:	
	void printFeatureFocus(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) {
		out << L"    <focus type=\"confidence\"";
		out << L" val"<< PatternFeature::val_confidence << "=\"" << getConfidence() << L"\"";
		out << L" />\n";
	}
};

#endif
