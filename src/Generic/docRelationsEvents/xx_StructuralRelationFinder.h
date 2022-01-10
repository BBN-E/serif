// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_STRUCTURAL_RELATION_FINDER_H
#define xx_STRUCTURAL_RELATION_FINDER_H

#include "Generic/docRelationsEvents/StructuralRelationFinder.h"
#include "Generic/theories/RelMentionSet.h"

class GenericStructuralRelationFinder : public StructuralRelationFinder {
private:
	friend class GenericStructuralRelationFinderFactory;

public:

	~GenericStructuralRelationFinder() {}

	RelMentionSet *findRelations(DocTheory* docTheory) {
		return _new RelMentionSet();
	}

	bool isActive() { return false; }

private:
	GenericStructuralRelationFinder() {}
};

class GenericStructuralRelationFinderFactory: public StructuralRelationFinder::Factory {
	virtual StructuralRelationFinder *build() { return _new GenericStructuralRelationFinder(); }
};


#endif


