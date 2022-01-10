// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/ICEWSDB.h"
#include "Generic/actors/AWAKEDB.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/JabariTokenMatcher.h"
#include "Generic/actors/AWAKEActorInfo.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/SymbolUtilities.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_array.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include <vector>
#include <boost/scoped_ptr.hpp>

namespace {
	Symbol NNP_TAG(L"NNP");
	Symbol APOS_SYM(L"'");
	Symbol APOS_S_SYM(L"'s");
	Symbol DASH_SYM(L"-");
	Symbol COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM(L"COMPOSITE_ACTOR_IS_PAIRED_ACTOR");
	Symbol BLOCK_ACTOR_SYM(L"BLOCK_ACTOR");
}

// Should we use a memory pool for trie objects?  We create around
// 220k of them.

/** "JabariTokenMatcher::Trie" class: a trie used to compactly encode
  * and efficiently search the pattern set.  Each trie node contains a 
  * MatchInfo, recording the set of patterns (if any) that match at
  * that trie node.  In addition, it contains three sets of pointers 
  * to "next-token" tries: one that is used for exact string matches;
  * one that is used for "normal" matches -- i.e., case-insensitive 
  * matches where plural/passive stemming is allowed; and one that is
  * used for prefix matches. */
template<typename ValueIdType, typename PatternIdType>
class JabariTokenMatcher<ValueIdType, PatternIdType>::Trie {
public:
	struct MatchIds {
		ValueIdType valueId;
		PatternIdType patternId;
		Symbol code;
		float weight;
		MatchIds(ValueIdType valueId, PatternIdType patternId, Symbol code, float weight)
			: valueId(valueId), patternId(patternId), code(code), weight(weight) {}
	};
	struct MatchInfo {
		std::vector<MatchIds> match_ids;
		size_t pattern_strlen;
		MatchInfo(): pattern_strlen(0) {}
	};
	static void stripPunctuation(bool value) { _strip_punctuation() = value; }
private:
	boost::scoped_ptr<MatchInfo> _matchInfo;
	typedef boost::shared_ptr<Trie> Trie_ptr;
	typedef std::pair<Symbol, Trie_ptr> SymbolTriePair;

	// This is used for "normal" matches (case-insensitive, 
	// and plural/passive stemming is allowed).
	boost::scoped_ptr<Symbol::HashMap<Trie_ptr> > _next_by_normal_match;

	// This is used for acronym match (at most one should match)
	boost::scoped_ptr<Symbol::HashMap<Trie_ptr> > _next_by_acronym_match;

	// This is used for exact match (at most one should match)
	boost::scoped_ptr<Symbol::HashMap<Trie_ptr> > _next_by_exact_match;

	// This is used for prefix match (more than one can match)
	boost::scoped_ptr<std::vector<SymbolTriePair> > _next_by_prefix_match;

	bool _skip_posessives;

	static bool& _strip_punctuation() {
		static bool v = 0;
		return v;
	}
public:
	typedef enum {NORMAL_MATCH, PREFIX_MATCH, ACRONYM_MATCH, EXACT_MATCH} MatchType;

	void addMatch(const MatchIds &match_ids, int start_toknum, int end_toknum, size_t pattern_strlen, std::vector<Match> &result, MatchType matchType) {
		// Check if we already have a match in result with this valueId.
		// If so, then either replace it (if the new match is based on 
		// a longer pattern), or keep it and discard the new match (if
		// it's based on a shorter pattern).
		bool is_done = false; // to avoid returning from a BOOST_FOREACH
		BOOST_FOREACH(Match match, result) {
			if (is_done)
				continue;
			bool isAcronym = (matchType==ACRONYM_MATCH);
			if ((match.start_token==start_toknum) && (match.end_token==end_toknum) && (match.id == match_ids.valueId)) {
				if (newMatchIsBetter(match_ids, pattern_strlen, isAcronym, match)) {
					PatternIdType patternId = match_ids.patternId.isNull() ? match.patternId : match_ids.patternId;
					match = Match(match_ids.valueId, patternId, match_ids.code, start_toknum, 
						end_toknum, pattern_strlen, match_ids.weight, isAcronym);
				}
				is_done = true;
			}
		}
		if (is_done)
			return;
		// Otherwise, add the new match
		result.push_back(Match(match_ids.valueId, match_ids.patternId, match_ids.code, start_toknum, 
			end_toknum, pattern_strlen, match_ids.weight, (matchType==ACRONYM_MATCH)));
	}

