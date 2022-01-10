// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/RegexPattern.h"

#include "Generic/common/Sexp.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"

#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternWordSet.h"
#include "Generic/patterns/MatchStorage.h"
#include "Generic/patterns/TextPattern.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/features/TopLevelPFeature.h"

#include <boost/lexical_cast.hpp>

// Private symbols
namespace {
	Symbol reSym(L"re");
	Symbol dontAllowHeadsSym(L"DONT_ALLOW_HEADS");
	Symbol matchWholeExtentSym(L"MATCH_FULL_EXTENT");
	Symbol topMentionsOnlySym(L"TOP_MENTIONS_ONLY");
	Symbol dontAddSpacesSym(L"DONT_ADD_SPACES");
	Symbol noneSym(L"NONE");
}

RegexPattern::RegexPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
: _allow_heads(true), _match_whole_extent(false), _add_spaces(true), _top_mentions_only(false)
{
	initializeFromSexp(sexp, entityLabels, wordSets);
	if (_subpatterns.empty()) 
		throwError(sexp, "RegexPattern must have at least one subpattern");
}

bool RegexPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol atom = childSexp->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (atom == dontAllowHeadsSym) {
		_allow_heads = false;
	} else if (atom == topMentionsOnlySym) {
		_top_mentions_only = true;
	} else if (atom == matchWholeExtentSym) {
		_match_whole_extent = true;
	} else if (atom == dontAddSpacesSym) {
		_add_spaces = false;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
	return true;
}

bool RegexPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == reSym) {
		int n_subpatterns = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_subpatterns; j++)
			_subpatterns.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
		return true;
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
}

// this can't be done in the constructor because the shortcuts have not been replaced
void RegexPattern::createRegex() {
	/* TODO: Stop adding spaces around the regex, use \b instead. 
	         The sentence that is being matched against should 
			 ont have spaces added either. Will require a lot of 
			 testing to verify that behavior isn't changed. */
	if (!_regex_pattern.empty())
		return;

	// Create regex and store which parenthetical inside the regex corresponds to what subpattern
	_subpatternToSubmatchMap.resize(_subpatterns.size());
	std::wstring regexString(L"");
	if (_match_whole_extent) regexString.append(L"^");
	regexString.append(L" ");
	
	int submatch_count = 0;
	for (size_t i = 0; i < _subpatterns.size(); i++) {
		Pattern_ptr dp = _subpatterns[i];
		if (TextPattern_ptr tp = boost::dynamic_pointer_cast<TextPattern>(dp)) {
			std::wstring text = tp->getText();
			submatch_count++;
		    _subpatternToSubmatchMap[i] = submatch_count;
			submatch_count += TextPattern::countSubstrings(text, L"(");
			submatch_count -= TextPattern::countSubstrings(text, L"\\(");

			regexString.append(L"(");
			regexString.append(text);
			regexString.append(L")");
			if (_add_spaces) regexString.append(L" ");
		} else {
			regexString.append(L"(!@RP_MATCH@!)");
			if (_add_spaces) regexString.append(L" ");
			submatch_count++;
		    _subpatternToSubmatchMap[i] = submatch_count;
		}
	}

	// Even if we aren't adding spaces, we need to put a space in before the end, since 
	// the sentence will have a space at the end. And if _match_whole_extent is on, we 
	// want to make sure the spaces match up.
	if (!_add_spaces) regexString.append(L" ");

	if (_match_whole_extent) 
		regexString.append(L"$"); 
	else {
		// remove trailing space, and add "look ahead" 
		// Make sure a space exists after the regex match,
		// but don't consume it. This makes it so we can find 
		// two matches back to back.
		regexString = regexString.substr(0, regexString.length() - 1);
		regexString += L"(?= )";
	}

	_regex_string = regexString;
	_regex_pattern = boost::wregex(regexString, boost::regex::perl|boost::regex::icase);

	createFilterRegexps();
}

