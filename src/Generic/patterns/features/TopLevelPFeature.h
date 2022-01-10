// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOP_LEVEL_PFEATURE_H
#define TOP_LEVEL_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/Symbol.h"

/** A pseudo-feature used to record the names of the top-level patterns
  * that matched to create a feature set.  (More documentation for what
  * this gets used for would be welcome!) */
class TopLevelPFeature: public PatternFeature {
private:
	TopLevelPFeature(Pattern_ptr pattern, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(TopLevelPFeature, Pattern_ptr, const LanguageVariant_ptr&);
	TopLevelPFeature(Symbol pattern_label, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(TopLevelPFeature, Symbol, const LanguageVariant_ptr&);
	TopLevelPFeature(Pattern_ptr pattern, Symbol pattern_label, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TopLevelPFeature, Pattern_ptr, Symbol, const LanguageVariant_ptr&);
	
	TopLevelPFeature(Pattern_ptr pattern);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(TopLevelPFeature, Pattern_ptr);
	TopLevelPFeature(Symbol pattern_label);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(TopLevelPFeature, Symbol);
	TopLevelPFeature(Pattern_ptr pattern, Symbol pattern_label);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(TopLevelPFeature, Pattern_ptr, Symbol);
public:
	/** Return the name of the top-level pattern that matched to create 
	  * the feature set containing this TopLevelPFeature. */
	Symbol getPatternLabel(){ return _pattern_label; }

	// Overridden virtual methods.
	virtual int getSentenceNumber() const { return -1; }
	virtual int getStartToken() const { return -1; }
	virtual int getEndToken() const { return -1; }
	bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<TopLevelPFeature> f = boost::dynamic_pointer_cast<TopLevelPFeature>(other);
		return f && f->getPatternLabel() == getPatternLabel();
	}
	virtual void setCoverage(const DocTheory * docTheory) { /* nothing to do */ }
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) { /* nothing to do */ }
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	TopLevelPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	Symbol _pattern_label;
};
typedef boost::shared_ptr<TopLevelPFeature> TopLevelPFeature_ptr;

#endif
