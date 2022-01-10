// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STRUTURAL_RELATION_FINDER_H
#define STRUTURAL_RELATION_FINDER_H

#include <boost/shared_ptr.hpp>
#include "Generic/theories/RelMentionSet.h"

class DocTheory;

class StructuralRelationFinder {
public:
	/** Create and return a new StructuralRelationFinder. */
	static StructuralRelationFinder *build() { return _factory()->build(); }
	/** Hook for registering new StructuralRelationFinder factories */
	struct Factory { virtual StructuralRelationFinder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

    virtual ~StructuralRelationFinder() {}

	/**
	 * This does the work. It adds relations to the DocTheory
     **/
	virtual RelMentionSet *findRelations(DocTheory *docTheory) = 0;
	virtual bool isActive() = 0;

protected:
	StructuralRelationFinder() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/docRelationsEvents/en_StructuralRelationFinder.h"
//#else
//	#include "Generic/docRelationsEvents/xx_StructuralRelationFinder.h"
//#endif

#endif
