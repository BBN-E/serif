// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ZONED_RELATION_FINDER_H
#define ZONED_RELATION_FINDER_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/RelMentionSet.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED ZonedRelationFinder {
public:
	// Create and return a new ZonedRelationFinder
	static ZonedRelationFinder* build() { return _factory()->build(); }

	// Hook for registering new factories
	struct Factory { 
		virtual ~Factory() {}
		virtual ZonedRelationFinder* build() = 0; 
	};		

    virtual ~ZonedRelationFinder() {}

	// Entry point: finds relations in a document's regions
	virtual RelMentionSet* findRelations(DocTheory* docTheory) = 0;

protected:
	ZonedRelationFinder() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

#endif
