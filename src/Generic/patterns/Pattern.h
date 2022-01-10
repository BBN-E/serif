// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_H
#define PATTERN_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <vector>
#include <map>
#include <set>
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternWordSet.h"
#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/common/InternalInconsistencyException.h"

// Forward declaratations

class Sexp;
BSP_DECLARE(EntityLabelPattern);
BSP_DECLARE(PatternMatcher);
BSP_DECLARE(PatternFeatureSet);
BSP_DECLARE(Pattern);

/** Abstract base class for patterns.
  *
  */
class Pattern: private boost::noncopyable, public boost::enable_shared_from_this<Pattern> {
public:	
	virtual ~Pattern() {};

	/** Hash mapping from entity labels (Symbols) to corresponding Entity Label Patterns */
	typedef Symbol::HashMap<EntityLabelPattern_ptr> EntityLabelPatternMap;

	/** Parse the given s-expression, and return the pattern that it describes.
	  *
	  * @param entityLabels The set of entity labels.  This is used by MentionPattern to
	  *                     check that the user doesn't try to require or block a 
	  *                     non-existent label.
	  *
	  * @param wordSets     A mapping from WordSet names to the actual WordSets.
	  */
	static boost::shared_ptr<Pattern> parseSexp(Sexp *sexp, const Symbol::HashSet &entityLabels,
		const PatternWordSetMap& wordSets, bool allow_shortcut_label=false);

	/** Return a string name for this pattern.  This should be used for display/serialization
	  * only; to check the type of a pattern, use boost::dynamic_pointer_cast. */
	virtual std::string typeName() const = 0;

	/** A value denoting that no score has been specified. */
	const static float UNSPECIFIED_SCORE; // =-1
	const static int   UNSPECIFIED_SCORE_GROUP; // =-1

	/** Return the shortcut name that can be used to refer to this pattern,
	  * or Symbol() if there is no shortcut name for this pattern. */
	Symbol getShortcut() const { return _shortcut; }

	/** Return the PatternReturn for this pattern. */
	PatternReturn_ptr getReturn() const { return _return; }

	/** Add the PatternReturn of this pattern, plus any patterns
	  * it contains (directly or indirectly), to the given vector.
	  *
	  * @param output PatternReturnVecSeq for a given pattern
	  *
	  * @author afrankel@bbn.com
	  * @date 2010.10.18
	  **/
	virtual void getReturns(PatternReturnVecSeq & output) const;

	/** Return the label of this pattern's PatternReturn. Returns an empty symbol if
	  * either (a) it's an old-style PatternReturn with a label but the label has not been set
	  * or (b) it's a new-style PatternReturn that uses a map rather than a label. 
	  * It has a somewhat lengthy name to distinguish it from ReturnPatternFeature::getReturnLabel(). */
	Symbol getReturnLabelFromPattern() const;

	/** Returns true if the PatternReturn is either
	  * (a) an old-style PatternReturn with a label that has not been set
	  * or (b) a new-style PatternReturn that uses a map rather than a label. */
    bool returnLabelIsEmpty() const;

	/** Return the number of value pairs, which will be zero unless (a) a new-style PatternReturn map
	 *  is being used and (b) it contains at least one pair. */
	size_t getNReturnValuePairs() const;

	/** Return true if this pattern is a top-level return pattern.  This is 
	  * typically indicated by including the atom "toplevel-return" in the
	  * s-expression for the pattern. */
	bool isReturnValueTopLevel() const { return _toplevel_return; }

	/** Write a description of this pattern to the given output stream.  This
	  * is intended for debugging purposes only. */
	virtual void dump(std::ostream &out, int indent = 0) const = 0;

	/** Calls dump(). */
	std::string toDebugString(int indent) const;

	Symbol getID() const { return _id; }

	/** Returns an ID if there is one, otherwise a shortcut if there is one, otherwise a type name in brackets. */
 	std::string getDebugID() const;

	// This method is redefined by CombinationPattern, IntersectionPattern, and UnionPattern.
	virtual Symbol getFirstValidID() const { return _id; }

	// This method is called by getFirstValidID() for CombinationPattern, IntersectionPattern, and UnionPattern.
	virtual Symbol getFirstValidIDForCompositePattern(const std::vector<Pattern_ptr> & pattern_vec) const;

	float getScore() const { return _score; }

	int getScoreGroup() const { return _score_group; }

	typedef Symbol::HashMap<boost::shared_ptr<Pattern> > SymbolToPatternMap;

