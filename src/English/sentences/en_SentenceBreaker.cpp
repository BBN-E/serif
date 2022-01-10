// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include <math.h>
#include <vector>
#include <wchar.h>
#include <boost/algorithm/string.hpp> 
#include <boost/foreach.hpp> 
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>

#include "Generic/common/leak_detection.h"

#include "English/sentences/en_SentenceBreaker.h"
#include "Generic/sentences/StatSentenceBreaker.h"
#include "Generic/common/limits.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/InputUtil.h"
#include "English/common/en_StringTransliterator.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Span.h"
#include "English/sentences/WordSet.h"
#include "Generic/reader/DefaultDocumentReader.h"

#define MAX_ABBREV_WORD_LEN        3
#define EOS_PUNCTUATION            L".?!"
#define SECONDARY_EOS_PUNCTUATION  L";,"
#define SPLIT_PUNCTUATION          L"?!"
#define OPENING_PUNCTUATION        L"(`'[{\"\x2018\x201C`"
#define CLOSING_PUNCTUATION        L")'']}\"\x2019\x201D\x00B4"
#define BULLET_PUNCTUATION         L"-"

static Symbol BLOG_SOURCE_SYM = Symbol(L"blog");
static Symbol WEBLOG_SOURCE_SYM = Symbol(L"weblog");
static Symbol USENET_SOURCE_SYM = Symbol(L"usenet");
static Symbol UNKNOWN_SOURCE_SYM = Symbol(L"UNKNOWN");

class Metadata;

EnglishSentenceBreaker::EnglishSentenceBreaker() {

	_nonFinalAbbreviations = NULL;
	_noSplitAbbreviations = NULL;
	_knownWords = NULL;

	std::string input_file = ParamReader::getParam("sentence_breaker_debug_file");
	DEBUG = (!input_file.empty());
	if (DEBUG) _debugStream.open(input_file.c_str());

	input_file = ParamReader::getRequiredParam("tokenizer_non_final_abbrevs");
	_nonFinalAbbreviations = _new WordSet(input_file.c_str(), false);

	input_file = ParamReader::getRequiredParam("tokenizer_no_split_abbrevs");
	_noSplitAbbreviations = _new WordSet(input_file.c_str(), false);
	
	input_file = ParamReader::getRequiredParam("tokenizer_short_words");
	_knownWords = _new WordSet(input_file.c_str(), false);

	input_file = ParamReader::getRequiredParam("lowercase_headline_words");
	_lowercaseHeadlineWords = _new WordSet(input_file.c_str(), false);
	
	input_file = ParamReader::getRequiredParam("sentence_breaker_dateline_parentheticals");
	_datelineParentheticals = InputUtil::readFileIntoSet(input_file, false, false);

	// Dateline handling modes:
	//
	// NONE: do not handle datelines specially at all
	// conservative: only break on known dateline parenthetical constructions, e.g. (AP)
	// aggressive: also break on certain punctuation near the beginning of the document, 
	//   and be somewhat more aggressive about breaking on parentheticals
	// very aggressive: break on certain punctuation when followed by something that looks like 
	//   more dateline stuff; break on "buried" bylines
	//
	// When in doubt, 'aggressive' is probably the best bet for unknown possibly-news data.
	//
	std::string dateline_mode_str = ParamReader::getRequiredParam("sentence_breaker_dateline_mode");
	if (dateline_mode_str == "NONE")
		DATELINE_MODE = DL_NONE;
	else if (dateline_mode_str == "conservative")
		DATELINE_MODE = DL_CONSERVATIVE;
	else if (dateline_mode_str == "aggressive")
		DATELINE_MODE = DL_AGGRESSIVE;
	else if (dateline_mode_str == "very_aggressive")
		DATELINE_MODE = DL_VERY_AGGRESSIVE;
	else throw UnexpectedInputException("EnglishSentenceBreaker::EnglishSentenceBreaker()", 
		"Parameter 'sentence_breaker_dateline_mode' must be set to 'NONE', 'conservative', 'aggressive' or 'very_aggressive'.");

	input_file = ParamReader::getRequiredParam("sentence_breaker_rarely_capitalized_words");
	std::set<std::wstring> tempSet = InputUtil::readFileIntoSet(input_file, false, false);
	if (DATELINE_MODE == DL_VERY_AGGRESSIVE) {
		BOOST_FOREACH(std::wstring word, tempSet) {
			// Add surrounding whitespace
			word = L" " + word + L" ";
			_rarelyCapitalizedWords.insert(word);
			// Now add the all-uppercase version; the first version should have been regular capitalized
			boost::to_upper(word);
			_rarelyCapitalizedWords.insert(word);
		}
	}

	_skipHeadlines = ParamReader::isParamTrue("skip_headlines");
	_downcaseHeadlines = ParamReader::isParamTrue("downcase_headlines");
	if (_skipHeadlines && _downcaseHeadlines) {
		throw UnexpectedInputException("EnglishSentenceBreaker::EnglishSentenceBreaker()", 
		"Parameters 'skip_headlines' and 'downcase_headlines' cannot both be set to true.");
	}

	_useITEASentenceBreakHeuristics = ParamReader::isParamTrue("use_itea_sentence_break_heuristics");
	_useGALEHeuristics = ParamReader::isParamTrue("use_GALE_sentence_break_heuristics");
	_useWholeDocDatelineMode = ParamReader::isParamTrue("use_dateline_mode_on_whole_document");
	_useRegionContentFlags = ParamReader::isParamTrue("sentence_breaker_use_region_content_flags");
	_breakOnDoubleCarriageReturns = ParamReader::isParamTrue("sentence_break_on_double_carriage_returns");
	_aggressiveDoubleCarriageReturnsSplit = ParamReader::isParamTrue("split_aggressively_on_double_carriage_returns"); // Split on double carriage return even when the next character is lowercase
	_alwaysUseBreakableColons = ParamReader::isParamTrue("always_use_breakable_colons");
	_breakLongSentences = true;
	_breakListSentences = ParamReader::isParamTrue("break_long_list_sentences");
	_listSeparators = UnicodeUtil::toUTF16StdString(ParamReader::getParam("sentence_breaker_list_separators"));
	_minListSeparators = ParamReader::getOptionalIntParamWithDefaultValue("sentence_breaker_min_list_separators", 10);
	_max_token_breaks = ParamReader::getOptionalIntParamWithDefaultValue("en_sentence_breaker_max_token_breaks", 100);
	_treatUnknownDocsAsWebText = ParamReader::isParamTrue("treat_unknown_docs_as_web_text");
	_short_line_length = ParamReader::getOptionalIntParamWithDefaultValue("sentence_breaker_short_line_length", 40);
	_break_on_portion_marks = ParamReader::isParamTrue("sentence_break_on_portion_marks");
	_ignore_page_breaks = ParamReader::isParamTrue("sentence_breaker_ignore_page_breaks");
	_dont_break_in_parentheticals = ParamReader::isParamTrue("tokenizer_ignore_parentheticals");
	_break_on_footnote_numbers = ParamReader::isParamTrue("break_on_footnote_numbers");

	// These are pieces of punctuation that help us deal with datelines
	_punctuatedLeads.push_back(L":");
	_punctuatedLeads.push_back(L"--");
	_punctuatedLeads.push_back(L" / ");
	_punctuatedLeads.push_back(L"==");
	_punctuatedLeads.push_back(L"\u2014\u2014"); //em-dash
	_punctuatedLeads.push_back(L" - ");
	_punctuatedLeads.push_back(L" \u2014 "); //em-dash
	_punctuatedLeads.push_back(L"\u2022"); //bullet
	_punctuatedLeads.push_back(L" * ");
	_parenPairs.push_back(std::pair<std::wstring, std::wstring>(L"(",L")"));
	_parenPairs.push_back(std::pair<std::wstring, std::wstring>(L"[",L"]"));
	_parenPairs.push_back(std::pair<std::wstring, std::wstring>(L" /",L"/ "));

	for (int i = 1; i <= 31; i++) {
		std::wstringstream temp;
		temp << i;
		_days.insert(temp.str());
	}

	_months.insert(L"january");
	_months.insert(L"february");
	_months.insert(L"march");
	_months.insert(L"april");
	_months.insert(L"may");
	_months.insert(L"june");
	_months.insert(L"july");
	_months.insert(L"august");
	_months.insert(L"september");
	_months.insert(L"october");
	_months.insert(L"november");
	_months.insert(L"december");
}

EnglishSentenceBreaker::~EnglishSentenceBreaker() {
	if (_nonFinalAbbreviations != NULL) {
		delete _nonFinalAbbreviations;
	}
	if (_noSplitAbbreviations != NULL) {
		delete _noSplitAbbreviations;
	}
	if (_knownWords != NULL) {
		delete _knownWords;
	}
	if (_lowercaseHeadlineWords != NULL) {
		delete _lowercaseHeadlineWords;
	}
	if (DEBUG) _debugStream.close();
}

/**
 * Sets the current document pointer to point to the
 * document passed in and resets the current sentence
 * number to 0.
 */
void EnglishSentenceBreaker::resetForNewDocument(const Document *doc) {
	_curDoc = doc;
	_curRegion = 0;
	_cur_sent_no = 0;
	while (!_quoteLevel.empty())
		_quoteLevel.pop();
}

/**
 * Takes an array of pointers to LocatedStrings corresponding to the regions of
 * the document text and the number of regions. It puts an array of pointers to
 * Sentences where specified by results arg, and returns its size.
 *
 * Note that it is assumed that every region boundary is also a sentence
 * boundary, that is, no single sentence can span two regions.
 *
 * The caller is responsible for deleting the array of Sentences and all the
 * Sentence objects stored in it.
 *
 * @param results an output parameter containing an array of
 *                pointers to Sentences that will be filled in
 *                with the sentences found by the sentence breaker.
 * @param max_sentences the size of the results arrays, i.e., the maximum number
 *                      of sentences that can broken up and placed in the array.
 * @param text an array of pointers to text regions from the input file.
 * @param num_regions the size of the text array, i.e., the number of text regions.
 * @return the number of sentences found and placed in the results array.
 * @throws UnexpectedInputException if more than max_sentences sentences are found.
 */
