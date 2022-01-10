// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPOSITION_SET_H
#define PROPOSITION_SET_H

#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/common/limits.h"
#include "Generic/common/UTF8OutputStream.h"

class Proposition;
class SynNode;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED PropositionSet : public SentenceSubtheory {
private:
	PropositionSet() : _defArrayUpdated(false), _mentionSet(0) {}
public:

	PropositionSet(const MentionSet* mentionSet);
	PropositionSet(const PropositionSet &other, int sent_offset, const MentionSet* mentionSet);
	~PropositionSet();

	/** Add new proposition to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * PropositionSet's responsibility to delete the Proposition */
	void takeProposition(Proposition *prop);

	int getNPropositions() const { return static_cast<int>(_props.size()); }
	Proposition *getProposition(int i) const;
	Proposition *findPropositionByUID(int uid) const;
	Proposition *findPropositionByNode(const SynNode *node) const;
	const MentionSet *getMentionSet() const {return _mentionSet; }
	void setMentionSet(const MentionSet* mentionSet); // for backwards compatible state files
	
	/** Replace the MentionSet pointed to by this PropositionSet with 
	  * a new MentionSet of the same size.  The Mentions in the new 
	  * MentionSet should correspond one-to-one with the Mentions in
	  * the original MentionSet.  This is used by 
	  * SentenceTheory::adoptSubtheory to update the mention set
	  * pointer when the mention set for a sentence is changed. */
	void replaceMentionSet(const MentionSet* mentionSet);

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{	return SentenceTheory::PROPOSITION_SUBTHEORY; }

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const PropositionSet &it)
		{ it.dump(out, 0); return out; }

	void dump(UTF8OutputStream &out, int indent = 0) const;
	friend UTF8OutputStream &operator <<(UTF8OutputStream &out,
									 const PropositionSet &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	PropositionSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit PropositionSet(SerifXML::XMLTheoryElement elem, int sentence_number);
	const wchar_t* XMLIdentifierPrefix() const;

	/** Return the Proposition that defines the Mention with the given 
	  * index (returned by Mention::getIndex()). */
	Proposition *getDefinition(int index) const;

	/** These two methods (fillDefinitionsArray and isDefinitionArrayFilled)
	  * are deprecated; they were formerly used to explicitly update a cache
	  * used by getDefinition(); but the cache is now handled internally and
	  * automatically. */
	void fillDefinitionsArray() const {};                  // DEPRECATED
	bool isDefinitionArrayFilled() const { return true; }  // DEPRECATED
private:
	std::vector<Proposition*> _props;
	
	// The parse that this proposition set is based on.
	const MentionSet* _mentionSet;

	// This is a cache used to record a mapping from Mention index to 
	// Proposition.  It can be regenerated at any time using 
	// updateDefinitionsArray().
	mutable std::map<int, Proposition*> _definitions;
	mutable bool _defArrayUpdated;
	void updateDefinitionsArray() const;
};

#endif
