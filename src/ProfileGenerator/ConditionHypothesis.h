// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef CONDITION_HYPOTHESIS_H
#define CONDITION_HYPOTHESIS_H

#include "boost/shared_ptr.hpp"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"

#include <string>
#include <vector>
#include <set>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(ConditionHypothesis);

/*! \brief An implementation of GenericHypothesis used by slot types where we expect 
    medical conditions.
    
*/
class ConditionHypothesis : public GenericHypothesis
{
public:
	friend ConditionHypothesis_ptr boost::make_shared<ConditionHypothesis>(PGFact_ptr const&);

	// Virtual parent class functions, implemented here
	bool isEquiv(GenericHypothesis_ptr hypoth);
	std::wstring getDisplayValue();
	int rankAgainst(GenericHypothesis_ptr hypo);
	bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	bool isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	void addSupportingHypothesis(GenericHypothesis_ptr hypo);
	bool isSimilar(GenericHypothesis_ptr hypoth);

	// Class-specific functions
	std::wstring getNormalizedDisplayValue();

private:
	ConditionHypothesis(PGFact_ptr fact);

	std::wstring _value;
	std::wstring _normalizedValue;

};

#endif
