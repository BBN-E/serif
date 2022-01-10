// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NESTED_NAME_SPAN_H
#define NESTED_NAME_SPAN_H

#include "Generic/theories/NameSpan.h"

class NestedNameSpan : public NameSpan {
public:
	const NameSpan *parent;

	NestedNameSpan(int start, int end, EntityType type, NameSpan *parent)
		: NameSpan(start, end, type), parent(parent) {}
	NestedNameSpan(const NestedNameSpan &src)
		: NameSpan(src), parent(src.parent) {}

	NestedNameSpan() : NameSpan(), parent(0) {}

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, NestedNameSpan &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	NestedNameSpan(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit NestedNameSpan(SerifXML::XMLTheoryElement elem);
	virtual const wchar_t* XMLIdentifierPrefix() const;
};

#endif
