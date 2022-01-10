// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UR_HEADFINDER_H
#define UR_HEADFINDER_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/trainers/HeadFinder.h"
#include <boost/shared_ptr.hpp>
#include <vector>


class UrduHeadFinder : public HeadFinder{
private:
	friend class UrduHeadFinderFactory;

public:
	int get_head_index();

private:
	UrduHeadFinder();
	~UrduHeadFinder();
};

class UrduHeadFinderFactory: public HeadFinder::Factory {
	virtual HeadFinder *build() { return _new UrduHeadFinder(); }
};

#endif
