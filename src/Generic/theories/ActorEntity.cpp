// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/ActorEntity.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/common/ParamReader.h"

#include <boost/foreach.hpp>

void ActorEntity::updateObjectIDTable() const {
	throw InternalInconsistencyException("ActorEntity::updateObjectIDTable",
		"ActorEntity does not currently have state file support");
}
void ActorEntity::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("ActorEntity::saveState",
		"ActorEntity does not currently have state file support");
}
void ActorEntity::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("ActorEntity::resolvePointers",
		"ActorEntity does not currently have state file support");
}

void ActorEntity::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	elem.saveTheoryPointer(X_entity_id, _entity);
	elem.setAttribute(X_confidence, _confidence);

	if (_sourceNote != Symbol())
		elem.setAttribute(X_source_note, _sourceNote);

	if (!isUnmatchedActor()) {
		elem.setAttribute<size_t>(X_actor_uid, _actor.id.getId());
		elem.setAttribute<Symbol>(X_actor_db_name, _actor.id.getDbName());
	}

	std::vector<const Theory*> actorMentionList;
	BOOST_FOREACH(ActorMention_ptr am, _actorMentions) {
		actorMentionList.push_back(am.get());
	}

	if (actorMentionList.size() > 0)
		elem.saveTheoryPointerList(X_actor_mention_ids, actorMentionList);

	elem.setAttribute(X_actor_name, _actor.actorName);
}

ActorEntity::ActorEntity(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;

	SerifXML::XMLIdMap *idMap = elem.getXMLSerializedDocTheory()->getIdMap();

	// register our id -- note this is the raw pointer, not shared_ptr!  Be sure
	// to use shared_from_this to convert to a shared pointer if you look it up!
	elem.loadId(this); 

	_entity = elem.loadTheoryPointer<Entity>(X_entity_id);
	_confidence = elem.getAttribute<double>(X_confidence);

	if (elem.hasAttribute(X_actor_mention_ids)) {
		std::vector<ActorMention*> actorMentionList = 
			elem.loadNonConstTheoryPointerList<ActorMention>(X_actor_mention_ids);
		BOOST_FOREACH(ActorMention* am, actorMentionList) {
			ActorMention_ptr actorMention = am->shared_from_this();
			ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
			assert(pnam);
			_actorMentions.push_back(pnam);
		}
	}

	std::wstring actorName;
	if (elem.hasAttribute(X_actor_name))
		actorName = elem.getAttribute<std::wstring>(X_actor_name);
	else
		actorName = elem.getText<std::wstring>(); // for backwards compatibility

	if (elem.hasAttribute(X_actor_uid))
		_actor = ActorIdentifiers(ActorId(static_cast<long>(elem.getAttribute<size_t>(X_actor_uid)), elem.getAttribute<Symbol>(X_actor_db_name)), actorName, Symbol(), ActorPatternId());
	else
		_actor = ActorIdentifiers();

	if (elem.hasAttribute(X_source_note))
		_sourceNote = elem.getAttribute<Symbol>(X_source_note);
	else
		_sourceNote = Symbol();
}

ActorEntity::ActorEntity(const Entity *entity, Symbol sourceNote) 
	: _confidence(0), _entity(entity), _sourceNote(sourceNote), _actor(ActorIdentifiers())
{ }

ActorEntity::ActorEntity(const Entity *entity, Symbol sourceNote, const ActorIdentifiers &identifiers) 
	: _confidence(0), _entity(entity), _sourceNote(sourceNote), _actor(identifiers)
{ }

const Entity *ActorEntity::getEntity() const {
	return _entity;
}

bool ActorEntity::isUnmatchedActor() const {
	return _actor.id == ActorId();
}

void ActorEntity::addActorMention(ProperNounActorMention_ptr pnam) {
	_actorMentions.push_back(pnam);
}

std::vector<ProperNounActorMention_ptr> &ActorEntity::getActorMentions() {
	return _actorMentions;
}

double ActorEntity::getImportanceScore() {
	if (_actorMentions.size() > 0)
		// all ActorMentions should have same importance_score
		return _actorMentions[0]->getImportanceScore();
	else
		return 0.0;
}
