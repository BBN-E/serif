// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Metadata.h"
#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/sentences/DefaultSentenceBreaker.h"
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Sentence.h"
#include "Generic/common/limits.h"
#include "Generic/common/StringView.h"
#include <string>
#include <algorithm>
#include <boost/foreach.hpp> 
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

using namespace std;

const wchar_t NEWLINE = 10;
const wchar_t SPACE = 32;
const wchar_t FULL_STOP = 46;
const wchar_t QUESTION_MARK = 63;
const wchar_t EXCLAMATION_POINT = 33;
const wchar_t AMERICAN_END_QUOTE = 34;
const wchar_t SINGLE_QUOTE = 39;
// for right-to-left languages, << is end quote
const wchar_t * RL_END_QUOTE = Symbol(L"\x00AB").to_string(); 

int NUM_WHITESPACE_PER_SENT = 200;

class Metadata;
class DefaultSentenceBreaker;

/*
	Break up a sentence using punctuation if it exists:
		period+space
		period+quotation
		european-style quotation <<
	Else use white space to break it up

	If the resulting sentence is very long, break it up again

	In Arabic or other right-to-left languages,
	the << and ellipses will appear in the wrong place in the XML,
	but careful inspection of the unicode shows that the right thing
	is happening; everything between << xxx >> inclusive is in the sentence,
	the '...' comes at the end of the sentence
*/


DefaultSentenceBreaker::DefaultSentenceBreaker()
{
	// parameter used in both tokenization and sentence-breaking
	// includes both the word as-is and also a lowercase version
	std::string non_final_abbrevs_file = ParamReader::getParam("tokenizer_non_final_abbrevs"); 
	if (non_final_abbrevs_file != "") {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(non_final_abbrevs_file));
		UTF8InputStream& stream(*stream_scoped_ptr);
		if (!stream.fail()) {
			std::wstring line;
			while (!stream.eof()) {
				stream.getLine(line);
				boost::algorithm::trim(line);
				if (line.size() == 0 || line.at(0) == L'#')
					continue;
				_nonFinalAbbrevs.insert(line);
				std::transform(line.begin(), line.end(), line.begin(), towlower);
				_nonFinalAbbrevs.insert(line);
			}
		}
		stream.close();
	}
	_max_token_breaks = 100; // Overridden by language


	_skipTableSentences = ParamReader::isParamTrue("skip_table_sentences");
	_breakTableSentences = ParamReader::isParamTrue("break_table_sentences") || _skipTableSentences;
	_minTableRows = (size_t)ParamReader::getOptionalIntParamWithDefaultValue("sentence_breaker_min_table_rows", 20);
}

DefaultSentenceBreaker::~DefaultSentenceBreaker() 
{}

void DefaultSentenceBreaker::resetForNewDocument(const Document *doc) 
{
	_curDoc = doc;
	_curRegion = 0;
	_cur_sent_no = 0;
}

int DefaultSentenceBreaker::getSentencesRaw(Sentence **results, int max_sentences,
		const Region* const* regions, int num_regions) 
{ 
	if (num_regions < 1) {
		SessionLogger::warn_user("no_document_regions") << "No readable content in this document.";
		return 0;
	}
	
	int start_index = 0;
	for (int i = 0; i < num_regions; i++) {
		start_index = 0;
		_curRegion = regions[i];
		
		while ( start_index < _curRegion->getString()->length()) { 
			if (_cur_sent_no == max_sentences) {
				SessionLogger::warn_user("too_many_sentences") << "Truncating document with more than the maximum " << max_sentences << " sentences."; 
				break;
			}
			start_index = getNextSentence(results, start_index, _curRegion->getString());
			
            
			if (results[_cur_sent_no]){
				if (results[_cur_sent_no]->getNChars() > 0) {
					if (results[_cur_sent_no]->getNChars() > getMaxSentenceChars()) 
						simpleAttemptLongSentenceBreak(results, max_sentences);
					_cur_sent_no++;
				}
			}
		}
	}
	return _cur_sent_no;
}



