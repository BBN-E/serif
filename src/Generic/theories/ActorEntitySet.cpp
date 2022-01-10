// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/ActorEntitySet.h"
#include <boost/foreach.hpp>

ActorEntitySet::ActorEntitySet() { }

ActorEntitySet::~ActorEntitySet() { }

void ActorEntitySet::updateObjectIDTable() const {
	throw InternalInconsistencyException("ActorEntitySet::updateObjectIDTable",
		"ActorEntitySet does not currently have state file support");
}
void ActorEntitySet::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("ActorEntitySet::saveState",
		"ActorEntitySet does not currently have state file support");
}
void ActorEntitySet::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("ActorEntitySet::resolvePointers",
		"ActorEntitySet does not currently have state file support");
}


void ActorEntitySet::addActorEntity(ActorEntity_ptr actorEntity) {
	if (_actorEntities.find(actorEntity->getEntity()) == _actorEntities.end()) {
		_actorEntities[actorEntity->getEntity()] = std::vector<ActorEntity_ptr>(1, actorEntity);
		return;
	}

	_actorEntities[actorEntity->getEntity()].push_back(actorEntity);
}

std::vector<ActorEntity_ptr> ActorEntitySet::find(const Entity *entity) {
	ActorEntityMap::const_iterator it = _actorEntities.find(entity);
	if (it == _actorEntities.end())
		return std::vector<ActorEntity_ptr>();

	return (*it).second;
}


std::vector<ActorEntity_ptr> ActorEntitySet::getAll() const {
	std::vector<ActorEntity_ptr> results;

	ActorEntityMap::const_iterator iter;

	for (iter = _actorEntities.begin(); iter != _actorEntities.end(); ++iter) {
		std::vector<ActorEntity_ptr> actorEntities = iter->second;

		BOOST_FOREACH(ActorEntity_ptr ae, actorEntities) 
			results.push_back(ae);
	}
	return results;
}

bool compareActorEntities(const ActorEntity_ptr a, const ActorEntity_ptr b) 
{
	if (a->getEntity()->getID() < b->getEntity()->getID())
		return true;
	if (b->getEntity()->getID() < a->getEntity()->getID())
		return false;

	if (a->getActorId() == ActorId())
		return true;
	if (b->getActorId() == ActorId())
		return false;

	return a->getActorId().getId() < b->getActorId().getId();
}

void ActorEntitySet::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;
	
	std::vector<ActorEntity_ptr> allActorEntities = getAll();

	// Sort ActorEntitys for consistent output
	std::sort(allActorEntities.begin(), allActorEntities.end(), compareActorEntities);

	BOOST_FOREACH(ActorEntity_ptr ae, allActorEntities) {
		elem.saveChildTheory(X_ActorEntity, ae.get(), this);
	}
}

ActorEntitySet::ActorEntitySet(SerifXML::XMLTheoryElement elem, const DocTheory* theory) {
	using namespace SerifXML;

	elem.loadId(this);
	XMLTheoryElementList emElems = elem.getChildElementsByTagName(X_ActorEntity);
	size_t n_actor_mentions = emElems.size();
	for (size_t i = 0; i < n_actor_mentions; ++i) {
		addActorEntity(boost::make_shared<ActorEntity>(emElems[i]));
	}
}

