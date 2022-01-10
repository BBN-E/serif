// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGION_SPAN_CREATOR_H
#define REGION_SPAN_CREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/reader/RegionSpan.h"


class RegionSpanCreator : public SpanCreator {
public:
	RegionSpanCreator() {}
	~RegionSpanCreator() {}

	virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
		return _new RegionSpan(start_offset, end_offset, (Symbol*) param);
	}

	virtual Symbol getIdentifier() { return Symbol(L"REGION"); }
};


#endif
