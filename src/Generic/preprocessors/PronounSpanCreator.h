// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRONOUN_SPAN_CREATOR_H
#define PRONOUN_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/PronounSpan.h"

namespace DataPreprocessor {


	class PronounSpanCreator : public SpanCreator {
	public:
		PronounSpanCreator() {}
		~PronounSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new PronounSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"PRONOUN"); }
	};

} // namespace DataPreprocessor

#endif
