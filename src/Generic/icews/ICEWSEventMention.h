// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_MENTION_H
#define ICEWS_EVENT_MENTION_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/icews/EventType.h"
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include <boost/unordered_map.hpp>

class DatabaseConnection;
class Proposition;

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
	typedef std::vector< std::pair<Symbol, ActorMention_ptr> > ParticipantList;
    
	BOOST_MAKE_SHARED_8ARG_CONSTRUCTOR(ICEWSEventMention, ICEWSEventType_ptr, const ParticipantList&, Symbol, Symbol, ValueMention*, std::vector<const Proposition*>, Symbol, bool);
	~ICEWSEventMention() {}

	/** Return an ActorMention identifying the actor with the specified
	  * role in this event mention.  If no actor is associated with the
	  * given role, then return NULL.  In standard ICEWS events, two roles
	  * are defined: Source and Target. */
	ActorMention_ptr getParticipant(Symbol role) const;

	/** Return true if this mention has better participants than
	  * the other mention provided. "Better" means, for example,
	  * that this mention has a SOURCE and a TARGET while the other
	  * has only a SOURCE. */
	bool hasBetterParticipantsThan(boost::shared_ptr<ICEWSEventMention> other);
	static Symbol SOURCE_SYM;
	static Symbol TARGET_SYM;

	bool hasSameEntityPlayingMultipleRoles(const DocTheory *docTheory) const;

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
	ValueMention * getTimeValueMention() const;
	std::vector<const Proposition*> getPropositions() const;
	Symbol getOriginalEventId() const { return _originalEventId; }
	bool isReciprocal() const { return _is_reciprocal; }

	/** Test whether tense is valid */
	static Symbol NEUTRAL_TENSE;
	static Symbol CURRENT_TENSE;
	static Symbol HISTORICAL_TENSE;
	static Symbol ONGOING_TENSE;
	static Symbol NULL_TENSE;
	static bool isValidTense(Symbol sym);

	const ParticipantList& getParticipantList() const { return _participants; }

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

	/* Add a LOCATION role to an event */
	void addLocationRole(ProperNounActorMention_ptr loc) { 
		_participants.push_back(std::make_pair(Symbol(L"LOCATION"), loc)); 
	}
	
	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	explicit ICEWSEventMention(SerifXML::XMLTheoryElement elem);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"icewsevent"; }

	void dump(std::wostream &stream, const char* indent="");

	const SentenceTheory *getSentenceTheory();
	
private:
	// Constructor
	ICEWSEventMention(ICEWSEventType_ptr eventType, const ParticipantList& participants, Symbol patternId, Symbol tense, ValueMention * timeValueMention, std::vector<const Proposition *> propositions, Symbol originalEventId, bool is_reciprocal);
	// The participants in this event.
	ParticipantList _participants;
	// The even type.
	ICEWSEventType_ptr _eventType;
	// The pattern was used to create this event mention
	Symbol _patternId;
	// The "historical" status of this event
	Symbol _event_tense;

	// The time of the event
	ValueMention * _timeValueMention;

	// If we can find it, store the proposition which was the main trigger for this event
	std::vector<const Proposition *> _propositions;

	// The ID of the original event from which this was derived (e.g. from which it was binarized)
	Symbol _originalEventId;
	// Is this a reciprocal event? That is, should its source be consider both source and target?
	bool _is_reciprocal;

};
typedef boost::shared_ptr<ICEWSEventMention> ICEWSEventMention_ptr;

std::wostream & operator << ( std::wostream &stream, ICEWSEventMention_ptr eventMention );
std::ostream & operator << ( std::ostream &stream, ICEWSEventMention_ptr eventMention );

#endif
