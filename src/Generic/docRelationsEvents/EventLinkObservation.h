// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_LINK_OBSERVATION_H
#define EVENT_LINK_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/EventMention.h"
class Entity;


class EventLinkObservation : public DTObservation {
public:
	EventLinkObservation() : DTObservation(_className) {}

	~EventLinkObservation() {}

	virtual DTObservation *makeCopy();
	void setEventMentions(EventMention *vMention1, EventMention *vMention2);
	void setEntitySet(const EntitySet *entitySet);

	Entity *getNthArgEntity(EventMention *vMent, int n);

	EventMention * getVMention1() { return _vMention1; }
	EventMention * getVMention2() { return _vMention2; }

private:
	static const Symbol _className;

	// features of the event
	EventMention *_vMention1;
	EventMention *_vMention2;
	
	const EntitySet *_entitySet;
};

#endif