/*
	Find one substring that is a sentence,
	return the length of that sentence to become
	the starting point of the next one.
*/
int DefaultSentenceBreaker::getNextSentence(Sentence **results, int start_index, const LocatedString* input)
{
	// Start after any initial whitespace.
	while ((start_index < input->length()) && (iswspace(input->charAt(start_index)))) {
		start_index++;
	}

	// If we've reached the end, there's no sentence.
	if (start_index >= input->length()) {
		results[_cur_sent_no] = 0;
		return start_index;
	}

	// Look for possible sentence breaks, take first one that appears
	int stop_index = start_index;

	// No replacements -- ONLY sentence breaking -- so it's OK to move to just a wstring rather than a LocatedString
	wstring input_string = input->toWString();
	
	stop_index = findEllipsis(start_index, input_string);
	
	if (stop_index == start_index) {
		stop_index = findPeriodPlus(start_index, input_string);
	}
	
	if (stop_index == start_index)
		stop_index = findClosingQuotation(start_index, input_string);
	
	if (stop_index == start_index)
		stop_index = findWhiteSpaceXTimes(start_index, input_string);
	//
	//
	// Give up and take the whole string if no sentence indicators were found
	if (stop_index == start_index)
		stop_index = input->length();

	
	LocatedString *sub;
	sub = input->substring(start_index, stop_index);
//	if(_curDoc->getSourceType()==DefaultDocumentReader::DF_SYM)
	//results[_cur_sent_no] = _new Sentence(_curDoc, _curZone, _cur_sent_no, sub);
	//else
	results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
	delete sub;

	return stop_index;
}

int DefaultSentenceBreaker::findEllipsis(int start_index, wstring input_string)
{
	size_t first_stop(0);
	size_t next_stop(0);
	size_t last_stop(0);
	
	first_stop = input_string.find_first_of(FULL_STOP, start_index);
	if (first_stop > 0)
		next_stop = input_string.find_first_of(FULL_STOP,first_stop+1);
	else 
		first_stop = 0;
	if (next_stop - first_stop == 1) {
		last_stop = first_stop;
		while (next_stop - last_stop == 1) {
			last_stop = next_stop;
			next_stop = input_string.find_first_of(FULL_STOP,last_stop+1);
		}
		return static_cast<int>(last_stop)+1; // last full stop within the ellipsis
	}
	else
		return start_index; //next full stop in string is not an ellipsis
}	

int DefaultSentenceBreaker::findPeriodPlus(int start_index, wstring input_string)
{
	int stop_index = start_index;
	size_t full_stop_index = input_string.find(FULL_STOP, start_index);

	// if we found a period in the middle of the string, make sure it's not a non-final abbrev
	// if it is, keep looking
	while (full_stop_index != std::wstring::npos && full_stop_index != input_string.size() - 1) {
		std::wstring final_word = getWordEndingAt(static_cast<int>(full_stop_index), input_string, start_index);
		if (_nonFinalAbbrevs.find(final_word) != _nonFinalAbbrevs.end()) {
			full_stop_index = input_string.find(FULL_STOP, full_stop_index+1);
		} else break;
	}

	// no period found
	if (full_stop_index == std::wstring::npos)
		return start_index;
	
	// make sure the period is followed by a space or a quote or something like that, otherwise keep looking
	while ((full_stop_index+1) > 0 && (full_stop_index+1) < input_string.length()) {
		const wchar_t next_char = input_string.at(full_stop_index+1);
		if (next_char == SPACE || 
			next_char == NEWLINE ||
			next_char == AMERICAN_END_QUOTE ||
			next_char == SINGLE_QUOTE)
		{
			stop_index = static_cast<int>(full_stop_index)+2;
			break;
		}
		full_stop_index = input_string.find_first_of(FULL_STOP, full_stop_index+1);
	}	

	return stop_index;
}