// Private helper for createFilterRegexps().
namespace {
	struct greaterLength{
	    bool operator () (const std::wstring& s1, const std::wstring& s2)
		{return s1.length() > s2.length();}
	};
}

// Initialize the _filter_regexps vector.  See the docs for _filter_regexps for
// details about how these regexps are used.
void RegexPattern::createFilterRegexps()
{
	// Extract all the text pieces from the regexp pattern.  Sort them in 
	// descending order of length, since the longer regexps are more likely
	// to fail first.  (Shorter regexps are often things like ".{0,20}" or
	// "of".)
	typedef std::set<std::wstring, greaterLength> TextPieces;
	TextPieces text_pieces;
	for (size_t i = 0; i < _subpatterns.size(); i++) {
		Pattern_ptr dp = _subpatterns[i];
		if (TextPattern_ptr tp = boost::dynamic_pointer_cast<TextPattern>(dp)) {
			text_pieces.insert(tp->getText());
		}
	}
	for(TextPieces::iterator tp=text_pieces.begin(); tp!=text_pieces.end(); ++tp) {
		boost::wregex pattern(*tp, boost::regex::perl|boost::regex::icase);
		_filter_regexps.push_back(pattern);
	}
}

PatternFeatureSet_ptr RegexPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) 
{ 
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return matchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}

	std::vector<PatternFeatureSet_ptr> multipleReturns = matchImpl(patternMatcher, sTheory, 0, sTheory->getTokenSequence()->getNTokens() - 1, false);

	if (multipleReturns.size() == 0) 
		return PatternFeatureSet_ptr();

	return multipleReturns.at(0);
}

PatternFeatureSet_ptr RegexPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides)
{
	// fall_through_children not relevant here

	const MentionSet *ms = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
	int start_token = arg->getStartToken(ms);
	int end_token = arg->getEndToken(ms);
	std::vector<PatternFeatureSet_ptr> multipleReturns = matchImpl(patternMatcher, patternMatcher->getDocTheory()->getSentenceTheory(sent_no), start_token, end_token, false);

	if (multipleReturns.size() == 0) 
		return PatternFeatureSet_ptr();

	return multipleReturns.at(0);

}

PatternFeatureSet_ptr RegexPattern::matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node)
{
	// EMB 2/14/11: modeled on matchesArgument
	int start_token = node->getStartToken();
	int end_token = node->getEndToken();
	std::vector<PatternFeatureSet_ptr> multipleReturns = matchImpl(patternMatcher, patternMatcher->getDocTheory()->getSentenceTheory(sent_no), start_token, end_token, false);

	if (multipleReturns.size() == 0) 
		return PatternFeatureSet_ptr();

	return multipleReturns.at(0);

}

PatternFeatureSet_ptr RegexPattern::matchesMentionImpl(PatternMatcher_ptr patternMatcher, const Mention *mention, int start_token, int end_token) {
	int sent_no = mention->getSentenceNumber();
	std::vector<PatternFeatureSet_ptr> multipleReturns = matchImpl(patternMatcher, patternMatcher->getDocTheory()->getSentenceTheory(sent_no), start_token, end_token, false);

	if (multipleReturns.size() == 0) 
		return PatternFeatureSet_ptr();

	PatternFeatureSet_ptr sfs = multipleReturns.at(0);
	if (_match_whole_extent) {
		if (!returnLabelIsEmpty() || (getNReturnValuePairs() > 0)) {
			// replace the return value with a return feature that contains the mention
			for (size_t i = 0; i < sfs->getNFeatures(); i++) {
				if (ReturnPatternFeature_ptr rf = boost::dynamic_pointer_cast<ReturnPatternFeature>(sfs->getFeature(i))) {
					PatternFeature_ptr srf = boost::make_shared<MentionReturnPFeature>(
						shared_from_this(), mention, noneSym, patternMatcher->getActiveLanguageVariant(), rf->getConfidence());
					sfs->replaceFeature(i, srf);
				}
			}
		}
	}

	return sfs;
}

