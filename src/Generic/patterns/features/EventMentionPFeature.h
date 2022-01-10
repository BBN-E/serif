// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef EVENT_MENTION_PFEATURE_H
#define EVENT_MENTION_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"

class EventMention;

/** A feature used to store information about a successful EventPattern match.
  * In addition to the basic PatternFeature information, each MentionPFeature 
  * keeps a reference to the event mention that was matched.
  */
class EventMentionPFeature : public PatternFeature {
private:
	EventMentionPFeature(Pattern_ptr pattern, const EventMention *mention, int sent_no, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(EventMentionPFeature, Pattern_ptr, const EventMention*, int, const LanguageVariant_ptr&);
	
	EventMentionPFeature(const EventMention *mention, int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant, float confidence);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(EventMentionPFeature, const EventMention*, int, int, int, const LanguageVariant_ptr&, float);

public:	

	/** Return the EventMention that was matched to create this feature. */
	const EventMention *getEventMention() const { return _eventMention; }

	// Overridden virtual methods.
	virtual int getSentenceNumber() const { return _sent_no; }
	virtual int getStartToken() const { return _start_token; }
	virtual int getEndToken() const { return _end_token; }
	virtual bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<EventMentionPFeature> f = boost::dynamic_pointer_cast<EventMentionPFeature>(other);
		return f && f->getEventMention() == getEventMention();
	}
	virtual void setCoverage(const DocTheory * docTheory);	
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher);
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	EventMentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	const EventMention *_eventMention;
	int _sent_no;
	int _start_token, _end_token;
};

typedef boost::shared_ptr<EventMentionPFeature> EventMentionPFeature_ptr;

#endif