int DefaultSentenceBreaker::findWhiteSpaceXTimes(int start_index, wstring input_string)
{
	int stop_index = start_index;
	int ws_count = 0;
	int ws_found = 0;
	
	while (ws_count < NUM_WHITESPACE_PER_SENT){
		ws_found = static_cast<int>(input_string.find(SPACE,stop_index+1));
		if (ws_found > 0) {
			stop_index = ws_found;
			ws_count++;
		}
		else {
			// no more spaces found, take the remainder of the string
			stop_index = static_cast<int>(input_string.length());
			break;
		}
	}
	return stop_index;
}

int DefaultSentenceBreaker::findClosingQuotation(int start_index, wstring input_string)
{
	int stop_index = start_index;
	int found_quote = static_cast<int>(input_string.find(AMERICAN_END_QUOTE, start_index));
	if (found_quote > 0)
		stop_index = found_quote+1;
	else {

		found_quote = static_cast<int>(input_string.find(RL_END_QUOTE, start_index));
		if (found_quote > 0)
			stop_index = found_quote+1;
	}
	return stop_index;
}



void DefaultSentenceBreaker::simpleAttemptLongSentenceBreak(Sentence **results, int max_sentences) 
{
	const LocatedString* locstring = results[_cur_sent_no]->getString();
	int start_index = 0;
	int stop_index = 0;
	LocatedString *sub;
	wstring input_string = locstring->toWString();

	while (start_index < locstring->length()){
		stop_index = findWhiteSpaceXTimes(start_index, input_string);
		sub = locstring->substring(start_index, stop_index);
		_cur_sent_no++;
		results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
		start_index = stop_index;
	}
	delete sub;
	if (_cur_sent_no > max_sentences) {
		throw UnexpectedInputException("DefaultSentenceBreaker::attemptLongSentenceBreak()",
									   "Document contains more than max_sentences sentences");
	}
	// decrement sent_no to point to last complete sentence
	_cur_sent_no--;
}




