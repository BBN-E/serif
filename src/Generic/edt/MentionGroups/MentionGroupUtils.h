// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_GROUP_UTILS_H
#define MENTION_GROUP_UTILS_H

#include "Generic/common/AttributeValuePair.h"

#include <set>
#include <vector>

class Symbol;
class LinkInfoCache;
class MentionGroup;

class MentionGroupUtils {
public:
	static std::set<Symbol> getSymbolValueSet(std::vector<AttributeValuePair_ptr> pairs);
	static std::set<int> getIntValueSet(std::vector<AttributeValuePair_ptr> pairs);

	static void addSymbolFeatures(std::vector<AttributeValuePair_ptr>& features, 
		Symbol extractorName, Symbol featureName, std::wstring value, LinkInfoCache& cache);

	static bool featureIsUniqueInDoc(Symbol extractorName, Symbol featureName, Symbol value,
		const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache); 
};

#endif