int EnglishSentenceBreaker::getSentencesRaw(Sentence **results,
									 int max_sentences,
									 const Region* const* regions,
									 int num_regions)
{
	if (num_regions < 1 ) {
		SessionLogger::warn_user("no_document_regions") << "No readable content in this document.";
		return 0;
	}


	_isAllLowercase = true;
	for (int m = 0; m < num_regions; m++) {
		_curRegion = regions[m];
		const LocatedString *text = _curRegion->getString();
		for (int k = 0; k < text->length(); k++) {
			if (iswupper(text->charAt(k))) {
				_isAllLowercase = false;
				break;
			}
		}
		if (!_isAllLowercase)
			break;
	}

	if (_curDoc->getSourceType() == BLOG_SOURCE_SYM ||
		_curDoc->getSourceType() == WEBLOG_SOURCE_SYM ||
		_curDoc->getSourceType() == USENET_SOURCE_SYM ||
		(_treatUnknownDocsAsWebText && _curDoc->getSourceType() == UNKNOWN_SOURCE_SYM))
	{
		_isWebText = true;
	} else _isWebText = false;

	// if you want to turn off sentence breaking and have it use line breaks as sentence breaks,
	// this is the section to trigger for you.
	bool use_line_breaks_as_sentence_breaks = false;
	if (use_line_breaks_as_sentence_breaks) {
		for (int i = 0; i < num_regions; i++) {
			_curRegion = regions[i];
			const LocatedString *text = _curRegion->getString();
			const int len = text->length();
			int start = 0;
			int end = 0;
			if (len == 0)
				continue;
			while (end <= len) {
				if (_cur_sent_no == max_sentences) {
					throw UnexpectedInputException("EnglishSentenceBreaker::getSentences()",
												"Document contains more than max_sentences sentences");
				}
				// EMB: change here to make sure a final gets created even if the last character isn't a '\n'
				//      (probably shouldn't happen in this kind of input, but it could)
				if (end != len && text->charAt(end) != L'\n')
					end++;
				else {
					LocatedString *sentenceString = text->substring(start, end);
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
					delete sentenceString;
					_cur_sent_no++;
					end++;
					start = end;
				}
			}			
		}
		return _cur_sent_no;
	}

	int text_index = 0;

	// Read the rest of the document.
	int last_headline_sent_no = max_sentences;  // Used to do intelligent dateline processing
	for (int i = 0; i < num_regions; i++) {
		_curRegion = regions[i];
		LocatedString *text = _new LocatedString(*_curRegion->getString());

		// optionally override behavior based on region flags
		if (_useRegionContentFlags) {
			// if region is double-spaced text, can't use two newlines to break paragraphs/sentences
			_breakOnDoubleCarriageReturns = !_curRegion->hasContentFlag(Region::DOUBLE_SPACED);

			// if region is justified, don't treat it as web text
			_isWebText = !_curRegion->hasContentFlag(Region::JUSTIFIED);
		}
		
		// fix any funny whitespace
		text->replaceNonstandardUnicodeWhitespace();

		// JG. normalize unkown dates to avoid the sentence breaker having a field-day with them...
		// some ?'s are escaped because ??- is a trigraph for ~
		text->replace( L"???\?-?\?-??", L"XXXX-XX-XX" );

		// Optionally remove page breaks so we don't necessarily break on them
		if (_ignore_page_breaks)
			_removePageBreaks(text);

		// Get length after any removals in the located string, lest we loop forever later
		const int len = text->length();

		if (_useGALEHeuristics && _isRemovableGALERegion(text)) {
			if (DEBUG)_debugStream << L"Removed region: " << text->toString() << L"\n";
			continue;
		}
		// don't want to just do this on the first region... in case the first region is null...
		if (_useWholeDocDatelineMode || _cur_sent_no == 0 || (_cur_sent_no-1) == last_headline_sent_no)	{
	
			// First try to find a dateline sentence at the beginning.
			_thisSentenceIsListItem = false;
			_lastSentenceIsListItem = false;
			text_index = getDatelineSentences(results, max_sentences, text, 0);
		}

		_thisSentenceIsListItem = false;
		_lastSentenceIsListItem = false;
		while (text_index < len) {
			if (_cur_sent_no == max_sentences) {
				throw UnexpectedInputException("EnglishSentenceBreaker::getSentences()",
											   "Document contains too many sentences to process");
			}
			if (_useWholeDocDatelineMode || (DATELINE_MODE >= DL_VERY_AGGRESSIVE && _cur_sent_no < 3)) {				
				// Try to look for dateline breaks;
				//   if we find one, reset all this itemized list business, otherwise
				//   restore it to what it was
				bool store_thisSentenceIsListItem = _thisSentenceIsListItem;
				bool store_lastSentenceIsListItem = _lastSentenceIsListItem;
				int new_text_index = getDatelineSentences(results, max_sentences, text, text_index);
				if (text_index == new_text_index) {
					_thisSentenceIsListItem = store_thisSentenceIsListItem;
					_lastSentenceIsListItem = store_lastSentenceIsListItem;
				} else {
					_thisSentenceIsListItem = false;
					_lastSentenceIsListItem = false;
					text_index = new_text_index;
				}
			}
			if ((results[_cur_sent_no] = getNextSentence(&text_index, text)) != NULL) {
				if (_skipHeadlines && _isLikelyHeadline(results[_cur_sent_no]->getString())) {
					delete results[_cur_sent_no];
				} else if (_useGALEHeuristics && _isRemovableGALESentence(results[_cur_sent_no])) {
					if (DEBUG)
						_debugStream << L"Removed sentence: " << results[_cur_sent_no]->getString()->toString() << L"\n";
					delete results[_cur_sent_no];
				} else {
					bool didSpecialBreak = false;
					if (_breakTableSentences) {
						didSpecialBreak = attemptTableSentenceBreak(results, max_sentences);
					}
					if (!didSpecialBreak && _breakListSentences) {
						didSpecialBreak = attemptListSentenceBreak(results, max_sentences);
					}
					if (!didSpecialBreak && _breakLongSentences) {
						didSpecialBreak = attemptLongSentenceBreak(results, max_sentences);
					}
					if (_curRegion->getRegionTag() == Symbol(L"HEADLINE")) {
						last_headline_sent_no = _cur_sent_no;
					}
					_cur_sent_no++;
				}
			}
		}
		text_index = 0;
		delete text;
	}
	//for (int j = 0; j < _cur_sent_no; j++) {
	//	std::cout << "Sentence " << results[j]->getStartEDTOffset() << " " << results[j]->getEndEDTOffset() << "\n";
	//}
	return _cur_sent_no;
}

std::vector<int> EnglishSentenceBreaker::_findPunctuatedLeadBreak(const LocatedString *input, int start, int max_end, std::wstring punct) {

	// Break a sentence at the given punctuation, if it:
	//   * is within 40 characters of the beginning of this sentence
	//   * is anywhere in the sentence if the punctuation is " * "
	//   * is preceded by an uppercase word and followed by a likely sentence start (only if VERY AGGRESSIVE)
	//
	// All of this is only done if we are in "aggressive" mode. Most of these correspond to what
	//  what was previously known as "EELD sentence breaking". The EELD corpus was a diverse 
	//  newswire corpus.

	std::vector<int> breaks;	
	if (DATELINE_MODE < DL_AGGRESSIVE)
		return breaks;

	int limit = 40;
	// Always break on " * "
	if (punct == L" * ")
		limit = max_end;
	if (DATELINE_MODE >= DL_VERY_AGGRESSIVE) {
		if (punct == L":")
			limit = max_end;		
		if (punct == L"--")
			limit = 60;
	}

	int pos = input->indexOf(punct.c_str(), start);
	if (pos == -1 || pos == start || pos >= max_end)
		return breaks;

	// Don't break on times, like 3:30
	if (punct == L":" && pos > 0 && iswdigit(input->charAt(pos-1)) && pos + 1 < input->length() && iswdigit(input->charAt(pos+1)))
		return breaks;

	std::wstring prev_word = _getPrevToken(input, pos, false);
	std::wstring next_word = _getNextToken(input, pos + static_cast<int>(punct.size()), false);
	
	// Don't break on web addresses
	std::wstring prev_word_lc = prev_word;
	boost::to_lower(prev_word_lc);
	if (punct == L":" && prev_word_lc.find(L"http") != std::wstring::npos)
		return breaks;

	bool is_good_break = false;

	// Break after specified punctuation if it is near the beginning of the sentence (limit == 40)
	if (pos < limit + start) {
		is_good_break = true;
	}

	// If we are being very aggressive, split before and after constructions like this:
	//   ALL_CAPS ALL_CAPS specified_punctuation 
	// We tried requiring that the following be a "likely sentence break", but settled for just it being a capitalized word
	// For example: 
	//   March 21, 2012 CAMBRIDGE: Today, we modified SERIF.
	//   March 21, 2012 --break-- CAMBRIDGE: --break-- Today, we modified SERIF.
	// We also split here after no matter what if the next word is capitalized
	if (DATELINE_MODE >= DL_VERY_AGGRESSIVE) {

		bool next_word_capitalized = (next_word.size() > 0 && _isExplicitlyUppercase(next_word.at(0)));

		if (next_word_capitalized)
			is_good_break = true;

		// if we are all caps and are not preceded by a parenthetical or comma, try to find the all caps section that precedes us
		if (is_good_break &&
			_isAllUppercaseWord(prev_word) &&
			prev_word.find(L")") == std::wstring::npos && prev_word.find(L"]") == std::wstring::npos) 
		{
			// move backward to the first lowercase letter or digit
			int start_upper = pos;
			while (start_upper > 0 && !iswdigit(input->charAt(start_upper)) && !iswlower(input->charAt(start_upper)))
				start_upper--;
			// if this doesn't take us back to the beginning, now move forward to the next white space
			if (start_upper > start) {			
				while (start_upper < pos && !iswspace(input->charAt(start_upper)))
					start_upper++;
				// break before the all-caps section starts, unless we're immediately preceded by a comma
				if (start_upper > start + 1 && start_upper < pos && input->charAt(start_upper-1) != L',')
					breaks.push_back(start_upper);
			}

		} 

	}

	if (is_good_break) {
		// Break before the punctuation if it is " * "
		if (punct == L" * ")
			breaks.push_back(pos);

		// Always break after the punctuation
		breaks.push_back(pos + static_cast<int>(punct.length()));

		return breaks;
	}

	return breaks;
}

std::vector<int> EnglishSentenceBreaker::_findOtherLeadBreak(const LocatedString *input, int start, int max_end) {

	int earliest_break = max_end;
	
	std::vector<int> breaks;
	if (DATELINE_MODE < DL_VERY_AGGRESSIVE)
		return breaks;

	// Deal with Factiva-style line-breaks
	std::wstring news_agency_str = L"news agency";
	std::wstring web_site_str = L"web site";
	int report_index = input->indexOf(L"Text of report", start);	
	if (report_index != -1 && report_index < earliest_break) {
		int news_agency_index = input->indexOf(news_agency_str.c_str(), report_index);
		if (news_agency_index != -1 && news_agency_index - report_index < 50 && news_agency_index < earliest_break) {

			// Text of report by Angolan news agency Angop web site Ecunha, 17 January
			int web_site_index = input->indexOf(web_site_str.c_str(), news_agency_index);
			if (web_site_index != -1 && web_site_index - news_agency_index < 50 && web_site_index < earliest_break) {
				std::wstring next_word = _getNextToken(input, web_site_index + static_cast<int>(web_site_str.size()), true);
				if (next_word != L"on") {
					earliest_break = web_site_index + static_cast<int>(web_site_str.size());
				}
			} else {			
				// Text of report by Anil K. Joseph by Indian news agency PTI Beijing, 15 January:
				int comma_index = input->indexOf(L",", news_agency_index);
				if (comma_index != -1 && comma_index < earliest_break && comma_index - news_agency_index < 50) {
					LocatedString *temp = input->substring(news_agency_index + static_cast<int>(news_agency_str.size()), comma_index);
					std::wstring focusRegion = temp->toString();
					boost::trim(focusRegion);
					delete temp;
					std::vector<std::wstring> words;
					boost::split(words, focusRegion, boost::is_any_of(L" "));
					if (words.size() == 2) {
						earliest_break = input->indexOf(words.at(1).c_str(), news_agency_index);
					} 
				}
			}
		}
	}
	

	BOOST_FOREACH(std::wstring day, _days) {
		int day_index = input->indexOf(day.c_str(), start);
		if (day_index != -1 && day_index < earliest_break) {
			std::wstring next_word = _getNextToken(input, day_index + static_cast<int>(day.size()), true);
			std::wstring next_word_lc = next_word;
			boost::to_lower(next_word_lc);
			if (_months.find(next_word_lc) != _months.end()) {	
				std::wstring next_word_with_punct = _getNextToken(input, day_index + static_cast<int>(day.size()), false);
				if (next_word_with_punct.find(L")") == std::wstring::npos && next_word_with_punct.find(L"]") == std::wstring::npos) {
					int break_index = input->indexOf(next_word.c_str(), day_index);
					break_index += static_cast<int>(next_word.size());
					if (break_index < max_end) {
						std::wstring next_next_word = _getNextToken(input, break_index, true);
						if (next_next_word.size() > 0) {
							wchar_t nn_char = next_next_word.at(0);
							if (iswupper(nn_char) || nn_char == L':' || nn_char == L'-') {
								earliest_break = break_index;
							}
						}
					}
				}
			}
		}
	}

	// Break on words that are rarely capitalized unless they actually start
	//   a new sentence. This user-supplied list should include the capitalizations
	//   we actually expect to find, e.g. "The", "THE", "In", "IN", etc.
	BOOST_FOREACH(std::wstring rcw, _rarelyCapitalizedWords) {
		int index = input->indexOf(rcw.c_str(), start);
		if (index > 0 && index < earliest_break) {
			std::wstring prev_word = _getPrevToken(input, index, false);
			if (prev_word.length() == 0)
				continue;
			if (prev_word.find(L",") != std::wstring::npos || prev_word.find(L"``") != std::wstring::npos)
				continue;
			if (prev_word == L"of")
				continue;
			if ((prev_word == L"daily" || prev_word == L"newspaper") &&
				(rcw == L" THE " || rcw == L" The "))
				continue;
			std::wstring next_word = _getNextToken(input, index + static_cast<int>(rcw.length()), false);			
			if (next_word.length() == 0)
				continue;
			if (iswupper(next_word.at(0)))
				continue;
			earliest_break = index;
		}
	}
	if (earliest_break != max_end)
		breaks.push_back(earliest_break);

	return breaks;

}