PatternFeatureSet_ptr RegexPattern::matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children)
{	
	// fall_through_children not relevant here

	int start_token = mention->getNode()->getStartToken();
	int end_token = mention->getNode()->getEndToken();
	
	return matchesMentionImpl(patternMatcher, mention, start_token, end_token);
}

PatternFeatureSet_ptr RegexPattern::matchesMentionHead(PatternMatcher_ptr patternMatcher, const Mention *mention)
{	
	int start_token = mention->getNode()->getHead()->getStartToken();
	int end_token = mention->getNode()->getHead()->getEndToken();
	
	return matchesMentionImpl(patternMatcher, mention, start_token, end_token);
}

PatternFeatureSet_ptr RegexPattern::matchesValueMentionImpl(PatternMatcher_ptr patternMatcher, const ValueMention *valueMention, int start_token, int end_token) {
	int sent_no = valueMention->getSentenceNumber();
	std::vector<PatternFeatureSet_ptr> multipleReturns = matchImpl(patternMatcher, patternMatcher->getDocTheory()->getSentenceTheory(sent_no), start_token, end_token, false);

	if (multipleReturns.size() == 0) 
		return PatternFeatureSet_ptr();

	PatternFeatureSet_ptr sfs = multipleReturns.at(0);
	if (_match_whole_extent) {
		if (!returnLabelIsEmpty() || getNReturnValuePairs() > 0) {
			// replace the return value with a return feature that contains the ValueMention
			for (size_t i = 0; i < sfs->getNFeatures(); i++) {
				if (ReturnPatternFeature_ptr rf = boost::dynamic_pointer_cast<ReturnPatternFeature>(sfs->getFeature(i))) {
					PatternFeature_ptr srf = boost::make_shared<ValueMentionReturnPFeature>(
						shared_from_this(), valueMention, patternMatcher->getActiveLanguageVariant());
					sfs->replaceFeature(i, srf);
				}
			}
		}
	}
	return sfs;
}

PatternFeatureSet_ptr RegexPattern::matchesValueMention(PatternMatcher_ptr patternMatcher, const ValueMention *valueMention)
{	
	int start_token = valueMention->getStartToken();
	int end_token = valueMention->getEndToken();
	
	return matchesValueMentionImpl(patternMatcher, valueMention, start_token, end_token);
}

// Note this isn't truly implemented as intended.  RegexPattern will only ever return a single PatternFeatureSet as it stands now.
std::vector<PatternFeatureSet_ptr> RegexPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) 
{ 
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return multiMatchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	return matchImpl(patternMatcher, sTheory, 0, sTheory->getTokenSequence()->getNTokens() - 1, true);
}

