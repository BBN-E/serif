/**
 * Abstract class interface (with factory methods) for
 * parameters used in ReadingMacroOperators.
 *
 * @file ReadingMacroParameter.cpp
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ReadingMacroParameter.h"

Symbol ReadingMacroParameter::INDIVIDUAL_SYM(L"individual");
Symbol ReadingMacroParameter::PREDICATE_SYM(L"predicate");
Symbol ReadingMacroParameter::RELATION_SYM(L"relation");
Symbol ReadingMacroParameter::ROLE_SYM(L"role");
