// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef QUOTE_PFEATURE_H
#define QUOTE_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>

class Mention;

/** A feature used to store information about a successful QuotationPattern 
  * match.  In addition to the basic PatternFeature information, each 
  * QuotationPattern defines:
  *
  *  - focusType: identifies what kind of information is contained in the 
  *    feature.  One of: quotation_speaker, relevant_quotation_semgement,
  *    irrelevant_quotation_segment.
  *
  *  - speakerMention: For QuotePFeatures whose focus type is quotation_speaker,
  *    this contains the speaker of the quotation.
  */
class QuotePFeature: public PatternFeature {
private:
	QuotePFeature(Pattern_ptr pattern, int sent_no, int start_token, int end_token, bool relevant, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(QuotePFeature, Pattern_ptr, int, int, int, bool, const LanguageVariant_ptr&);

	QuotePFeature(Pattern_ptr pattern, const Mention* mention, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(QuotePFeature, Pattern_ptr, const Mention*, const LanguageVariant_ptr&);
	
	QuotePFeature(Pattern_ptr pattern, int sent_no, int start_token, int end_token, bool relevant);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(QuotePFeature, Pattern_ptr, int, int, int, bool);

	QuotePFeature(Pattern_ptr pattern, const Mention* mention);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(QuotePFeature, Pattern_ptr, const Mention*);
public:
	/** If this QuotePFeature is a speaker feature (focusType=quotation_speaker), then
	  * return a mention identifying the speaker.  Otherwise, return NULL. */
	const Mention* getSpeakerMention() { return _speakerMention; }

	/** Return a symbol indicating what kind of information is contained in this feature:
	  *   - quotation_speaker: This feature identifies the speaker of a quotation.
	  *   - relevant_quotation_segment: This feature identifies a relevant quotation segment.
	  *   - irrelevant_quotation_segment: This feature identifies an irrelevant quotation segment.
	  */
	Symbol getFocusType() { return _focusType; }

	// Overridden virtual methods.
	int getSentenceNumber() const { return _sent_no; }
	int getStartToken() const { return _start_token; }
	int getEndToken() const { return _end_token; }
	bool equals(PatternFeature_ptr other);
	virtual void setCoverage(const DocTheory * docTheory) { /* nothing to do */ }
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) { /* nothing to do */ }
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	QuotePFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	Symbol _focusType;
	const Mention *_speakerMention;
	int _sent_no;
	int _start_token, _end_token;
};

typedef boost::shared_ptr<QuotePFeature> QuotePFeature_ptr;

#endif