	bool newMatchIsBetter(const MatchIds &newMatch, size_t newPatternStrlen, bool newMatchIsAcronymMatch, const Match &oldMatch) { 
		if (oldMatch.isAcronymMatch != newMatchIsAcronymMatch) return !newMatchIsAcronymMatch;
		if (oldMatch.weight != newMatch.weight) return (newMatch.weight > oldMatch.weight);
		if (oldMatch.pattern_strlen != newPatternStrlen) return (newPatternStrlen > oldMatch.pattern_strlen);
		if (oldMatch.patternId != newMatch.patternId) return ( newMatch.patternId < oldMatch.patternId );
		return false;
	}

	void matchTokens(const TokenSequence *toks, const Symbol *posTags, size_t start_toknum, size_t current_toknum, std::vector<Match> &result, MatchType matchType) {
		// If this Trie node contains any matches, then add them.
		if (_matchInfo) {
			BOOST_FOREACH(MatchIds match_ids, _matchInfo->match_ids) {
				addMatch(match_ids, static_cast<int>(start_toknum), static_cast<int>(current_toknum)-1, static_cast<int>(_matchInfo->pattern_strlen), result, matchType);
			}
		}

		if (static_cast<int>(current_toknum) < toks->getNTokens()) {
			Symbol caseSensitiveTokenSym = toks->getToken(current_toknum)->getSymbol();
			const Symbol &posTag = posTags[current_toknum];
			std::wstring caseInsensitiveTokenStr(caseSensitiveTokenSym.to_string());
			boost::to_upper(caseInsensitiveTokenStr);
			if (_strip_punctuation()) {
				static boost::wregex PUNCT_RE(L"[\\.\\-]");
				caseInsensitiveTokenStr = boost::regex_replace(caseInsensitiveTokenStr, PUNCT_RE, L"");
			}

			if (start_toknum == current_toknum && caseInsensitiveTokenStr.size() == 0)
				return;

			Symbol caseInsensitiveTokenSym(caseInsensitiveTokenStr);
			Symbol stemmedTokenSym = caseSensitiveTokenSym;
			stemmedTokenSym = SymbolUtilities::stemWord(caseSensitiveTokenSym, posTag);
			Symbol stemmedCaseInsensitiveTokenSym(boost::to_upper_copy(std::wstring(stemmedTokenSym.to_string())));
			typename Symbol::template HashMap<Trie_ptr>::iterator match_it;

			// Check if this token occurs as a key in _next_by_normal_match.
			if (_next_by_normal_match) {
				match_it = _next_by_normal_match->find(caseInsensitiveTokenSym);
				if (match_it != _next_by_normal_match->end())
					(*match_it).second->matchTokens(toks, posTags, start_toknum, current_toknum+1, result, NORMAL_MATCH);
				// Check if the stemmed token occurs as a key in _next_by_normal_match.
				if (stemmedCaseInsensitiveTokenSym != caseInsensitiveTokenSym) {
					match_it = _next_by_normal_match->find(stemmedCaseInsensitiveTokenSym);
					if (match_it != _next_by_normal_match->end())
						(*match_it).second->matchTokens(toks, posTags, start_toknum, current_toknum+1, result, NORMAL_MATCH);
				}
			}

			// Check if this token occurs as a key in _next_by_exact_match.
			if (_next_by_exact_match) {
				match_it = _next_by_exact_match->find(caseSensitiveTokenSym);
				if (match_it != _next_by_exact_match->end())
					(*match_it).second->matchTokens(toks, posTags, start_toknum, current_toknum+1, result, EXACT_MATCH);
			}

			// Check if this token occurs as a key in _next_by_acronym_match.
			if (_next_by_acronym_match) {
				match_it = _next_by_acronym_match->find(caseSensitiveTokenSym);
				if (match_it != _next_by_acronym_match->end())
					(*match_it).second->matchTokens(toks, posTags, start_toknum, current_toknum+1, result, ACRONYM_MATCH);
			}

			// Check if any element of _next_by_prefix_match is a prefix of this token.
			if (_next_by_prefix_match) {
				BOOST_FOREACH(const SymbolTriePair &pair, *_next_by_prefix_match) {
					if (boost::starts_with(caseInsensitiveTokenStr, pair.first.to_string()))
						pair.second->matchTokens(toks, posTags, start_toknum, current_toknum+1, result, PREFIX_MATCH);
				}
			}

			// Skip possessive markers ("'s" and "'") when appropriate.
			if (_skip_posessives && ((caseInsensitiveTokenSym == APOS_SYM) || 
				                     (caseInsensitiveTokenSym == APOS_S_SYM) ||
									 (caseInsensitiveTokenStr.size()==0)))
				matchTokens(toks, posTags, start_toknum, current_toknum+1, result, matchType);

			// Skip tokens that just consist of a single dash.
			if (caseInsensitiveTokenSym == DASH_SYM)
				matchTokens(toks, posTags, start_toknum, current_toknum+1, result, matchType);
		}
	}

