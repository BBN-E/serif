// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_H
#define VALUE_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/ValueType.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"

#include <iostream>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif


class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;


class SERIF_EXPORTED Value : public Theory {
public:
	int getID() const { return _ID; }
	ValueType getFullType() const { return _type; }
	Symbol getType() const { return _type.getBaseTypeSymbol(); }
	Symbol getSubtype() const { return _type.getSubtypeSymbol(); }

	int getSentenceNumber() const { return _valMention->getSentenceNumber(); }
	int getStartToken() const { return _valMention->getStartToken(); }
	int getEndToken() const { return _valMention->getEndToken(); }
	ValueMentionUID getValMentionID() const { return _valMention->getUID(); }

	bool isTimexValue() const { return _type.getBaseTypeSymbol() == Symbol(L"TIMEX2"); }

	Symbol getTimexVal() const { return _timexVal; }
	Symbol getTimexAnchorVal() const { return _timexAnchorVal; }
	Symbol getTimexAnchorDir() const { return _timexAnchorDir; }
	Symbol getTimexSet() const { return _timexSet; }
	Symbol getTimexMod() const { return _timexMod; }
	Symbol getTimexNonSpecific() const { return _timexNonSpecific; }

	void setTimexVal(Symbol sym) { _timexVal = sym; }
	void setTimexAnchorVal(Symbol sym) { _timexAnchorVal = sym; }
	void setTimexAnchorDir(Symbol sym) { _timexAnchorDir = sym; }
	void setTimexSet(Symbol sym) { _timexSet = sym; }
	void setTimexMod(Symbol sym) { _timexMod = sym; }
	void setTimexNonSpecific(Symbol sym) { _timexNonSpecific = sym; }

protected:
	int _ID;
	ValueType _type;

	ValueMention *_valMention;
	
	Symbol _timexVal;
	Symbol _timexAnchorVal;
	Symbol _timexAnchorDir;
	Symbol _timexSet;
	Symbol _timexMod;
	Symbol _timexNonSpecific;

public:
	Value(ValueMention *ment, int value_id);
	Value(Value &other, int set_offset, int sent_offset, std::vector<ValueMentionSet*> mergedValueMentionSets, const ValueMentionSet* documentValueMentionSet, ValueMentionSet::ValueMentionMap &documentValueMentionMap);

	bool isSpecificDate() const;

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Value(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Value(SerifXML::XMLTheoryElement elem, int value_id);
	const wchar_t* XMLIdentifierPrefix() const;
};

#endif
