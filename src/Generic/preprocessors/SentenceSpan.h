// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_SPAN_H
#define SENTENCE_SPAN_H

#include "Generic/theories/Span.h"

namespace DataPreprocessor {

	class SentenceSpan : public Span {
	public:
		SentenceSpan()
			: Span() {}
		SentenceSpan(EDTOffset start_offset, EDTOffset end_offset)
			: Span(start_offset, end_offset, true, true) {}
		~SentenceSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Sentence"); }

		virtual bool restrictSentenceBreak() const { return !_maySentenceBreak; }
		virtual bool enforceTokenBreak() const { return _mustBreakAround; }
	};

} // namespace DataPreprocessor

#endif
