// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_HEADFINDER_H
#define xx_HEADFINDER_H

#include "Generic/trainers/HeadFinder.h"


class GenericHeadFinder : public HeadFinder {
private:
	friend class GenericHeadFinderFactory;

	GenericHeadFinder() {}

public:
	int get_head_index() { return 0; }

};

class GenericHeadFinderFactory: public HeadFinder::Factory {
	virtual HeadFinder *build() { return _new GenericHeadFinder(); }
};


#endif

