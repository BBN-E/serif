// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_SPAN_CREATOR_H
#define SENTENCE_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/SentenceSpan.h"

namespace DataPreprocessor {

	class SentenceSpanCreator : public SpanCreator {
	public:
		SentenceSpanCreator() {}
		~SentenceSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new SentenceSpan(start_offset, end_offset);
		}

		virtual Symbol getIdentifier() { return Symbol(L"SENT"); }
	};

} // namespace DataPreprocessor

#endif
