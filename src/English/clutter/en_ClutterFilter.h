// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_CLUTTER_FILTER_H
#define EN_CLUTTER_FILTER_H

#include "Generic/clutter/SetClutterFilter.h"
#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/theories/DocTheory.h"

#include "English/clutter/en_ATEAInternalClutterFilter.h"

#include <string>

class EnglishClutterFilter: public SetClutterFilter {
private:
  	friend class EnglishClutterFilterFactory;

public:
	virtual ~EnglishClutterFilter();
	virtual void filterClutter (DocTheory* docTheory);

protected:

	bool _filterOn;

private:
	EnglishClutterFilter();

};

class EnglishClutterFilterFactory: public ClutterFilter::Factory {
	virtual ClutterFilter *build() { return _new EnglishClutterFilter(); }
};


#endif
