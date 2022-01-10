// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PSEUDO_PATTERN_H
#define PSEUDO_PATTERN_H

#include "common/Symbol.h"
#include "Generic/patterns/Pattern.h"

/** A class that acts like a pattern, but does not actually match anything.  
  * This is used by the NLP Demo to store facts that are found by pattern 
  * matching into the deatabase in a uniform way. 
  *
  * Note: this approach may need to be revised, since pattern types are no
  * longer recorded using a member variable.
  */
class PseudoPattern: public Pattern {
private:
	PseudoPattern(float score, Symbol id, bool toplevel_return, PatternReturn_ptr patternReturn);
public:
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(PseudoPattern, float, Symbol, bool, PatternReturn_ptr);

	virtual std::string typeName() const { return "PseudoPattern"; }

	void dump(std::ostream &out, int indent = 0) const;
};

typedef boost::shared_ptr<PseudoPattern> PseudoPattern_ptr;

#endif

