// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef TOKEN_SPAN_PFEATURE_H
#define TOKEN_SPAN_PFEATURE_H

#include <boost/shared_ptr.hpp>
#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/common/BoostUtil.h"

/** A psuedo-feature used to identify a set of tokens in a sentence.
  */
class TokenSpanPFeature : public PseudoPatternFeature {
private:
	TokenSpanPFeature(int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant)
		: PseudoPatternFeature(languageVariant), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(TokenSpanPFeature, int, int, int, const LanguageVariant_ptr&);
	
		TokenSpanPFeature(int sent_no, int start_token, int end_token)
		: PseudoPatternFeature(), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TokenSpanPFeature, int, int, int);
public:	
	virtual int getSentenceNumber() const { return _sent_no; }
	virtual int getStartToken() const { return _start_token; }
	virtual int getEndToken() const { return _end_token; }
	virtual void setCoverage(const DocTheory * docTheory) { /* nothing to do */ }
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) { /* do nothing*/ }
	virtual bool equals(PatternFeature_ptr other) { 
		boost::shared_ptr<TokenSpanPFeature> f = boost::dynamic_pointer_cast<TokenSpanPFeature>(other);
		return PatternFeature::simpleEquals(other) && f;
	}
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	TokenSpanPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	int _sent_no;
	int _start_token;
	int _end_token;
};
typedef boost::shared_ptr<TokenSpanPFeature> TokenSpanPFeature_ptr;

#endif

