// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/discmodel/CorefUtils.h"

const Symbol DTCorefObservation::_copulaSym(L":COPULA");
const Symbol DTCorefObservation::_apposSym(L":APPOSITIVE");
const Symbol DTCorefObservation::NOMINAL_LVL = Symbol(L"NOM");
const Symbol DTCorefObservation::NAME_LVL = Symbol(L"NM_LVL");

DTObservation *DTCorefObservation::makeCopy() {
	DTCorefObservation *copy = _new DTCorefObservation();

	copyTo(copy);
	return copy;
}


void DTCorefObservation::populate(MentionUID mentionId, int entityId, int hobbs_distance, 
								  Symbol preLink, Symbol commonEntityLink) 
{
	DTNoneCorefObservation::populate(mentionId);
	_ent_id = entityId;
	_entity = _entitySet->getEntity(entityId);
	_hobbs_distance = hobbs_distance;
	_preLink = preLink;
	_commonEntityLink = commonEntityLink;

	setEntityMentionLevel();
	setLastEntityMention();
}

void DTCorefObservation::setEntityMentionLevel() {
	_entityMentionLevel = NOMINAL_LVL;
	for (int m = 0; m < _entity->getNMentions(); m++) {
		Mention* entMent = _entitySet->getMention(_entity->getMention(m)); 
		Mention::Type mentMentionType = entMent->getMentionType();
		if (mentMentionType == Mention::NAME)
			_entityMentionLevel = NAME_LVL;
	}
}

void DTCorefObservation::setLastEntityMention() {
	MentionUID lastID = CorefUtils::getLatestMention(_entitySet, _entity, _ment);

	if (lastID.isValid())
		_lastEntityMention = _entitySet->getMention(lastID);
	else
		_lastEntityMention = 0;
}
										   
										   
void DTCorefObservation::copyTo(DTCorefObservation *other){
	DTNoneCorefObservation::copyTo(other);
	other->_ent_id = _ent_id;
	other->_entity = _entity;
	other->_hobbs_distance = _hobbs_distance;
	other->_preLink = _preLink;
	other->_commonEntityLink = _commonEntityLink;

	other->setEntityMentionLevel();
	other->setLastEntityMention();
}
