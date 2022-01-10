// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELMENTION_SET_H
#define RELMENTION_SET_H

#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceItemIdentifier.h"

#include <iostream>

class RelMention;

class StateSaver;
class StateLoader;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED RelMentionSet : public SentenceSubtheory {
public:
	RelMentionSet() : _score(0) {}
	RelMentionSet(const RelMentionSet &other, int sent_offset, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet);
	RelMentionSet(std::vector<RelMentionSet*> splitRelMentionSets, std::vector<int> sentenceOffsets, std::vector<MentionSet*> mergedMentionSets);
	~RelMentionSet();

	/** Add new relation to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * RelMentionSet's responsibility to delete the RelMention */
	void takeRelMention(RelMention *relation);
	void takeRelMentions(RelMentionSet *relmentionset);

	// Clear _rmentions (but do not delete what it points to)
	void clear() { _rmentions.clear(); }

	int getNRelMentions() const { return static_cast<int>(_rmentions.size()); }
	RelMention *getRelMention(int i) const;
	RelMention *findRelMentionByUID(RelMentionUID uid) const;

	float getScore() const { return _score; }


	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{	return SentenceTheory::RELATION_SUBTHEORY; }


	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const RelMentionSet &it)
		{ it.dump(out, 0); return out; }


	// implemented for use with relation training only
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	RelMentionSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit RelMentionSet(SerifXML::XMLTheoryElement elem, int sentno);
	const wchar_t* XMLIdentifierPrefix() const;

private:
	std::vector<RelMention *> _rmentions;

	float _score;
};

#endif