std::vector<PatternFeatureSet_ptr> RegexPattern::matchImpl(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, int start_token, int end_token, bool multi_match) { 
	static long dbg_counter = 0;
	std::vector<PatternFeatureSet_ptr> returnVector;
	createRegex();

	// Do a preliminary check to see if there's any chance that this regexp pattern will
	// match this sentence.  If not, then return an empty vector.
	std::wstring sentence_string = sTheory->getTokenSequence()->toString();
	for (size_t i=0; i< _filter_regexps.size(); i++) {
		SessionLogger::dbg("RegexPattern") << "RegexPattern::matchImpl() checking _filter_regexp " << i << " of " << _filter_regexps.size();
		if (!boost::regex_search(sentence_string, _filter_regexps[i])) {
			SessionLogger::dbg("RegexPattern") << "RegexPattern::matchImpl() returning empty vector";
			return returnVector; // empty vector.
		}
	} 

	MatchStorage rpMatches(static_cast<int>(_subpatterns.size()));

	// For every non text pattern, match against sentence, and store results in rpMatches
	for (size_t i = 0; i < _subpatterns.size(); i++) {
		Pattern_ptr dp = _subpatterns[i];

		if (boost::dynamic_pointer_cast<TextPattern>(dp)) {
			continue;
		}
		bool found = false;
		if (MentionMatchingPattern_ptr mp = boost::dynamic_pointer_cast<MentionMatchingPattern>(dp)) {
			for (int j = 0; j < sTheory->getMentionSet()->getNMentions(); j++) {
				Mention *ment = sTheory->getMentionSet()->getMention(j);
				int ment_start = ment->getNode()->getStartToken();
				int ment_end = ment->getNode()->getEndToken();
				
				if (ment_start < start_token || ment_end > end_token)
					continue;

				PatternFeatureSet_ptr sfs = mp->matchesMention(patternMatcher, ment, true); // falling through sets in regexes is always OK
				if (sfs) {
					found = true;
					const SynNode *node = ment->getNode();

					if (_top_mentions_only) {
						// Go up the node chain until you've reached a non-mention (or the top)
						const SynNode *parent = node;
						while (parent->getParent() != 0 && parent->getParent()->hasMention()) 
							parent = parent->getParent();
						// If the parent mention doesn't have the same head as we do, we're done
						if (parent->getHeadPreterm() != node->getHeadPreterm())
							continue;
					}
					
					// rpMatches will take ownership of the PatternFeatureSet
					rpMatches.storeMatch(static_cast<int>(i), ment, 0, sfs, false);
					const SynNode *headNode = ment->getHead();
					if (_allow_heads && headNode != 0 && 
						(node->getStartToken() != headNode->getStartToken() || node->getEndToken() != headNode->getEndToken())) {
						// will make a new PatternFeatureSet so MatchData does not share SnippetPatternFeatureSets
						PatternFeatureSet_ptr sfs_head = mp->matchesMention(patternMatcher, ment, true); // falling through sets in regexes is always OK
						rpMatches.storeMatch(static_cast<int>(i), ment, 0, sfs_head, true);
					}
				}
			}
		}

		if (ValueMentionMatchingPattern_ptr vp = boost::dynamic_pointer_cast<ValueMentionMatchingPattern>(dp)) {
			for (int j = 0; j < sTheory->getValueMentionSet()->getNValueMentions(); j++) {
				ValueMention *vm = sTheory->getValueMentionSet()->getValueMention(j);
				int vm_start = vm->getStartToken();
				int vm_end = vm->getEndToken();
				
				if (vm_start < start_token || vm_end > end_token)
					continue;

				PatternFeatureSet_ptr sfs = vp->matchesValueMention(patternMatcher, vm);
				if (sfs) {
					found = true;
					rpMatches.storeMatch(static_cast<int>(i), 0, vm, sfs, false);
				}
			}
		}

		if (!found) {
			return returnVector;
		}
	}

	// Here we've found all non-text patterns in the sentence somewhere
	std::wstring sentence;
	boost::wsmatch boostMatchResults;

	MatchStorage::MatchData **currentMatchData = _new MatchStorage::MatchData*[_subpatterns.size()];
	//with (I think) the potential of replacing each token with !@RP_MATCH@! (12 chars long), we should expand the sentence size by 12
	int* offsetToStartTokenMap = new int[12*sentence_string.size()+1]; 
	int* offsetToEndTokenMap = new int[12*sentence_string.size()+1]; 

	int inner_loop_count = 0;
	while (rpMatches.getNextSentence(sTheory->getTokenSequence(), start_token, end_token, sentence, 
		currentMatchData, offsetToStartTokenMap, offsetToEndTokenMap)) 
	{
		dbg_counter++;
		inner_loop_count++;
		if (inner_loop_count > 1000) {
			SessionLogger::warn("regex_pattern_inner_loop") << "Inner loop in RegexPattern exceeds 1000. Bailing out.\n";
			break;
		}
		std::vector<float> scores;
		SessionLogger::dbg("dbg_count_00") << "counter : " << dbg_counter;
		SessionLogger::dbg("dump_regex_0") << "matching: " << _regex_string.c_str() << "\n";
		SessionLogger::dbg("dump_regex_1") << "to      : " << sentence.c_str() << "\n";

		boost::wsregex_iterator iter(sentence.begin(), sentence.end(), _regex_pattern);
		boost::wsregex_iterator end;

		int last_match_end = 0;
		for ( ; iter != end; ++iter) {
			// Match!

			boostMatchResults = *iter;
			
			SessionLogger::dbg("dump_regex_fm_0") << "Found match!\n";
			const boost::wssub_match& boostFullMatch = boostMatchResults[0];

			int match_char_start = (int)boostMatchResults.prefix().length() + last_match_end;
			int match_char_end = match_char_start + (int)boostFullMatch.length() - 1;

			last_match_end = match_char_end + 1;

			// check for zero length regex match, or all whitespace match
			if (match_char_end < match_char_start || isAllWhitespace(sentence, match_char_start, match_char_end)) 
				continue;

			int regex_start_token = offsetToStartTokenMap[match_char_start];
			int regex_end_token = offsetToEndTokenMap[match_char_end];

			int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
			PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
			// Add a feature for the return value (if we have one)
			if (!returnLabelIsEmpty() || getNReturnValuePairs() > 0)
				sfs->addFeature(boost::make_shared<TokenSpanReturnPFeature>(shared_from_this(), sent_no, regex_start_token, regex_end_token, patternMatcher->getActiveLanguageVariant()));
			// Add a feature for the pattern itself
			sfs->addFeature(boost::make_shared<TokenSpanPFeature>(sent_no, regex_start_token, regex_end_token, patternMatcher->getActiveLanguageVariant()));
			// Add a feature for the ID (if we have one)
			addID(sfs);
			// Initialize our score
			sfs->setScore(this->getScore());
			
			// walk over subpatterns to collect PatternFeatures of them 
			// (or make a new PatternFeature, in the case of a TextPattern)
			for (size_t i = 0; i < _subpatterns.size(); i++) {
				Pattern_ptr dp = _subpatterns[i];
				if (boost::dynamic_pointer_cast<TextPattern>(dp)) {
					if (!dp->returnLabelIsEmpty() || dp->getNReturnValuePairs() > 0) {
						int submatch_start_char, submatch_end_char;
						int submatch_index = _subpatternToSubmatchMap[i];
						const boost::wssub_match& boostSubMatch = boostMatchResults[submatch_index];

						findOffsetsOfSubmatch(boostSubMatch, boostFullMatch, match_char_start, submatch_start_char, submatch_end_char);

						// check for zero length submatch match or all whitespace match
						if (submatch_end_char < submatch_start_char || isAllWhitespace(sentence, submatch_start_char, submatch_end_char)) {
							sfs = PatternFeatureSet_ptr();
							break;
						}

						int submatch_start_token = offsetToStartTokenMap[submatch_start_char];
						int submatch_end_token = offsetToEndTokenMap[submatch_end_char];

						// Add a feature for the text pattern's return value
						sfs->addFeature(boost::make_shared<TokenSpanReturnPFeature>(dp, sent_no, submatch_start_token, submatch_end_token, patternMatcher->getActiveLanguageVariant()));
						// Add a feature for the text pattern itself
						sfs->addFeature(boost::make_shared<TokenSpanPFeature>(sent_no, submatch_start_token, submatch_end_token, patternMatcher->getActiveLanguageVariant()));
						// Add a feature for the text pattern's ID (if it has one).  (I don't believe
						// that this should ever be true, but I'm preserving the behavior of old code
						// just in case I'm wrong.)
						if (!dp->getID().is_null()) {
							sfs->addFeature(boost::make_shared<TopLevelPFeature>(dp, patternMatcher->getActiveLanguageVariant()));
						}
					}
				} else {
					MatchStorage::MatchData *md = currentMatchData[i];
					scores.push_back(md->patternFeatureSet->getScore());
					sfs->addFeatures(md->patternFeatureSet);
				}
			}
			
			if (sfs) {
				//BOOST_FOREACH(float score, scores) {
				//	SessionLogger::dbg("BRANDY") << score;	
				//}
				sfs->setScore(_scoringFunction(scores, _score));
				//SessionLogger::dbg("BRANDY") << getDebugID() << ": adding to return vector with original score " << _score << " merged score " << sfs->getScore() << "\n";
				returnVector.push_back(sfs);
				if (!multi_match) break;
			}
		}
		if (!multi_match && returnVector.size() > 0)
			break;
	}

	delete [] offsetToStartTokenMap;
	delete [] offsetToEndTokenMap;
	delete [] currentMatchData;
	return returnVector;
}

