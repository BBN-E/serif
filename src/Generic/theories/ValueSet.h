// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_SET_H
#define VALUE_SET_H

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

#include "Generic/theories/Theory.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

class SERIF_EXPORTED ValueSet : public Theory {
public:
	ValueSet(int n_values);
	ValueSet(std::vector<ValueSet*> splitValueSets, std::vector<int> sentenceOffsets, std::vector<ValueMentionSet*> mergedValueMentionSets, const ValueMentionSet* documentValueMentionSet, std::vector<ValueMentionSet::ValueMentionMap> &documentValueMentionMaps);
	~ValueSet();

	/** Add new value to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * ValueSet's responsibility to delete the Value */
	void takeValue(int i, Value *value);

	int getNValues() const { return _n_values; }
	Value *getValue(int i) const;
	Value *getValueByValueMention(ValueMentionUID uid) const;

	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	ValueSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit ValueSet(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

protected:
	int _n_values;
	Value **_values;

};
#endif
