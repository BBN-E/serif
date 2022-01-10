// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LIST_SPAN_H
#define LIST_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/CorefSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {

	class ListSpan : public CorefSpan {
	public:
		ListSpan()
			: CorefSpan() {}
		ListSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: CorefSpan(start_offset, end_offset, attributes->coref_id) {}
		~ListSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"List"); }
	};

} // namespace DataPreprocessor

#endif
