// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENERIC_PFEATURE_H
#define GENERIC_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"

class GenericPFeature: public PatternFeature {
private:
	GenericPFeature(Pattern_ptr pattern, int sent_no, const LanguageVariant_ptr& languageVariant): 
	   PatternFeature(pattern, languageVariant, 1), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(GenericPFeature, Pattern_ptr, int, const LanguageVariant_ptr&);
	
	GenericPFeature(Pattern_ptr pattern, int sent_no): 
	   PatternFeature(pattern, 1), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(GenericPFeature, Pattern_ptr, int);
public:
	virtual int getSentenceNumber() const;
	virtual int getStartToken() const;
	virtual int getEndToken() const;
	virtual bool equals(PatternFeature_ptr other);
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const; /* do nothing */
	virtual void setCoverage(const DocTheory * docTheory); /* do nothing */
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher); /* do nothing */
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	GenericPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	int _sent_no;
};

#endif
