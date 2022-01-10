// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/preprocessors/TypedSpan.h"
#include "Generic/state/XMLTheoryElement.h"

namespace DataPreprocessor {

	void TypedSpan::loadXML(SerifXML::XMLTheoryElement spanElem) {
		using namespace SerifXML;
		Span::loadXML(spanElem);
		_originalType = spanElem.getAttribute<Symbol>(X_original_type);
		if (spanElem.hasAttribute(X_mapped_type))
			_mappedType = spanElem.getAttribute<Symbol>(X_mapped_type);
		else
			_mappedType = Symbol();
		if (spanElem.hasAttribute(X_mapped_subtype))
			_mappedSubtype = spanElem.getAttribute<Symbol>(X_mapped_subtype);
		else
			_mappedSubtype = Symbol();
	}

	void TypedSpan::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
		using namespace SerifXML;
		Span::saveXML(spanElem);
		spanElem.setAttribute(X_original_type, _originalType);
		if (!_mappedType.is_null())
			spanElem.setAttribute(X_mapped_type, _mappedType);
		if (!_mappedSubtype.is_null())
			spanElem.setAttribute(X_mapped_subtype, _mappedSubtype);
	}

} // namespace DataPreprocessor
