// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/correctanswers/CorrectValue.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"
#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"
#include "common/SessionLogger.h"
#include "theories/TokenSequence.h"

Symbol CorrectValue::NULL_SYM = Symbol(L"NULL");

CorrectValue::CorrectValue() :
	_start_offset(0), _end_offset(0), _start_tok(0), _end_tok(0), _sentence_number(-1)
{ 
	_system_value_mention_id = ValueMention::UID();
	_annotationValueID = Symbol();
	_valueType = ValueType();
}

void CorrectValue::loadFromSexp(Sexp *valueSexp)
{
	int num_children = valueSexp->getNumChildren();
	if (num_children != 3 && num_children != 9)
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "valueSexp has an invalid number of children");

	Sexp *typeSexp = valueSexp->getNthChild(0);
	if (!typeSexp->isAtom())
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value type atom in correctAnswerSexp");

	if (isValueType(typeSexp->getValue())) {
		_valueType = ValueType(typeSexp->getValue());
	} else {
		_valueType = ValueType();
	}

	Sexp *valueIDSexp;
	if (num_children == 9 && isTimexValue()) {
		Sexp *timexValSexp = valueSexp->getNthChild(1);
		Sexp *timexAnchorValSexp = valueSexp->getNthChild(2);
		Sexp *timexAnchorDirSexp = valueSexp->getNthChild(3);
		Sexp *timexSetSexp = valueSexp->getNthChild(4);
		Sexp *timexModSexp = valueSexp->getNthChild(5);
		Sexp *timexNonSpecificSexp = valueSexp->getNthChild(6);
		valueIDSexp = valueSexp->getNthChild(7);

		if (!timexValSexp->isAtom() || !timexAnchorValSexp->isAtom() || !timexAnchorDirSexp->isAtom() ||
			!timexSetSexp->isAtom() || !timexModSexp->isAtom() || !timexNonSpecificSexp->isAtom())
		{
			throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value timex atoms in correctAnswerSexp");
		}

		_timexVal = (timexValSexp->getValue() == NULL_SYM) ? Symbol() : timexValSexp->getValue();
		_timexAnchorVal = (timexAnchorValSexp->getValue() == NULL_SYM) ? Symbol() : timexAnchorValSexp->getValue();
		_timexAnchorDir = (timexAnchorDirSexp->getValue() == NULL_SYM) ? Symbol() : timexAnchorDirSexp->getValue();
		_timexSet = (timexSetSexp->getValue() == NULL_SYM) ? Symbol() : timexSetSexp->getValue();
		_timexMod = (timexModSexp->getValue() == NULL_SYM) ? Symbol() : timexModSexp->getValue();
		_timexNonSpecific = (timexNonSpecificSexp->getValue() == NULL_SYM) ? Symbol() : timexNonSpecificSexp->getValue();
	}
	else if (num_children == 3) {
		valueIDSexp = valueSexp->getNthChild(1);
	}
	else {
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Non-timex valueSexp has an invalid number of children");
	}

	if (!valueIDSexp->isAtom())
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value id atom in correctAnswerSexp");
	_annotationValueID = valueIDSexp->getValue();


	Sexp* mentionSexp = valueSexp->getNthChild(num_children - 1);
	int num_mention_children = mentionSexp->getNumChildren();
	if (num_children < 3)
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "mentionSexp doesn't have at least 3 children");

	Sexp *mentionIDSexp = mentionSexp->getNthChild(0);
	Sexp *startSexp = mentionSexp->getNthChild(1);
	Sexp *endSexp = mentionSexp->getNthChild(2);

	if (!mentionIDSexp->isAtom() || !startSexp->isAtom() || !endSexp->isAtom())
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value mention atoms in correctAnswerSexp");

	_annotationValueMentionID = mentionIDSexp->getValue();
	_start_offset = EDTOffset(_wtoi(startSexp->getValue().to_string()));
	_end_offset = EDTOffset(_wtoi(endSexp->getValue().to_string()));
}