	Trie(bool skip_possessives=false)
		: _matchInfo(), _skip_posessives(skip_possessives), _next_by_normal_match(), 
		_next_by_acronym_match(), _next_by_exact_match(), _next_by_prefix_match()
	{}

	struct PatternToken {
		Symbol tokenSym;
		MatchType matchType;
		PatternToken(Symbol tokenSym, MatchType matchType)
			: tokenSym(tokenSym), matchType(matchType) {}
	};

	size_t addPattern(const std::vector<PatternToken> &pattern, size_t pattern_strlen, MatchIds matchIds, size_t index=0) {
		if (index == pattern.size()) {
			if (!_matchInfo) _matchInfo.reset(_new MatchInfo());
			_matchInfo->pattern_strlen = std::max(_matchInfo->pattern_strlen, pattern_strlen);
			/*
			// If we already have an equivalent match, then don't add a duplicate.
			BOOST_FOREACH(MatchIds m, _matchInfo.match_ids) {
				if (m.valueId == matchIds.valueId) {
					SessionLogger::dbg("ICEWS") << "Warning: Two equivalent patterns (" << m.patternId.getId() << " and " 
						<< matchIds.patternId.getId() << ") both generate the same value (" 
						<< m.valueId.getId() << ")." << std::endl;
				}
			}
			*/
			_matchInfo->match_ids.push_back(matchIds);
			return 0;
		} else {
			if (pattern[index].matchType == PREFIX_MATCH) {
				return addPatternByPrefix(pattern, pattern_strlen, matchIds, index);
			} else {
				Symbol tokenSym = pattern[index].tokenSym;
				return addPatternByToken(tokenSym, pattern, pattern_strlen, matchIds, index);
			}
		}
	}

private:
	size_t addPatternByPrefix(const std::vector<PatternToken> &pattern, size_t pattern_strlen, MatchIds matchIds, size_t index) {
		if (!_next_by_prefix_match) _next_by_prefix_match.reset(_new std::vector<SymbolTriePair>());
		BOOST_FOREACH(const SymbolTriePair &pair, *_next_by_prefix_match) {
			if (pair.first == pattern[index].tokenSym) {
				return pair.second->addPattern(pattern, pattern_strlen, matchIds, index+1);
			}
		}
		Trie_ptr next = boost::make_shared<typename JabariTokenMatcher::Trie>();
		_next_by_prefix_match->push_back(SymbolTriePair(pattern[index].tokenSym, next));
		return 1 + next->addPattern(pattern, pattern_strlen, matchIds, index+1);
	}

