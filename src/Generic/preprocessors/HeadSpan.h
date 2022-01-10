// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_SPAN_H
#define HEAD_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/CorefSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {

	class HeadSpan : public CorefSpan {
	public:
		HeadSpan()
			: CorefSpan() {}
		HeadSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: CorefSpan(start_offset, end_offset, attributes->coref_id) {}
		~HeadSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Head"); }
	};

} // namespace DataPreprocessor

#endif
