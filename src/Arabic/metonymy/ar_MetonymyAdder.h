// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_METONYMY_ADDER_H
#define ar_METONYMY_ADDER_H

#include "Generic/common/SymbolHash.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/DebugStream.h"
#include "Generic/metonymy/MetonymyAdder.h"

class MentionSet;
class PropositionSet;
class Proposition;
class Argument;
class SymbolHash;
class Mention;

class ArabicMetonymyAdder : public MetonymyAdder {
private:
	friend class ArabicMetonymyAdderFactory;
public:


	void resetForNewSentence() {}
	void resetForNewDocument(DocTheory *docTheory) {}

	virtual void addMetonymyTheory(const MentionSet *mentionSet,
				                   const PropositionSet *propSet);
private:
	ArabicMetonymyAdder();
	bool _use_gpe_roles;
	static Symbol _in1Sym;
	static Symbol _gpeLoc[8];
	bool _hasLocRole(const Mention* ment);
	bool _hasOrgRole(const Mention* ment, bool checkedForLoc);
	static const int _nloc = 8;
	static Symbol _gpeOrg[1];
	static const int _norg = 1;
	DebugStream _debug;
	SymbolHash* _cities;
	void loadSymbolHash(SymbolHash *hash, const char* file);

};

class ArabicMetonymyAdderFactory: public MetonymyAdder::Factory {
	virtual MetonymyAdder *build() { return _new ArabicMetonymyAdder(); }
};

#endif