	/** Expand any shortcuts in this pattern, and return the result.   
	  * Typically, this return value will be the pattern itself (which is
	  * modified in-place); however, in some cases (such as shortcut 
	  * patterns themselves), the returned value may be a different 
	  * pattern. */
	virtual boost::shared_ptr<Pattern> replaceShortcuts(const SymbolToPatternMap &refPatterns) {
		return shared_from_this(); }

	/** Cast this pattern to the given type.  If it does not actually have the
	  * appropriate type, then throw an InternalInconsistencyException.  If you
	  * are not sure whether the pattern should have the given type, then you
	  * should use boost::dynamic_pointer_cast<> instead, and check if the result
	  * is zero. */
	template<class PatternType>
	boost::shared_ptr<PatternType> castTo() {
		boost::shared_ptr<PatternType> ptr = boost::dynamic_pointer_cast<PatternType>(shared_from_this());
		if (!ptr)
			throw InternalInconsistencyException("Pattern::castTo", "Pattern does not have the expected type!");
		return ptr;
	}

	template <typename PatternType> 
	static void registerPatternType(Symbol typeName) {
		ensurePatternTypeIsNotRegistered(typeName);
		_additionalPatternTypeFactories()[typeName] = boost::shared_ptr<PatternFactory>(_new PatternFactoryFor<PatternType>());
	}

protected: /******** Helper Functions for Subclasses *********/

	/** Report an error encountered while parsing an sexpr into a Pattern. Use this
	 *  version (which has access to the pattern type) rather than throwErrorStatic
	 *  unless you're in a static method. */
	void throwError(const Sexp *sexp, const char *reason);
	/** Use this version if you're in a static method. */
	static void throwErrorStatic(const Sexp *sexp, const char *reason);

	/** If the given pointer points to a shortcut, then replace it with the
	  * target of that shortcut instead.  If the pattern does not have the
	  * type indicated by the template parameter, then report an error.  (If
	  * the pointer is NULL, then it is left unchanged.) 
	  *
	  * Note: the first argument is a reference, which gets modified.
	  **/
	template<class PatternType>
	void replaceShortcut(boost::shared_ptr<Pattern>& pattern, const SymbolToPatternMap &refPatterns) {
		if (pattern) {
			boost::shared_ptr<Pattern> replacement = pattern->replaceShortcuts(refPatterns);
			if (!boost::dynamic_pointer_cast<PatternType>(replacement))
				reportBadShortcutType(pattern, replacement);
			pattern = replacement;
		}
	}

	/** Given an sexpression that contains a list of word symbols (possibly including 
	  * wordset symbols), fill the given set (listOfWords) with those words.  Wordsets
	  * are expanded to the words they contain.  If skipFirstChild is true, then the
	  * first child of the sexp will not be treated as a word.  (This is the default,
	  * since usually the first child of the sexp indicates what kind of words are being
	  * listed).  If errorOnUnknownWordSetSymbol is true, and the list includes a symbol
	  * that looks like a wordset name (i.e., that contains at least one capitalized 
	  * letter), then raise an exception; otherwise, just assume that it should be used
	  * as a word. */
	Symbol createMultiWordSymbol(Sexp *sexp);
	void fillListOfWords(Sexp *sexp, const PatternWordSetMap& wordSets, std::set<Symbol>& listOfWords, 
	                     bool skipFirstChild=true, bool errorOnUnknownWordSetSymbol=true);
	void fillListOfWordsWithWildcards(Sexp *sexp, const PatternWordSetMap& wordSets, std::set<Symbol>& words, std::set<Symbol>& wordPrefixes);
	void addToWordArrayWithWildcards(const Symbol &element, std::set<Symbol>& words, std::set<Symbol>& wordPrefixes);
	bool wordMatchesWithPrefix(Symbol word, std::set<Symbol>& words, std::set<Symbol>& wordPrefixes, bool return_true_if_empty);


	/** Given an sexpression that contains a list of symbols, fill the given set 
	  * (listOfSymbols) with those symbols.  If skipFirstChild is true, then the
	  * first child of the sexp will not be treated as a word.  */
	void fillListOfSymbols(Sexp *sexp, std::set<Symbol>& listOfSymbols, bool skipFirstChild=true);

protected:
	Pattern();
	Pattern(boost::shared_ptr<Pattern> pattern);

