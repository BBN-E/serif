// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEQ_EXPANDER_H
#define SEQ_EXPANDER_H


#include "PropTreeExpander.h"


class SequenceExpander : public PropTreeExpander {
public: 
	virtual void expand(const PropNodes& pnodes) const;
	virtual ~SequenceExpander() {};
protected:
	void addExpander(const PropTreeExpander::ptr& expander) {_expanders.push_back(expander);}
private:
	std::vector<PropTreeExpander::ptr> _expanders;
};


#endif

