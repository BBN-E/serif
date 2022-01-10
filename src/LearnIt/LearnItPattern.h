#pragma once

#include <map>
#include <string>
#include <vector>
#include <set>
#include "boost/noncopyable.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/algorithm/string.hpp"
#include "Generic/common/bsp_declare.h"
#include "SlotFillerTypes.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignmentTable.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "Generic/patterns/PatternMatcher.h"

class DocTheory;
class SentenceTheory;    // defined in "Generic/theories/SentenceTheory.h"
BSP_DECLARE(PatternSet)   // defined in "Generic/patterns/PatternSet.h"
BSP_DECLARE(PatternFeatureSet) // defined in "distill-generic/features/PatternFeatureSet.h"
BSP_DECLARE(Target) // defined in "itlearn/Target.h"
BSP_DECLARE(LearnItPattern)
/**
 * A Brandy-based pattern that can be used to search for relations and concepts.  This
 * C++ interface to patterns is read-only, and provides three attributes:
 *
 *   - A pointer to the target that this pattern is intended to find.
 *   - A unique name.
 *   - A Brandy pattern string.
 *
 * The Brandy pattern string used is expected to be defined such that the pattern
 * features returned by a match will include return expressions named SLOTX and SLOTY.
 * 
 * Patterns are immutable, and should always be accessed via shared pointers.
 */
class LearnItPattern: private boost::noncopyable
{
public:
	/** Construct a new pattern, and return a shared pointer to that pattern. */
	static LearnItPattern_ptr create(Target_ptr target, const std::wstring& name, 
		const std::wstring& pattern_string, bool active, bool rejected,
		float precision, float recall, bool multi=true, 
		const LanguageVariant_ptr& language=LanguageVariant::getLanguageVariant(),
		const std::wstring& keywordString=std::wstring()) 
	{ 
		return LearnItPattern_ptr(new LearnItPattern(target, name, pattern_string, language, active, 
										rejected, precision, recall, multi, keywordString)); 
	}

	/** Match this pattern against a given sentence, and return a vector containing one 
	  * PatternFeatureSet for each match. */
	std::vector<PatternFeatureSet_ptr> applyToSentence(const SentenceTheory &sent_theory);
	std::vector<PatternFeatureSet_ptr> applyToSentence(const DocTheory *docTheory, 
											const SentenceTheory &sent_theory);
	std::vector<PatternFeatureSet_ptr> applyToSentence(const AlignedDocSet_ptr doc_set, 
											const SentenceTheory &sent_theory);
											
	void makeMatcher(const AlignedDocSet_ptr doc_set);
	void makeMatcher(const DocTheory* doc_set);

	/** Return a pointer to the target that this pattern is intended to find. */
	Target_ptr getTarget() const { return _target; }

	/** Return the name for this pattern.  There is a one-to-one correspondence between 
	  * pattern names and pattern strings.  Currently, the Pattern name is the Python
	  * expression used to create this pattern object. */
	const std::wstring& getName() const { return _name; }

	/** Return the Brandy pattern string for this pattern.  This s-expression is used to create
	  * a PatternSet. */
	const std::wstring& getPatternString() const { return _pattern_string; }
	
	LanguageVariant_ptr language() const { return _language; }
	bool active() const { return _active; }
	float precision() const { return _precision; }
	float recall() const { return _recall; }
private:
	// Use the create() factory method to create new patterns.
	LearnItPattern(Target_ptr target, const std::wstring& name, 
		const std::wstring& pattern_string, const LanguageVariant_ptr& language, bool active, bool rejected, 
		float precision, float recall, bool multi, const std::wstring& keywordString);
	
	bool preFilterPattern(const SentenceTheory &sent_theory) const;
	
	Target_ptr _target;
	std::wstring _name;  
	std::wstring _pattern_string;
	LanguageVariant_ptr _language;
	bool _active;
	bool _rejected;
	float _precision;
	float _recall;
	bool _multi;
	std::vector<std::wstring> keywords;
	PatternMatcher_ptr _matcher;

	/** The pre-compiled Brandy pattern corresponding to _pattern_string.*/
	PatternSet_ptr _query_pattern_set;
};