// Attempts to break sentence into ~n_split_sents smaller sentences
std::vector<Sentence*> DefaultSentenceBreaker::attemptLongSentenceBreak(const Sentence *sentence, int n_split_sents) {
	std::vector<Sentence*> results;

	if (n_split_sents == 0 ||  n_split_sents == 1) 
		return results;
	
	int orig_sent_len = sentence->getString()->length();
	int avg_num_chars = orig_sent_len/n_split_sents;
	
	SessionLogger::dbg("brk_sent_0") << "DefaultSentenceBreaker::attemptLongSentenceBreak(),"
							 << " Attempting to break sentence with " << orig_sent_len
							 << " characters into smaller sentences with " << avg_num_chars
							 << " characters on average.\n";
	
	int sent_start = 0;
	int curr_char = avg_num_chars;
	
	while (curr_char < orig_sent_len) { 

		int window_size, left_idx, right_idx;
		bool found_breakpoint = false;
		int breakpoint = -1;
		std::string justification = "";
		
		/* First, lets look to see if there's a comma nearby */
		for (window_size = 0; window_size < 100 && !found_breakpoint; window_size++) {
			left_idx = curr_char - window_size;
			right_idx = curr_char + window_size;

			// stop searching if both left and right index are out of bounds
			if ((left_idx <= sent_start) && ((right_idx >= orig_sent_len) || (right_idx - sent_start + 1 > getMaxSentenceChars())))
				break;

			if ((left_idx > sent_start) &&
				isSecondaryEOSChar(sentence->getString()->charAt(left_idx)) && 
				!inRestrictedBreakSpan(_curDoc->getMetadata(), sentence->getString(), left_idx + 1)) 
			{
				breakpoint = left_idx;
				justification = "punctuation";
				found_breakpoint = true;
			}
			else if ((right_idx < orig_sent_len) && (right_idx - sent_start + 1 <= getMaxSentenceChars()) &&
					isSecondaryEOSChar(sentence->getString()->charAt(right_idx)) &&
				    !inRestrictedBreakSpan(_curDoc->getMetadata(), sentence->getString(), right_idx + 1))
			{
				breakpoint = right_idx;
				justification = "punctuation";
				found_breakpoint = true;
			}
		}

		/* No comma, so just break at a whitespace point */
		for (window_size = 0; !found_breakpoint; window_size++) {
			left_idx = curr_char - window_size;
			right_idx = curr_char + window_size;
			
			// stop searching when both left and right index are out of bounds
			if ((left_idx <= sent_start) && ((right_idx >= orig_sent_len) || (right_idx - sent_start + 1 > getMaxSentenceChars())))
				break;

			if ((left_idx > sent_start) &&
				iswspace(sentence->getString()->charAt(left_idx)) &&
				!inRestrictedBreakSpan(_curDoc->getMetadata(), sentence->getString(), left_idx + 1)) 
			{
				breakpoint = left_idx;
				justification = "whitespace";
				found_breakpoint = true;
			}
			else if ((right_idx < orig_sent_len) && (right_idx - sent_start + 1 <= getMaxSentenceChars()) &&
					iswspace(sentence->getString()->charAt(right_idx)) &&
					!inRestrictedBreakSpan(_curDoc->getMetadata(), sentence->getString(), right_idx + 1)) 
			{
				breakpoint = right_idx;
				justification = "whitespace";
				found_breakpoint = true;
			}
		}

		
		if (!found_breakpoint) {
			SessionLogger::warn("bad_sentence_break") << " Could not find good break point, breaking at random.";
		}

		/* Can't find a good break, so find the first non-restricted breakpoint near curr_char */
		for (window_size = 0; !found_breakpoint; window_size++) {
			left_idx = curr_char - window_size;
			right_idx = curr_char + window_size;

			// stop searching when both left and right index are out of bounds
			if ((left_idx <= sent_start) && ((right_idx >= orig_sent_len) || (right_idx - sent_start + 1 > getMaxSentenceChars())))
				break;
				
			if ((left_idx > sent_start) &&
				!inRestrictedBreakSpan(_curDoc->getMetadata(), sentence->getString(), left_idx + 1)) 
			{
				breakpoint = left_idx;
				justification = "random";
				found_breakpoint = true;
			}
			else if ((right_idx < orig_sent_len) && (right_idx - sent_start + 1 <= getMaxSentenceChars()) &&
					!inRestrictedBreakSpan(_curDoc->getMetadata(), sentence->getString(), right_idx + 1)) 
			{
				breakpoint = right_idx;
				justification = "random";
				found_breakpoint = true;
			}
		}

		if (!found_breakpoint) {
			throw UnexpectedInputException("DefaultSentenceBreaker::attemptLongSentenceBreak()",
										   "Could not find unrestricted sentence break point within max_sentence_chars range.");
		}

		int next_sent_no = sentence->getSentNumber() + static_cast<int>(results.size());
		results.push_back(createSubSentence(sentence, next_sent_no, sent_start, breakpoint, justification));
		sent_start = breakpoint + 1;
		curr_char = sent_start + avg_num_chars;
	
		// if the remaining string is an acceptable size, finish out the rest of sentence
		if ((sent_start < orig_sent_len) && isAcceptableSentenceString(sentence->getString(), sent_start, orig_sent_len)) {
			next_sent_no = sentence->getSentNumber() + static_cast<int>(results.size());
			results.push_back(createFinalSubSentence(sentence, next_sent_no, sent_start));
			sent_start = orig_sent_len + 1;
			curr_char = orig_sent_len + 1;
		}

	}

	return results;
}

bool DefaultSentenceBreaker::attemptLongSentenceBreak(Sentence **results, int max_sentences) {
	Sentence *thisSentence = results[_cur_sent_no];

	int token_break_count = countPossibleTokenBreaks(thisSentence->getString());
	if (token_break_count <= _max_token_breaks) 
		return false;

	int n_split_sents = static_cast<int>(ceil((double (token_break_count))/_max_token_breaks));
	std::vector<Sentence*> subSentences = attemptLongSentenceBreak(thisSentence, n_split_sents);

	// if no sub-sentences returned, keep this sentence as-is
	if (subSentences.empty()) 
		return false;

	if (_cur_sent_no + static_cast<int>(subSentences.size()) > max_sentences) {
		throw UnexpectedInputException("DefaultSentenceBreaker::attemptLongSentenceBreak()",
			"Document contains more than max_sentences sentences");
	}

	BOOST_FOREACH(Sentence *sent, subSentences) {
		results[_cur_sent_no++] = sent;
	}

	// decrement sent_no to point to last complete sentence
	_cur_sent_no--; 

	delete thisSentence;

	return true;
}

