// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Span.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <sstream>

void Span::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Span::saveXML", "Expected context to be NULL");
	OffsetGroup start, end;
	start.edtOffset = EDTOffset(_start_offset);
	end.edtOffset = EDTOffset(_end_offset);
	spanElem.saveOffsets(start, end);
	spanElem.setAttribute(X_span_type, getSpanType());
}
	
void Span::loadXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	OffsetGroup start, end;
	spanElem.loadOffsets(start, end);
	_start_offset = start.edtOffset;
	_end_offset = end.edtOffset;
}

namespace {
	Symbol::HashMap<Span::XMLSpanLoader*> &getSpanRegistry() {
		static Symbol::HashMap<Span::XMLSpanLoader*> _registry;
		return _registry;
	}
}
	
int Span::registerSpanLoader(Symbol id, XMLSpanLoader *loader) {
	// This is the global registry of span classes:
	static Symbol::HashMap<Span::XMLSpanLoader*> &_registry = getSpanRegistry();
	if (_registry.find(id) != _registry.end())
		throw InternalInconsistencyException("Span::registerSpanLoader",
			"The same id was registered twice.");
	_registry[id] = loader;
	return 0;
}

Span* Span::loadSpanFromXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	std::wstring span_type = spanElem.getAttribute<std::wstring>(X_span_type);
	XMLSpanLoader *loader = getSpanRegistry()[span_type];
	if (loader == NULL) {
		std::wstringstream err;
		err << "Unknown span type \"" << span_type << "\".";
		spanElem.reportLoadError(err.str());
		return 0;
	} else {
		return (*loader)(spanElem);
	}
}

const wchar_t* Span::XMLIdentifierPrefix() const {
	return L"Span";
}


