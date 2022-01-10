// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SPANCREATOR_H
#define SPANCREATOR_H

#include "Generic/theories/Span.h"
#include "Generic/common/Symbol.h"

class SpanCreator
{
public:
	SpanCreator() {}
	virtual ~SpanCreator() {}
	virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) = 0;
	virtual Symbol getIdentifier();
};
#endif
