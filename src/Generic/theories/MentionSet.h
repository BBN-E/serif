// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_SET_H
#define MENTION_SET_H

#include "Generic/common/limits.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Mention.h"

class SynNode;
class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED MentionSet : public SentenceSubtheory {
private:
	MentionSet()
		: _parse(0), _sent_no(0), _n_mentions(0), _mentions(0),
		  _name_score(0), _desc_score(0) {}
public:

	MentionSet(const Parse *parse, int sentence_number=0);
	MentionSet(const MentionSet &other, int sent_offset = 0, const Parse *parse = NULL);

	~MentionSet();

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::MENTION_SUBTHEORY; }

	int getNMentions() const { return _n_mentions; }
	/// ID may be the UID (doc-level ID) or the index (sentence-level ID)
	Mention *getMention(MentionUID ID) const;
	Mention *getMention(int index) const;
	Mention *getMentionByNode(const SynNode *node) const;

	void changeEntityType(MentionUID ID, EntityType type);
	void changeMentionType(MentionUID ID, Mention::Type type);

	const Parse *getParse() const {return _parse; }
	int getSentenceNumber() const { return _sent_no; }
	void setNameScore(float score) { _name_score = score; }
	float getNameScore() const { return _name_score; }
	void setDescScore(float score) { _desc_score = score; }
	float getDescScore() const { return _desc_score; }

	int countPopulatedMentions() const;
	int countEDTMentions() const;

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const MentionSet &it)
		{ it.dump(out, 0); return out; }


	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	MentionSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit MentionSet(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;


private:
	const Parse *_parse;
	int _sent_no;

	int _n_mentions;
	Mention **_mentions;

	float _name_score;
	float _desc_score;


	static int countReferenceCandidates(const SynNode *node);
	int initializeMentionArray(Mention **mentions,
							   const SynNode *node,
							   int index);
	void setParseNodeMentionIndices();
};

#endif
