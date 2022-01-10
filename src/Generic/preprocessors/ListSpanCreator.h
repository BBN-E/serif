// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LIST_SPAN_CREATOR_H
#define LIST_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/ListSpan.h"

namespace DataPreprocessor {

	class ListSpanCreator : public SpanCreator {
	public:
		ListSpanCreator() {}
		~ListSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new ListSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"LIST"); }
	};

} // namespace DataPreprocessor

#endif