/*void CorrectValue::loadFromERE(ERE *valueERE)
{
	int num_children = valueERE->getNumChildren();
	if (num_children != 3 && num_children != 9)
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "valueSexp has an invalid number of children");

	Sexp *typeSexp = valueSexp->getNthChild(0);
	if (!typeSexp->isAtom())
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value type atom in correctAnswerSexp");

	if (isValueType(typeSexp->getValue())) {
		_valueType = ValueType(typeSexp->getValue());
	} else {
		_valueType = ValueType();
	}

	Sexp *valueIDSexp;
	if (num_children == 9 && isTimexValue()) {
		Sexp *timexValSexp = valueSexp->getNthChild(1);
		Sexp *timexAnchorValSexp = valueSexp->getNthChild(2);
		Sexp *timexAnchorDirSexp = valueSexp->getNthChild(3);
		Sexp *timexSetSexp = valueSexp->getNthChild(4);
		Sexp *timexModSexp = valueSexp->getNthChild(5);
		Sexp *timexNonSpecificSexp = valueSexp->getNthChild(6);
		valueIDSexp = valueSexp->getNthChild(7);

		if (!timexValSexp->isAtom() || !timexAnchorValSexp->isAtom() || !timexAnchorDirSexp->isAtom() ||
			!timexSetSexp->isAtom() || !timexModSexp->isAtom() || !timexNonSpecificSexp->isAtom())
		{
			throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value timex atoms in correctAnswerSexp");
		}

		_timexVal = (timexValSexp->getValue() == NULL_SYM) ? Symbol() : timexValSexp->getValue();
		_timexAnchorVal = (timexAnchorValSexp->getValue() == NULL_SYM) ? Symbol() : timexAnchorValSexp->getValue();
		_timexAnchorDir = (timexAnchorDirSexp->getValue() == NULL_SYM) ? Symbol() : timexAnchorDirSexp->getValue();
		_timexSet = (timexSetSexp->getValue() == NULL_SYM) ? Symbol() : timexSetSexp->getValue();
		_timexMod = (timexModSexp->getValue() == NULL_SYM) ? Symbol() : timexModSexp->getValue();
		_timexNonSpecific = (timexNonSpecificSexp->getValue() == NULL_SYM) ? Symbol() : timexNonSpecificSexp->getValue();
	}
	else if (num_children == 3) {
		valueIDSexp = valueSexp->getNthChild(1);
	}
	else {
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Non-timex valueSexp has an invalid number of children");
	}

	if (!valueIDSexp->isAtom())
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value id atom in correctAnswerSexp");
	_annotationValueID = valueIDSexp->getValue();


	Sexp* mentionSexp = valueSexp->getNthChild(num_children - 1);
	int num_mention_children = mentionSexp->getNumChildren();
	if (num_children < 3)
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "mentionSexp doesn't have at least 3 children");

	Sexp *mentionIDSexp = mentionSexp->getNthChild(0);
	Sexp *startSexp = mentionSexp->getNthChild(1);
	Sexp *endSexp = mentionSexp->getNthChild(2);

	if (!mentionIDSexp->isAtom() || !startSexp->isAtom() || !endSexp->isAtom())
		throw UnexpectedInputException("CorrectValue::loadFromSexp()",
									   "Didn't find value mention atoms in correctAnswerSexp");

	_annotationValueMentionID = mentionIDSexp->getValue();
	_start_offset = EDTOffset(_wtoi(startSexp->getValue().to_string()));
	_end_offset = EDTOffset(_wtoi(endSexp->getValue().to_string()));
}*/

bool CorrectValue::isValueType(Symbol type) {
	return ValueType::isValidValueType(type);
}

bool CorrectValue::setStartAndEndTokens(TokenSequence *tokenSequence) {
	if (!CorrectAnswers::isWithinTokenSequence(tokenSequence, _start_offset, _end_offset)) 
		return false;

	CorrectAnswers::findTokens(tokenSequence, _start_offset, _end_offset, _start_tok, _end_tok);

	return true;
}

std::string CorrectValue::to_string() const {
	std::stringstream result;
	result << _annotationValueMentionID.to_debug_string() << " "
		<< _valueType.getBaseTypeSymbol().to_debug_string() << "."
		<< _valueType.getSubtypeSymbol().to_debug_string() << " "
		<< "[" << _start_offset.value() << ":" << _end_offset.value() << "]";
	return result.str();
}
