// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef REL_MENTION_PFEATURE_H
#define REL_MENTION_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"

class RelMention;

/** A feature used to store information about a successful RelPattern match.
  * In addition to the basic PatternFeature information, each MentionPFeature 
  * keeps a reference to the rel mention that was matched.
  */
class RelMentionPFeature : public PatternFeature {
private:
	RelMentionPFeature(Pattern_ptr pattern, const RelMention *mention, int sent_no, 
		const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(RelMentionPFeature, Pattern_ptr, const RelMention*, int, const LanguageVariant_ptr&);

	RelMentionPFeature(const RelMention *mention, int sent_no, int start_token, int end_token, 
		 const LanguageVariant_ptr& languageVariant, float confidence);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(RelMentionPFeature, const RelMention*, int, int, int, const LanguageVariant_ptr&, float);
	
	RelMentionPFeature(Pattern_ptr pattern, const RelMention *mention, int sent_no);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RelMentionPFeature, Pattern_ptr, const RelMention*, int);

	RelMentionPFeature(const RelMention *mention, int sent_no, int start_token, int end_token, float confidence);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(RelMentionPFeature, const RelMention*, int, int, int, float);
public:	

	/** Return the RelMention that was matched to create this feature. */
	const RelMention *getRelMention() const { return _relMention; }

	// Overridden virtual methods.
	int getSentenceNumber() const { return _sent_no; }
	int getStartToken() const { return _start_token; }
	int getEndToken() const { return _end_token; }
	bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<RelMentionPFeature> f = boost::dynamic_pointer_cast<RelMentionPFeature>(other);
		return f && f->getRelMention() == getRelMention();
	}
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void setCoverage(const DocTheory * docTheory);	
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher);
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	RelMentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	const RelMention *_relMention;
	int _sent_no;
	int _start_token, _end_token;
};

typedef boost::shared_ptr<RelMentionPFeature> RelMentionPFeature_ptr;

#endif
