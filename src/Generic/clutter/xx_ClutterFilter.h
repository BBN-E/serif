// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_CLUTTER_FILTER_H
#define xx_CLUTTER_FILTER_H

#include "Generic/clutter/ClutterFilter.h"

// unimplemented generics filter. This does nothing
// it displays an error message upon initialization

class DefaultClutterFilter : public ClutterFilter {
private:
	friend class DefaultClutterFilterFactory;

public:
	/**
	 * WARNING: A GenericsFilter has not been defined if this constructor is being used.
	 */
	void filterClutter (DocTheory* docTheory) { }
	EntityClutterFilter *getEntityFilter () const { return 0; }
	RelationClutterFilter *getRelationFilter () const { return 0; }
	EventClutterFilter *getEventFilter () const { return 0; }

private:
	DefaultClutterFilter()
	{
        // This is commented out as it is expected behavior and so should not generate a warning.  According to Jessica:
        // "The clutter filter is only implemented for English and it only gets used in the ATEA system."
		//std::cerr << "<<<<<WARNING: Using unimplemented generic clutter filtering!>>>>>\n";
	}

};

class DefaultClutterFilterFactory: public ClutterFilter::Factory {
	virtual ClutterFilter *build() { return _new DefaultClutterFilter(); }
};

#endif