bool DefaultSentenceBreaker::attemptTableSentenceBreak(Sentence **results, int max_sentences) {
	Sentence *thisSentence = results[_cur_sent_no];

	// Only try this table check on a probable long sentence
	int token_break_count = countPossibleTokenBreaks(thisSentence->getString());
	if (token_break_count <= _max_token_breaks) 
		return false;

	// Find commonly occurring punctuation (likely to be a good fixed point)
	boost::unordered_map<wchar_t, size_t> punctuationCounts;
	int sum = 0;
	for (int i = 0; i < thisSentence->getNChars(); i++) {
		wchar_t thisChar = thisSentence->getString()->charAt(i);
		if (iswpunct(thisChar)) {
			punctuationCounts[thisChar]++;
			sum++;
		}
	}
	boost::unordered_set<wchar_t> commonPunctuation;
	for (boost::unordered_map<wchar_t, size_t>::iterator p = punctuationCounts.begin(); p != punctuationCounts.end(); p++) {
		if (p->second > sum/punctuationCounts.size()) {
			commonPunctuation.insert(p->first);
		}
	}

	// Break the candidate sentence down to its character classes
	std::wstringstream characterClassStream;
	wchar_t lastCharacterClass = L'X';
	for (int i = 0; i < thisSentence->getNChars(); i++) {
		// Determine which class this character falls in
		wchar_t character = thisSentence->getString()->charAt(i);
		wchar_t characterClass = L'X';
		if (iswspace(character))
			characterClass = L's';
		else if (iswdigit(character))
			characterClass = L'd';
		else if (commonPunctuation.count(character))
			characterClass = character;
		else
			characterClass = L'w';

		// Update our sequence of characters
		if (characterClass != lastCharacterClass) {
			if (lastCharacterClass != L'X')
				characterClassStream << lastCharacterClass;
			lastCharacterClass = characterClass;
		}
	}
	if (lastCharacterClass != L'X')
		characterClassStream << lastCharacterClass;
	std::wstring characterClasses = characterClassStream.str();

	// Initialize a view into our classified string, to minimize substring copies
	WStringView view;
	view.string = characterClasses.c_str();
	view.index = 0;
	view.length = characterClasses.length();
	boost::hash<WStringView> viewHasher;

	// Count suffixes by prefix
	size_t minPrefixLength = 4;
	size_t maxPrefixLength = (size_t)ceil(((float)characterClasses.length())/_minTableRows);
	boost::unordered_map<size_t, int> commonPrefixCounts;
	boost::unordered_map<size_t, WStringView> commonPrefixes;
	for (view.index = 0; view.index < characterClasses.length(); view.index++) {	
		for (view.length = minPrefixLength; view.length < maxPrefixLength && view.index + view.length < characterClasses.length(); view.length++) {
			size_t prefixHash = viewHasher(view);
			if (commonPrefixCounts.find(prefixHash) == commonPrefixCounts.end()) {
				commonPrefixCounts[prefixHash] = 1;
				commonPrefixes[prefixHash] = view;
			} else
				commonPrefixCounts[prefixHash]++;
		}
	}

	// Determine the longest, most common prefix
	std::wstring* mostCommonPrefixes = _new std::wstring[maxPrefixLength];
	int* mostCommonPrefixCounts = _new int[maxPrefixLength];
	for (size_t l = minPrefixLength; l < maxPrefixLength; l++)
		mostCommonPrefixCounts[l] = 0;
	for (boost::unordered_map<size_t, WStringView>::iterator cp = commonPrefixes.begin(); cp != commonPrefixes.end(); cp++) {
		//std::cout << cp->first << ": " << cp->second << std::endl;
		size_t i = cp->second.index;
		size_t l = cp->second.length;
		int c = commonPrefixCounts[cp->first];
		if (c > mostCommonPrefixCounts[l]) {
			mostCommonPrefixes[l] = characterClasses.substr(i, l);
			mostCommonPrefixCounts[l] = c;
		}
	}

	// Look for a sudden jump in count
	std::wstring bestPrefix = L"";
	size_t maxDiff = _minTableRows;
	for (size_t l = maxPrefixLength - 2; l >= minPrefixLength; l--) {
		size_t diff = static_cast<size_t>(abs(mostCommonPrefixCounts[l] - mostCommonPrefixCounts[l+1]));
		//std::cout << l << "\t" << mostCommonPrefixes[l] << "\t" << mostCommonPrefixCounts[l] << "\t" << diff << std::endl;
		if (diff > maxDiff) {
			// Take the biggest jump that isn't a substring of the current best prefix
			bool substring = bestPrefix.find(mostCommonPrefixes[l]) != std::wstring::npos;
			bool halfLength = mostCommonPrefixes[l].length() > bestPrefix.length()*0.5;
			//std::cout << mostCommonPrefixes[l] << " in " << bestPrefix << ": " << (substring ? "true" : "false") << std::endl;
			//std::cout << mostCommonPrefixes[l].length() << " > " << bestPrefix.length()*0.5 << ": " << (halfLength ? "true" : "false") << std::endl;
			if (!substring || halfLength) {
				maxDiff = diff;
				bestPrefix = mostCommonPrefixes[l];
			}
		}
	}
	delete[] mostCommonPrefixes;
	delete[] mostCommonPrefixCounts;

	// Convert the best prefix to a regex pattern
	boost::trim_if(bestPrefix, boost::is_any_of(L"s"));
	if (bestPrefix.size() == 0)
		return false;
	const boost::wregex escapeSearch(L"([\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\])");
	const std::wstring escapeReplace(L"\\\\\\1");
	std::wstring bestPattern = regex_replace(bestPrefix, escapeSearch, escapeReplace, boost::match_default | boost::format_sed);
	boost::replace_all(bestPattern, L"w", L"\\w+");
	boost::replace_all(bestPattern, L"d", L"\\w+");
	boost::replace_all(bestPattern, L"s", L"\\s+");
	boost::wregex rowSearch(bestPattern);

	// Get the likely table row matches, and insert rows in the spaces between
	size_t prevEnd = 0;
	boost::wsregex_iterator rowMatches(thisSentence->getString()->toWString().begin(), thisSentence->getString()->toWString().end(), rowSearch);
	boost::wsregex_iterator matchEnd;
	typedef std::pair<size_t, size_t> MatchBounds;
	std::vector<MatchBounds> tableRows;
	for (; rowMatches != matchEnd; rowMatches++) {
		size_t start = rowMatches->position();
		size_t end = start + rowMatches->length();
		//std::cout << "[" << start << "," << end << "): " << rowMatches->str() << std::endl;
		if (start != prevEnd)
			tableRows.push_back(MatchBounds(prevEnd, start));
		tableRows.push_back(MatchBounds(start, end));
		prevEnd = end;
	}
	size_t sentenceEnd = static_cast<size_t>(thisSentence->getNChars());
	if (prevEnd < sentenceEnd) {
		tableRows.push_back(MatchBounds(prevEnd, sentenceEnd));
	}

	// Only split if it's a table with enough rows
	if (tableRows.size() < _minTableRows)
		return false;

	// Check if we're keeping this table
	if (!_skipTableSentences) {
		// Create sentences from the row offsets
		SessionLogger::updateContext(2, boost::lexical_cast<std::string>(_cur_sent_no).c_str());
		SessionLogger::dbg("break_table") << "Probable table found with " << tableRows.size() << " >= " << _minTableRows << " row of pattern '" << bestPattern << "'";
		std::vector<Sentence*> subSentences;
		int next_sent_no = _cur_sent_no;
		BOOST_FOREACH(MatchBounds row, tableRows) {
			subSentences.push_back(createSubSentence(thisSentence, next_sent_no, static_cast<int>(row.first), static_cast<int>(row.second) - 1, "table"));
			next_sent_no++;
		}

		if (_cur_sent_no + static_cast<int>(subSentences.size()) > max_sentences) {
			// Too many table rows!
			throw UnexpectedInputException("DefaultSentenceBreaker::attemptTableSentenceBreak()",
				"Document contains more than max_sentences sentences");
		}

		// Keep our newly split up table
		BOOST_FOREACH(Sentence *sent, subSentences) {
			results[_cur_sent_no++] = sent;
		}
	}

	// Decrement sent_no to point to last complete sentence
	_cur_sent_no--; 

	// Don't leak the original long table sentence
	delete thisSentence;

	// Indicate that we modified the sentence
	return true;
}

