// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/preprocessors/DescriptorNPSpan.h"
#include "Generic/state/XMLTheoryElement.h"

namespace DataPreprocessor {
void IdfSpan::loadXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	CorefSpan::loadXML(spanElem);
	_type = spanElem.getAttribute<Symbol>(X_idf_type);
	_role = spanElem.getAttribute<Symbol>(X_idf_role);
}

void IdfSpan::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
	using namespace SerifXML;
	CorefSpan::saveXML(spanElem);
	spanElem.setAttribute(X_idf_type, _type);
	spanElem.setAttribute(X_idf_role, _role);
}
} // namespace DataPreprocessor
