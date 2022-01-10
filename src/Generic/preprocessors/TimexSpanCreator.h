// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TIMEX_SPAN_CREATOR_H
#define TIMEX_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/TimexSpan.h"

namespace DataPreprocessor {

	class TimexSpanCreator : public SpanCreator {
	public:
		TimexSpanCreator() {}
		~TimexSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new TimexSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"TIMEX"); }
	};

} // namespace DataPreprocessor

#endif
