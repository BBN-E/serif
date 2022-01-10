// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENERICS_FILTER_H
#define GENERICS_FILTER_H

#include "Generic/theories/DocTheory.h"
#include <boost/shared_ptr.hpp>


class GenericsFilter {
public:
	/** Create and return a new GenericsFilter. */
	static GenericsFilter *build() { return _factory()->build(); }
	/** Hook for registering new GenericsFilter factories. */
	struct Factory { virtual GenericsFilter *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	/**
	 * Potentially set generic flags of entities of docTheory
	 */
	virtual void filterGenerics(DocTheory* docTheory) = 0;

	virtual ~GenericsFilter() {}

protected:
	/**
	 * Default constructor. Nothing to initialize.
	 */
	GenericsFilter() {}

private:
	static boost::shared_ptr<Factory> &_factory();

};

//#if defined(ENGLISH_LANGUAGE)
//	#include "English/generics/en_GenericsFilter.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/generics/ch_GenericsFilter.h"
////#elif defined(ARABIC_LANGUAGE)
////	#include "Arabic/generics/ar_GenericsFilter.h"
//#else
//	#include "Generic/generics/xx_GenericsFilter.h"
//#endif

#endif
