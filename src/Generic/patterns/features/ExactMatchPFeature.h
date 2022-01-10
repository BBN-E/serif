// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef EXACT_MATCH_PFEATURE_H
#define EXACT_MATCH_PFEATURE_H

#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/common/BoostUtil.h"

/** A pseudo-feature used to record the fact that a specific sentence 
  * contains an exact match for a query.  The start & end token will 
  * be the start and end tokens of the sentence.  */
class ExactMatchPFeature : public PseudoPatternFeature {
private:
	ExactMatchPFeature(int sent_no, const LanguageVariant_ptr& languageVariant): PseudoPatternFeature(languageVariant), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ExactMatchPFeature, int, const LanguageVariant_ptr&);
	
	ExactMatchPFeature(int sent_no): PseudoPatternFeature(), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ExactMatchPFeature, int);
public:	
	virtual int getSentenceNumber() const { return _sent_no; }
	virtual int getStartToken() const { return _start_token; }
	virtual int getEndToken() const { return _end_token; }
	virtual bool equals(PatternFeature_ptr other);
	virtual void setCoverage(const DocTheory * docTheory);
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher);
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	ExactMatchPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	int _sent_no;
	int _start_token;
	int _end_token;
};

#endif