	size_t addPatternByToken(Symbol tokenSym, const std::vector<PatternToken> &pattern, size_t pattern_strlen, MatchIds matchIds, size_t index) {
		bool exact_match = (pattern[index].matchType==EXACT_MATCH);
		bool acronym_match = (pattern[index].matchType==ACRONYM_MATCH);
		boost::scoped_ptr<Symbol::HashMap<Trie_ptr> > &next_map = 
			(exact_match?_next_by_exact_match:(acronym_match?_next_by_acronym_match:_next_by_normal_match));

		if (!next_map) next_map.reset(_new Symbol::HashMap<Trie_ptr>());
		typename Symbol::template HashMap<Trie_ptr>::iterator it = next_map->find(tokenSym);
		if (it != next_map->end()) {
			return (*it).second->addPattern(pattern, pattern_strlen, matchIds, index+1);
		} else {
			Trie_ptr next = boost::make_shared<typename JabariTokenMatcher::Trie>(!(exact_match||acronym_match));
			(*next_map)[tokenSym] = next;
			return 1 + next->addPattern(pattern, pattern_strlen, matchIds, index+1);
		}
	}
};


template<typename ValueIdType, typename PatternIdType>
void JabariTokenMatcher<ValueIdType, PatternIdType>::match(const TokenSequence *toks, const Symbol* posTags, size_t index,
                   std::vector<typename JabariTokenMatcher<ValueIdType, PatternIdType>::Match>& result)
{
	return _root->matchTokens(toks, posTags, index, index, result, Trie::NORMAL_MATCH);
}


template<typename ValueIdType, typename PatternIdType>
std::vector<typename JabariTokenMatcher<ValueIdType, PatternIdType>::Match>
JabariTokenMatcher<ValueIdType, PatternIdType>::match(const TokenSequence *toks, const Symbol* posTags, size_t index) {
	std::vector<Match> result;
	match(toks, posTags, index, result);
	return result;
}

namespace {
	// Helper function for JabariTokenMatcher::addPattern().
	// TODO: recursive call has SPACE(n!) growth on number of dashes, which crashes by
	// using too much memory on a malformed pattern string. Add some checking?
	void findAllSplitsOnDash(const std::wstring &patternStr, std::vector<std::wstring> &result) {
		std::wstring::size_type pos = patternStr.find_first_of(L'-');
		if (pos == std::wstring::npos) {
			result.push_back(patternStr);
		} else {
			std::wstring lhs = patternStr.substr(0, pos);
			std::vector<std::wstring> rhs_expanded;
			findAllSplitsOnDash(patternStr.substr(pos+1), rhs_expanded);
			BOOST_FOREACH(const std::wstring &rhs, rhs_expanded) {
				result.push_back(lhs+L'-'+rhs);      // foo-bar     (one token)
				result.push_back(lhs+rhs);           // foobar      (one token)
				result.push_back(lhs+L"_ "+rhs);     // foo_ bar    (two tokens)
			}
		}
	}
}

template<typename ValueIdType, typename PatternIdType>
void JabariTokenMatcher<ValueIdType, PatternIdType>::addBBNActorPattern(
	std::wstring actorString, PatternIdType patternId, ValueIdType valueId, bool is_acronym, float confidence, bool requires_context) 
{
	boost::replace_all(actorString, L" ", L"_");
	
	if (is_acronym)
		actorString.append(L"=");
	else
		actorString.append(L"_");
	while (actorString.find(L"__") != std::wstring::npos)
		boost::replace_all(actorString, L"__", L"_");
	boost::replace_all(actorString, L"_=", L"=");

	if (requires_context)
		_patterns_requiring_context.insert(patternId);

	addPattern(actorString, patternId, valueId, Symbol(), confidence);
}


