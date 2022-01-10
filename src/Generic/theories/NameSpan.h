// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_SPAN_H
#define NAME_SPAN_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include <string>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class TokenSequence;

class NameSpan : public Theory {
public:
	int start;
	int end;
	EntityType type;

	
	NameSpan(int start, int end, EntityType type)
		: start(start), end(end), type(type) {}
	NameSpan(const NameSpan &src)
		: start(src.start), end(src.end), type(src.type) {}

	NameSpan() : start(0), end(0), type(EntityType::getOtherType()) {}

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, NameSpan &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	NameSpan(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit NameSpan(SerifXML::XMLTheoryElement elem);
	virtual const wchar_t* XMLIdentifierPrefix() const;
};

#endif
