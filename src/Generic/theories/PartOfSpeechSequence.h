// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PART_OF_SPEECH_SEQUENCE_H
#define PART_OF_SPEECH_SEQUENCE_H

#include "Generic/theories/PartOfSpeech.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/common/limits.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED PartOfSpeechSequence : public SentenceSubtheory {
private:
	int _n_tokens;
	PartOfSpeech** _posSequence;
	float _score;
	int _sent_no;
	const TokenSequence* _tokenSequence;

public: 
	PartOfSpeechSequence(const TokenSequence *tokenSequence);
	PartOfSpeechSequence(const PartOfSpeechSequence &other, const TokenSequence *tokenSequence);
	~PartOfSpeechSequence();
	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
		{ return SentenceTheory::POS_SUBTHEORY; }

	PartOfSpeechSequence(const TokenSequence *tokenSequence, int sent_no, int n_tokens);

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }
	void setTokenSequence(const TokenSequence* tokenSequence); // for backwards compatible state files

	bool isEmpty() const;
	int getNTokens() const { return _n_tokens; };
	PartOfSpeech* getPOS(int i) const;
	int addPOS(Symbol pos, float prob, int token_num);
	float getScore() const { return _score; };
	void setScore(float score) { _score  = score; };

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const PartOfSpeechSequence &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	PartOfSpeechSequence(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit PartOfSpeechSequence(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
};
#endif
