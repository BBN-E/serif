// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_NP_SPAN_CREATOR_H
#define DESCRIPTOR_NP_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/preprocessors/DescriptorNPSpan.h"

namespace DataPreprocessor {


	class DescriptorNPSpanCreator : public SpanCreator {
	public:
		DescriptorNPSpanCreator() {}
		~DescriptorNPSpanCreator() {}

		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			return _new DescriptorNPSpan(start_offset, end_offset, (const Attributes *)param);
		}

		virtual Symbol getIdentifier() { return Symbol(L"DESCRIPTOR"); }
	};

} // namespace DataPreprocessor

#endif
