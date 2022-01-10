// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SET_CLUTTER_FILTER_H
#define SET_CLUTTER_FILTER_H

#include "Generic/clutter/ClutterFilter.h"
#include "Generic/common/GrowableArray.h"

#include <string>

/* defines a dynamic set of filters
 * when run it is running each of the filters in turn.
 */
class SetClutterFilter: public ClutterFilter {
public:
	SetClutterFilter(){};
	virtual ~SetClutterFilter() {
		// Delete the Filters
		for (int i = 0; i<set.length(); i++)
			delete set[i];
	};


protected:
	GrowableArray<ClutterFilter*> set;

	void addFilter(ClutterFilter *filter){
		set.add(filter);
	}
	

	void filterClutterRaw (DocTheory* docTheory){
		for (int i = 0; i<set.length(); i++)
			set[i]->filterClutter(docTheory);
	}
public:
	virtual void filterClutter (DocTheory* docTheory) {
		filterClutterRaw(docTheory);
	};

};

#endif
