// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_MENTION_H
#define ICEWS_EVENT_MENTION_H

#include "Generic/theories/Theory.h"
#include "ICEWS/Identifiers.h"
#include "ICEWS/ActorMention.h"
#include "ICEWS/EventType.h"
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Generic/common/BoostUtil.h"
#include <boost/unordered_map.hpp>

class DatabaseConnection;

namespace ICEWS {

/** A SERIF Theory used to identify a single mention of an ICEWS-style 
  * event.  Each ICEWS event mention consists of an event type and a
  * mapping from roles (Symbols) to participants (ActorMentions).  The 
  * event type includes a CAMEO event code, along with information
  * about the set of available roles.  Standard ICEWS event types define
  * two roles: a "source" actor and a "target" actor.
  *
  * Each ICEWSEventMention is also tagged with the name of the pattern
  * that identified the event mention.
  *
  * ICEWSEventMention objects are meant to be accessed via boost shared
  * pointers.  They are not copyable.
  *
  * New ICEWSEventMention objects should be created using 
  * boost::make_shared().  Raw pointers to ActorMentions (such as those 
  * returned by SerifXML::XMLTheoryElement::loadTheoryPointer) can be 
  * converted into shared pointers using ActorMention::shared_from_this().
  */
class ICEWSEventMention : public Theory, public boost::enable_shared_from_this<ICEWSEventMention>, private boost::noncopyable {
public:	
	/** Mapping type used to map from role labels to participants. */
	typedef boost::unordered_map<Symbol, ActorMention_ptr, Symbol::Hash, Symbol::Eq> ParticipantMap;

	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ICEWSEventMention, ICEWS::ICEWSEventType_ptr, const ParticipantMap&, Symbol, Symbol);
	~ICEWSEventMention() {}

	/** Return an ActorMention identifying the actor with the specified
	  * role in this event mention.  If no actor is associated with the
	  * given role, then return NULL.  In standard ICEWS events, two roles
	  * are defined: Source and Target. */
	ActorMention_ptr getParticipant(Symbol role) const;

	/** Return an EventType object specifying what kind of event is 
	  * identified by this event mention. */
	ICEWSEventType_ptr getEventType() const;

	/** Return a symbol that identifies the SERIF pattern that found
	  * this event mention.  This is mostly intended for logging and
	  * pattern evaluation. */
	Symbol getPatternId() const;

	/** Set the event tense as directed */
	void setEventTense(Symbol tense);

	Symbol getEventTense() const;

	/** Test whether tense is valid */
	static Symbol NEUTRAL_TENSE;
	static Symbol CURRENT_TENSE;
	static Symbol HISTORICAL_TENSE;
	static Symbol ONGOING_TENSE;
	static bool isValidTense(Symbol sym);

	const ParticipantMap& getParticipantMap() const { return _participants; }

	/** Determine the ICEWS sentence number for the sentence containing
	  * this event mention.  Note that we use our own sentence
	  * segmentation, so this is *not* the same as the Serif sentence
	  * number.  It is looked up using SentenceSpan fields in the 
	  * document's metadata.  (This method will raise an exception if 
	  * no SentenceSpan data is included in the metadata.) */
	size_t getIcewsSentNo(const DocTheory* docTheory) const;

	/** Return a text string extracted from the document that describes 
	  * this event. */
	std::wstring getEventText() const;

	/** Decide whether this event should be stored in the database */
	bool isDatabaseWorthyEvent() const;

	/** Modify the event type of an event. */
	void setEventType(ICEWSEventType_ptr newEventType);

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	explicit ICEWSEventMention(SerifXML::XMLTheoryElement elem);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"icewsevent"; }

	void dump(std::wostream &stream, const char* indent="");
	
private:
	// Constructor
	ICEWSEventMention(ICEWSEventType_ptr eventType, const ParticipantMap& participants, Symbol patternId, Symbol tense);
	// The participants in this event.
	ParticipantMap _participants;
	// The even type.
	ICEWSEventType_ptr _eventType;
	// The pattern was used to create this event mention
	Symbol _patternId;
	// The "historical" status of this event
	Symbol _event_tense;

	/** We are still considering whether to add this:
	// A set of propositions that are associated with this event.  These
	// are extracted from the pattern that we use to find the event, and
	// can be used to determine what text is associated with the event.
	// They might also be used by EventMentionPatterns, if they prove
	// useful.  (The pair contains a sentence number and a prop pointer.)
	std::set<std::pair<int, const Proposition*> > _associatedPropositions;
	*/
};
typedef boost::shared_ptr<ICEWSEventMention> ICEWSEventMention_ptr;

} // End namespace ICEWS

std::wostream & operator << ( std::wostream &stream, ICEWS::ICEWSEventMention_ptr eventMention );
std::ostream & operator << ( std::ostream &stream, ICEWS::ICEWSEventMention_ptr eventMention );

#endif
