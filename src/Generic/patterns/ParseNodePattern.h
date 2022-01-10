// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_NODE_PATTERN_H
#define PARSE_NODE_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/BoostUtil.h"
#include <vector>


/** A pattern used to search for parse nodes that satisfy a given set of
  * criteria.
  */
class ParseNodePattern: public Pattern,
	public SentenceMatchingPattern, public ParseNodeMatchingPattern
{
private:
	ParseNodePattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ParseNodePattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:
	/** Sentence-level matchers */
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	/** Theory object matchers */
	PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "ParseNodePattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;

private:
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	std::vector<Symbol> _tags;
	std::vector<Symbol> _blockTags;
	std::set<Symbol> _headwords;
	std::set<Symbol> _headwordPrefixes;
	std::set<Symbol> _blockedHeadwords;
	std::set<Symbol> _blockedHeadwordPrefixes;
	std::vector<Pattern_ptr > _premods;
	std::vector<Pattern_ptr > _optionalPremods;
	Pattern_ptr _head;
	std::vector<Pattern_ptr > _postmods;
	std::vector<Pattern_ptr > _optionalPostmods;
	Pattern_ptr _mentionPattern;
	Pattern_ptr _regexPattern;

	/*********************************************************************
	 * Helper Methods
	 *********************************************************************/
	PatternFeatureSet_ptr matchesSentenceRecursive(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);
	void multiMatchesSentenceRecursive(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);
	std::vector<PatternFeatureSet_ptr> _multiMatchReturnVector;

	void addToVector(std::vector<Symbol> &wordList, Symbol wordOrWordSet, const PatternWordSetMap& wordSets);

	// Raise an exception if the given pattern is not a MentionPattern, 
	// ValueMentionPattern, or SynNodePattern.
	void checkPatternType(Pattern_ptr pattern) const;
};

typedef boost::shared_ptr<ParseNodePattern> ParseNodePattern_ptr;

#endif
