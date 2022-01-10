// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_CLUTTER_FILTER_H
#define AR_CLUTTER_FILTER_H

#include "Generic/clutter/SetClutterFilter.h"
#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/theories/DocTheory.h"

//#include "English/clutter/en_ATEAInternalClutterFilter.h"

#include <string>

class ArabicClutterFilter: public SetClutterFilter {
private:
	friend class ArabicClutterFilterFactory;

public:
	virtual ~ArabicClutterFilter();
	virtual void filterClutter (DocTheory* docTheory);

protected:

	bool _filterOn;

private:
	ArabicClutterFilter();

};

class ArabicClutterFilterFactory: public ClutterFilter::Factory {
	virtual ClutterFilter *build() { return _new ArabicClutterFilter(); }
};


#endif
