// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRONOUN_SPAN_H
#define PRONOUN_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/CorefSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {

	class PronounSpan : public CorefSpan {
	public:
		PronounSpan()
			: CorefSpan() {}
		PronounSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: CorefSpan(start_offset, end_offset, attributes->coref_id) {}
		~PronounSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Pronoun"); }
	};

} // namespace DataPreprocessor

#endif