// Returns a new Sentence created from a substring of the sentence passed in
Sentence* DefaultSentenceBreaker::createSubSentence(const Sentence *sentence, int sent_no, int start, int end, std::string justification) {
	SessionLogger::dbg("subst_0") << "---Substring from: " << start << " to " << end << " [" << justification << "]\n";
	LocatedString* subString = sentence->getString()->substring(start, end+1);

	Sentence *subSentence=_new Sentence(sentence->getDocument(), sentence->getRegion(), sent_no, subString);
	
	delete subString;
	return subSentence;
}

// Returns a new Sentence created from a substring of the sentence passed in
Sentence* DefaultSentenceBreaker::createFinalSubSentence(const Sentence *sentence, int sent_no, int start) {
	SessionLogger::dbg("final_subst_0") << "Final Substring from: " << start << " to end\n";
	LocatedString* subString = sentence->getString()->substring(start);
	Sentence *subSentence= _new Sentence(sentence->getDocument(), sentence->getRegion(), sent_no, subString);
	delete subString;
	return subSentence;
}

// Returns true if a sentence break is NOT permitted between index and index-1 
bool DefaultSentenceBreaker::inRestrictedBreakSpan(Metadata *metadata, const LocatedString *input, int index) const {
	if (index < 1 || index >= input->length()) 
		return false;

	bool restrict_break = false;

	EDTOffset last_index = input->lastEndOffsetEndingAt<EDTOffset>(index - 1);
	EDTOffset next_index = input->start<EDTOffset>(index);
	Metadata::SpanList *list = metadata->getCoveringSpans(last_index);
	for (int i = 0; i < list->length(); i++)
	{
		Span *coveringSpan = (*list)[i];
		if (coveringSpan->restrictSentenceBreak() && 
			coveringSpan->covers(next_index)) 
		{
			restrict_break = true;
			break;
		}
	}
	delete list;
	return restrict_break;
}