std::vector<int> EnglishSentenceBreaker::_findBuriedByline(const LocatedString *input, int start, int max_end) {

	// This function is largely defined for the Factiva/ICEWS corpus, where we are getting data
	//  with the first sentence all smushed together, including bylines. For this reason we
	//  only do this when the setting is set to "very aggressive".
	//
	// It relies heavily on casing, especially the fact that "By " is unlikely to be followed
	//  by a capitalized word if this is not a dateline. It does not find bylines that are
	//  in all-caps. It will also miss various datelines when they come in different formats;
	//  this is largely designed to deal with common Factiva formats.

	std::vector<int> breaks;
	if (DATELINE_MODE < DL_VERY_AGGRESSIVE)
		return breaks;

	int by_pos = input->indexOf(L"By ", start);
	if (by_pos == -1)
		by_pos = input->indexOf(L"Posted by ", start);
	// lowercase "by", we'll only use this in certain contexts
	bool lowercase_by = false;
	if (by_pos == -1) { 
		by_pos = input->indexOf(L"by ", start);
		lowercase_by = true;
	}
	if (by_pos == -1)
		return breaks;
	if (by_pos >= max_end)
		return breaks;

	LocatedString *byStringLS = input->substring(by_pos, input->length());
	std::wstring byString = byStringLS->toString();
	delete byStringLS;

	// Look for "by/By First [Middle] [Middle] Last" all on one line
	size_t next_new_line = byString.find(L"\n");
	if (next_new_line != std::wstring::npos && (by_pos == 0 || input->charAt(by_pos - 1) == '\n')) {
		std::wstring delineatedLine = byString.substr(0, next_new_line);
		std::vector<std::wstring> wordsInLine; 
		boost::split(wordsInLine, delineatedLine, boost::is_any_of("\t "));
		if (wordsInLine.size() >= 3 && wordsInLine.size() < 6) {
			bool bad_word = false;
			for (size_t i = 1; i < wordsInLine.size(); i++) {
				std::wstring word = wordsInLine.at(i);
				if (!_isPossibleNameWord(word)) {
					bad_word = true;
					break;
				}
			}
			if (!bad_word) {
				std::wstring split_word = wordsInLine.at(wordsInLine.size() - 1);
				// break after last word in delineatedLine
				int end = input->indexOf(split_word.c_str(), by_pos) + static_cast<int>(split_word.length()); 
				breaks.push_back(by_pos);
				breaks.push_back(end);
				return breaks;
			}
		}
	}
	if (lowercase_by) // Don't trust lowercase by except for above test(s)
		return breaks;

	// Special cases for FACTIVA corpus
	std::wstring special_str = L"Of DOW JONES NEWSWIRES";
	int special = input->indexOf(special_str.c_str());
	if (special == -1) {
		special_str = L"Of THE WALL STREET JOURNAL";
		special = input->indexOf(special_str.c_str());
	}
	if (special > by_pos) {
		breaks.push_back(by_pos);
		breaks.push_back(special + static_cast<int>(special_str.size()));
		return breaks;
	}

	// get the words in the alleged byline
	std::vector<std::wstring> words;
	boost::split(words, byString, boost::is_any_of("\t\n "));

	if (words.size() < 2)
		return breaks;

	bool name_is_all_uppercase = _isAllUppercaseWord(words.at(1));

	int split_count = -1;

	// Construction #1: blah blah By Someone ALLCAPS	
	// Construction #2: blah blah By Someone ... Writer/Correspondent/Reporter blah blah
	int reporter_index = -1;
	int words_in_name = 0;
	for (int i = 1; i < int(words.size()) && i < 10; i++) {
		std::wstring word = words.at(i);

		// Being followed by "A" or "The" is a good indication the name is over
		if (word == L"A" || word == L"The") {
			split_count = i;
			break;
		}
		// Being followed by ALL_CAPS_WORD is another good indication the name is over
		if (!name_is_all_uppercase && _isAllUppercaseWord(word)) {
			// first make usre this isn't an initial
			if (word.size() > 1 && word.at(1) != L'.') {
				// This is valid if the ALL_CAPS_WORD is followed by a comma, 
				//   if we've seen a reporter-like word so far,
				//   or if we are only a few tokens away from the "By" or an "and"
				if (words.at(i).find(L",") != std::wstring::npos || words_in_name < 4 || reporter_index != -1)
				{
					split_count = i;				
				}
				break;
			}
		}

		// this makes things unsafe... there is a byline to be found, but we don't know where it is
		if (word== L"Of" || word == L"In")
			break;

		// keep track of this for later
		std::wstring lc_word = word;
		boost::to_lower(lc_word);
		if (lc_word.find(L"writer") == 0 || lc_word.find(L"correspondent") == 0 || lc_word.find(L"reporter") == 0  || lc_word.find(L"editor") == 0) {	
			reporter_index = i;
		} else if (word == L"and") {
			words_in_name = 0;
		} else if (!_isPossibleNameWord(word)) {
			break; // probably not in a byline, so let's get out of here
		}

		words_in_name++;
	}

	// Since split_count could be the last word, make sure you check below
	//   that we are not past the end of an array
	if (split_count == -1 && reporter_index > 1) {
		split_count = reporter_index + 1;
	}
	
	// If this is near the end of the region, just split before "By"
	// This will NOT work unless you have pre-region-broken text, of course
	if (words.size() < 6 || (size_t) split_count >= words.size()) {
		breaks.push_back(by_pos);
		return breaks;
	}

	if (split_count != -1) {
		// This is kind of a poor man's attempt at keeping track of offsets
		// We know the word we'd like to split on, so we now go look for it in the LocatedString
		//  and split on that offset. 
		std::wstring split_word = words.at(split_count);
		int end = input->indexOf(split_word.c_str(), by_pos);
		breaks.push_back(by_pos);
		breaks.push_back(end);
	}

	return breaks;
}

std::vector<int> EnglishSentenceBreaker::_findParentheticalDatelineBreak(const LocatedString *input, int start, int max_end, std::wstring left, std::wstring right) {

	// This will break before and after things like:
	//    (AP)
	//    [ATTN: Adds image]
	//    /AzerTaC/
	//    (Eds: Note date of release)
	// etc.
	//
	// This does NOT perform any sentence breaking inside the parenthetical itself.
	//
	// Also, this does not deal intelligently with nested parentheticals. However, these seem relatively rare.

	std::vector<int> breaks;
	if (_dont_break_in_parentheticals)
		return breaks;
	int left_pos = input->indexOf(left.c_str(), start);
	if (left_pos == -1 || left_pos + 1 >= input->length() || left_pos > max_end)
		return breaks;
	int right_pos = input->indexOf(right.c_str(), left_pos + 1);
	if (right_pos == -1) {
		return breaks;
	} 	

	// Don't break on (c), as it's probably a copyright symbol
	if (input->indexOf(L"(c)") == left_pos)
		return breaks;

	bool is_good_sentence_break = false;

	// Always break on leading parentheticals
	if (left_pos == 0 || left_pos == 1)
		is_good_sentence_break = true;

	std::wstring following_word_punct = _getNextToken(input, right_pos + 1, false);
	std::wstring following_word_no_punct = _getNextToken(input, right_pos + 1, true);

	// if parenthetical is followed by something lowercase, don't break here
	if (!is_good_sentence_break && following_word_punct != L"" && _isExplicitlyLowercase(following_word_punct.at(0)))
		return breaks;
	
	// List of pre-loaded "first words" that will trigger a break
	// Includes known news agencies as well as many things like "ATTN" and "correction"
	std::wstring first_word_punct = _getNextToken(input, left_pos + 1, false);
	std::wstring first_word_no_punct = _getNextToken(input, left_pos + 1, true);
	boost::to_lower(first_word_punct);
	boost::to_lower(first_word_no_punct);
	if (_datelineParentheticals.find(first_word_punct) != _datelineParentheticals.end() || 
		_datelineParentheticals.find(first_word_no_punct) != _datelineParentheticals.end())
		is_good_sentence_break = true;	
	
	if (!is_good_sentence_break && DATELINE_MODE >= DL_AGGRESSIVE) {

		// If a parenthetical is followed by a dash or underscore, we break
		if (following_word_punct.size() > 0 && (following_word_punct.at(0) == '-' || following_word_punct.at(0) == '_'))
			is_good_sentence_break = true;

		// If a parenthetical is followed by a clearly sentence-starting word, we break
		if (following_word_no_punct == L"The" || following_word_no_punct == L"A")
			is_good_sentence_break = true;

		// Break on parentheticals that contain various words
		LocatedString *parenthetical = input->substring(left_pos+1, right_pos);
		std::wstring parenthetical_str = parenthetical->toString();
		boost::to_lower(parenthetical_str);
		delete parenthetical;

		// [&quot;...] is common in Factiva
		if (parenthetical_str.find(L"&quot;") == 0 && left == L"[")
			is_good_sentence_break = true;		

		// These typically also indicate that this is news markup
		if (parenthetical_str.find(L"id:") == 0 ||
			parenthetical_str.find(L"headline") != std::wstring::npos ||
			parenthetical_str.find(L"translated") != std::wstring::npos ||
			parenthetical_str.find(L"transcribed") != std::wstring::npos ||
			parenthetical_str.find(L"times") != std::wstring::npos ||
			parenthetical_str.find(L"herald") != std::wstring::npos ||
			parenthetical_str.find(L"post") != std::wstring::npos ||
			parenthetical_str.find(L"news") != std::wstring::npos ||
			parenthetical_str.find(L"part 1") != std::wstring::npos ||
			parenthetical_str.find(L"part 2") != std::wstring::npos ||
			parenthetical_str.find(L"the universal") != std::wstring::npos ||
			parenthetical_str.find(L"daily") != std::wstring::npos ||
			parenthetical_str.find(L"journal") != std::wstring::npos ||
			parenthetical_str == L"sports")
		{
			is_good_sentence_break = true;
		}	
	}

	if (is_good_sentence_break) {
		breaks.push_back(left_pos);
		breaks.push_back(right_pos+1);
	}

	return breaks;
}

bool EnglishSentenceBreaker::_isAllUppercaseWord(std::wstring word) {
	// Return true if there are no lowercase letters in this word, and at least
	//  one uppercase letter, e.g. 'HELLO', 'HELLO:', or 'HE3LLO', but not '3:'.
	bool has_upper = false;
	for (size_t i = 0; i < word.length(); i++) {
		if (_isExplicitlyLowercase(word.at(i)))
			return false;
		else if (_isExplicitlyUppercase(word.at(i)))
			has_upper = true;
	}
	return has_upper;
}

bool EnglishSentenceBreaker::_isNumber(std::wstring word) {
	for (size_t i = 0; i < word.length(); i++) {
		if (!iswdigit(word.at(i)))
			return false;
	}
	return true;
}

/**
 * This regular expression determines whether or not a string is a likely
 * portion mark.
 *
 * We implement a rough version of the syntax described in Enclosure 4
 * of this document:
 *   http://www.dtic.mil/whs/directives/corres/pdf/520001_vol2.pdf
 *
 * We don't restrict the markings much, and allow for an arbitrary number of
 * categories, types per category, and subcontrols. This isn't correct, but
 * is more flexible than correctly encoding each category such as classification
 * or dissemination.
 *
 * We need different versions of the regex pre-compiled to handle differences in case.
 *
 * @author nward@bbn.com
 * @date 2013.06.20
 **/
const boost::wregex EnglishSentenceBreaker::portion_mark_upper_re(
	// Foreign classifications lack a first category
	L"(//)?"

	// One or more categories each containing one or more category types
	L"[A-Z][-, A-Z]*"
	L"(/[A-Z][-, A-Z]*)*"
	L"(//"
		L"[A-Z][-, A-Z]*"
		L"(/[A-Z][-, A-Z]*)*"
	L")*"
);
const boost::wregex EnglishSentenceBreaker::portion_mark_lower_re(
	// Foreign classifications lack a first category
	L"(//)?"

	// One or more categories each containing one or more category types
	L"[a-z][-, a-z]*"
	L"(/[a-z][-, a-z]*)*"
	L"(//"
		L"[a-z][-, a-z]*"
		L"(/[a-z][-, a-z]*)*"
	L")*"
);

const boost::wregex EnglishSentenceBreaker::portion_mark_basic_re(
	L"(U|C|S|TS"
	L"|U *N *C *L *A *S *S *I *F *I *E *D"
	L"|C *L *A *S *S *I *F *I *E *D"
	L"|S *E *C *R *E *T"
	L"|T *O *P *S *E *C *R *E *T"
	L")"
);

bool EnglishSentenceBreaker::_isPortionMark(const LocatedString *input, int start, int end) {
	std::wstring portion = input->substringAsWString(start, end);
	boost::trim(portion);
	// Check for match to small set of known classification markers
	if (boost::regex_match(portion, portion_mark_basic_re))
		return true;
	// Otherwise, must contain at least one instance of "//"
	if (portion.find(L"//") == std::wstring::npos)
		return false;

	// We enforce uppercase portion marks unless the document has been downcased
	if (_curDoc->isDowncased())
		return boost::regex_match(portion, portion_mark_lower_re);
	else
		return boost::regex_match(portion, portion_mark_upper_re);
}

bool EnglishSentenceBreaker::_isPossibleNameWord(std::wstring word) {
	// Assumes multi-case data
	if (word == L"")
		return false;
	if (iswupper(word.at(0)))
		return true;
	if (word.find(L"al") == 0)
		return true;
	if (word == L"de" || word == L"la" || word == L"les" || word == L"del")
		return true;
	return false;
}

