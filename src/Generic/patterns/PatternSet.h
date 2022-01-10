// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PATTERN_SET_H
#define PATTERN_SET_H

#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/Attribute.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/EntityLabelPattern.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

class PatternFeatureSet;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;
typedef boost::shared_ptr<Pattern> Pattern_ptr;

/** A collection of patterns, along with supporting data structures, that
  * can be used to search documents.  Each PatternSet consists of:
  *
  *   - One or more "top-level patterns".  These are the primary patterns
  *     that define what is returned by the PatternSet as a whole.
  *
  *   - Zero or more "reference patterns", which are used to define pattern
  *     macros: each reference pattern associates a shortcut name with a
  *     pattern.  Other patterns can then use that shortcut name to include
  *     a copy of the reference pattern.  Reference patterns may refer to
  *     one another, though circular references are not supported.  
  *
  *   - Zero or more "word-sets", which are used to define lexical macros: 
  *     each WordSet associates a label with an unordered set of words (or 
  *     word regexs).  Patterns can then use that wordset label rather than
  *     listing the lexical items out inline.
  *
  *   - Zero or more "entity-label patterns".  These are patterns that are
  *     run on each sentence in the document before the top-level patterns
  *     are run.  Their job is to assign labels to interesting entities in
  *     the document, which can then be used to simplify the top-level 
  *     patterns.  For example, an entity-label pattern might find all
  *     entities that appear to refer to defendants.
  *
  *   - Zero or more "document-level patterns".  These are matched against
  *     each sentence in the document before the top-level patterns are run
  *     (but after entity-label patterns are run).  If a document-level 
  *     pattern matches against any sentence in the document, then the
  *     document-level pattern is said to have fired.  The DocumentPattern
  *     pattern subclass can be used to check whether a given document-level
  *     pattern has fired (using its pattern ID).
  *
  *   - Zero or more "backoff levels".  [XX] document what these mean!
  *
  *   - An optional set of blocked propositions.  This overrides the default
  *     set of blocked propositions.
  */
class PatternSet: private boost::noncopyable {

private: // Use boost::make_shared<PatternSet> to construct PatternSets.

	/** Construct an empty PatternSet, containing no patterns. */
	PatternSet();
	BOOST_MAKE_SHARED_0ARG_CONSTRUCTOR(PatternSet);

	/** Construct a PatternSet containing the patterns specified by the
	  * given s-expression. */
	PatternSet(Sexp *sexp);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternSet, Sexp*);

	/** Construct a PatternSet by reading an Sexp from the given file,
	  * with quotes, hash comments, and macro expansion enabled (with
	  * the include_once flag set to true.) */
	PatternSet(const char* filename, bool encrypted=false);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternSet, const char*);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(PatternSet, const char*, bool);

	/** Construct a PatternSet containing the supplied pattern as its sole
		top level pattern, with nothing else */
	PatternSet(Pattern_ptr pat);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternSet, Pattern_ptr);
public:
	~PatternSet() {}

	bool validForPreParseMatching() const;

	/*********************************************************************
	 * Accessors
	 *********************************************************************/

	// Name
	Symbol getPatternSetName() const;

	// Top-level Patterns
	size_t getNTopLevelPatterns() const;
	Pattern_ptr getNthTopLevelPattern(size_t n) const;
	
	// Entity label patterns
	size_t getNEntityLabelPatterns() const;
	EntityLabelPattern_ptr getNthEntityLabelPattern(size_t n);
	
	// Document patterns
	size_t getNDocPatterns() const;
	Pattern_ptr getNthDocPattern(size_t n) const;
	
	// Backoff levels.
	int getNBackoffLevels() const;
	Symbol getNthBackoffLevel(int n) const;

	// Entity labels.
	const Symbol::HashSet &getEntityLabelSet();

	// Return true if the given proposition status is blocked.
	void addBlockedPropositionStatus(PropositionStatusAttribute status);

	// Tell this pattern set to ignore proposition status when blocking patterns
	void doNotBlockAnyPatternsBasedOnStatus() { _do_not_block_patterns = true; }

	// Return true if the given proposition status is blocked.
	bool isBlockedPropositionStatus(PropositionStatusAttribute status) const;

	// Return true if all features should be kept when feature sets 
	// are combined (rather than just keeping the feature set with the
	// highest score);
	bool keepAllFeaturesWhenCombiningFeatureSets() const { return _keep_all_features; }
	void getPatternReturns(IDToPatternReturnVecSeqMap & output_map, bool dump) const;

private:
	// The name of this PatternSet.
	Symbol _patternSetName;
	bool _do_not_block_patterns;

	// The patterns that define this pattern set.
	std::vector<Pattern_ptr> _topLevelPatterns;
	std::vector<Pattern_ptr> _docPatterns;
	std::vector<EntityLabelPattern_ptr> _entityLabelPatterns;

	// The backoff levels (what are these?)
	std::vector<Symbol> _backoffLevels;

	// The reference patterns, wordsets, and entity labels.  Strictly
	// speaking, we don't need to keep these after we're done
	// constructing the PatternSet, since we expand shortcuts and
	// wordsets in the PatternSet constructor.  But it might be
	// helpful to keep them around for debugging; and they would also
	// be useful if we add methods that can be used to add patterns to
	// an existing PatternSet.
	Pattern::SymbolToPatternMap _referencePatterns;
	PatternWordSetMap _wordSets;
	Symbol::HashSet _entityLabels;

	std::set<PropositionStatusAttribute> _blockedPropositionStatuses;
	static const std::set<PropositionStatusAttribute> &getDefaultBlockedPropositionStatuses();


	/** If true, then when feature sets are combined by combineSnippets(), 
	  * the features from the combined featuresets should be merged.  This
	  * is only done for ...
	*/
	bool _keep_all_features;

	// [XX] NOT CURRENTLY IMPLEMENTED!!
	//control whether all proposition and mention matches for a given pattern in a given sentence are added to a single SFS (default), 
	//or if each match gets its own SFS (=true). The second behavior is more consistent with Relation and Event matching
	bool _single_match_for_ment_prop; 

	/*********************************************************************
	 * Private Helper Methods
	 *********************************************************************/

	/** Helper for constructors */
	void initializeFromSexp(Sexp *sexp);

	/** Parse each pattern in the given sexp, and use them to populate the given
	  * vector 'patterns'.  It is assumed that the first element of the sexp 
	  * should be ignored, and all subsequent elements are patterns. */
	void loadPatterns(Sexp *sexp, std::vector<Pattern_ptr> &patterns);

	/** Report an error encountered while parsing an sexpr into a Pattern. */
	static void throwParseError(const Sexp *sexp, const char *reason);
};

typedef boost::shared_ptr<PatternSet> PatternSet_ptr;

#endif

