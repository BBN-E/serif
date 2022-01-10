// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XDOC_CLIENT_H
#define XDOC_CLIENT_H

class GenericXDocClient {
public:
	GenericXDocClient() {}
    
	/**
	 * This does the work. It adds relations to the DocTheory
     **/
	virtual void performXDoc(DocTheory *docTheory) = 0;
};

// language-specific includes determine which implementation is used
#ifdef EMC
	#include "XDocClient/emc_XDocClient.h"
#else
	#include "Generic/xdoc/xx_XDocClient.h"
#endif

#endif