template<typename ValueIdType, typename PatternIdType>
void JabariTokenMatcher<ValueIdType, PatternIdType>::addPattern(std::wstring patternStr, PatternIdType patternId, 
																ValueIdType valueId, Symbol code, float weight) 
{
	typename Trie::MatchIds match(valueId, patternId, code, weight);
	
	boost::to_upper(patternStr); // case insensitive -- do we want this?? [xxxxx]

	// Insert a space after '_' or '='.
	boost::replace_all(patternStr, L"_", L"_ ");
	boost::replace_all(patternStr, L"=", L"= ");
	boost::trim(patternStr);
	// Remove space & underscore around dash.
	static boost::wregex dash_re(L"[ _]+-[ _]+");
	patternStr = boost::regex_replace(patternStr, dash_re, L"-");

	// Expand dash tokens in various ways.
	std::vector<std::wstring> expandedPatternStrings;
	findAllSplitsOnDash(patternStr, expandedPatternStrings);
	BOOST_FOREACH(const std::wstring& expandedPatternStr, expandedPatternStrings) {
		// Split the pattern into tokens.
		std::vector<std::wstring> pieces;
		boost::split(pieces, expandedPatternStr, boost::is_any_of(L" \n\r\t"), boost::token_compress_on);
		// Parse each token into a Trie::PatternToken.
		std::vector<typename Trie::PatternToken> pattern;
		BOOST_FOREACH(std::wstring piece, pieces) {
			if (piece.length() == 0)
				continue;
			wchar_t last_char = piece[piece.length()-1];
			if (last_char == L'_') {
				pattern.push_back(typename Trie::PatternToken(Symbol(piece.substr(0, piece.length()-1)), Trie::NORMAL_MATCH));
			} else if (last_char == L'=') {
				if (piece.length()>1 && piece[piece.length()-2]==L'!')
					pattern.push_back(typename Trie::PatternToken(Symbol(piece.substr(0, piece.length()-2)), Trie::EXACT_MATCH));
				else
					pattern.push_back(typename Trie::PatternToken(Symbol(piece.substr(0, piece.length()-1)), Trie::ACRONYM_MATCH));
			} else {
				pattern.push_back(typename Trie::PatternToken(piece, Trie::PREFIX_MATCH));
			}
		}
		// Add the pattern to our root trie.
		_size += _root->addPattern(pattern, expandedPatternStr.size(), match);
	}
}

namespace { // Helper methods declarations.
	template<typename ValueIdType> ValueIdType makeValueId(const char* value, Symbol code, Symbol db_name);
	template<typename ValueIdType> ValueIdType getIdForCode(Symbol code, const std::string &kind);
	template<typename ValueIdType> const char *getCodeField();
}

