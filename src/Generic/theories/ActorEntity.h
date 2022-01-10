// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_ENTITY_H
#define ACTOR_ENTITY_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/common/BoostUtil.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>


/** A SERIF Theory used to connect a Serif Entity to a BBN Actor database 
  * actor. An unmatched actor" ActorEntity is used to signify an Entity 
  * that doesn't match the actor database.
  *
  * One Entity might have several ActorEntitys associated with it. 
  * Confidences for ActorEntitys associated with a particular Entity
  * must add to 1.
  *
  * New ActorEntity objects should be created using boost::make_shared().
  * Raw pointers to ActorEntitys (such as those returned by 
  * SerifXML::XMLTheoryElement::loadTheoryPointer) can be converted into
  * shared pointers using ActorEntity::shared_from_this().
  */
class ActorEntity : public Theory, public boost::enable_shared_from_this<ActorEntity>, private boost::noncopyable {
public:	
	struct ActorIdentifiers {
		ActorId id;
		Symbol code;
		ActorPatternId patternId;
		std::wstring actorName;
		explicit ActorIdentifiers(ActorId id=ActorId(), std::wstring actorName=L"UNKNOWN-ACTOR", Symbol code=Symbol(), ActorPatternId patternId=ActorPatternId()): id(id), code(code), patternId(patternId), actorName(actorName) {}
	};

	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ActorEntity, const Entity*, Symbol, const ActorEntity::ActorIdentifiers&);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ActorEntity, SerifXML::XMLTheoryElement);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ActorEntity, const Entity*, Symbol);
	~ActorEntity() {}

	/** Return the entity mention that corresponds with this actor. */
	const Entity* getEntity() const;
	
	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"actorentity"; }

	void setConfidence(double confidence) { _confidence = confidence; }
	double getConfidence() { return _confidence; }

	bool isUnmatchedActor() const;

	/** Return a foreign key into the actors table, identifying the 
	  * named actor who is identified by this ActorEntity. */
	ActorId getActorId() const { return _actor.id; }

	void addActorMention(ProperNounActorMention_ptr pnam);
	std::vector<ProperNounActorMention_ptr> &getActorMentions();

	double getImportanceScore();

private:

	ActorEntity(const Entity *entity, Symbol sourceNote, const ActorEntity::ActorIdentifiers&);
	/** Creates unmatched actor ActorEntity */
	ActorEntity(const Entity *entity, Symbol sourceNote);
	explicit ActorEntity(SerifXML::XMLTheoryElement elem);

	/** The Entity that this ActorEntity refers to */
	const Entity *_entity;
	double _confidence;
	ActorIdentifiers _actor;
	Symbol _sourceNote;

	/** ActorMentions that made this ActorEntity */
	std::vector<ProperNounActorMention_ptr> _actorMentions;
};

typedef boost::shared_ptr<ActorEntity> ActorEntity_ptr;

#endif
