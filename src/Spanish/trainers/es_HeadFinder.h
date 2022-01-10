// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_HEADFINDER_H
#define ES_HEADFINDER_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/trainers/HeadFinder.h"
#include <boost/shared_ptr.hpp>
#include <vector>

class AncoraHeadRule;

class SpanishHeadFinder : public HeadFinder{
private:
	friend class SpanishHeadFinderFactory;

public:
	int get_head_index();

private:
	SpanishHeadFinder();
	~SpanishHeadFinder();
	typedef boost::shared_ptr<AncoraHeadRule> AncoraHeadRule_ptr;
	std::vector<AncoraHeadRule_ptr> _headRules;
	bool _verbose;
};

class SpanishHeadFinderFactory: public HeadFinder::Factory {
	virtual HeadFinder *build() { return _new SpanishHeadFinder(); }
};

#endif
