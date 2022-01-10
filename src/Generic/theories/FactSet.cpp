// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/FactSet.h"
#include "Generic/theories/Fact.h"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

FactSet::FactSet() { }

FactSet::~FactSet() { }

void FactSet::updateObjectIDTable() const {
	throw InternalInconsistencyException("FactSet::updateObjectIDTable",
		"FactSet does not currently have state file support");
}
void FactSet::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("FactSet::saveState",
		"FactSet does not currently have state file support");
}
void FactSet::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("FactSet::resolvePointers",
		"FactSet does not currently have state file support");
}

void FactSet::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	BOOST_FOREACH(Fact_ptr fact, _facts) {
		elem.saveChildTheory(X_Fact, fact.get(), context);
	}
}

FactSet::FactSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory) {
	using namespace SerifXML;

	elem.loadId(this);

	XMLTheoryElementList factElems = elem.getChildElementsByTagName(X_Fact);
	size_t n_facts = factElems.size();
	for (size_t i = 0; i < n_facts; ++i)
		addFact(boost::make_shared<Fact>(factElems[i], theory));
}

void FactSet::addFact(Fact_ptr fact) {
	_facts.push_back(fact);
}
