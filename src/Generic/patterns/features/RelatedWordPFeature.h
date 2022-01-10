// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef RELATED_WORD_PFEATURE_H
#define RELATED_WORD_PFEATURE_H

#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "common/Symbol.h"

/** A pseudo-feature used to record the presence of a word that is related
  * to some query. */
class RelatedWordPFeature : public PseudoPatternFeature {
private:
	RelatedWordPFeature(int start_token, int end_token, int sent_no, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(RelatedWordPFeature, int, int, int, const LanguageVariant_ptr&);
	
	RelatedWordPFeature(int start_token, int end_token, int sent_no);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RelatedWordPFeature, int, int, int);

public:
	virtual int getSentenceNumber() const { return _sent_no; }
	virtual int getStartToken() const { return _start_token; }
	virtual int getEndToken() const { return _end_token; }	
	virtual bool equals(PatternFeature_ptr other) { 
		boost::shared_ptr<RelatedWordPFeature> f = boost::dynamic_pointer_cast<RelatedWordPFeature>(other);
		return PatternFeature::simpleEquals(other) && f;
	}
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void setCoverage(const DocTheory * docTheory) {/* do nothing */}
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) {/* do nothing */}
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	RelatedWordPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	int _sent_no;
	int _start_token, _end_token;
};

#endif