Pattern_ptr RegexPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	for (size_t j = 0; j < _subpatterns.size(); j++) {
		replaceShortcut<Pattern>(_subpatterns[j], refPatterns);
		if ( ! (boost::dynamic_pointer_cast<MentionMatchingPattern>(_subpatterns[j]) ||
			    boost::dynamic_pointer_cast<ValueMentionMatchingPattern>(_subpatterns[j]) || 
				boost::dynamic_pointer_cast<TextPattern>(_subpatterns[j])) ) {
            std::ostringstream ostr;
			_subpatterns[j]->dump(ostr);
			SessionLogger::err("SERIF") << "Invalid Regex subpattern: \n" << ostr.str();
			throw UnexpectedInputException("MentionPattern::replaceShortcuts()", 
					"subpatterns of RegexPatterns must be MentionPatterns, ValueMentionPatterns, or TextPatterns");
		}
	}
	return shared_from_this();
}

void RegexPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "RegexPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	// change local value of indent
	indent += 2;
	if (!_allow_heads) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "DON'T ALLOW HEADS\n";
	}
	if (_match_whole_extent) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "MATCH FULL EXTENT\n";
	}
	for (int i = 0; i < indent; i++) out << " ";
	out << "Subpatterns:\n";
	// change local value of indent
	indent += 2;
	for (size_t i = 0; i < _subpatterns.size(); i++) {
		Pattern_ptr subpattern = _subpatterns[i];
		subpattern->dump(out, indent);
	}
}

