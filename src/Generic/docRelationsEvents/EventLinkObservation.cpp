// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/theories/EntitySet.h"

const Symbol EventLinkObservation::_className(L"event-link");

DTObservation *EventLinkObservation::makeCopy() {
	EventLinkObservation *copy = _new EventLinkObservation();

	copy->setEventMentions(_vMention1, _vMention2);
	return copy;
}

void EventLinkObservation::setEventMentions(EventMention *vMention1, EventMention *vMention2) {
	_vMention1 = vMention1;
	_vMention2 = vMention2;

}

void EventLinkObservation::setEntitySet(const EntitySet *entitySet) {
	_entitySet = entitySet;
}

Entity *EventLinkObservation::getNthArgEntity(EventMention *vMent, int n) {
	return _entitySet->getEntityByMention(vMent->getNthArgMention(n)->getUID());
}
