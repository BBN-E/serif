// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/common/UTF8OutputStream.h"

TopLevelPFeature::TopLevelPFeature(Pattern_ptr pattern, const LanguageVariant_ptr& languageVariant)
: PatternFeature(pattern,languageVariant), _pattern_label(pattern->getID()) {}

TopLevelPFeature::TopLevelPFeature(Symbol pattern_label, const LanguageVariant_ptr& languageVariant)
: PatternFeature(Pattern_ptr(),languageVariant), _pattern_label(pattern_label) {}

TopLevelPFeature::TopLevelPFeature(Pattern_ptr pattern, Symbol pattern_label, const LanguageVariant_ptr& languageVariant) 
: PatternFeature(pattern,languageVariant), _pattern_label(pattern_label) {}

TopLevelPFeature::TopLevelPFeature(Pattern_ptr pattern)
: PatternFeature(pattern), _pattern_label(pattern->getID()) {}

TopLevelPFeature::TopLevelPFeature(Symbol pattern_label)
: PatternFeature(Pattern_ptr()), _pattern_label(pattern_label) {}

TopLevelPFeature::TopLevelPFeature(Pattern_ptr pattern, Symbol pattern_label) 
: PatternFeature(pattern), _pattern_label(pattern_label) {}

void TopLevelPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
	if (!_pattern_label.is_null()) {
		out << L"    <focus type=\"top_level_pattern\"";
		out << L" val0=\"" << _pattern_label << L"\"";
		out << L" />\n";
	}
}		

void TopLevelPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	if (!_pattern_label.is_null())
		elem.setAttribute(X_pattern_label, _pattern_label);
}

TopLevelPFeature::TopLevelPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_pattern_label))
		_pattern_label = elem.getAttribute<Symbol>(X_pattern_label);
	else 
		_pattern_label = Symbol();
}
