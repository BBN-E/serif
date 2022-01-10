// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef BACKOFF_LEVEL_PFEATURE_H
#define BACKOFF_LEVEL_PFEATURE_H

#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "common/Symbol.h"

/** A pseudo-feature used to record a "backoff level". */
class BackoffLevelPFeature : public PseudoPatternFeature {
private:
	BackoffLevelPFeature(Symbol level): _level(level) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(BackoffLevelPFeature, Symbol);
public:	
	bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<BackoffLevelPFeature> f = boost::dynamic_pointer_cast<BackoffLevelPFeature>(other);
		return PatternFeature::simpleEquals(other) && f && f->getLevel() == getLevel();
	}
	Symbol getLevel() const { return _level; }
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		out << L"    <focus type=\"backoff_level\"";
		out << L" val"<< PatternFeature::val_backoff_level << "=\"" << getLevel() << L"\"";
		out << L" />\n";
	}
private:
	const Symbol _level;
};

#endif
