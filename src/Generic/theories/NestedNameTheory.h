// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NESTED_NAME_THEORY_H
#define NESTED_NAME_THEORY_H

#include "Generic/theories/NameTheory.h"
#include <boost/foreach.hpp> 

class NestedNameSpan;

class NestedNameTheory : public NameTheory {
private:
	const NameTheory *_nameTheory;

public:
	NestedNameTheory(const TokenSequence *tokSequence, const NameTheory *nameTheory, int n_name_spans=0, NestedNameSpan** nameSpans=0) 
		: NameTheory(tokSequence, n_name_spans, (NameSpan**)nameSpans), _nameTheory(nameTheory) {}
	NestedNameTheory(const TokenSequence *tokSequence, const NameTheory *nameTheory, std::vector<NestedNameSpan*> nestedNameSpans);
	NestedNameTheory(const NestedNameTheory &other, const TokenSequence *tokSequence) 
		: NameTheory(other, tokSequence), _nameTheory(other._nameTheory) {}

	const NameTheory* getNameTheory() const { return _nameTheory; }
	void setNameTheory(const NameTheory* nameTheory); 

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::NESTED_NAME_SUBTHEORY; }

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	NestedNameTheory(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit NestedNameTheory(SerifXML::XMLTheoryElement elem);
	virtual const wchar_t* XMLIdentifierPrefix() const;
}; 

#endif
