// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef JABARI_TOKEN_MATCHER
#define JABARI_TOKEN_MATCHER

#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorInfo.h"
#include <vector>
#include <limits.h>
class TokenSequence;
class DocTheory;
class SentenceTheory;

/** A regexp-like class that searches a sequence of tokens to find 
* matches for a set of Jabari-style actor/agent patterns.  Each 
* such pattern specifies a sequence of (optionally stemmed) tokens 
* that maps to a foreign key identifier (either an AgentId or an 
* ActorId).  In addition, we record the foreign key identifier of 
* the Jabari pattern (either an AgentPatternId or an 
* ActorPatternId). */
template<typename ValueIdType, typename PatternIdType>
class JabariTokenMatcher: private boost::noncopyable {
public:

	/** Data structure used to record a single Jabari pattern match. */
	struct Match {
		/** The id of the pattern that matched. */
		PatternIdType patternId;

		/** The id of the actor or agent identified by the pattern
		* that matched. */
		ValueIdType id;

		/** The unique_code of the actor identified by this pattern,
		* or the agent_code of the agent identified. */
		Symbol code;

		/** The index of the first token in the token sequence that
		* matched (inclusive). */
		int start_token;

		/** The index of the last token in the token sequence that
		* matched (inclusive). */
		int end_token;

		/** The string length of the Jabari pattern that matched.  This
		* can be used as a feature when deciding between mutually-
		* exclusive pattern matches (generally longer patterns are
		* more reliable). */
		size_t pattern_strlen;

		/** Weight adjustment for this match; can be positive or 
		* negative.  Higher weights will tend to make this match
		* win out over other candidate matches. */ 
		float weight;

		/** True for acronym patterns (patterns that end in "="), false 
		* for all other matches.  We need to be extra careful with
		* acronym matches to make sure their use is justified. */
		bool isAcronymMatch;

		Match(ValueIdType id, PatternIdType patternId, Symbol code, int start_token, int end_token, size_t pattern_strlen, float weight, bool isAcronymMatch):
		id(id), patternId(patternId), code(code), start_token(start_token), end_token(end_token), pattern_strlen(pattern_strlen), weight(weight), isAcronymMatch(isAcronymMatch) {}
	};

	std::vector<std::vector<Match> > findAllMatches(const DocTheory *docTheory, int sentence_cutoff=INT_MAX);
	std::vector<Match> findAllMatches(const SentenceTheory *sentTheory);

	/** Search the given token sequence, and create a Match for each
	* pattern that matches the token sequence starting at the given
	* token index.  Add these new matches to the end of the "result"
	* vector.  */
	void match(const TokenSequence *toks, const Symbol* posTags, size_t start_token, std::vector<Match>& result);

	/** Search the given token sequence, and return a Match for each
	* pattern that matches the token sequence starting at the given
	* token index. */
	std::vector<Match> match(const TokenSequence *toks, const Symbol* posTags, size_t start_token);

	/** Add a new pattern to this matcher.  Patterns should be specified
	* using "Jabari style" pattern strings. */
	void addPattern(std::wstring jabariStylePattern,
		PatternIdType patternId, ValueIdType valueId, Symbol code, float weight);

	/** Add a new pattern to this matcher.  Patterns are simply actor names. 
	* They will be converted to "Jabari style pattern strings. */
	void addBBNActorPattern(std::wstring actorString,
		PatternIdType patternId, ValueIdType valueId, bool is_acronym, float confidence, bool requires_context);

	bool patternRequiresContext(PatternIdType patternID) {
		return _patterns_requiring_context.find(patternID) != _patterns_requiring_context.end();
	}

	/** Construct a new token matcher containing no patterns. 
	    Provide with actor patterns from BBN Actor DB if they have 
		already been read in. */
	JabariTokenMatcher(const char* kind, bool read_patterns=true, ActorInfo_ptr actorInfo=ActorInfo_ptr());

private:
	// Private implementation class (defined in JabariTokenMatcher.cpp)
	class Trie;
	boost::shared_ptr<Trie> _root;
	std::set<PatternIdType> _patterns_requiring_context;
	std::string _kind;
	size_t _size;
};

// Typedefs for the two intended instantiations of this template.  
// (If any other instantiations are added in the future, then 
// explicit template instantiation statements must be added to 
// JabariTokenMatcher.cpp)
typedef JabariTokenMatcher<ActorId, ActorPatternId> ActorTokenMatcher;
typedef JabariTokenMatcher<AgentId, AgentPatternId> AgentTokenMatcher;
typedef JabariTokenMatcher<CompositeActorId, ActorPatternId> CompositeActorTokenMatcher;
typedef boost::shared_ptr<ActorTokenMatcher> ActorTokenMatcher_ptr;
typedef boost::shared_ptr<AgentTokenMatcher> AgentTokenMatcher_ptr;
typedef boost::shared_ptr<CompositeActorTokenMatcher> CompositeActorTokenMatcher_ptr;
typedef ActorTokenMatcher::Match ActorMatch;
typedef AgentTokenMatcher::Match AgentMatch;
typedef CompositeActorTokenMatcher::Match CompositeActorMatch;


#endif

