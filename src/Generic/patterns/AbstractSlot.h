// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ABSTRACT_SLOT_H
#define ABSTRACT_SLOT_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

class PropMatch;
class SentenceTheory;

/** Abstract base class for Slots that defines the interface used by 
  * PatternMatcher and TopicPattern to read slot information.  A 
  * concrete implementation could be either a Slot object itself, or 
  * a proxy object that delgates to a Slot.
  */
class AbstractSlot {
public:
	/* This enumeration defines the match types that are available for slots.
	 * The static const array ALL_MATCH_TYPES can be used to iterate over the
	 * different match types. */
	typedef enum {NODE, EDGE, FULL} MatchType;
	static const MatchType ALL_MATCH_TYPES[3];

	// Abstract virtual methods
	virtual std::wstring getBackoffRegexText() const = 0;
	virtual const SentenceTheory* getSlotSentenceTheory() const = 0;
	virtual boost::shared_ptr<PropMatch> getMatcher(MatchType matchType) const = 0;
	virtual bool requiresProptrees() const = 0;
	virtual ~AbstractSlot() {}
};

#endif
