#ifndef XX_EVENT_UTILITIES_H
#define XX_EVENT_UTILITIES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/EventUtilities.h"
#include "Generic/theories/Proposition.h"

class GenericEventUtilities {
	// Note: this class is intentionally not a subclass of EventUtilities.
	// See EventUtilities.h for an explanation.
public:

	static Symbol getStemmedNounPredicate(const Proposition *prop) { 
		return prop->getPredSymbol();
	}
	static Symbol getStemmedVerbPredicate(const Proposition *prop) { 
		return prop->getPredSymbol();
	}

	static int compareEventMentions(EventMention *mention1, EventMention *mention2) {
		return 0;
	}

	static void postProcessEventMention(EventMention *mention,
										MentionSet *mentionSet)
	{}

	static bool isInvalidEvent(EventMention *mention) { return false; }

	static void fixEventType(EventMention *mention, Symbol correctType) {}

	static void populateForcedEvent(EventMention *mention, const Proposition *prop, 
		Symbol correctType, const MentionSet *mentionSet) {}

	static void addNearbyEntities(EventMention *mention,
									   SurfaceLevelSentence *sentence, EntitySet *entitySet) {}

	static void identifyNonAssertedProps(const PropositionSet *propSet, 
										   const MentionSet *mentionSet, 
										   bool *isNonAsserted) {}

	static bool includeInConnectingString(Symbol tag, Symbol next_tag) { return true; }
	static bool includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag) { return true; }

	static void runLastDitchDateFinder(EventMention *evMention,
											const TokenSequence *tokens, 
											ValueMentionSet *valueMentionSet,
											PropositionSet *props) {}

	static Symbol getReduced2005EventType(Symbol sym) { return sym; }
	static bool isMoneyWord(Symbol sym) { return false; }
};


#endif

