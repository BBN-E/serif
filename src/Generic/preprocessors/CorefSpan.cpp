// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/preprocessors/CorefSpan.h"
#include "Generic/state/XMLTheoryElement.h"

namespace DataPreprocessor {
void CorefSpan::loadXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	Span::loadXML(spanElem);
	_id = spanElem.getAttribute<int>(X_span_id);
	// [xx] this assumes we've already deserialized the parent --
	// will that always be true??
	_parent = spanElem.loadOptionalTheoryPointer<CorefSpan>(X_parent_id);
}

void CorefSpan::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
	using namespace SerifXML;
	Span::saveXML(spanElem);
	spanElem.setAttribute(X_span_id, _id);
	// [xx] this assumes we've already serialized the parent --
	// will that always be true??
	spanElem.saveTheoryPointer(X_parent_id, _parent);
}
} // namespace DataPreprocessor
