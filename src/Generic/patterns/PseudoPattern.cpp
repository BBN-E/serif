// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/PseudoPattern.h"

PseudoPattern::PseudoPattern(float score, Symbol id, bool toplevel_return, PatternReturn_ptr patternReturn)
{
	_score = score;
	setID(id);
	_return = patternReturn;
	_toplevel_return = toplevel_return;
}


void PseudoPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "PseudoPattern:\n";
	if (!getID().is_null()) out << "  id = { " << getID() << " }\n";
	out << "  score = { " << getScore() <<  " }\n";
}

