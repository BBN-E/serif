// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_THEORY_H
#define NAME_THEORY_H

#include "Generic/theories/NameSpan.h"

#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"


class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;


class NameTheory : public SentenceSubtheory {
protected:
	std::vector<NameSpan*> nameSpans;
	float score;
	const TokenSequence *_tokenSequence;

public:
	// Create a NameTheory with room for n_name_spans elements.  If nameSpans
	// is not NULL, then take ownership of the name spans.
	NameTheory(const TokenSequence *tokSequence, int n_name_spans=0, NameSpan** nameSpans=0);
	NameTheory(const TokenSequence *tokSequence, std::vector<NameSpan*> nameSpans);
	NameTheory(const NameTheory &other, const TokenSequence *tokSequence);

	~NameTheory();

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }
	void setTokenSequence(const TokenSequence* tokenSequence); // for backwards compatible state files

	/** Add new name. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * NameTheory's responsibility to delete the NameSpan */
	void takeNameSpan(NameSpan *nameSpan);

	void removeNameSpan(int i);

	float getScore() const { return score; }
	void setScore(float score_) { score = score_; }
	int getNNameSpans() const { return static_cast<int>(nameSpans.size()); }
	NameSpan* getNameSpan(int i) const;
	std::wstring getNameString(int i) const;

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::NAME_SUBTHEORY; }

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const NameTheory &it)
		{ it.dump(out, 0); return out; }


	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	NameTheory(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit NameTheory(SerifXML::XMLTheoryElement elem);
	virtual const wchar_t* XMLIdentifierPrefix() const;
};

#endif
