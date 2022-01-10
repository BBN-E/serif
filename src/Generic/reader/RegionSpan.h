// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGION_SPAN_H
#define REGION_SPAN_H

#include "Generic/theories/Span.h"
/*
*/
class RegionSpan : public Span {
public:
	RegionSpan()
		: Span() {}
	RegionSpan(EDTOffset start_offset, EDTOffset end_offset, Symbol* name)
		: Span(start_offset, end_offset, true, false), _region_type(*name) {}
	~RegionSpan() {}

	virtual bool restrictSentenceBreak() const { return !_maySentenceBreak; }
	virtual bool enforceTokenBreak() const { return _mustBreakAround; }
	virtual Symbol getSpanType() const {
		return _region_type; }

	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual void loadXML(SerifXML::XMLTheoryElement elem);
public:
	Symbol _region_type;

};


#endif
