#ifndef EN_EVENT_UTILITIES_H
#define EN_EVENT_UTILITIES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/EventUtilities.h"
#include "Generic/common/SymbolHash.h"
class Mention;
class SurfaceLevelSentence;
class EventMention;
class Proposition;
class PropositionSet;

class EnglishEventUtilities {
	// Note: this class is intentionally not a subclass of EventUtilities.
	// See EventUtilities.h for an explanation.
public:
	static Symbol getStemmedNounPredicate(const Proposition *prop);
	static Symbol getStemmedVerbPredicate(const Proposition *prop);

	static int compareEventMentions(EventMention *mention1, EventMention *mention2);
	static bool isInvalidEvent(EventMention *mention);

	static void postProcessEventMention(EventMention *mention,
										MentionSet *mentionSet);

	static void fixEventType(EventMention *mention, Symbol correctType);

	static void populateForcedEvent(EventMention *mention, const Proposition *prop, 
									Symbol correctType, const MentionSet *mentionSet);
	static void addNearbyEntities(EventMention *mention,
								  SurfaceLevelSentence *sentence, EntitySet *entitySet);

	static void identifyNonAssertedProps(const PropositionSet *propSet, 
		const MentionSet *mentionSet, bool *isNonAsserted);	
	static SymbolHash * _nonAssertedIndicators;

	static bool includeInConnectingString(Symbol tag, Symbol next_tag);
	static bool includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag);

	static void runLastDitchDateFinder(EventMention *evMention,
											const TokenSequence *tokens, 
											ValueMentionSet *valueMentionSet,											
											PropositionSet *props);

	static Symbol getReduced2005EventType(Symbol sym);

	
	static bool isMoneyWord(Symbol sym);


private:
	static void runLastDitchDateFinder(EventMention *mention,
									   MentionSet *mentionSet);

	static bool isOrgOrPerMention(const Mention *ment);
	static bool isPlaceMention(const Mention *ment);
	static bool isInanimateMention(const Mention *ment);
	static bool isAgentMention(const Mention *ment);
	static bool isPerMention(const Mention *ment);
	static bool isHelperVerb(SurfaceLevelSentence *sentence, int index);

	static bool addDate(EventMention *mention, SurfaceLevelSentence *sentence, int index, EntitySet *entitySet);
	static bool addPrep(EventMention *mention, SurfaceLevelSentence *sentence, int index, EntitySet *entitySet);
	static bool addMoney(EventMention *mention, SurfaceLevelSentence *sentence, int index, EntitySet *entitySet);
	static bool addWeapon(EventMention *mention, SurfaceLevelSentence *sentence, int index, EntitySet *entitySet);
	static bool addAgent(EventMention *mention,	SurfaceLevelSentence *sentence,
							int index, Symbol role, EntitySet *entitySet);

	static Symbol getDefaultType(Symbol eventType, Symbol role, 
									  const Mention *mention);

	static bool isOnSlotList(Symbol slotName, Symbol *list);

};



#endif

