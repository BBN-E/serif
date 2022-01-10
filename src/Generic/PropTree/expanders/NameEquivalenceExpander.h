// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_EQUIV_EXPANDER_H
#define NAME_EQUIV_EXPANDER_H

#include "PropTreeExpander.h"

class NameEquivalenceExpander : public PropTreeExpander {
public:
	typedef std::map<std::wstring,double> NameSynonyms;
	typedef std::map<std::wstring,NameSynonyms> NameDictionary;

	NameEquivalenceExpander(const NameDictionary& equivNames, double threshold);
	void expand(const PropNodes& pnodes) const;
private:
	const NameDictionary& _equivNames;
	double _threshold;
};



#endif

