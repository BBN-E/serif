// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/NestedNameSpan.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"

#include <iostream>


void NestedNameSpan::dump(std::ostream &out, int indent) const {
	NameSpan::dump(out, indent);	
	out << "; parent: ";
	parent->dump(out, indent);
}

void NestedNameSpan::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void NestedNameSpan::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"NestedNameSpan", this);

	stateSaver->saveInteger(start);
	stateSaver->saveInteger(end);
	stateSaver->saveSymbol(type.getName());
	stateSaver->savePointer(parent);

	stateSaver->endList();
}

NestedNameSpan::NestedNameSpan(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"NestedNameSpan");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	start = stateLoader->loadInteger();
	end = stateLoader->loadInteger();
	type = EntityType(stateLoader->loadSymbol());
	parent = static_cast<NameSpan*>(stateLoader->loadPointer());

	stateLoader->endList();
}

void NestedNameSpan::resolvePointers(StateLoader *stateLoader) {
	parent = static_cast<NameSpan*>(stateLoader->getObjectPointerTable().getPointer(parent));
}


const wchar_t* NestedNameSpan::XMLIdentifierPrefix() const {
	return L"nname";
}

void NestedNameSpan::saveXML(SerifXML::XMLTheoryElement namespanElem, const Theory *context) const {
	using namespace SerifXML;
	
	NameSpan::saveXML(namespanElem, context);
	
	// Pointer to parent name:
	namespanElem.saveTheoryPointer(X_parent, parent); 
}

NestedNameSpan::NestedNameSpan(SerifXML::XMLTheoryElement nameSpanElem) : NameSpan(nameSpanElem)
{
	using namespace SerifXML;
	parent = nameSpanElem.loadTheoryPointer<NameSpan>(X_parent);
}
