// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef SIMPLE_STRING_HYPOTHESIS_H
#define SIMPLE_STRING_HYPOTHESIS_H

#include "boost/shared_ptr.hpp"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"

#include <string>
#include <vector>
#include <set>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(SimpleStringHypothesis);

/*! \brief An implementation of GenericHypothesis used by slot types where we will use
    exact string comparison to determine equality
    
*/
class SimpleStringHypothesis : public GenericHypothesis
{
public:
	friend SimpleStringHypothesis_ptr boost::make_shared<SimpleStringHypothesis>(PGFact_ptr const&, PGFactArgument_ptr const&);

	bool isEquiv(GenericHypothesis_ptr hypoth);
	std::wstring getDisplayValue();
	const std::wstring& getValue() { return _value; }

private:
	SimpleStringHypothesis(PGFact_ptr fact, PGFactArgument_ptr targetArg);

	std::wstring _value;

};

#endif