template<typename ValueIdType, typename PatternIdType>
JabariTokenMatcher<ValueIdType, PatternIdType>::JabariTokenMatcher(const char* kind, bool read_patterns, ActorInfo_ptr actorInfo): _kind(kind), _size(0) {
	Trie::stripPunctuation(ParamReader::isParamTrue("strip_punctuation_when_matching_jabari_patterns"));

	_root = boost::make_shared<Trie>();
	if (!read_patterns) return;

	bool use_awake_db_for_icews = ParamReader::isParamTrue("use_awake_db_for_icews");
	
	// Read a list of suppressed patterns.
	std::set<std::wstring> suppressedPatterns;
	std::string supressedPatternFilename = ParamReader::getParam(std::string("icews_")+kind+"_suppressed_patterns");
	if (!supressedPatternFilename.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(supressedPatternFilename));
		UTF8InputStream& stream(*stream_scoped_ptr);
		while (!(stream.eof() || stream.fail())) {
			std::wstring line;
			std::getline(stream, line);
			line = line.substr(0, line.find_first_of('#')); // Remove comments.
			boost::trim(line);
			if (!line.empty())
				suppressedPatterns.insert(line);
		}
	}

	// Read patterns from the ICEWS database.
	if (!use_awake_db_for_icews && !boost::iequals(_kind, "composite_actor") && !boost::iequals(_kind, "bbn_actor")) {
		std::ostringstream query;
		size_t num_patterns = 0;
		size_t num_suppressed_patterns = 0;
		std::string code_field = getCodeField<ValueIdType>();
		query << "SELECT p.pattern_id, " << _kind << "_id, "
			<< "a." << code_field << ", p.pattern from dict_" << _kind << "patterns p"
			<< " JOIN dict_" << _kind << "s a USING(" << _kind << "_id)";
		DatabaseConnectionMap dbs = ICEWSDB::getNamedDbs();
		for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
			for (DatabaseConnection::RowIterator result = i->second->iter(query); result!=i->second->end(); ++result) {
				PatternIdType patternId(result.getCellAsInt32(0), i->first);
				Symbol code(result.getCellAsSymbol(2));
				ValueIdType valueId = makeValueId<ValueIdType>(result.getCell(1), code, i->first);
				std::wstring pattern(result.getCellAsWString(3));
				// Add the pattern (unless it's suppressed)
				if (suppressedPatterns.find(pattern) == suppressedPatterns.end()) {
					// Check for unmarked acronyms.
					static const boost::wregex unmarkedAcronymRegex(L"^[BCDFGHJKLMNPQRSTVWXZ]+_$");
					if (boost::regex_match(pattern, unmarkedAcronymRegex))
						pattern = pattern.substr(0, pattern.size()-1)+L"=";
					addPattern(pattern, patternId, valueId, code, 0);
					++num_patterns;
				} else {
					++num_suppressed_patterns;
				}
			}
		}
		SessionLogger::info("ICEWS") << "Loaded " << num_patterns << " " 
			<< _kind << " patterns from dict_" << _kind << "patterns";
		if (num_suppressed_patterns)
			SessionLogger::info("ICEWS") << "  Suppressed " << num_suppressed_patterns << " " 
				<< _kind << " patterns from dict_" << _kind << "patterns";
	}

	// BBN Actor database
	if (boost::iequals(_kind, "bbn_actor")) {
		if (!actorInfo) {
			throw InternalInconsistencyException("JabariTokenMatcher<ValueIdType, PatternIdType>::JabariTokenMatcher", 
				"If kind=bbn_actor, you must provide AWAKEActorInfo");
		}
		size_t num_patterns = 0;
		size_t num_suppressed_patterns = 0;

		std::ostringstream query;

		BOOST_FOREACH(ActorPattern *ap, actorInfo->getPatterns()) {
			PatternIdType patternId(ap->pattern_id.getId(), ap->pattern_id.getDbName());
			// I'm sure there's a better way to make a ValueIdType from an ActorId here
			std::stringstream ss; 
			ss << ap->actor_id.getId();
			ValueIdType valueId = makeValueId<ValueIdType>(ss.str().c_str(), Symbol(), ap->pattern_id.getDbName());

			std::wstring pattern(ActorPattern::getNameFromSymbolList(ap->pattern));

			if (suppressedPatterns.find(pattern) == suppressedPatterns.end()) {
				addBBNActorPattern(pattern, patternId, valueId, ap->acronym, ap->confidence, ap->requires_context);
				++num_patterns;
			} else {
				++num_suppressed_patterns;
			}
		}
		SessionLogger::info("ACTOR_MATCH") << "Loaded " << num_patterns <<
			" patterns from actor_strings";
		if (num_suppressed_patterns)
			SessionLogger::info("ACTOR_MATCH") << "  Suppressed " << num_suppressed_patterns << " " <<
			" patterns from actor_strings";
	}

	if (boost::iequals(_kind, "bbn_agent")) {
		size_t num_patterns = 0;
		size_t num_suppressed_patterns = 0;

		std::ostringstream query;
		query << "SELECT agentstringid, agentid, string from agentstring";
		DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());
		Symbol dbName(AWAKEDB::getDefaultDbName());

		for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) {
			PatternIdType patternId(row.getCellAsInt32(0), dbName);
			ValueIdType valueId = makeValueId<ValueIdType>(row.getCellAsString(1).c_str(), Symbol(), dbName);
			std::wstring pattern(row.getCellAsWString(2));

			if (suppressedPatterns.find(pattern) == suppressedPatterns.end()) {
				addPattern(pattern, patternId, valueId, dbName, 0.95F);
				++num_patterns;
			} else {
				++num_suppressed_patterns;
			}
		}
		SessionLogger::info("ACTOR_MATCH") << "Loaded " << num_patterns <<
			" patterns from AgentString";
		if (num_suppressed_patterns)
			SessionLogger::info("ACTOR_MATCH") << "  Suppressed " << num_suppressed_patterns << " " <<
			" patterns from AgentString";
	}

	// Read patterns from the pattern file.
	if (!use_awake_db_for_icews) {
		std::string patternFilename = ParamReader::getParam(std::string("icews_")+kind+"_patterns");
		size_t num_file_patterns = 0;
		if (!patternFilename.empty()) {
			boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(patternFilename));
			UTF8InputStream& stream(*stream_scoped_ptr);
			while (!(stream.eof() || stream.fail())) {
				std::wstring line;
				std::getline(stream, line);
				line = line.substr(0, line.find_first_of('#')); // Remove comments.
				boost::trim(line);
				if (!line.empty()) {
					std::vector<std::wstring> pieces;
					boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);
					if (pieces.size() != 3) {
						std::ostringstream err;
						err << "Bad line in " << patternFilename << ": \"" << line << "\"";
						throw UnexpectedInputException("JabariTokenMatcher::JabariTokenMatcher", err.str().c_str());
					}
					float score = boost::lexical_cast<float>(pieces[1]);
					Symbol code(pieces[0]);
					// Look up the code for the value id.
					ValueIdType valueId = getIdForCode<ValueIdType>(code, _kind);
					if (valueId.isNull() && 
						(code != COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM) && (code != BLOCK_ACTOR_SYM)) 
					{
						// SessionLogger TODO
						SessionLogger::err("ICEWS") << "Code not found in database while reading " << patternFilename 
							<< ": \"" << code.to_debug_string() << "\"";				
					} else {
						addPattern(pieces[2], PatternIdType(), valueId, code, score);
						++num_file_patterns;
					}
				}
			}
		}
		SessionLogger::info("ICEWS") << "Loaded " << num_file_patterns 
			<< " " << _kind << "patterns from " << patternFilename;
	}
	SessionLogger::info("ICEWS") << "JabariTokenMatcher for " << _kind
		<< " contains " << _size << " trie nodes.";
}