// Returns a count of the number of whitespace/punctuation spans
// between index start and index end in string; multiple 
// contiguous chars count as one span. Estimate of number of tokens.
int DefaultSentenceBreaker::countPossibleTokenBreaks(const LocatedString* string, int start, int end) {
	int tb = 0;
	for (int i = start; i < end && i < string->length(); i++) {
		if (iswspace(string->charAt(i)) || iswpunct(string->charAt(i))) {
			tb++;
			while (i < string->length()-1 && (iswspace(string->charAt(i+1)) || iswpunct(string->charAt(i+1))))
				i++;
		}
	}
	return tb;
}

int DefaultSentenceBreaker::countPossibleTokenBreaks(const LocatedString* string){
	return countPossibleTokenBreaks(string, 0, string->length());
}

// Returns true if substring [start, end) is an acceptable sub-sentence for
// use in attemptLongSentenceBreak
bool DefaultSentenceBreaker::isAcceptableSentenceString(const LocatedString *sentence, int start, int end) { 
	return (end - start < getMaxSentenceChars()); 
}

std::wstring DefaultSentenceBreaker::getWordEndingAt(int index, std::wstring input_string, int origin) {
	int start = index - 1;
	while (start >= origin && !iswspace(input_string.at(start)))
		start--;

	int length = index - start;
	return input_string.substr(start+1, length);
}
