// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_SPAN_H
#define IDF_SPAN_H

#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/Span.h"
#include "Generic/preprocessors/CorefSpan.h"

namespace DataPreprocessor {

	class IdfSpan : public CorefSpan {
	public:
		IdfSpan()
			: CorefSpan() { _type = SymbolConstants::nullSymbol; _role = SymbolConstants::nullSymbol; }
		IdfSpan(EDTOffset start_offset, EDTOffset end_offset, Symbol type)
			: CorefSpan(start_offset, end_offset) { _type = type; _role = SymbolConstants::nullSymbol; }
		IdfSpan(EDTOffset start_offset, EDTOffset end_offset, Symbol type, int id)
			: CorefSpan(start_offset, end_offset, id) { _type = type; _role = SymbolConstants::nullSymbol; }
		IdfSpan(EDTOffset start_offset, EDTOffset end_offset, Symbol type, Symbol role)
			: CorefSpan(start_offset, end_offset) { _type = type; _role = role; }
		IdfSpan(EDTOffset start_offset, EDTOffset end_offset, Symbol type, Symbol role, int id)
			: CorefSpan(start_offset, end_offset, id) { _type = type; _role = role; }
		~IdfSpan() {}

		Symbol getType() { return _type; }
		Symbol getRole() { return _role; }
		bool isDescriptor() { return wcsstr(_type.to_string(), L"_DESC") != NULL; }
		bool isPronoun() { return wcsstr(_type.to_string(), L"_PRO") != NULL; }
		bool isPrenominal() { return wcsstr(_type.to_string(), L"_PRE") != NULL; }
		bool isName() { return wcsstr(_type.to_string(), L"_") == NULL; }

		virtual bool restrictSentenceBreak() const { return !_maySentenceBreak; }
		virtual bool enforceTokenBreak() const { return _mustBreakAround; }

		virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
		virtual void loadXML(SerifXML::XMLTheoryElement elem);
	private:
		Symbol _type;
		Symbol _role;
	};

} // namespace DataPreprocessor

#endif
