// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_COREF_OBSERVATION_H
#define DT_COREF_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/edt/discmodel/DTNoneCorefObservation.h"

class MentionSet;
class Mention;
class EntitySet;
class Entity;

//class DocumentRelMentionSet;

/** A DTCorefObservation represents all the information about a mention and the 
  * entity it is considering linking to.
  */

class DTCorefObservation : public DTNoneCorefObservation {
public:
	DTCorefObservation() : DTNoneCorefObservation() {}
	DTCorefObservation(const DocumentMentionInformationMapper *infoMap) : DTNoneCorefObservation(infoMap) {}

	virtual DTObservation *makeCopy();

	/// To be able to recycle the instance, reset these values for each decision point
	void populate(MentionUID mentionID, int entityID) { 
		populate(mentionID, entityID, -1, _noLinkSym); 
	}
	void populate(MentionUID mentionID, int entityID, int hobbs_distance)  { 
		populate(mentionID, entityID, hobbs_distance, _noLinkSym); 
	}
	void populate(MentionUID mentionID, int entityID, int hobbs_distance, Symbol preLink) {
		populate(mentionID, entityID, hobbs_distance, _noLinkSym, _noLinkSym); 
	}
	void populate(MentionUID mentionID, int entityID, int hobbs_distance, Symbol preLink, Symbol commonEntityLink);

	const MentionSet *getMentionSet() { return _mentionSet; }
	Entity *getEntity() { return _entity; }
	int getHobbsDistance() { return _hobbs_distance; }
	Symbol getPreLinkSymbol() { return _preLink; }
	Symbol getCommonEntityLink() { return _commonEntityLink; }

	Symbol getEntityMentionLevel() { return _entityMentionLevel; }

	Mention *getLastEntityMention() { return _lastEntityMention; }
	
	static const Symbol _copulaSym;
	static const Symbol _apposSym;
	
	static const Symbol NOMINAL_LVL;
	static const Symbol NAME_LVL;

protected:
	int _ent_id;
	Entity *_entity;
	int _hobbs_distance;
	Symbol _preLink;
	Symbol _commonEntityLink;
	Symbol _entityMentionLevel;

	Mention *_lastEntityMention;

	using DTNoneCorefObservation::copyTo;
	void copyTo(DTCorefObservation *other);
	void setEntityMentionLevel();
	void setLastEntityMention();
};

#endif
