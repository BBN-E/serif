// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TYPED_SPAN_H
#define TYPED_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/Span.h"

namespace DataPreprocessor {

	class TypedSpan : public Span {
	public:
		TypedSpan() : Span() { _originalType = Symbol(); _mappedType = Symbol(); _mappedSubtype = Symbol(); }
		TypedSpan(EDTOffset start_offset, EDTOffset end_offset, const Symbol& originalType, const Symbol& mappedType, const Symbol& mappedSubtype)
			: Span(start_offset, end_offset, false, true) { _originalType = originalType; _mappedType = mappedType; _mappedSubtype = mappedSubtype; }
		~TypedSpan() {}

		Symbol getOriginalType() const { return _originalType; }
		void setOriginalType(Symbol type) { _originalType = type; }

		Symbol getMappedType() const { return _mappedType; }
		void setMappedType(Symbol type) { _mappedType = type; }

		Symbol getMappedSubtype() const { return _mappedSubtype; }
		void setMappedSubtype(Symbol subtype) { _mappedSubtype = subtype; }

		virtual bool restrictSentenceBreak() const { return !_maySentenceBreak; }
		virtual bool enforceTokenBreak() const { return _mustBreakAround; }

		virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
		virtual void loadXML(SerifXML::XMLTheoryElement elem);
	private:
		Symbol _originalType;
		Symbol _mappedType;
		Symbol _mappedSubtype;
	};

} // namespace DataPreprocessor

#endif
