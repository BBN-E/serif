// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DP_NAME_SPAN_H
#define DP_NAME_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/TypedSpan.h"

namespace DataPreprocessor {

	class NamedSpan : public TypedSpan {
	public:
		NamedSpan() : TypedSpan() {}
		NamedSpan(EDTOffset start_offset, EDTOffset end_offset, const Symbol& originalType, const Symbol& mappedType, const Symbol& mappedSubtype)
			: TypedSpan(start_offset, end_offset, originalType, mappedType, mappedSubtype) {}
		~NamedSpan();

		virtual Symbol getSpanType() const { return Symbol(L"Name"); }
	};

} // namespace DataPreprocessor

#endif
