// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PSEUDO_PATTERN_FEATURE_H
#define PSEUDO_PATTERN_FEATURE_H

#include "Generic/patterns/features/PatternFeature.h"

/** An abstract base class for "pseudo-features", which are not actually 
  * generated by any pattern, but instead are added directly to a feature 
  * set (e.g., by AnswerFinder::addExtraFeatures()).  Unlike real features, 
  * thse pseudo-features have no pattern that generated them -- i.e., 
  * getPattern() will always return a null pointer.
  *
  * In addition, most pseudo-features do not have any useful location 
  * information -- i.e., getSentenceNumber(), getStartToken(), and 
  * getEndToken() all return -1.  However, there are a few exceptions
  * (such as ExactMatchPFeature, which has a sentence number).
  **/
class PseudoPatternFeature : public PatternFeature {
public:	
	virtual int getSentenceNumber() const { return -1; }
	virtual int getStartToken() const { return -1; }
	virtual int getEndToken() const { return -1; }
	virtual void setCoverage(const DocTheory * docTheory) {/* nothing to do*/}
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) {/* nothing to do*/}
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const { PatternFeature::saveXML(elem, idMap); }
	PseudoPatternFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) : PatternFeature(elem, idMap) { }
protected:
	PseudoPatternFeature(const LanguageVariant_ptr& languageVariant,float confidence=1): 
		 PatternFeature(Pattern_ptr(),languageVariant,confidence) {}
	PseudoPatternFeature(float confidence=1): 
		 PatternFeature(Pattern_ptr(),confidence) {}
};

#endif
