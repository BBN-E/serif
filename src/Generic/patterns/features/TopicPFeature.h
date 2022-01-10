// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOPIC_PFEATURE_H
#define TOPIC_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>

class Mention;

/** A feature used to store information about a successful TopicPattern match.
  * In addition to the basic PatternFeature information, each TopicPFeature 
  * contains:
  *
  *  - relevanceScore
  *  - mention
  */
class TopicPFeature: public PatternFeature {
private:
	TopicPFeature(Pattern_ptr pattern, int sent_no, float relevance, Symbol querySlot, const Mention *mention,
		const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(TopicPFeature, Pattern_ptr, int, float, Symbol, const Mention*,
		const LanguageVariant_ptr&);
		
	TopicPFeature(Pattern_ptr pattern, int sent_no, float relevance, Symbol querySlot, const Mention *mention);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(TopicPFeature, Pattern_ptr, int, float, Symbol, const Mention*);
public:

	const Mention* getMention() const { return _mention; }
	bool isExactMatchRelevance() { return _relevance_score > 1; }

	// Overridden virtual methods.
	int getSentenceNumber() const { return _sent_no; }
	int getStartToken() const { return _start_token; }
	int getEndToken() const { return _end_token; }
	bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<TopicPFeature> f = boost::dynamic_pointer_cast<TopicPFeature>(other);
		return PatternFeature::simpleEquals(other) && f && f->getMention() == getMention() && f->getQuerySlot() == getQuerySlot() && f->getRelevanceScore() == getRelevanceScore();
	}
	virtual void setCoverage(const DocTheory * docTheory);
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher);
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	TopicPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
	Symbol getQuerySlot() const { return _querySlot; }
	float getRelevanceScore() const { return _relevance_score; }

private:
	const Mention* _mention;
	int _sent_no;
	float _relevance_score;
	int _start_token;
	int _end_token;
	Symbol _querySlot;
};

#endif
