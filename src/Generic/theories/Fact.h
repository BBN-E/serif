// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FACT_H
#define FACT_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/FactArgument.h"

class DocTheory;
class Symbol;

class Fact : public Theory {

public:

	Fact(Symbol factType, Symbol patternID, double score, int score_group, int start_sentence, int end_sentence, int start_token, int end_token);
	~Fact();


	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	Fact(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"fact"; }

	void addFactArgument(FactArgument_ptr argument);

	// Basic accessors & setters
	std::vector<FactArgument_ptr>& getArguments() { return _arguments; }
	Symbol getFactType() { return _factType; }
	Symbol getPatternID() { return _patternID; }
	double getScore() { return _score; }
	int getScoreGroup() { return _score_group; }
	int getStartSentence() { return _start_sentence; }
	int getEndSentence() { return _end_sentence; }
	int getStartToken() { return _start_token; }
	int getEndToken() { return _end_token; }

private:
	std::vector<FactArgument_ptr> _arguments;
	
	Symbol _factType;
	Symbol _patternID;
	double _score;
	int _score_group;
	int _start_sentence;
	int _end_sentence;
	int _start_token;
	int _end_token;

};

typedef boost::shared_ptr<Fact> Fact_ptr;

#endif 
