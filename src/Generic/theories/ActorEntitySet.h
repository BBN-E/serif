// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_ENTITY_SET_H
#define ACTOR_ENTITY_SET_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ActorEntity.h"

#include <boost/noncopyable.hpp>

/** Contains ActorEntity objects which map Entities to Actors
  * in the BBN Actor Database. Each Entity may map to several
  * ActorEntitys. */
class ActorEntitySet : public Theory, private boost::noncopyable {
public:	
	ActorEntitySet();
	~ActorEntitySet();

	/** Append the ActorEntity to the list of matching actors for 
	  * an Entity. */
	void addActorEntity(ActorEntity_ptr actorEntity);

	/** Return the list of ActorEntities for a given Entity */
	std::vector<ActorEntity_ptr> find(const Entity *entity);

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	ActorEntitySet(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"actorentityset"; }

	/** HashMap type used to store the ActorEntities */
	typedef std::map<const Entity *, std::vector<ActorEntity_ptr> > ActorEntityMap;

	/** Return a vector containing all ActorEntities in this set */
	std::vector<ActorEntity_ptr> getAll() const;


private:
	ActorEntityMap _actorEntities;
};

#endif
