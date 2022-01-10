// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ProfileGenerator/SimpleStringHypothesis.h"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/foreach.hpp"
#include "boost/bind.hpp"

#include <iostream>
#include <wchar.h>

SimpleStringHypothesis::SimpleStringHypothesis(PGFact_ptr fact, PGFactArgument_ptr targetArg) {
	addFact(fact);
	_value = targetArg->getResolvedStringValue();
}

std::wstring SimpleStringHypothesis::getDisplayValue() {
	return _value;
}

bool SimpleStringHypothesis::isEquiv(GenericHypothesis_ptr hypoth) {
	SimpleStringHypothesis_ptr simpleStringHypoth = boost::dynamic_pointer_cast<SimpleStringHypothesis>(hypoth);
	if (simpleStringHypoth == SimpleStringHypothesis_ptr()) // cast failed
		return false;
	return (_value.compare(simpleStringHypoth->getValue()) == 0);	
}
