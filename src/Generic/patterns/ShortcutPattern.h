// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SHORTCUT_PATTERN_H
#define SHORTCUT_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>

class ShortcutPattern: public Pattern {
private:
	explicit ShortcutPattern(const Symbol& shortcut) { setShortcut(shortcut); }
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ShortcutPattern, const Symbol&);
public:
	virtual std::string typeName() const { return "ShortcutPattern"; }
	void dump(std::ostream &out, int indent) const;
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
};

typedef boost::shared_ptr<ShortcutPattern> ShortcutPattern_ptr;

#endif
