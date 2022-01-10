// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PHRASE_HYPOTHESIS_H
#define PHRASE_HYPOTHESIS_H

#include "boost/shared_ptr.hpp"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"

#include <string>
#include <vector>
#include <set>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PhraseHypothesis);

/*! \brief An implementation of GenericHypothesis used by slot types where we will use
    bag-of-words comparison
    
*/
class PhraseHypothesis : public GenericHypothesis
{
public:
	friend PhraseHypothesis_ptr boost::make_shared<PhraseHypothesis>(PGFact_ptr const&);
	
	// Virtual parent class functions, implemented here
	bool isEquiv(GenericHypothesis_ptr hypoth);
	bool isSimilar(GenericHypothesis_ptr hypoth);
	void addSupportingHypothesis(GenericHypothesis_ptr hypo);
	int rankAgainst(GenericHypothesis_ptr hypo);
	std::wstring getDisplayValue();
	bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	bool isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale);

	// Class-specific getters
	int getCoreFactID() { return _coreFact->getFactId();}
	PGFact_ptr getCoreFact() { return _coreFact; }
	const std::set<std::wstring>& getBigWords() { return _bigWords; }
	
	// Class-specific helpers
	bool isSpecialIllegalHypothesis(std::vector<PhraseHypothesis_ptr>& factIDs, std::string& rationale);
	bool isPartiallyTranslated(std::string& rationale);
	bool isInternallyEllided(std::string& rationale);

	// Static helpers
	static std::vector<PhraseHypothesis_ptr> existingProfilePhrases(Profile_ptr existing_profile);

private:
	PhraseHypothesis(PGFact_ptr fact);

	// Class-specific member variables
	std::set<std::wstring> _bigWords;
	std::wstring _display_value;
	PGFact_ptr _coreFact;
	void setCoreFact(PGFact_ptr fact);

};

#endif