bool RegexPattern::findOffsetsOfSubmatch(const boost::wssub_match& boostSubMatch, const boost::wssub_match& boostFullMatch, 
										 int match_char_start, int &submatch_char_start, int &submatch_char_end) const
{
	if (boostSubMatch.matched) {
		bool found = true;
		std::wstring::const_iterator submatch_start;
		for (submatch_start = boostFullMatch.first, submatch_char_start = match_char_start;
			 submatch_start != boostSubMatch.first; submatch_start++, submatch_char_start++) 
		{
			// Just to make sure the submatch is not outside the match.
			// This should never happen, though.
			if (submatch_start == boostFullMatch.second) {
				found = false;
				break;
			}
		}
		if (found) {
			submatch_char_end = submatch_char_start + (int)boostSubMatch.length() - 1;
			return true;
		}
	}
	return false;
}


bool RegexPattern::isAllWhitespace(std::wstring &str, int start_offset, int end_offset) const
{
	for (int i = start_offset; i <= end_offset; i++) {
		if (!iswspace(str.at(i)))
			return false;
	}
	return true;
}

/**
 * Redefinition of parent class's virtual method that fills a sequence of 
 * PatternReturnVec objects for a given top-level pattern.
 *
 * @param sequence of PatternReturnVec objects (possibly empty) to which 
 * PatternReturnVec objects will be appended
 *
 * @author afrankel@bbn.com
 * @date 2010.10.20
 **/
void RegexPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	for (size_t n = 0; n < _subpatterns.size(); ++n) {
		if (_subpatterns[n])
			_subpatterns[n]->getReturns(output);
	}
}

bool RegexPattern::containsOnlyTextSubpatterns() const {
	for (size_t i = 0; i < _subpatterns.size(); i++) {
		Pattern_ptr dp = _subpatterns[i];
		if (!boost::dynamic_pointer_cast<TextPattern>(dp))
			return false;
	}
	return true;
}