std::wstring EnglishSentenceBreaker::_getNextToken(const LocatedString *input, int start, bool letters_only) {

	// Get the token immediately preceding this position. Tokens are defined by whitespace.
	// If letters_only is set to true, we also break on non-letters.
	std::wstring word = L"";
	while (start < input->length() && iswspace(input->charAt(start)))
		start++;
	while (start < input->length()) {
		wchar_t next_char = input->charAt(start);
		// strip basic punctuation
		if (iswspace(next_char))
			break;
		if (letters_only && iswpunct(next_char))
			break;
		word += next_char;
		start++;
	}
	return word;

}

std::wstring EnglishSentenceBreaker::_getPrevToken(const LocatedString *input, int start, bool letters_only) {

	// Get the token immediately preceding this position. Tokens are defined by whitespace.
	// If letters_only is set to true, we also break on non-letters.
	std::wstring word = L"";
	start--;
	while (start > 0 && iswspace(input->charAt(start)))
		start--;
	while (start > 0) {
		wchar_t prev_char = input->charAt(start);
		if (iswspace(prev_char))
			break;
		// strip basic punctuation
		if (letters_only && iswpunct(prev_char))
			break;
		word = prev_char + word;
		start--;
	}
	return word;
}

int EnglishSentenceBreaker::_breakOffFromDatelineSentences(Sentence **results, int max_sentences, const LocatedString *input, int start, int max_end) {

	// We will look for many possible breaks for datelines, choosing
	//  the one that comes earliest in the sentence. This function is called
	//  repeatedly until no more such breaks can be found.
	//
	// Aggressiveness of each sub-function is controlled within itself.
	
	std::vector<int> breaks;

	// Punctuation that signals a possible break, e.g. "--" or " * "
	BOOST_FOREACH(std::wstring punct, _punctuatedLeads) {
		std::vector<int> result = _findPunctuatedLeadBreak(input, start, max_end, punct);
		if (result.size() > 0 && (breaks.size() == 0 || result.at(0) < breaks.at(0))) {
			breaks = result;
		}
	}

	// Start and end of a byline, e.g. "By John Smith"
	std::vector<int> result = _findBuriedByline(input, start, max_end);
	if (result.size() > 0 && (breaks.size() == 0 || result.at(0) < breaks.at(0))) {
		breaks = result;
	}

	// Find other lead breaks (these are currently all 'very aggressive')
	std::vector<int> result2 = _findOtherLeadBreak(input, start, max_end);
	if (result2.size() > 0 && (breaks.size() == 0 || result2.at(0) < breaks.at(0))) {
		breaks = result2;
	}

	// Parentheticals that signal a break before and after
	typedef std::pair<std::wstring,std::wstring> wstring_pair_t;
	BOOST_FOREACH(wstring_pair_t my_pair, _parenPairs) {
		std::vector<int> result = _findParentheticalDatelineBreak(input, start, max_end, my_pair.first, my_pair.second);
		// These are supposed to come in pairs; the first one being less than previous breaks is no good if
		//   it's just the start of the phrase!
		if (result.size() > 1 && result.at(0) == start) {
			if (breaks.size() == 0 || result.at(1) < breaks.at(0))
				breaks = result;
		} else if (result.size() > 0 && (breaks.size() == 0 || result.at(0) < breaks.at(0))) {
			breaks = result;
		}
	}

	// These should be coming in sorted, but I worry
	std::sort(breaks.begin(), breaks.end());

	for (size_t h = 0; h < breaks.size(); h++) {
		int my_break = breaks[h];
		if (my_break > max_end)
			continue;
		if (my_break != start) {
			bool found_non_whitespace = false;
			for (int i = my_break; i < input->length(); i++) {
				if (!iswspace(input->charAt(i))) {
					found_non_whitespace = true;
					break;
				}
			}
			if (!found_non_whitespace)
				break;
			LocatedString *sub = input->substring(start, my_break);	
			
			
			results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
			

			// Make sure the dateline isn't too long
			if (_breakListSentences) {
				attemptListSentenceBreak(results, max_sentences);
			}
			if (_breakLongSentences) {
				attemptLongSentenceBreak(results, max_sentences);
			}

			delete sub;
			_cur_sent_no++;
			start = my_break;
		}
	}

	return start;
}


/**
* @param results an output parameter containing an array of
*                pointers to Sentences that will be filled in
*                with the sentences found by the sentence breaker.
* @param input the input string to read from.
* @return the next index into the located string after the end of the
*         found dateline sentences, or <code>0</code> if no dateline
*         sentences were found.
*/
int EnglishSentenceBreaker::getDatelineSentences(Sentence **results, int max_sentences, const LocatedString *input, int start) {
	if (DATELINE_MODE == DL_NONE)
		return start;

	while (true) {
		// First identify how far this sentence would go if we didn't break off the datelines.
		// We don't want to break beyond this point.
		int offset = start;
		Sentence *tempSentence = getNextSentence(&offset, input);
		int max_end = offset;	
		delete tempSentence;

		while (start < input->length() && iswspace(input->charAt(start)))
			start++;

		// Keep breaking off pieces from the beginning until we don't find any more.
		int end = _breakOffFromDatelineSentences(results, max_sentences, input, start, max_end);
		if (end == start)
			break;
		start = end;				
	}

	/*if (_cur_sent_no != 0) {
		for (int temp = 0; temp < _cur_sent_no; temp++) {
			std::wstring temp_wstr = results[temp]->getString()->toString();
			std::wstring temp_str(temp_wstr.begin(), temp_wstr.end());
			std::cout << temp_str << " !break! ";
		}
		LocatedString *temp = input->substring(start, input->length());
		std::wstring temp_wstr = temp->toString();
		std::wstring temp_str(temp_wstr.begin(), temp_wstr.end());
		std::cout << temp_str << "\n";
		delete temp;
	}*/

	if (start == 0) {
		// This is a hold-over from a previous corpus, but it doesn't hurt to leave it in
		int upper = min(input->length(), 50);
		for (int pos = start; pos < upper; pos++) {
			if (matchWashPostDateline(input, pos)) {
				int end = pos + 1;
				LocatedString *sub = input->substring(start, end);
				
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
				
				delete sub;
				_cur_sent_no++;
				return end;
			}
		}
	}

	return start;
}

/**
 * The sentence is read starting from <code>*offset</code>.
 *
 * @param offset a value-result parameter; on entry to the function this parameter
 *               should refer to an integer representing the first offset of the
 *               next sentence, and on exit it will refer to an integer representing
 *               the next index into the located string after the end of the found
 *               sentence.
 * @param input the input string to read from.
 * @return a new Sentence object representing the next sentence found, or
 *         <code>NULL</code> if there were no more sentences.
 */