	// Helper methods for building pattern constructors.  The subclass 
	// constructor should call initializeFromSexp(), which will call the
	// virtual methods initializeFromAtom and initializeFromSubexpression
	// on each atom/subexpression in sexp.  The default implementations 
	// of these methods handle pattern expressions that are common to all
	// patterns (e.g., score and id).  Subclasses should therefore call their
	// parent's implementation.  These two methods should return true if
	// they successfully handled the given expression, and false otherwise.
	void initializeFromSexp(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, 
		const PatternWordSetMap& wordSets);

	/** Set the shortcut name that can be used to refer to this pattern.
	  * This method may only be called once per Pattern. */
	void setShortcut(const Symbol& shortcut);

	/** Set the shortcut name that can be used to refer to this pattern.
	  * This method may only be called once per Pattern. */
	void setID(const Symbol& id);

	void logFailureToInitializeFromAtom(const Sexp * atomSexp);
	void logFailureToInitializeFromChildSexp(const Sexp * childSexp);

	// Should any of these become private?
	float _score;
	int _score_group;
	ScoringFactory::ScoreFunctionPtr _scoringFunction;

	// Our return subpattern (if we have one)
	PatternReturn_ptr _return;
	// Is our return a top-level return?
	bool _toplevel_return;

	/** Return a feature set containing a single GenericPFeature pointing to
	  * this pattern.  If this pattern has an id, then the feature set will also
	  * contain a TopLevelPFeature pointing to this pattern.  The feature
	  * set's score will be set to _score. */
	PatternFeatureSet_ptr makeEmptyFeatureSet();

	/** If this pattern has an ID, then add a new TopLevelPFeature for it
	  * to the given feature set.*/
	void addID(PatternFeatureSet_ptr pfs);

	// [XX] Add better docs than this!
	// Utility function used in PropPattern, EventPattern, and RelationPattern.
	// argsRequired specified if all of the arguments in the 'argPatterns' vector are required or optional.
	PatternFeatureSet_ptr fillAllFeatures(
			std::vector<std::vector<PatternFeatureSet_ptr> > req_i_j_to_sfs, std::vector<std::vector<PatternFeatureSet_ptr> > opt_i_j_to_sfs,
			PatternFeatureSet_ptr allFeatures, 
			std::vector<Pattern_ptr> reqArgPatterns, std::vector<Pattern_ptr> optArgPatterns,
			bool require_one_to_one_argument_mapping = false,
			bool require_all_arguments_to_match_some_pattern = false,
			bool allow_many_to_many_mapping = false) const;
private:

	void reportBadShortcutType(boost::shared_ptr<Pattern> shortcut, boost::shared_ptr<Pattern> replacement);

protected:
	// Symbols used by multiple patterns
	const static Symbol scoreSym;
	const static Symbol scoreGroupSym;
	const static Symbol scoreFnSym;
	const static Symbol shortcutSym;
	const static Symbol singleMatchOverrideSym;
	const static Symbol idSym;
	
	// An override identifier for this pattern (force multi-match to call single-match instead)
	bool _force_single_match_sentence;
public: // hmm..
	const static Symbol returnSym;
	const static Symbol topLevelReturnSym;

private:
	// A shortcut name that can be used to refer to this pattern.
	Symbol _shortcut;

	// An identifier for this pattern.
	Symbol _id;
	
	static bool debug_props;

	// Helper method used by fillAllFeatures()
	std::pair<std::pair<int,float>,std::vector<int> > getBestScore(std::vector<std::vector<PatternFeatureSet_ptr> > i_j_to_sfs, 
															  int max_i, int max_j, std::set<int> used_j_indices, 
															  std::vector<Pattern_ptr> argPatterns, int numOptArgsAtFront,
															  bool allow_many_to_many_mapping) const;

	//======================================================================
	// Support for addding new Pattern subtypes (see registerPatternType()).
	struct PatternFactory {
		virtual Pattern_ptr buildPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) = 0;
	};
	typedef boost::shared_ptr<PatternFactory> PatternFactory_ptr;
	template<typename PatternSubtype>
	struct PatternFactoryFor: public PatternFactory {
		Pattern_ptr buildPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
			return boost::make_shared<PatternSubtype>(sexp, entityLabels, wordSets);
		}
	};
	static Symbol::HashMap<PatternFactory_ptr>& _additionalPatternTypeFactories();
	static void ensurePatternTypeIsNotRegistered(Symbol typeName);
};

typedef boost::shared_ptr<Pattern> Pattern_ptr;

/*********************************************************************
 * Language-Variant Switching Abstract Base class for patterns
 *********************************************************************/
class LanguageVariantSwitchingPattern : public Pattern {
public:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

protected:
	LanguageVariant_ptr _languageVariant;
};



#endif
