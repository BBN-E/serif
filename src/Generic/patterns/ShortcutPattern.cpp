// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UnexpectedInputException.h"

void ShortcutPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "ShortcutPattern: " << getShortcut() << "\n";
}

Pattern_ptr ShortcutPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	SymbolToPatternMap::const_iterator it = refPatterns.find(getShortcut());
	if (it == refPatterns.end()) {
		std::ostringstream err;
		err << "Error replacing shortcut " << getShortcut() << ": reference pattern not found; id: " << getDebugID();
		throw UnexpectedInputException("ShortcutPattern::shortcutReplacement()", err.str().c_str());
	}
	return (*it).second;
}

