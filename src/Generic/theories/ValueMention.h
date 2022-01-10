// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_MENTION_H
#define VALUE_MENTION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/ValueType.h"
#include "Generic/theories/SentenceItemIdentifier.h"

#include <iostream>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class ValueMentionSet;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class TokenSequence;
class Value;

class SERIF_EXPORTED ValueMention : public Theory {
public:
	// Document-level identifiers for value-mentions.
	typedef ValueMentionUID UID;

	static ValueMentionUID makeUID(int sentence, int index, bool doclevel = false);

	static int getIndexFromUID(ValueMentionUID uid);

protected:

	ValueMentionUID _uid;
	int _sent_no;
	ValueType _type;

	int _start_tok;
	int _end_tok;

	// WARNING: this should only be set after the doc-values stage
	Value *_docValue;

public:
	ValueMention();
	ValueMention(int sent_no, ValueMentionUID uid, int start, int end, Symbol type);
	ValueMention(ValueMention &source);

	int getSentenceNumber() const;
	int getIndex() const;
	ValueMentionUID getUID() const { return _uid; }
	ValueType getFullType() const { return _type; }
	Symbol getType() const { return _type.getBaseTypeSymbol(); }
	Symbol getSubtype() const { return _type.getSubtypeSymbol(); }
	bool isTimexValue() const { return _type.getBaseTypeSymbol() == Symbol(L"TIMEX2"); }

	void setDocValue(Value *docVal) { _docValue = docVal; }
	const Value* getDocValue() const { return _docValue; }

	int getStartToken() const { return _start_tok; }
	int getEndToken() const { return _end_tok; }

	std::wstring toCasedTextString(const TokenSequence* ts) const;
	std::string toDebugString(const TokenSequence *tokens) const;
	std::wstring toString(const TokenSequence *tokens) const;

	virtual void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const ValueMention &it)
		{ it.dump(out, 0); return out; }


	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	virtual void loadState(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit ValueMention(SerifXML::XMLTheoryElement elem, int sent_no, int valmention_no);
	const wchar_t* XMLIdentifierPrefix() const;
};

#endif
