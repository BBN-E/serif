// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef NPCHUNK_THEORY_H
#define NPCHUNK_THEORY_H


#include "Generic/common/limits.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;


class NPChunkTheory : public SentenceSubtheory {
private:
		Parse* _parse;
		const TokenSequence* _tokenSequence;

public:
	int n_npchunks;
	int npchunks[MAX_NP_CHUNKS][2];
	float score;

public:
	NPChunkTheory(const TokenSequence *tokSequence) : n_npchunks(0), score(0), _parse(0), _tokenSequence(tokSequence) {}
	NPChunkTheory(const NPChunkTheory &other, const TokenSequence *tokSequence);
	~NPChunkTheory(){ if (_parse != NULL) _parse->loseReference();};

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }
	void setTokenSequence(const TokenSequence* tokenSequence); // for backwards compatible state files

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::NPCHUNK_SUBTHEORY; }

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const NPChunkTheory &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	NPChunkTheory(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit NPChunkTheory(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
public:
	bool inChunk(int tokNum, int chunkNum);
	int getChunk(int tokNum);
	Parse* getParse(){return _parse;};
	void setParse(SynNode* root){ _parse = _new Parse(_tokenSequence, root, score); _parse->gainReference(); };
	void takeParse(Parse* parse) { _parse = parse; _parse->gainReference(); }
};

#endif
