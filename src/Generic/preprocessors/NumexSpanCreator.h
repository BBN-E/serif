// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NUMEX_SPAN_CREATOR_H
#define NUMEX_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/NumexSpan.h"

namespace DataPreprocessor {

	class NumexSpanCreator : public SpanCreator {
	public:
		NumexSpanCreator() {}
		~NumexSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new NumexSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"NUMEX"); }
	};

} // namespace DataPreprocessor

#endif
