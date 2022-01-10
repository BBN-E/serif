// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENAMEX_SPAN_CREATOR_H
#define ENAMEX_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/EnamexSpan.h"

namespace DataPreprocessor {


	class EnamexSpanCreator : public SpanCreator {
	public:
		EnamexSpanCreator() {}
		~EnamexSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new EnamexSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"ENAMEX"); }
	};

} // namespace DataPreprocessor

#endif
