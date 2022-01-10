// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGEX_PATTERN_H
#define REGEX_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/common/BoostUtil.h"
#include <boost/regex.hpp>


class RegexPattern: public LanguageVariantSwitchingPattern, public SentenceMatchingPattern, public ArgumentValueMatchingPattern
{
private:
	RegexPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RegexPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	// Sentence-level matchers
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Theory-level matchers
	PatternFeatureSet_ptr matchesMentionHead(PatternMatcher_ptr patternMatcher, const Mention *ment);
	PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *ment, bool fall_through_children);
	PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);
	PatternFeatureSet_ptr matchesValueMention(PatternMatcher_ptr patternMatcher, const ValueMention *valueMent);
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "RegexPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual bool allowFallThroughToChildren() const { return true; }

	bool containsOnlyTextSubpatterns() const;
private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	boost::wregex _regex_pattern;
	std::wstring _regex_string; // just for debugging

	/** A set of regular expressions that must *all* match somewhere in the sentence, if this 
	  * RegexpPattern has any chance of matching the sentence.  We check these first, to 
	  * filter out sentences that have no chance of matching, for efficiency.  (In the LearnIt
	  * project, we use a very large number of RegexpPatterns, and match them against a large
	  * number of sentences.  This optimization reduces runtime of the pattern matching stage
	  * by around 30-40%.) */
	std::vector<boost::wregex> _filter_regexps;
	void createFilterRegexps();

	/** The subpatterns that make up this regexp pattern. */
	std::vector<Pattern_ptr> _subpatterns;

	/** Mapping from subpatterns (by index) to the corresponding match group
	  * in the constructed regular expression.  I.e., if you want to get the
	  * text for _subpatterns[7], then you should look in match group number
	  * _subpatternToSubmatchMap[7]. */
	std::vector<int> _subpatternToSubmatchMap;

	bool _top_mentions_only; // default: false
	bool _allow_heads; // default: true
	bool _match_whole_extent; // default: false
	bool _add_spaces; // default: true

	/*********************************************************************
	 * Helper Methods
	 *********************************************************************/

	// Initialize _regex_pattern -- this must be done after shortcuts are replaced.
	void createRegex();

	// Helper methods to implement match methods.
	std::vector<PatternFeatureSet_ptr> matchImpl(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, int start_token, int end_token, bool multi_match);
	PatternFeatureSet_ptr matchesMentionImpl(PatternMatcher_ptr patternMatcher, const Mention *mention, int start_token, int end_token);
	PatternFeatureSet_ptr matchesValueMentionImpl(PatternMatcher_ptr patternMatcher, const ValueMention *valueMention, int start_token, int end_token);

	bool findOffsetsOfSubmatch(const boost::wssub_match& boostSubMatch, const boost::wssub_match& boostFullMatch, 
		int match_char_start, int &submatch_char_start, int &submatch_char_end) const;

	bool isAllWhitespace(std::wstring &str, int start_offset, int end_offset) const;

};
typedef boost::shared_ptr<RegexPattern> RegexPattern_ptr;

#endif
