// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EXPANDER_H
#define EXPANDER_H

#include <boost/shared_ptr.hpp>
#include <Generic/PropTree/PropNode.h>

class DocPropForest;

/** Abstract base class for PropTree Expanders.  Each expander must define
  * the method expand().
  */
class PropTreeExpander {
public:
	PropTreeExpander() {}
	void expandForest(DocPropForest& docForest) const;
	virtual void expand(const PropNodes& pnodes) const = 0;
	virtual ~PropTreeExpander() {};

	typedef boost::shared_ptr<PropTreeExpander> ptr;
};


#endif

