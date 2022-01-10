// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPOSITION_H
#define PROPOSITION_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Argument.h"
#include "Generic/common/limits.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/ParamReader.h"
#include <iostream>

#include "boost/foreach.hpp"

class SentenceTheory;
class Parse;
class SynNode;
class MentionSet;
class Mention;
class PropositionSet;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Proposition : public Theory {
public:
	typedef enum {VERB_PRED,
				  COPULA_PRED,
				  MODIFIER_PRED,
				  NOUN_PRED,
				  POSS_PRED,
				  LOC_PRED,
				  SET_PRED,
				  NAME_PRED,
				  PRONOUN_PRED,
				  COMP_PRED,
				  ANY_PRED}
		PredType;
	static const int N_PRED_TYPES = ANY_PRED + 1;
	static const char *PRED_TYPE_STRINGS[];
	static const wchar_t *PRED_TYPE_WSTRINGS[];
	static const char *getPredTypeString(PredType type);
	static PredType getPredTypeFromString(const wchar_t* typeString);

	Proposition(int ID, PredType predType, int n_args);
	Proposition(const Proposition &other, int sent_offset = 0, const Parse* parse = NULL);
	void setPredHead(const SynNode *predHead)
		{ _predHead = predHead; }
	void setParticle(const SynNode *particle)
		{ _particle = particle; }
	void setAdverb(const SynNode *adverb)
		{ _adverb = adverb; }
	void setNegation(const SynNode *negation)
		{ _negation = negation; }
	void setModal(const SynNode *modal)
		{ _modal = modal; }
	void setArg(int i, Argument &arg);
	void addStatus(PropositionStatusAttribute status) 
		{ _statuses.insert(status); }
	void addStatuses(Proposition* prop) 
		{ addStatuses(prop->getStatuses()); }
	void addStatuses(const std::set<PropositionStatusAttribute>& statuses) 
		{ BOOST_FOREACH(PropositionStatusAttribute status, statuses) { addStatus(status);} 	}

	~Proposition() {
		delete[] _args;
	}


	// accessors
	int getID() const { return _ID; }
	int getIndex() const { return _ID % MAX_SENTENCE_PROPS; }
	PredType getPredType() const { return _predType; }
	const SynNode *getPredHead() const { return _predHead; }
	const SynNode *getParticle() const { return _particle; }
	const SynNode *getAdverb() const { return _adverb; }
	const SynNode *getNegation() const { return _negation; }
	const SynNode *getModal() const { return _modal; }
	
	int getNArgs() const { return _n_args; }
	Argument *getArg(int i) const;

	Symbol getPredSymbol() const;

	const std::set<PropositionStatusAttribute>& getStatuses() const { return _statuses; }
	bool hasStatus(PropositionStatusAttribute status) const { return _statuses.find(status) != _statuses.end(); }
	bool hasAnyStatus() const { return _statuses.size() != 0; }
	
	/** for convenience, this gets the first mention argument whose
	  * role matches (0 if none found) */
	const Mention *getMentionOfRole(Symbol roleSym,
									const MentionSet *mentionSet) const;
	Symbol getRoleOfMention(const Mention* mention, const MentionSet* mentionSet) const;

	const Proposition *getPropositionOfRole(Symbol roleSym) const;

public:
	bool hasCycle(const SentenceTheory* sentTheory) const;
	bool hasCycle(const SentenceTheory* sentTheory, const Argument* childArg) const;
private:
	bool hasCycle(const SentenceTheory* sentTheory, std::vector<const Proposition*> & seen) const;
	bool hasCycle(const SentenceTheory* sentTheory, const Argument* childArg, std::vector<const Proposition*> & seen) const;

public:
	void dump(std::ostream &out, int indent = 0) const;
	void dump(std::wostream &wout, int indent = 0, bool=false) const;
	friend std::ostream &operator <<(std::ostream &out, const Proposition &it)
		{ it.dump(out, 0); return out; }
	void dump(UTF8OutputStream &out, int indent = 0) const;
	friend UTF8OutputStream &operator <<(UTF8OutputStream &out, const Proposition &it)
		{ it.dump(out, 0); return out; }

	std::wstring toString(int indent = 0) const;
	std::string toDebugString(int indent = 0) const;
	std::wstring getTerminalString(void) const;

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Proposition(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Proposition(SerifXML::XMLTheoryElement elem, int prop_id);
	void resolvePointers(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

	/*Convenenice for getting the 'extent' of a propsotion.  The extent of a proposition is not completely defined.
	For these purposes, extent is defined as:
		start token: first token of any argument (or argument of arguments) or the extent of the pred head
		end token: last token of any argument (or argument of arguments) or the extent of pred head
	Warning: startToken and endToken must be initialized outside of this function. 
		The function will look for arguments that start before the initial startToken and end after the initial endToken
	*/
	void getStartEndTokenProposition(const MentionSet* mentionSet, int& startToken, int& endToken) const;


private:
	int _ID;
	PredType _predType;
	const SynNode *_predHead;
	const SynNode *_particle;
	const SynNode *_adverb;
	const SynNode *_negation;
	const SynNode *_modal;
	int _n_args;
	Argument *_args;
	std::set<PropositionStatusAttribute> _statuses;
	static bool debug_props;
};

#endif
