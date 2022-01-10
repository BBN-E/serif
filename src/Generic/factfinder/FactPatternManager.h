// Copyright (c) 2012 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef FACT_PATTERN_MANAGER_H
#define FACT_PATTERN_MANAGER_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include "Generic/common/Symbol.h"

class PatternSet;
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;

class FactPatternManager {
public:
	FactPatternManager(std::string factPatternList);
	~FactPatternManager();

	size_t getNFactPatternSets() const { return _factPatternSets.size(); }
	PatternSet_ptr getFactPatternSet(size_t i) const { return _factPatternSets[i]; }
	Symbol getEntityTypeSymbol(size_t i) const { return _entityTypeSymbol[i]; }
	Symbol getExtraFlagSymbol(size_t i) const { return _extraFlagSymbol[i]; }

private:
	std::vector<PatternSet_ptr> _factPatternSets;
	std::vector<Symbol> _entityTypeSymbol;
	std::vector<Symbol> _extraFlagSymbol;
};

#endif

