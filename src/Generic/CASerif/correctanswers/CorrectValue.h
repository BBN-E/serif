// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_VALUE_H
#define CORRECT_VALUE_H

#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"
#include "common/Offset.h"
#include "theories/ValueType.h"
#include "theories/ValueMention.h"

class TokenSequence;

class CorrectValue {

private:
	
	ValueMention::UID _system_value_mention_id;
	ValueType _valueType;
	
	EDTOffset _start_offset;
	EDTOffset _end_offset;

	int _start_tok;
	int _end_tok;
	int _sentence_number;

	Symbol _annotationValueID;
	Symbol _annotationValueMentionID;

	Symbol _timexVal;
	Symbol _timexAnchorVal;
	Symbol _timexAnchorDir;
	Symbol _timexSet;
	Symbol _timexMod;
	Symbol _timexNonSpecific;

public:
	CorrectValue();
	~CorrectValue() {};

	void loadFromSexp(Sexp *valueSexp);
	//void loadFromERE(ERE *valueERE);
	ValueType getValueType() { return _valueType; }
	Symbol getTypeSymbol() { return _valueType.getNameSymbol(); }
	ValueMention::UID getSystemValueMentionID() { return _system_value_mention_id; }
	void setSystemValueMentionID(ValueMention::UID id) { _system_value_mention_id = id; }
	EDTOffset getStartOffset() { return _start_offset; }
	EDTOffset getEndOffset() { return _end_offset; }
	int getStartToken() { return _start_tok; }
	int getEndToken() { return _end_tok; }
	Symbol getAnnotationValueID() { return _annotationValueID; }
	Symbol getAnnotationValueMentionID() { return _annotationValueMentionID; }
	int getSentenceNumber() { return _sentence_number; }
	void setSentenceNumber(int sentno) { _sentence_number = sentno; }

	bool isValueType(Symbol type);
	bool setStartAndEndTokens(TokenSequence *tokenSequence);

	bool isTimexValue() const { return _valueType.getBaseTypeSymbol() == Symbol(L"TIMEX2"); }

	Symbol getTimexVal() const { return _timexVal; }
	Symbol getTimexAnchorVal() const { return _timexAnchorVal; }
	Symbol getTimexAnchorDir() const { return _timexAnchorDir; }
	Symbol getTimexSet() const { return _timexSet; }
	Symbol getTimexMod() const { return _timexMod; }
	Symbol getTimexNonSpecific() const { return _timexNonSpecific; }

	std::string to_string() const;

	static Symbol NULL_SYM;
};

#endif

