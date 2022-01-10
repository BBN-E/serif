// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_GENERICS_FILTER_H
#define ch_GENERICS_FILTER_H

#include "Generic/generics/GenericsFilter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"

/**
 * Identify entities as generic, based on a set of syntactic 
 * and link-based rules.
 */
class ChineseGenericsFilter : public GenericsFilter {
private:
  	friend class ChineseGenericsFilterFactory;
public:
	void filterGenerics(DocTheory* docTheory) {};
private:
	ChineseGenericsFilter(){}
};

	
class ChineseGenericsFilterFactory: public GenericsFilter::Factory {
	virtual GenericsFilter *build() { return _new ChineseGenericsFilter(); } 
};
 
#endif
