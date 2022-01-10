// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/reader/RegionSpan.h"
#include "Generic/state/XMLTheoryElement.h"

namespace {
	// When serializing/deserializing, identify 'RegionSpan' spans by giving 
	// them a span_type value of "RegionSpan".
	Span* loadXML(SerifXML::XMLTheoryElement e) { 
		Span *span = new RegionSpan(); 
		span->loadXML(e); 
		return span; 
	} 
	static int x = Span::registerSpanLoader(Symbol(L"RegionSpan"), loadXML);
}

void RegionSpan::loadXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	Span::loadXML(spanElem);
	_region_type = spanElem.getAttribute<Symbol>(X_region_type);
}

void RegionSpan::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
	using namespace SerifXML;
	Span::saveXML(spanElem);
	spanElem.setAttribute(X_span_type, X_RegionSpan);
	spanElem.setAttribute(X_region_type, _region_type);
}
