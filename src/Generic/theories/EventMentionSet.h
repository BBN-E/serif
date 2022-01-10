// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_MENTION_SET_H
#define EVENT_MENTION_SET_H

#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/ValueMentionSet.h"

#include <boost/unordered_map.hpp>
#include <iostream>

class EventMention;

class StateSaver;
class StateLoader;
class ValueMention;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED EventMentionSet : public SentenceSubtheory {
public:
	EventMentionSet(const Parse* parse) : _score(0) {_parse = parse;}
	EventMentionSet(const EventMentionSet &other, int sent_offset, int event_offset, const Parse* parse, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet, const PropositionSet* propSet, const ValueMentionSet* documentValueMentionSet, ValueMentionSet::ValueMentionMap &documentValueMentionMap);
	~EventMentionSet();

	/** Add new event to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * EventMentionSet's responsibility to delete the EventMention */
	void takeEventMention(EventMention *emention);
	void takeEventMentions(EventMentionSet *ementionset);

	// Clear _ementions (but do not delete what it points to)
	void clear() { _ementions.clear(); }

	int getNEventMentions() const { return static_cast<int>(_ementions.size()); }
	EventMention *getEventMention(int i) const;
	EventMention *findEventMentionByUID(EventMentionUID uid) const;

	float getScore() const { return _score; }


	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{	return SentenceTheory::EVENT_SUBTHEORY; }


	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const EventMentionSet &it)
		{ it.dump(out, 0); return out; }


	// implemented for use with event training only
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	EventMentionSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit EventMentionSet(SerifXML::XMLTheoryElement elem, int sent_no);
	void resolvePointers(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
	const Parse* getParse() const { return _parse; }

private:
	std::vector<EventMention *> _ementions;

	float _score;
	const Parse* _parse;
};

#endif
