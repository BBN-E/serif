// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TIMEX_SPAN_H
#define TIMEX_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {

	class TimexSpan : public IdfSpan {
	public:
		TimexSpan()
			: IdfSpan() {}
		TimexSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: IdfSpan(start_offset, end_offset, attributes->type, attributes->coref_id) {}
		~TimexSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Timex"); }
	};

} // namespace DataPreprocessor

#endif
