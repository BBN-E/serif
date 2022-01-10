// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef DEPENDENCY_PARSE_THEORY_H
#define DEPENDENCY_PARSE_THEORY_H


#include "Generic/common/limits.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class DepNode;


class DependencyParseTheory : public SentenceSubtheory {
private:
		Parse* _parse;
		const TokenSequence* _tokenSequence;

public:
	float score;

public:
	DependencyParseTheory(const TokenSequence *tokSequence) : score(0), _parse(0), _tokenSequence(tokSequence) {}
	DependencyParseTheory(const DependencyParseTheory &other, const TokenSequence *tokSequence);
	~DependencyParseTheory(){ if (_parse != NULL) _parse->loseReference();};

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }
	void setTokenSequence(const TokenSequence* tokenSequence); // for backwards compatible state files

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::DEPENDENCY_PARSE_SUBTHEORY; }

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const DependencyParseTheory &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	DependencyParseTheory(StateLoader *stateLoader);
	void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit DependencyParseTheory(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
public:
	Parse* getParse(){return _parse;};
	void setParse(SynNode *root){ _parse = _new Parse(_tokenSequence, root, score); _parse->gainReference(); };
};

#endif
