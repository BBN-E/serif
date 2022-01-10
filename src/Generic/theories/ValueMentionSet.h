// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_MENTION_SET_H
#define VALUE_MENTION_SET_H

#include "Generic/common/limits.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "boost/unordered_map.hpp"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class DocTheory;


class SERIF_EXPORTED ValueMentionSet : public SentenceSubtheory {
public:

	typedef boost::unordered_map<int, ValueMentionUID> ValueMentionMap;

	ValueMentionSet(const TokenSequence *tokenSequence, int n_values);
	ValueMentionSet(const ValueMentionSet &other, const TokenSequence *tokenSequence = NULL);
	ValueMentionSet(std::vector<ValueMentionSet*> splitValueMentionSets, std::vector<int> sentenceOffsets, std::vector<ValueMentionMap> &valueMentionMaps, int total_sentences);

	~ValueMentionSet();

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }
	void setTokenSequence(const TokenSequence* tokenSequence); // for backwards compatible state files

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::VALUE_SUBTHEORY; }	

	int getNValueMentions() const { return _n_values; }

	/** Add new ValueMention to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * ValueMentionSet's responsibility to delete the ValueMention */
	void takeValueMention(int i, ValueMention *vmention);

	/// ID may be the UID (doc-level ID) or the index (sentence-level ID)
	ValueMention *getValueMention(ValueMentionUID id) const;
	ValueMention *getValueMention(int index) const;

	// WARNING: Scores do not get saved to state-files, so this will be zero if you load from a state-file.
	float getScore() const { return _score; } 
	void setScore(float score) { _score = score; }

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const ValueMentionSet &it) {
		it.dump(out, 0); return out; 
	}


	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	ValueMentionSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit ValueMentionSet(SerifXML::XMLTheoryElement elem, DocTheory *docTheory=0);
	const wchar_t* XMLIdentifierPrefix() const;


private:
	int _n_values;
	ValueMention **_values;
	const TokenSequence *_tokenSequence;

	float _score;
};

#endif
