// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_GENERICS_FILTER_H
#define xx_GENERICS_FILTER_H

#include "Generic/generics/GenericsFilter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/SessionLogger.h"

// Unimplemented generics filter. This does nothing.
// It displays an error message upon initialization.

class DefaultGenericsFilter : public GenericsFilter {
private:
  	friend class DefaultGenericsFilterFactory;
public:

	void filterGenerics(DocTheory* docTheory) { }
private:
	/**
	 * WARNING: A DefaultGenericsFilter has not been defined if this constructor is being used.
	 */
	DefaultGenericsFilter()
	{
		SessionLogger::warn("unimplemented_class") << "<<<<<WARNING: Using unimplemented generic generics filtering!>>>>>\n";
	}

};

class DefaultGenericsFilterFactory: public GenericsFilter::Factory {
	virtual GenericsFilter *build() { return _new DefaultGenericsFilter(); } 
};

#endif