Sentence* EnglishSentenceBreaker::getNextSentence(int *offset, const LocatedString *input) {
	const int len = input->length();

	// Start after any initial whitespace.
	int start = *offset;
	while ((start < len) && (iswspace(input->charAt(start)))) {
		start++;
	}

	// If we've reached the end, there's no sentence.
	if (start >= len) {
		*offset = start;
		return NULL;
	}

	Metadata *metadata = _curDoc->getMetadata();

	// Track the quote level if the sentence starts with an opening quote character
	int end = start;
	if (isOpeningPunctuation(input->charAt(start))) {
		_quoteLevel.push(input->charAt(start));
		while (end < input->length() - 1 && input->charAt(end) == _quoteLevel.top())
			end++;
	}

	// Search for the next sentence break.
	bool found_break = false;
	while ((end < len) && (!found_break)) {

		SessionLogger::dbg("sentence_break_loop") << start << ":" << end << " " << input->substringAsWString(end, end+1);

		if (_useITEASentenceBreakHeuristics &&
			(_isListItemPeriod(input, start, end) || _isListItemParen(input, start, end)))
			_thisSentenceIsListItem = true;

		// TODO: this doesn't take into account "``What is your name?'' he asked."
		if (isSplitChar(input->charAt(end))) {
			//consume any repeated SplitChars
			while(isSplitChar(input->charAt(end)) && ((end+1)<len)) {
				end++;
			}
			int next_word = end + 1;
			//find the next real word, or the end of the document
			while ((next_word < len) && !iswalpha(input->charAt(next_word))) {
				next_word++;
			}
			if ((next_word >= len) || _isExplicitlyUppercase(input->charAt(next_word))) {
				//catch quotation marks or other closing punct before the next word			
				end = getIndexAfterClosingPunctuationHandlingQuotes(input, end);
				found_break = true;
				SessionLogger::dbg("sentence_break_loop") << "Matched isSplitChar() (1)";
			}
			else {
				// This level of detail is because opening and closing quotes may be ambiguous

				// First, check for any closing quotes we want to pass over
				bool closing_quotes = false;
				bool spaces = false;
				bool opening_quotes = false;
				int i = end;
				for (; i < next_word; i++)
					if (isClosingPunctuation(input->charAt(i)))
						closing_quotes = true;
					else
						break;
				int after_closing = i;

				// Then check for spaces and line breaks
				int lines = 0;
				for (; i < next_word; i++)
					if (iswspace(input->charAt(i))) {
						spaces = true;
						if (input->charAt(i) == '\n')
							lines++;
					} else
						break;

				// Then check for opening quotes
				int before_opening = i;
				for (; i < next_word; i++)
					if (isOpeningPunctuation(input->charAt(i)))
						opening_quotes = true;
					else
						break;

				// Now decide where to put the cursor
				if (spaces) {
					if (closing_quotes) {
						if (opening_quotes) {
							// Break between two quotes since the first one ended with a split char,
							// even though next quote doesn't start with uppercase
							found_break = true;
							end = after_closing;
							SessionLogger::dbg("sentence_break_loop") << "Matched isSplitChar() (2)";
						} else
							// Move past the end of the quote but no break, assuming split char
							// is part of the quote, not the quoting sentence
							end = before_opening;
					} else {
						if (opening_quotes) {
							// Break before a quote since the sentence ended with a split char,
							// even though next quote doesn't start with uppercase
							found_break = true;
							end = after_closing;
							SessionLogger::dbg("sentence_break_loop") << "Matched isSplitChar() (3)";
						} else if (_curDoc->isDowncased()) {
							// Break even though new sentence isn't cased because document isn't
							found_break = true;
							end = before_opening;
							SessionLogger::dbg("sentence_break_loop") << "Matched isSplitChar() (4)";
						} else if (_breakOnDoubleCarriageReturns && lines >= 2) {
							// Break without case indication because we have multiple line breaks after split char
							found_break = true;
							end = before_opening;
							SessionLogger::dbg("sentence_break_loop") << "Matched isSplitChar() (5)";
						} else
							// No break because we don't have case indicating start of new sentence
							end = before_opening;
					}
				} else
					// No space implies weird formatting, so no break and move on
					end++;
			}
		}
		// TODO: clean up spaghetti logic here... it's very feeble
		else if (matchFinalPeriod(input, end, start)) {
			// If the sentence ended with an ellipsis, skip to the end of it.
			int ellipsis_len = matchEllipsis(input, end);
			if (ellipsis_len > 0) {
				end += ellipsis_len - 1;
			}
			// If followed by any closing punctuation, include that too.
			end = getIndexAfterClosingPunctuationHandlingQuotes(input, end);
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched final period";
		} else if (matchSentenceFinalCitation(input, end, start)) {
			end = getIndexAfterClosingPunctuationHandlingQuotes(input, end); // If followed by any closing punctuation, include that too.
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched sentence final citation";
		} else if (_isCarriageReturnBreak(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched carriage return break";
			end++;
		}
		else if (_isClosingQuoteLevel(input, end)) {
			// Consume any non-quote punctuation after the quote, if we're breaking
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isClosingQuote";
			end++;
			while (end < input->length() && iswpunct(input->charAt(end)))
				end++;
			end++;
			_quoteLevel.pop();
		}
		else if (_isLastInStringOfDashes(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isLastInStringOfDashes";
			end++;
		} 
		else if (_isBreakableSemicolon(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isBreakableSemicolon";
			end++;
		}
		else if (_isWebText && _isLastInPunctuationString(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isLastInPunctuationString";
			end++;
		}
		else if (_isFollowedByStringOfDashes(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isFollowedByStringOfDashes";
			end++;
		} else if (_break_on_footnote_numbers && _isBreakableCenterPeriod(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isBreakableCenterPeriod";
			end++;
		} else if (_break_on_portion_marks && _isFollowedByPortionMark(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isFollowedByPortionMark";
			end++;
		}
		else if (_useGALEHeuristics && _isLastInStringOfSlashes(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isLastInStringOfSlashes";
			end++;
		}		
		else if ((_useGALEHeuristics || DATELINE_MODE >= DL_AGGRESSIVE) && _isBuriedDateline(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isBuriedDateline";
			end++;
		}	
		else if (_useGALEHeuristics && _isSentenceEndingWebAddress(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isSentenceEndingWebAddress";
			end++;
		}
		else if (_useGALEHeuristics && _isBreakableXMLGlyph(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isBreakableXMLGlyph";
			end++;
		}
		else if (_isWebText && _isFollowedByPunctuationString(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isFollowedByPunctuationString";
			end++;
		}
		else if (_useITEASentenceBreakHeuristics && _isFollowedByListItem(input, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isFollowedByListItem";
			end++;
		}
		//else if (_useITEASentenceBreakHeuristics && _isAtEndOfListItem(input, end)) {
		//	found_break = true;
		//	end++;
		//}
		else if (_useITEASentenceBreakHeuristics && _isListItemPeriod(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isListItemPeriod";
			end++;
		}
		else if (_useITEASentenceBreakHeuristics && _isListItemParen(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isListItemParen";
			end++;
		}
		else if (_useITEASentenceBreakHeuristics && _endsDashLine(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _endsDashLine";
			end++;
		}
		else if (_useITEASentenceBreakHeuristics && _isFollowedByDashLine(input, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isFollowedByDashLine";
			end++;
		}
		else if ((_alwaysUseBreakableColons || (_useITEASentenceBreakHeuristics && _lastSentenceIsListItem)) && _isBreakableColon(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isBreakableColon";
			end++;
		} 
		else if (DATELINE_MODE >= DL_VERY_AGGRESSIVE && _isClosingStartingOpenParen(input, start, end)) {
			found_break = true;
			SessionLogger::dbg("sentence_break_loop") << "Matched _isClosingStartingOpenParen";
			end++;
		} 
		else {
			end += max(1, matchEllipsis(input, end));
			// Note that this moves  end forward, with or without an ellipsis
		}

		if (found_break && (end < len)) {
			// If the sentence break happened in the middle of a span that
			// restricts sentence breaks, then we haven't found the end of
			// a sentence, so we'll keep going.
			if (inRestrictedBreakSpan(metadata, input, end)) {
				found_break = false;
				SessionLogger::dbg("sentence_break_loop") << "CANCEL BREAK: Matched inRestrictedBreakSpan";
				end++;
			}

			// If the sentence break is inside a parenthetical, keep going
			if (_dont_break_in_parentheticals && !_quoteLevel.empty() && _quoteLevel.top() == L'(') {
				found_break = false;
				SessionLogger::dbg("sentence_break_loop") << "CANCEL BREAK: inside a parenthetical";
			}
		}
	}

	// Trim off any whitespace at the end of the sentence
	*offset = end;
	while ((end > start) && (end > input->length() || iswspace(input->charAt(end - 1)))) {
		end--;
	}

	// Create the value of the sentence output parameter with the substring.
	LocatedString *sentenceString;
	if (end == start) {
		// Use the pre-trim sentence end
		sentenceString = input->substring(start, *offset);
	} else {
		// Use the trimmed sentence end
		sentenceString = input->substring(start, end);
	}

	if (_downcaseHeadlines && _isLikelyHeadline(sentenceString)) {
		SessionLogger::warn("downcase_headlines") << "Converting headline to lowercase: " << sentenceString->toString();
		sentenceString->toLowerCase();
	}

	Sentence *result;	
	result = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
	
	delete sentenceString;

	if (_thisSentenceIsListItem) {
		_lastSentenceIsListItem = true;
		_thisSentenceIsListItem = false;
	} else {
		_lastSentenceIsListItem = false;
	}
	return result;
}

bool EnglishSentenceBreaker::attemptListSentenceBreak(Sentence **results, int max_sentences) {
	Sentence *thisSentence = results[_cur_sent_no];

	// Track possible list separators (just a dumb count for now)
	boost::unordered_map<wchar_t, int> separatorCounts;
	for (int i = 0; i < thisSentence->getNChars(); i++) {
		wchar_t thisChar = thisSentence->getString()->charAt(i);
		if (isListSeparator(thisChar)) {
			separatorCounts[thisChar]++;
		}
	}

	// Determine most likely separator
	wchar_t separator = L' ';
	int maxSeparatorCount = 0;
	for (boost::unordered_map<wchar_t, int>::iterator sc = separatorCounts.begin(); sc != separatorCounts.end(); sc++) {
		if (sc->second > maxSeparatorCount) {
			separator = sc->first;
			maxSeparatorCount = sc->second;
		}
	}
	
	// Only split if it's a long list
	if (maxSeparatorCount < _minListSeparators)
		return false;

	// Attempt the split with the most common separator
	SessionLogger::updateContext(2, boost::lexical_cast<std::string>(_cur_sent_no).c_str());
	char* utf8char = UnicodeUtil::toUTF8Char(separator);
	SessionLogger::dbg("break_list") << "Probable list sentence found with " << maxSeparatorCount << " >= " << _minListSeparators << " separators '" << utf8char << "'";
	delete[] utf8char;
	std::vector<Sentence*> subSentences;
	int start = 0;
	int next_sent_no = _cur_sent_no;
	for (int i = 0; i < thisSentence->getNChars(); i++) {
		wchar_t thisChar = thisSentence->getString()->charAt(i);
		if (thisChar == separator) {
			// This expects inclusive offsets, but we want to exclude the list separator
			subSentences.push_back(createSubSentence(thisSentence, next_sent_no, start, i - 1, "list"));
			start = i + 1;
			next_sent_no++;
		}
	}
	if (start < thisSentence->getNChars()) {
		subSentences.push_back(createFinalSubSentence(thisSentence, next_sent_no, start));
	}

	if (_cur_sent_no + static_cast<int>(subSentences.size()) > max_sentences) {
		// Too many list items!
		throw UnexpectedInputException("EnglishSentenceBreaker::attemptListSentenceBreak()",
			"Document contains to many sentences to process");
	}

	// Keep our newly split up list
	BOOST_FOREACH(Sentence *sent, subSentences) {
		results[_cur_sent_no++] = sent;
	}

	// Decrement sent_no to point to last complete sentence
	_cur_sent_no--; 

	// Don't leak the original long list sentence
	delete thisSentence;

	// Indicate that we modified the sentence
	return true;
}

// Returns true if substring [start, end) is an acceptable sub-sentence for
// use in attemptLongSentenceBreak
bool EnglishSentenceBreaker::isAcceptableSentenceString(const LocatedString *string, int start, int end) { 
	if (end - start >= getMaxSentenceChars())
		return false; 
	
	return (countPossibleTokenBreaks(string, start, end) <= _max_token_breaks); 	
}

int EnglishSentenceBreaker::getIndexBeforeOpeningPunctuation(const LocatedString *input, int index) {
	const int len = input->length();
	do {
		index++;
	} while ((index < len) && isOpeningPunctuation(input->charAt(index)));
	return index - 1;
}

int EnglishSentenceBreaker::getIndexAfterClosingPunctuation(const LocatedString *input, int index) {
	const int len = input->length();
	do {
		index++;
	} while ((index < len) && isClosingPunctuation(input->charAt(index)));
	return index;
}

// Wrapper for getIndexAfterClosingPunctuation that returns the same result
// but makes sure that the quoteLevel is popped if necessary; this comes up
// when we break for another reason right before the end of an open quote
int EnglishSentenceBreaker::getIndexAfterClosingPunctuationHandlingQuotes(const LocatedString *input, int index) {
	// Wrapped operation
	int new_index = getIndexAfterClosingPunctuation(input, index);

	// Check for closing quotes
	for (int i = index; i < new_index; i++)
		if (_isClosingQuoteLevel(input, i))
			// Match closing quotes, so we don't start next sentence quoted incorrectly
			_quoteLevel.pop();

	return new_index;
}

bool EnglishSentenceBreaker::matchSentenceFinalCitation(const LocatedString *input, int index, int origin) {
	if (input->charAt(index) != L']') { return false; } // Quick check before we do further processing
	static const boost::wregex citation_re(L"\\.\\[\\d+\\]$");	
	const std::wstring text = input->substringAsWString(origin, index+1);
	return boost::regex_search(text, citation_re);
}

bool EnglishSentenceBreaker::matchFinalPeriod(const LocatedString *input, int index, int origin) {
	// Try some basic criteria first.
	if (!matchPossibleFinalPeriod(input, index, origin)) {
		return false;
	}

	const int len = input->length();

	// If this is the end of the string, it must be a final period.
	if (index == (len - 1)) {
		return true;
	}

	// If the punctuation is an ellipsis, it may not be the end of a sentence.
	int ellipsis_len = matchEllipsis(input, index);
	if (ellipsis_len > 0) {
		// TODO: should we consider more than three periods an ellipsis + a period?
		return matchLikelySentenceStart(input, index + ellipsis_len) && !matchCenterOfURL(input, index, ellipsis_len);
	}

	// Get the final word
	Symbol finalWord = getWordEndingAt(index, input, origin);

	// Get a version of our final word with no opening punctuation
	std::wstring final_word_no_opening_punct(finalWord.to_string());
	while (final_word_no_opening_punct.size() > 0 &&
		(final_word_no_opening_punct[0] == L'(' || final_word_no_opening_punct[0] == L'[' || final_word_no_opening_punct[0] == L'{')) {
		final_word_no_opening_punct = final_word_no_opening_punct.substr(1);
	}

	// If the last word before the period is in the list of non-final abbreviations,
	// then the period cannot mark the end of a sentence.
	if (_nonFinalAbbreviations->contains(final_word_no_opening_punct)) {
		return false;
	}

	// If the punctuation (i.e., the period or the period and subsequent closing punctuation
	// marks) is followed by non-whitespace, then it's not the end of a sentence.
	int break_pos = getIndexAfterClosingPunctuation(input, index);
	if (break_pos >= len) {
		return true;
	}
	else if (!iswspace(input->charAt(break_pos))) {
		return false;
	}

	// If the punctuation is part of a name that cannot be split, then it's not the
	// end of a sentence. Testing for non-splittable names of the form X.X. Xxxxxx is
	// now redundant due to the separate test for matchLikelyInitials, but there's no
	// restriction on non-splittable names to be of that form, so I'll leave the
	// test in.
	if (matchNonSplittableName(input, index, origin)) {
		return false;
	}

	// If the final word is likely to be an abbreviation and the text starting just
	// after it is not likely to be the start of a sentence, then this is probably
	// not the end of a sentence.
	if (matchLikelyAbbreviation(finalWord) && !matchLikelySentenceStart(input, break_pos))
	{
		return false;
	}

	// EMB 2/28/03: I believe that X.X. Xxxxxxx should never split. This is too often
	// a person name. X.X. will return true from matchLikelyAbbreviation, but since
	// Xxxxxxx is capitalized, matchLikelySentenceStart will return true as well.
	// So we need a special case:
	if (matchLikelyInitials(finalWord)) {
		return false;
	}

	// If we survived all the possible negative criteria, assume it's a final period.
	return true;
}

Symbol EnglishSentenceBreaker::getWordEndingAt(int index, const LocatedString *input, int origin) {
	int end = index + 1;
	int start = index - 1;

	// [wordchar or ']* -- so that for "doesn't", we get "doesn't" rather than "t",
	// even though isWordChar(') = false
	// also, don't allow -
	while (start >= origin) {
		if (!isWordChar(input->charAt(start)) || input->charAt(start) == L'-') {
			if (input->charAt(start) == L'\'' &&
			    start-1 >= origin &&
				isWordChar(input->charAt(start-1)))
				start--;
			else break;
		} else start--;
	}

	LocatedString *sub = input->substring(start + 1, end);
	Symbol ret = sub->toSymbol();
	delete sub;

	return ret;
}

bool EnglishSentenceBreaker::matchPossibleFinalPeriod(const LocatedString *input, int index, int origin) {
	// A sentence can't begin with a final period.
	if (index <= origin) {
		return false;
	}

	const int len = input->length();

	// If the period is the last character, it must be a final period.
	if ((index + 1) >= len) {
		return true;
	}

	// The char at the index must be a period.
	if (input->charAt(index) != L'.') {
		return false;
	}

	// The preceding char must be alphanumeric, closing punctuation, or
	// whitespace (in case the input text has already separated periods).
	wchar_t prev = input->charAt(index - 1);
	if (!iswalnum(prev) && !isClosingPunctuation(prev) && !iswspace(prev)) {
		return false;
	}

	// If it's an ellipsis, it could be final.
	if (matchEllipsis(input, index) > 0) {
		return true;
	}

	// If it's followed by anything else but whitespace or closing punctuation
	// followed in turn by whitespace, it can't be a final period.
	int pos = index + 1;
	while ((pos < input->length()) && isClosingPunctuation(input->charAt(pos))) {
		pos++;
	}
	// TODO: what if closing punctuation followed by end of line?
	if ((pos < len) && !iswspace(input->charAt(pos))) {
		return false;
	}

	// Otherwise, it's possible.
	return true;
}

bool EnglishSentenceBreaker::matchNonSplittableName(const LocatedString *input, int index, int origin) {
	// TODO: unify this with getWordEndingAt
	int end = index + 1;
	int start = index - 1;

	// Find the beginning of the word to the left of the period.
	while ((start >= origin) && (isWordChar(input->charAt(start)))) {
		start--;
	}

	// Find the end of the word to the right of the period.
	while ((end < input->length()) && iswspace(input->charAt(end))) {
		end++;
	}
	while ((end < input->length()) && iswalpha(input->charAt(end))) {
		end++;
	}

	LocatedString *substring = input->substring(start + 1, end);
	bool rv = _noSplitAbbreviations->contains(substring->toString());
	delete substring;

	return rv;
}

bool EnglishSentenceBreaker::matchLikelyInitials(Symbol word) {
	const wchar_t *s = word.to_string();
	const int len = (int)wcslen(s);

	// It must either begin with an uppercase letter
	if (!iswupper(s[0])) {
		return false;
	}

	Symbol withoutPeriod = Symbol(s, 0, len - 1);

	// If there is more than one period in the word,
	// then it's probably a set of initials
	if (wcsstr(withoutPeriod.to_string(), L".") != NULL) {
		return true;
	}

	return false;

}

bool EnglishSentenceBreaker::matchLikelyAbbreviation(Symbol word) {
	const wchar_t *s = word.to_string();
	const int len = (int)wcslen(s);

	if (len <= 1)
		return false;

	// Unless the document is all lowercase,
	// it must either begin with an uppercase letter or be
	// a.m. or p.m. to be a potential abbreviated word.
	if (!(_isAllLowercase || iswupper(s[0]) || (len >= 4 && wcsstr(&s[len - 3], L".m.")))) {
		return false;
	}

	Symbol withoutPeriod = Symbol(s, 0, len - 1);

	// If the word without the final period was in the list of known words and the
	// word with the final period is not, then it's not a recognized abbreviation.
	if (_knownWords->contains(withoutPeriod) && !_knownWords->contains(word)) {
		return false;
	}

	// If there is more than one period in the word, then it's probably an
	// abbreviation such as K.G.B. or F.B.I. 
	if (wcsstr(withoutPeriod.to_string(), L".") != NULL && !isLikelyWebAddress(withoutPeriod)) {
		return true;
	}

	// Otherwise, assume anything over the maximum length isn't an abreviation.
	return (len - 1) <= MAX_ABBREV_WORD_LEN;
}

bool EnglishSentenceBreaker::isLikelyWebAddress(Symbol word) {
	wstring wordStr = word.to_string();
	if (wordStr.length() >= 4) { 
		size_t position = wordStr.length() - 4;
		return 
			wordStr.rfind(L".com") == position ||
			wordStr.rfind(L".gov") == position ||
			wordStr.rfind(L".edu") == position ||
			wordStr.rfind(L".org") == position ||
			wordStr.rfind(L".mil") == position ||
			wordStr.rfind(L".net") == position;
	}
	return false;
}

// Returns true if the first word is capitalized but not the second word.
// TODO: what about one-word sentences? (e.g., "Yes.")
//       or sentences with proper for second word? (e.g., "Also Friday, the Pentagon announced...")
bool EnglishSentenceBreaker::matchLikelySentenceStart(const LocatedString *input, int pos) {

	// if the region is all lowercase we don't have enough info to
	// make a determination, so return false.
	if (_isAllLowercase)
		return false;

	const int len = input->length();

	// Skip to the first word.
	while ((pos < len) && !iswalpha(input->charAt(pos))) {
		pos++;
	}

	// The first word should be capitalized.
	if ((pos >= len) || !iswupper(input->charAt(pos))) {
		return false;
	}

	// Skip to the second word.
	while ((pos < len) && iswalpha(input->charAt(pos))) {
		pos++;
	}
	while ((pos < len) && !iswalpha(input->charAt(pos))) {
		pos++;
	}

	// Skip the word "of" if it appears next.
	if (((pos + 3) < len) &&
		(input->charAt(pos) == L'o') &&
		(input->charAt(pos + 1) == L'f'))
	{
		pos += 2;
		while ((pos < len) && !iswalpha(input->charAt(pos)))
		{
			pos++;
		}
	}

	// The second word should not be capitalized.
	if ((pos >= len) || iswupper(input->charAt(pos))) {
		return false;
	}

	return true;
}

bool EnglishSentenceBreaker::matchWashPostDateline(const LocatedString *input, int pos) {
	const int len = input->length();

	if ((pos >= len) || (input->charAt(pos) != L'-')) {
		return false;
	}
	pos++;

	int newlines = 0;
	while (newlines < 3) {
		while ((pos < len) &&
			   (input->charAt(pos) != L'\n') &&
			   (iswspace(input->charAt(pos))))
		{
			pos++;
		}
		if ((pos >= len) || (input->charAt(pos) != L'\n')) {
			return false;
		}
		newlines++;
	}
	return true;
}

/**
 * @param input the input string.
 * @param index the index at which to search.
 * @return the length of the ellipsis substring, or <code>0</code>
 *         if an ellipsis was not matched.
 */
int EnglishSentenceBreaker::matchEllipsis(const LocatedString *input, int index) {
	const int len = input->length();

	// We have to be at a period to match.
	if ((index >= len) || (input->charAt(index) != L'.')) {
		return 0;
	}

	int matched_periods = 0;
	int end = index;
	int last_period = index;
	while (true) {
		// Skip any whitespace.
		while ((end < len) && iswspace(input->charAt(end))) {
			end++;
		}
		if ((end >= len) || (input->charAt(end) != L'.')) {
			break;
		}
		last_period = end;
		end++;
		matched_periods++;
	}

	// as in "larouse...@yahoo.com" -- we don't want to break here
	if (last_period + 1 < len && input->charAt(last_period + 1) == L'@')
		return false;

	return (matched_periods < 3) ? 0 : ((last_period + 1) - index);
}

bool EnglishSentenceBreaker::matchCenterOfURL(const LocatedString *input, int index, int ellipsis_len) {
	if (!Tokenizer::matchURL(input, index))
		return false;

	int following_char_pos = index + ellipsis_len;
	if (following_char_pos >= input->length())
		return false;

	return !iswspace(input->charAt(following_char_pos));
}

bool EnglishSentenceBreaker::isWordChar(const wchar_t c) const {
	return !iswspace(c) && !isSplitChar(c) && !isClosingPunctuation(c);
}

bool EnglishSentenceBreaker::isSplitChar(const wchar_t c) const {
	return wcschr(SPLIT_PUNCTUATION, c) != NULL;
}

bool EnglishSentenceBreaker::isOpeningPunctuation(const wchar_t c) const {
	return wcschr(OPENING_PUNCTUATION, c) != NULL;
}

bool EnglishSentenceBreaker::isClosingPunctuation(const wchar_t c) const {
	return wcschr(CLOSING_PUNCTUATION, c) != NULL;
}

bool EnglishSentenceBreaker::isBulletChar(const wchar_t c) const {
	return wcschr(BULLET_PUNCTUATION, c) != NULL;
}

bool EnglishSentenceBreaker::isEOSChar(const wchar_t c) const {
	return wcschr(EOS_PUNCTUATION, c) != NULL;
}

bool EnglishSentenceBreaker::isSecondaryEOSChar(const wchar_t c) const {
	return wcschr(SECONDARY_EOS_PUNCTUATION, c) != NULL;
}

bool EnglishSentenceBreaker::isEOLChar(const wchar_t c) const {
	return (c == '\n');
}

int EnglishSentenceBreaker::max(int a, int b) {
	return ((a > b) ? a : b);
}

int EnglishSentenceBreaker::min(int a, int b) {
	return ((a < b) ? a : b);
}

// this test used with downcaseHeadlines to guess when a sentence is a headline
bool EnglishSentenceBreaker::_isLikelyHeadline(const LocatedString* locStr) {

	if (_curDoc->isDowncased()) {
		// Then we have no idea
		return false;
	}
	std::wstring str(locStr->toString());
	
	std::vector<std::wstring> words;
	boost::split(words, str, boost::is_any_of("\t\n -,.?!;:()[]{}'`\""));
	
	int n_nonempty_words = 0;
	BOOST_FOREACH(std::wstring word, words) {
		// Lowercase words longer than three-characters must be on the "safe" list, otherwise
		//   this is not a headline
		if (word.size() == 0) {
			continue;
		}		
		// Allow words of size 1 and 2, esp. since they are often parts of words separated by
		//   apostrophes or hyphens, e.g. Don't --> Don t
		if (word.size() > 2 && iswlower(word.at(0)) && !_lowercaseHeadlineWords->contains(word)) {
			return false;
		}
		// Don't count numbers, in particular, to avoid most datelines
		if (iswalpha(word.at(0))) {
			n_nonempty_words++;
		}		
	}
	
	// Most things with fewer than six words are actually 
	//   bylines/datelines, which should be handled differently
	if (n_nonempty_words < 6) {
		return false;
	}

	return true;

}

bool EnglishSentenceBreaker::_isExplicitlyLowercase(const wchar_t c) const {
	return (iswlower(c) && !_curDoc->isDowncased());
}

bool EnglishSentenceBreaker::_isExplicitlyUppercase(const wchar_t c) const {
	return (iswupper(c) && !_curDoc->isDowncased());
}

bool EnglishSentenceBreaker::_isSafeDoubleCarriageReturn(const LocatedString *input,
												  int start,
												  int end)
{
	if (!_breakOnDoubleCarriageReturns)
		return false;

	const int len = input->length();

	int counter = -1;
	if (input->charAt(end) == L'\n')
		counter = end + 1;
	else if (input->charAt(end) == L'\r' && end + 1 < len && input->charAt(end+1) == L'\n')
		counter = end + 2;
	else return false;

	bool found_second_newline = false;
	while (counter < len) {
		if (input->charAt(counter) == L'\n' || (input->charAt(counter) == L'\r' && counter + 1 < len && input->charAt(counter+1) == L'\n')) {
			found_second_newline = true;
			break;
		} else if (!iswspace(input->charAt(counter))) {
			break;
		} else {
			counter++;
		}
	}
	if (!found_second_newline)
		return false;

	int next_sentence = counter;
	// Skip to the next word
	while ((next_sentence < len) && (iswspace(input->charAt(next_sentence)))) {
		next_sentence++;
	}
	// The first word should not be lowercase
	if ((next_sentence >= len) || 
		(_isExplicitlyLowercase(input->charAt(next_sentence)) && 
		 !_aggressiveDoubleCarriageReturnsSplit))
	{
		return false;
	}

	int pos = next_sentence - 1;
	while (pos >= start) {
		if (iswspace(input->charAt(pos)))
			pos--;
		else break;
	}
	bool is_all_lower = true;
	while (pos >= start) {
		if (_isExplicitlyLowercase(input->charAt(pos)))
			pos--;
		else if (iswspace(input->charAt(pos)))
			break;
		else {
			is_all_lower = false;
			break;
		}
	}

/*	if (is_all_lower && input->charAt(next_sentence) != L'*') {
		if (end + 30 < len) {
			LocatedString *s = input->substring(start, end + 30);
			char result[1001];
			EnglishStringTransliterator::transliterateToEnglish(result, s->toString(), 1000);
			std::cerr << "\nNOT BREAKING: " << result;
			delete s;
		}
		return false;
	}*/

	/*LocatedString *sentenceString = input->substring(start, end);
	char result[1001];
	EnglishStringTransliterator::transliterateToEnglish(result, sentenceString->toString(), 1000);
	std::cerr << "\nbreaking after: " << result;
	delete sentenceString;
	if (end + 30 < len) {
		sentenceString = input->substring(end+2, end + 30);
		char result[1001];
		EnglishStringTransliterator::transliterateToEnglish(result, sentenceString->toString(), 1000);
		std::cerr << "\nbefore: " << result;
		delete sentenceString;
	}
	std::cerr << "\n";*/
	return true;
}

bool EnglishSentenceBreaker::_isCarriageReturnBreak(const LocatedString *input, int start, int end) {
	if (_isSafeDoubleCarriageReturn(input, start, end))
		return true;

	if (!_isWebText)
		return false;

	if (input->charAt(end) != L'\n')
		return false;

	int last_cr = end - 1;
	while (last_cr >= 0) {
		if (input->charAt(last_cr) == L'\n')
			break;
		last_cr--;
		if (end - last_cr > _short_line_length)
			break;
	}

	if (end - last_cr < _short_line_length) {
		return true;
	}

	int len = input->length();
	if (end + 2 >= len)
		return false;

	// cases like:
	// - item 1
	// - item 2
	// - item 3
	if (input->charAt(start) == L'-' ||
		 input->charAt(start) == L'*' ||
		 input->charAt(start) == L'=')
	{
		int first_non_wspace = end + 1;
		while (first_non_wspace < len &&
			iswspace(input->charAt(first_non_wspace)))
		{
			first_non_wspace++;
		}
		if (first_non_wspace < len &&
			input->charAt(end+1) == input->charAt(start))
		{
 			return true;
		}
	}

	return false;
}


// Break after seeing 4 or more dashes in a row followed by a space
bool EnglishSentenceBreaker::_isLastInStringOfDashes(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == L'-' &&
		end + 1 < input->length() &&
		input->charAt(end + 1) != L'-')
	{
		int i;
		for (i = end - 1; i > end - 4; i--) {
			if (i < start)
				return false;
			if (input->charAt(i) != L'-')
				return false;
		}
		return true;
	}
	return false;
}


// Break after seeing 3 or more slashes in a row followed by a space
bool EnglishSentenceBreaker::_isLastInStringOfSlashes(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == L'/' &&
		end + 1 < input->length() &&
		iswspace(input->charAt(end + 1)))
	{
		int i;
		for (i = end - 1; i > end - 3; i--) {
			if (i < start)
				return false;
			if (input->charAt(i) != L'/')
				return false;
		}
		return true;
	}
	return false;
}

// Break after seeing a long string of punctuation in a row followed by a space
bool EnglishSentenceBreaker::_isLastInPunctuationString(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == '\'' ||
		input->charAt(end) == '`' ||
		input->charAt(end) == '.' ||
		input->charAt(end) == '"')
		return false;

	if (iswpunct(input->charAt(end)) &&
		end + 1 < input->length() &&
		iswspace(input->charAt(end+1)))
	{
		int i;
		for (i = end - 1; i > end - 4; i--) {
			if (i < start)
				return false;
			if (!iswpunct(input->charAt(i)))
				return false;
			if (input->charAt(i) == '\'' ||
				input->charAt(i) == '`' ||
				input->charAt(i) == '.' ||
				input->charAt(i) == '"')
				return false;
		}
		return true;
	}
	return false;
}

// Break on semi-colon if it's preceded and folowed buy a space
bool EnglishSentenceBreaker::_isBreakableSemicolon(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == ';' && 
		(end == 0 || input->charAt(end - 1) == ' ') &&
		(end + 2 >= input->length() || input->charAt(end + 1) == ' '))
	{
		return true;
	}
	return false;
}

// Break on closing paren if we started with an open paren
bool EnglishSentenceBreaker::_isClosingStartingOpenParen(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == ')' && input->charAt(start) == '(')
	{
		return true;
	}
	return false;
}

// Break on the matching quote character if we are currently in an open quote
bool EnglishSentenceBreaker::_isClosingQuoteLevel(const LocatedString* input, int end)
{
	if (isClosingPunctuation(input->charAt(end)) && !_quoteLevel.empty()) {
		std::wstring openers = OPENING_PUNCTUATION;
		std::wstring closers = CLOSING_PUNCTUATION;
		wchar_t open = _quoteLevel.top();
		size_t open_index = openers.find(open);
		if (closers[open_index] == input->charAt(end)) {
			if (input->charAt(end) == L'\'' && end > 0 && iswalpha(input->charAt(end - 1)) && end < input->length() - 1 && iswalpha(input->charAt(end + 1)))
				// Don't match inside a probable contraction
				return false;
			else if (end < input->length() - 1 && input->charAt(end) == input->charAt(end + 1))
				// Don't match until the end of a sequence of closing quotes
				return false;
			else {
				// Check if the quote continues a sentence with a comma or semicolon, or in a cased document with lowercase, handling document bounds if punctuation not found
				int prevPunct = end - 1;
				while (prevPunct >= 0 && prevPunct < input->length() && (iswspace(input->charAt(prevPunct)) || input->charAt(prevPunct) == input->charAt(end)))
					prevPunct--;
				bool continue_inside_quote = (prevPunct >= 0 && prevPunct < input->length() && (input->charAt(prevPunct) == L',' || input->charAt(prevPunct) == L';'));

				int nextPunct = end + 1;
				while (nextPunct >= 0 && nextPunct < input->length() && iswspace(input->charAt(nextPunct)))
					nextPunct++;
				bool continue_outside_quote = (nextPunct >= 0 && nextPunct < input->length() && (input->charAt(nextPunct) == L',' || input->charAt(nextPunct) == L';' || _isExplicitlyLowercase(input->charAt(nextPunct))));

				if (continue_inside_quote || continue_outside_quote) {
					// Close the quote but don't break
					_quoteLevel.pop();
					return false;
				} else if (_dont_break_in_parentheticals && end > 0 && input->charAt(end) == L')' && input->charAt(end - 1) == L')') {
					// Close the double parentheses but don't break; these are handled in the tokenizer if parentheticals are being ignored
					_quoteLevel.pop();
					return false;
				} else
					// Break at end of quotation
					return true;
			}
		}
	}
	return false;
}

bool EnglishSentenceBreaker::isListSeparator(const wchar_t c) const {
	// Check if this is either a punctuation character or a separator specified in the parfile
	return ((_listSeparators.length() == 0 && iswpunct(c)) || wcschr(_listSeparators.c_str(), c) != NULL);
}

// Break in the middle of token: "thing.2324". This is a footnote number or page number
bool EnglishSentenceBreaker::_isBreakableCenterPeriod(const LocatedString *input, int start, int end) {
	if (end == 0 || end >= input->length() - 1)
		return false;

	if (input->charAt(end) == L'.' && 
		!iswdigit(input->charAt(end-1)) && 
		!iswspace(input->charAt(end-1)) && 
		iswdigit(input->charAt(end+1)))
	{
		return true;
	}	
	
	// Break on the whitespace after "thing.2324" so 2324 is in its own sentence
	if (iswspace(input->charAt(end)) && iswdigit(input->charAt(end-1))) {
		// back up to non-digit
		int index = end-1;
		while (index > 1 && iswdigit(input->charAt(index)))
			index--;
		if (input->charAt(index) == L'.' && 
			!iswdigit(input->charAt(index-1)) &&
			!iswspace(input->charAt(index-1)))
		{
			return true;
		}
	}

	// digits before and after period then a space, then a capital letter
	if (input->charAt(end) == L'.' &&
		iswdigit(input->charAt(end-1)) &&
		iswdigit(input->charAt(end+1)))
	{
		// go forward to non-digit
		int index = end + 2;
		while (index < input->length() && iswdigit(input->charAt(index)))
			index += 1;
		if (index == input->length() || !iswspace(input->charAt(index)))
			return false;
		// go forward to non-whitespace
		while (index < input->length() && iswspace(input->charAt(index)))
			index += 1;
		if (index == input->length() || !iswupper(input->charAt(index)))
			return false;
		return true;
	}

	// break on the whitespace after the case above
	if (iswspace(input->charAt(end)) && iswdigit(input->charAt(end-1))) {
		// move forward to non-whitespace
		int index = end + 1;
		while (index < input->length() && iswspace(input->charAt(index)))
			index += 1;
		if (index == input->length() || !iswupper(input->charAt(index)))
			return false;
		// back up to non-digit
		index = end - 1;
		while (index > 1 && iswdigit(input->charAt(index)))
			index--;
		
		if (input->charAt(index) == L'.' && 
			iswdigit(input->charAt(index-1)))
		{
			return true;
		}
	}

	// break on quote in thing."2341
	if ((input->charAt(end) == L'"' || input->charAt(end) == L'\'') &&
		input->charAt(end-1) == L'.' && iswdigit(input->charAt(end+1)))
	{
		return true;
	}

	// break on the whitespace after the case above
	if (iswspace(input->charAt(end)) && iswdigit(input->charAt(end-1)))
	{
		int index = end - 1;
		while (index > 1 && iswdigit(input->charAt(index)))
			index--;
		if ((input->charAt(index) == L'"' || input->charAt(index) == L'\'') &&
			 input->charAt(index - 1) == L'.')
		{
			return true;
		}
	}

	return false;
}

// Break before seeing a portion mark in parentheses
bool EnglishSentenceBreaker::_isFollowedByPortionMark(const LocatedString *input, int start, int end) {
	// Consume any non-line whitespace
	int index = end + 1;
	while (index < input->length() && iswspace(input->charAt(index)) && input->charAt(index) != '\n')
		index++;
	if (index >= input->length())
		return false;

	// Find open and closing parens (technically we're more broad here than just ASCII parentheses)
	int open = start;
	int close = end;
	if (isOpeningPunctuation(input->charAt(index))) {
		open = index;
		index++;
	} else
		return false;
	while (index < input->length() && input->charAt(index) != '\n' && !isClosingPunctuation(input->charAt(index)))
		index++;
	if (index < input->length() && isClosingPunctuation(input->charAt(index))) {
		close = index;
		index++;
	} else
		return false;

	// Check parenthetical contents as mark
	return _isPortionMark(input, open + 1, close);
}

// Break before seeing a long string of punctuation in a row followed by a space
bool EnglishSentenceBreaker::_isFollowedByPunctuationString(const LocatedString* input, int start, int end)
{
	if (iswpunct(input->charAt(end)))
		return false;

	int index = end + 1;
	while (index < input->length() && iswspace(input->charAt(index)))
		index++;

	// we've advanced index to next non-white space character
	int i;
	for (i = index; i < index + 4; i++) {
		if (i >= input->length())
			return false;
		if (!iswpunct(input->charAt(i)))
			return false;

		// ignore quote and split characters
		if (isOpeningPunctuation(input->charAt(i)) ||
			isClosingPunctuation(input->charAt(i)) ||
			isSplitChar(input->charAt(i)) ||
			isEOSChar(input->charAt(i)))
			return false;
	}
	return true;
}



// Break if next non-whitespace characters are a string of Dashes
bool EnglishSentenceBreaker::_isFollowedByStringOfDashes(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == L'-')
		return false;

	int index = end + 1;
	while (index < input->length() && iswspace(input->charAt(index)))
		index++;

	// we've advanced index to next non-white space character
	int i;
	for (i = index; i < index + 4; i++) {
		if (i >= input->length())
			return false;
		if (input->charAt(i) != L'-')
			return false;

	}
	return true;
}

// used for breaking lists like:
// A. Item
// B. Item
// C. Item
bool EnglishSentenceBreaker::_isFollowedByListItem(const LocatedString* input, int end)
{
	if (input->charAt(end) != L'\n')
		return false;
	if (end > 0 && input->charAt(end - 1) != L'\n' && iswspace(input->charAt(end - 1)))
		return false;

	// find next non-whitespace character and see if it matches "A.", "B.", "1.",
	// "2.", "(1)", "(A)", "1A." e.g.
	int index = end + 1;
	while (index < input->length() && iswspace(input->charAt(index)))
		index++;

	if (index >= input->length())
		return false;

	// check for "(1)", "(A)", etc.
	if (input->charAt(index) == L'(') {
		index++;

		if (index >= input->length())
			return false;

		// advance past all the numbers or 1 letter
		if (iswdigit(input->charAt(index))) {
			index++;
			while (index < input->length() && iswdigit(input->charAt(index)))
				index++;
		} else if (iswalpha(input->charAt(index))) {
			index++;
		} else
			return false;

		if (index >= input->length())
			return false;

		if (input->charAt(index) == L')')
			return true;

		return false;
	}

	// if the next character is a number, advance past all numbers
	// if the next character is a Roman numeral, advance past all Roman numerals
	// if it's a letter, advance once
	// if it's none of the above, then return false
	if (iswdigit(input->charAt(index))) {
		index++;
		while (index < input->length() && iswdigit(input->charAt(index)))
			index++;
		// allow advancing past one letter to catch "1A."
		if (index < input->length() && iswalpha(input->charAt(index)))
			index++;
	} else if (_isRoman(input->charAt(index))) {
		index++;
		while (index < input->length() && _isRoman(input->charAt(index)))
			index++;
	} else if (iswalpha(input->charAt(index))) {
		index++;
	} else
		return false;

	if (index >= input->length())
		return false;

	// It matches the pattern if a period or ")" is the next character
	if (input->charAt(index) == L'.' || input->charAt(index) == L')')
		return true;

	return false;
}


// break on "A." or "1." or "VI." if it's at the beginning of a sentence
bool EnglishSentenceBreaker::_isListItemPeriod(const LocatedString *input, int start, int end)
{
	if (input->charAt(end) != L'.')
		return false;
	if (input->length() <= end + 1)
		return false;
	if (!iswspace(input->charAt(end + 1)))
		return false;

	int index = end - 1;
	if (index < 0) return false;

	// if previous character is a digit, skip over all digits
	// if previous character is a Roman numeral, skip over all Roman numerals
	// if previous character is an alpha, skip over it
	// else return false
	if (iswdigit(input->charAt(index))) {
		index--;
		while (index >= 0 && iswdigit(input->charAt(index)))
			index--;
	} else if (_isRoman(input->charAt(index))) {
		index--;
		while (index >= 0 && _isRoman(input->charAt(index)))
			index--;
	} else if (iswalpha(input->charAt(index))) {
		index--;
		// allow skipping over a single digit, so as to sentence break on "1A."
		if (index >= 0 && iswdigit(input->charAt(index)))
			index--;
	} else
		return false;

	// now we should find only white space in the current sentence
	while (index >= start) {
		if (!iswspace(input->charAt(index))) {
			return false;
		}
		index--;
	}

	return true;
}

// break on "(1)" or "(A)" if it's at the beginning of a sentence
bool EnglishSentenceBreaker::_isListItemParen(const LocatedString *input, int start, int end)
{
	if (input->charAt(end) != L')')
		return false;

	int index = end - 1;
	if (index < 0) return false;

	// if previous character is a digit, skip over all digits
	if (iswdigit(input->charAt(index))) {
		index--;
		while (index >= 0 && iswdigit(input->charAt(index))) 
			index--;
	} else if (iswalpha(input->charAt(index)) && index > 0) {
		index--;
		// allow skipping over a single digit, so as to sentence break on "1A)"
		if (index >= 0 && iswdigit(input->charAt(index)))
			index--;
	} else
		return false;

	// now we should find a optional "(" 
	if (index >= 0 && input->charAt(index) == L'(')	
		index--;
	
	// now we should find only white space in the current sentence
	while (index >= start) {
		if (!iswspace(input->charAt(index))) {
			return false;
		}
		index--;
	}

	return true;
}

bool EnglishSentenceBreaker::_isAtEndOfListItem(const LocatedString* input, int end)
{
	if (input->charAt(end) != L'\n')
		return false;

	// Back up and skip over any preceding whitespace
	int index = end - 1;
	while (index >= 0 && iswspace(input->charAt(index)))
		index--;

	// now back up and find the previous carriage return
	while (index >= 0 && input->charAt(index) != '\n')
		index--;

	if (index < 0)
		return false;

	// find next non-whitespace character and see if it matches "A.", "B.", "1.", "2." e.g.
	while (index < input->length() && iswspace(input->charAt(index)))
		index++;

	if (index >= input->length())
		return false;

	// if the next character is a number, advance past all numbers
	// if the next character is a Roman numeral, advance past all Roman numerals
	// if it's a letter, advance once
	// if it's neither, then return false
	if (iswdigit(input->charAt(index))) {
		index++;
		while (index < input->length() && iswdigit(input->charAt(index)))
			index++;
	} else if (_isRoman(input->charAt(index))) {
		index++;
		while (index < input->length() && _isRoman(input->charAt(index)))
			index++;
	} else if (iswalpha(input->charAt(index))) {
		index++;
	} else
		return false;

	if (index >= input->length())
		return false;

	// It matches the pattern if a period is the next character
	if (input->charAt(index) == L'.')
		return true;

	return false;
}

bool EnglishSentenceBreaker::_endsDashLine(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) != L'\n')
		return false;

	bool found_dash = false;

	// Back up and skip over any preceding whitespace
	int index = end - 1;
	while (index >= 0) {
		if (input->charAt(index) != L'-' && !iswspace(input->charAt(index)))
			return false;

		if (input->charAt(index) == L'-')
			found_dash = true;

		if (index == start || input->charAt(index) == L'\n')
			return found_dash;

		index--;
	}

	return false;
}

namespace {
	/** Return true if the located string `input` contains a given substring at
	 * the specified position. */
	template<int N>
	bool substringMatches(const LocatedString * &input, int pos, const wchar_t (&substr)[N]) {
		return input->toWString().compare(pos, N-1, substr) == 0;
	}
}

bool EnglishSentenceBreaker::_isBuriedDateline(const LocatedString* input, int start, int end)
{
	if (input->charAt(end) == L'-') {
		if (substringMatches(input, start, L"(AFP)"))
			return true;		
		if (substringMatches(input, start, L"(AP)"))
			return true;
	}

	if (end + 5 < input->length()) {
		if (substringMatches(input, end+1, L"(AFP)"))
			return true;
		if (substringMatches(input, end+1, L"(AP)"))
			return true;
	}

	
	if (end + 10 < input->length()) {
		if (substringMatches(input, end+1, L"ATTENTION -"))
			return true;
	}
	
	if (DATELINE_MODE >= DL_VERY_AGGRESSIVE) {
		// track down '--' and ':' followed by capital letters and break no matter where we are
		if (end + 3 < input->length()) {
			int temp = 0;
			if (substringMatches(input, end+1, L"--")) {
				temp = end + 3;
			} else if (substringMatches(input, end+1, L":")) {
				temp = end + 2;
			}
			if (temp != 0) {
				while (temp < input->length() && iswspace(input->charAt(temp)))
					temp++;
				if (temp < input->length() && iswupper(input->charAt(temp)))
					return true;
			}
		}
	}

	return false;
}

bool EnglishSentenceBreaker::_isSentenceEndingWebAddress(const LocatedString *input, int start, int end) {
	if (end - 4 < start)
		return false;

	// we will break if we see ".com Xxxxxx" or ".com. Xxxxxx"
	if (input->charAt(end) != '.' && !iswspace(input->charAt(end)))
		return false;

	int strlen = input->length();
	if (input->charAt(end) == '.' && 
		(end + 1 == strlen || !iswspace(input->charAt(end + 1))))
	{
		return false;
	}

	if (input->indexOf(L".com", start) == end - 4 ||
		input->indexOf(L".org", start) == end - 4 ||
		input->indexOf(L".net", start) == end - 4 ||
		input->indexOf(L".gov", start) == end - 4 ||
		input->indexOf(L".edu", start) == end - 4)
	{
		for (int i = end + 1; i + 1 < strlen; i++) {
			if (iswspace(input->charAt(i)))
				continue;
			if (!iswupper(input->charAt(i)))
				return false;
			if (!iswlower(input->charAt(i+1)))
				return false;

			return true;			
		}
	}
	return false;
}

bool EnglishSentenceBreaker::_isFollowedByDashLine(const LocatedString *input, int end)
{
	if (input->charAt(end) != L'\n')
		return false;
	if (end > 0 && input->charAt(end - 1) != L'\n' && iswspace(input->charAt(end - 1)))
		return false;

	//bool found_dash = false;

	int index = end + 1;
	// skip over any whitespace
	while (index < input->length() && iswspace(input->charAt(index)))
		index++;
	if (index >= input->length())
		return false;

	if (input->charAt(index) == L'-')
		return true;

	return false;
}

bool EnglishSentenceBreaker::_isBreakableColon(const LocatedString *input, int start, int end) {
	if (input->charAt(end) != L':')
		return false;

	// Ignore colons inside of quotes
	if (!_quoteLevel.empty())
		return false;
	int localQuoteDepth = 0;
	for (int i = start; i <= end; i++)
		if (isOpeningPunctuation(input->charAt(i)))
			localQuoteDepth++;
		else if (isClosingPunctuation(input->charAt(i)))
			localQuoteDepth--;
	if (localQuoteDepth > 0)
		return false;

	// Ignore colons immediately preceding quotes
	int i = end + 1;
	while (i < input->length() && iswspace(input->charAt(i)))
		i++;
	if (i >= input->length() || isOpeningPunctuation(input->charAt(i)))
		return false;

	if (input->length() <= end + 1 ||
		!iswspace(input->charAt(end + 1)))
		return false;

	else
		return (end - start < 60);
}

bool EnglishSentenceBreaker::_isBreakableXMLGlyph(const LocatedString *input, int start, int end) {
	// &lt; or &rt;
	bool found_glyph = (end-3 >= start &&
		input->charAt(end) == L';' &&
		input->charAt(end-1) == L't' &&
		(input->charAt(end-2) == L'l' || input->charAt(end-2) == L'r') &&
		input->charAt(end-3) == L'&');

	// must have whitespace on at least one side of it
	return (found_glyph && 
		(end - 4 < start || iswspace(input->charAt(end-4)) ||
		 end + 1 >= input->length() || iswspace(input->charAt(end+1))));
}

bool EnglishSentenceBreaker::_isRoman(wchar_t c) {
	return
		(c == L'I' ||
		c == L'V' ||
		c == L'X' ||
		c == L'L' ||
		c == L'C' ||
		c == L'D' ||
		c == L'M' ||
		c == L'i' ||
		c == L'v' ||
		c == L'x');
}

// e.g. ATTENTION - UPDATES, INCORPORATES Health-pneumonia-Canada-deaths /// 
bool EnglishSentenceBreaker::_isRemovableGALESentence(Sentence* sent) {
	std::wstring wstr(sent->getString()->toString());
	if (wstr.find(L"ATTENTION -") == 0) {
		return true;
	}

	return false;	
}

bool EnglishSentenceBreaker::_isRemovableGALERegion(const LocatedString *input) {

	int len = input->length();
	bool found_slash = false;
	bool found_hyphen = false;
	bool found_colon = false;
	bool found_non_lower = false;

	int first_non_space = 0;
	while (first_non_space < len && iswspace(input->charAt(first_non_space))) {
		first_non_space++;
	}
	if (first_non_space == len)
		return false;

	// find first space
	int next_space = 0;
	for (int i = first_non_space; i < len; i++) {
		if (iswspace(input->charAt(i))) {
			next_space = i;
			break;
		} else if (input->charAt(i) == L'/') {
			found_slash = true;
		} else if (input->charAt(i) == L'-') {
			found_hyphen = true;
		} else if (input->charAt(i) == L':') {
			found_colon = true;
		} else if (iswlower(input->charAt(i)) || iswdigit(input->charAt(i))) {
			// ok
		} else found_non_lower = true;
	}

	// are there any more non-whitespace characters after the first space? if so, return
	for (int m = next_space + 1; m < len; m++) {
		if (!iswspace(input->charAt(m)))
			return false;
	}

	// e.g. str/mm/jvg/mfc
	if (found_slash && !found_hyphen && !found_non_lower)
		return true;

	// don't want to remove regions that are just a date, e.g. 2005-12-03T07:05:00
	// this should potentially be improved, but it will do for now
	if (found_hyphen && found_colon)
		return false;

	// e.g:	US-war-Iraq-checkpoint
	//		Iran-Britain-embassy
	//		Japan-confidence
	if (found_hyphen && !found_slash)
        return true;

	return false;	
}

/**
 * Case-insensitive search regex for the word "page" followed by an integer.
 *
 * @author nward@bbn.com
 * @date 2013.06.20
 **/
const boost::wregex EnglishSentenceBreaker::page_number_re(
	L"page\\s+\\d+",
	boost::regex_constants::icase
);

/**
 * Removes substrings from the input document string that match
 * the page number pattern; additional removes adjacent empty lines
 * or header/footer lines that are part of the page number
 * containing line.
 **/
void EnglishSentenceBreaker::_removePageBreaks(LocatedString *input) {
	// Keep track of the breaks we're going to remove
	std::vector<BreakBound> page_breaks;

	// Loop through the region's content
	for (int index = 0; index < input->length() - 1;) {
		// Find the bounds of this line
		int start = input->startOfLine(index);
		int end = input->endOfLine(index);
		if (start == end) {
			// Skip empty lines
			index = input->startOfNextNonEmptyLine(end);
			continue;
		}

		// Get page break candidate line
		std::wstring possible_page_break_line = input->substringAsWString(start, end);

		// Check for page breaks and portion marks
		if ((end - start < _short_line_length && boost::regex_search(possible_page_break_line, page_number_re)) || _isPortionMark(input, start, end)) {
			// Expand to consume adjacent newlines and page headers/footers
			do {
				start = input->endOfPreviousNonEmptyLine(start);
				int next_start = input->startOfLine(start);
				if (next_start == start)
					break;
				std::wstring possible_header_footer_line = input->substringAsWString(next_start, start);
				if (possible_page_break_line.find(possible_header_footer_line) != std::wstring::npos) {
					// Partial header/footer repeat, skip
					start = next_start;
				} else
					break;
			} while (start > 0);
			do {
				end = input->startOfNextNonEmptyLine(end);
				int next_end = input->endOfLine(end);
				if (next_end == end)
					break;
				std::wstring possible_header_footer_line = input->substringAsWString(end, next_end);
				if (possible_page_break_line.find(possible_header_footer_line) != std::wstring::npos) {
					// Partial header/footer repeat, skip
					end = next_end;
				} else
					break;
			} while (end < input->length());

			// Mark this span for deletion
			page_breaks.push_back(BreakBound(start, end));

			// Skip to the end of the span
			index = end;
		} else {
			// Skip empty lines
			index = input->startOfNextNonEmptyLine(end);
		}
	}

	// Actually delete, but in reverse order so that our indices stay valid
	BOOST_REVERSE_FOREACH(BreakBound page_break, page_breaks) {
		SessionLogger::updateContext(2, boost::lexical_cast<std::string>(_cur_sent_no).c_str());
		SessionLogger::dbg("ignore_page_breaks") << "Removing page break: " << input->substringAsWString(page_break.first, page_break.second);
		input->remove(page_break.first, page_break.second);
	}
}