template<typename ValueIdType, typename PatternIdType>
std::vector<std::vector<typename JabariTokenMatcher<ValueIdType, PatternIdType>::Match> >
JabariTokenMatcher<ValueIdType, PatternIdType>::findAllMatches(const DocTheory *docTheory, int sentence_cutoff)
{
	int num_sents = std::min(docTheory->getNSentences(), sentence_cutoff);

	std::vector<std::vector<Match> > result;
	size_t num_matches = 0;
	SessionLogger::dbg("ICEWS") << "Finding " << _kind << " mentions in " 
		<< docTheory->getDocument()->getName();
	for (int sentno=0; sentno<num_sents; ++sentno) {
		result.push_back(std::vector<Match>());
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
		const TokenSequence *tokSeq = sentTheory->getTokenSequence();
		boost::scoped_array<Symbol> posTags(_new Symbol[tokSeq->getNTokens()]);
		sentTheory->getPrimaryParse()->getRoot()->getPOSSymbols(posTags.get(), tokSeq->getNTokens());
		for (int toknum=0; toknum<=tokSeq->getNTokens(); ++toknum) {
			match(tokSeq, posTags.get(), toknum, result.back()); // stores result in actorMatches[-1]
			num_matches += result.back().size();
		}
	}
	SessionLogger::dbg("ICEWS") << "  Found " << num_matches << " "
		<< _kind << " pattern matches.";
	return result;
}

