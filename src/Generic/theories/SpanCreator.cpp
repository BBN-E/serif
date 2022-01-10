// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/SpanCreator.h"

Symbol SpanCreator::getIdentifier() {
	return (*_new Symbol(L"generic"));
}

/*
Span* SpanCreator::createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
	return _new Span();
}
*/
