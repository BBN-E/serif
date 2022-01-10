// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_SPAN_CREATOR_H
#define NAME_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/NamedSpan.h"

namespace DataPreprocessor {

	class NamedSpanCreator : public SpanCreator {
	public:
		NamedSpanCreator() {}
		~NamedSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			const Symbol* types = (const Symbol *)param;
			return _new NamedSpan(start_offset, end_offset, types[0], types[1], types[2]);
		}

		virtual Symbol getIdentifier() { return Symbol(L"NAME"); }
	};

} // namespace DataPreprocessor

#endif