template<typename ValueIdType, typename PatternIdType>
std::vector<typename JabariTokenMatcher<ValueIdType, PatternIdType>::Match>
JabariTokenMatcher<ValueIdType, PatternIdType>::findAllMatches(const SentenceTheory *sentTheory)
{
	std::vector<Match> result;

	const TokenSequence *tokSeq = sentTheory->getTokenSequence();
	boost::scoped_array<Symbol> posTags(_new Symbol[tokSeq->getNTokens()]);
	sentTheory->getPrimaryParse()->getRoot()->getPOSSymbols(posTags.get(), tokSeq->getNTokens());
	for (int toknum=0; toknum<=tokSeq->getNTokens(); ++toknum) {
		match(tokSeq, posTags.get(), toknum, result); 
	}
	
	return result;
}

namespace { // Helper methods definitions.
	template<typename ValueIdType>
	ValueIdType makeValueId(const char* value, Symbol code, Symbol db_name) {
		return value ? ValueIdType(boost::lexical_cast<typename ValueIdType::id_type>(value), db_name) : ValueIdType();
	}

	template<typename ValueIdType>
	ValueIdType getIdForCode(Symbol code, const std::string &kind) {
		std::string code_field = getCodeField<ValueIdType>();
		std::ostringstream valueIdQuery;
		valueIdQuery << "SELECT " << kind << "_id FROM dict_" << kind << "s"
			<< " WHERE " << code_field << "=" 
			<< DatabaseConnection::quote(UnicodeUtil::toUTF8StdString(code.to_string()));
		DatabaseConnectionMap dbs = ICEWSDB::getNamedDbs();
		for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
			for (DatabaseConnection::RowIterator result = i->second->iter(valueIdQuery); result!=i->second->end(); ++result)
				return makeValueId<ValueIdType>(result.getCell(0), code, i->first);
		}
		return ValueIdType(); // not found.
	}

	template<> const char *getCodeField<ActorId>() { return "unique_code"; }
	template<> const char *getCodeField<AgentId>() { return "agent_code"; }
	template<> const char *getCodeField<CompositeActorId>() { return "NONE"; }

	template<>
	CompositeActorId makeValueId<CompositeActorId>(const char* value, Symbol code, Symbol db_name) {
		throw UnexpectedInputException("makeValueId<CompositeActorId>",
			"Composite actors may only be read from a file, not the database");
	}

	template<>
	CompositeActorId getIdForCode<CompositeActorId>(Symbol code, const std::string &kind) { 
		static boost::wregex composite_code_re(L"^(.*)::(.*)$");
		boost::wsmatch match;
		std::wstring codeString(code.to_string());
		if (boost::regex_match(codeString, match, composite_code_re)) {
			return CompositeActorId(getIdForCode<AgentId>(Symbol(match.str(1)), "agent"),
			                        getIdForCode<ActorId>(Symbol(match.str(2)), "actor"));
		} else {
			throw UnexpectedInputException("getIdForCode<CompositeActorId>",
				"Expected code to have the form \"AGENTID::ACTORID\"");
		}
	}

}


// Explicit instantiations:
template class JabariTokenMatcher<ActorId, ActorPatternId>;
template class JabariTokenMatcher<AgentId, AgentPatternId>;
template class JabariTokenMatcher<CompositeActorId, ActorPatternId>;

