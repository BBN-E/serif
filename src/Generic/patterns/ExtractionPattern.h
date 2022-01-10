// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EXTRACTION_PATTERN_H
#define EXTRACTION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <vector>

/** An abstract base class for EventPattern and RelationPattern, which define
  * similar sets of constraints.
  */
class ExtractionPattern: public Pattern {
public:
	~ExtractionPattern() {}

protected:
	ExtractionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	std::vector<Symbol> _types;
	std::vector<Pattern_ptr> _args;
	std::vector<Pattern_ptr> _optArgs;
	std::vector<Pattern_ptr> _blockedArgs;

	/*********************************************************************
	 * Overridden virtual methods
	 *********************************************************************/
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;

	/*********************************************************************
	 * Helper Methods
	 *********************************************************************/
	bool matchesType(Symbol type) const;
};

#endif
