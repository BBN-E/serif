#ifndef XX_EVENT_FINDER_H
#define XX_EVENT_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/EventMentionSet.h"

class Parse;
class MentionSet;
class EntitySet;
class PropositionSet;

class GenericEventFinder : public EventFinder {
public:
	GenericEventFinder() {}

	void resetForNewSentence() {}

	EventMentionSet *getEventTheory(const Parse *parse,
			                       const MentionSet *mentionSet,
			                       const EntitySet *entitySet,
			                       const PropositionSet *propSet)
	{
		return _new EventMentionSet();
	}

};


#endif
