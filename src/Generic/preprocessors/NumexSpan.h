// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NUMEX_SPAN_H
#define NUMEX_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {


	class NumexSpan : public IdfSpan {
	public:
		NumexSpan()
			: IdfSpan() {}
		NumexSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: IdfSpan(start_offset, end_offset, attributes->type, attributes->coref_id) {}
		~NumexSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Numex"); }
	};

} // namespace DataPreprocessor

#endif
