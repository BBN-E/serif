// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FACT_SET_H
#define FACT_SET_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Fact.h"

#include <boost/noncopyable.hpp>

class FactSet : public Theory {
public:	
	FactSet();
	~FactSet();

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	FactSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"factset"; }

	void addFact(Fact_ptr fact);

private:
	std::vector<Fact_ptr> _facts;

};

#endif
