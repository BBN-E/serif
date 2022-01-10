// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_SPAN_CREATOR_H
#define HEAD_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/HeadSpan.h"

namespace DataPreprocessor {

	class HeadSpanCreator : public SpanCreator {
	public:
		HeadSpanCreator() {}
		~HeadSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new HeadSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"HEAD"); }
	};

} // namespace DataPreprocessor

#endif
