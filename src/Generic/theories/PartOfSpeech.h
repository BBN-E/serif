// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PART_OF_SPEECH_H
#define PART_OF_SPEECH_H

#include <iostream>
#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED PartOfSpeech : public Theory {

private:
	std::vector<std::pair<Symbol,float> > _pos_probs;

public: 
	PartOfSpeech() {};
	~PartOfSpeech() {};
	
	static Symbol UnlabeledPOS;

	int getNPOS() const;
	Symbol getLabel(int i) const;
	float getProb(int i) const;
	int addPOS(Symbol pos, float prob);

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const PartOfSpeech &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	PartOfSpeech(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit PartOfSpeech(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
};
#endif
