// Copyright 2013 by Raytheon BBN Technologies
// All Rights Reserved.

#include <math.h>
#include <vector>
#include <wchar.h>
#include <boost/algorithm/string.hpp> 
#include <boost/foreach.hpp> 
#include <boost/regex.hpp>

#include "Generic/common/leak_detection.h"

#include "Urdu/sentences/ur_SentenceBreaker.h"
#include "Generic/common/limits.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/InputUtil.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Span.h"

class Metadata;
static wchar_t URDU_PERIOD = 0x06d4;
static wchar_t SPLIT_PUNCTUATION[] = {0x002e, 0x003f, 0x0021, 0x0025, 0x06d4, 0x061f}; //.?!% URDU PERIOD, BACKWARDS QUESTION MARK
static wchar_t CLOSING_PUNCTUATION[] = {0x0029, 0x005d, 0x007d, 0x0027, 0x0022, 0x06d4}; //)]}'" URDU PERIOD
static wchar_t SECONDARY_EOS_SYMBOL[] = {0x002d, 0x2026, 0x005f}; //- ELLIPSIS _
static wchar_t CARRIAGE_RETURN[] = {L"\n"};
UrduSentenceBreaker::UrduSentenceBreaker() {

	_breakLongSentences = true;
	_max_whitespace = ParamReader::getOptionalIntParamWithDefaultValue("en_sentence_breaker_max_whitespace", 100);
	std::string input_file = ParamReader::getParam("sentence_breaker_debug_file");
	DEBUG = (!input_file.empty());
	if (DEBUG) _debugStream.open(input_file.c_str());
}

UrduSentenceBreaker::~UrduSentenceBreaker() {
	if (DEBUG) _debugStream.close();
}

/**
 * Sets the current document pointer to point to the
 * document passed in and resets the current sentence
 * number to 0.
 */
void UrduSentenceBreaker::resetForNewDocument(const Document *doc) {
	_curDoc = doc;
	_curRegion = 0;
	_cur_sent_no = 0;
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
int UrduSentenceBreaker::getSentencesRaw(Sentence **results,
									 int max_sentences,
									 const Region* const* regions,
									 int num_regions)
{
	if (num_regions < 1) {
		SessionLogger::warn("no_reg_0") << "No regions in document. Thus, no sentences.";
		return 0;
	}


	int text_index = 0;

	// Read the rest of the document.
	int last_headline_sent_no = max_sentences;  // Used to do intelligent dateline processing
	for (int i = 0; i < num_regions; i++) {
		_curRegion = regions[i];
		LocatedString *text = _new LocatedString(*_curRegion->getString());
		const int len = text->length();
	
		while (text_index < len) {
			if (_cur_sent_no == max_sentences) {
				throw UnexpectedInputException("UrduSentenceBreaker::getSentences()",
											   "Document contains more than max_sentences sentences");
			}
			if ((results[_cur_sent_no] = getNextSentence(&text_index, text)) != NULL) {
				if (_breakLongSentences) {
					attemptLongSentenceBreak(results, max_sentences, _curDoc->getMetadata());
					}
				if (_curRegion->getRegionTag() == Symbol(L"HEADLINE")) {
						last_headline_sent_no = _cur_sent_no;
					}
				_cur_sent_no++;
			}
			
		}
		text_index = 0;
		delete text;
	}

	return _cur_sent_no;
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
Sentence* UrduSentenceBreaker::getNextSentence(int *offset, const LocatedString *input) {
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

	// Search for the next sentence break.
	int end = start;
	bool found_break = false;
	while ((end < len) && (!found_break)) {
		if (isSplitChar(input->charAt(end))) {
			//consume any repeated SplitChars
			while(isSplitChar(input->charAt(end))) {
				end++;
			}
			int next_word = end + 1;
			//find the next real word, or the end of the document
			while ((next_word < len) && !iswalpha(input->charAt(next_word))) {
				next_word++;
			}
			if (next_word >= len) {
				//catch quotation marks or other closing punct before the next word			
				end = getIndexAfterClosingPunctuation(input, end);
				found_break = true;
			}
			else if (!Tokenizer::matchURL(input, end) && !matchEmail(input, end)) {
					found_break = true;
					end++;
			}
			
			
		} else if (matchFinalPeriod(input, end, start)) {
			// If the sentence ended with an ellipsis, skip to the end of it.
			int ellipsis_len = matchEllipsis(input, end);
			if (ellipsis_len > 0) {
				end += ellipsis_len - 1;
			}
			// If followed by any closing punctuation, include that too.
			end = getIndexAfterClosingPunctuation(input, end);
			found_break = true;
		} else if (isCarriageReturn(input->charAt(end))) {
			found_break = true;
			end++;
		} else {
			end += max(1, matchEllipsis(input, end));
			// Note that this moves  end forward, with or without an ellipsis
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

	Sentence *result = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
	delete sentenceString;

	return result;
}


std::wstring UrduSentenceBreaker::_getNextToken(const LocatedString *input, int start, bool letters_only) {

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
		if (letters_only && !iswlower(next_char) && !iswupper(next_char))
			break;
		word += next_char;
		start++;
	}
	return word;

}

std::wstring UrduSentenceBreaker::_getPrevToken(const LocatedString *input, int start, bool letters_only) {

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
		if (letters_only && !iswlower(prev_char) && !iswupper(prev_char))
			break;
		word = prev_char + word;
		start--;
	}
	return word;
}


bool UrduSentenceBreaker::isSplitChar(const wchar_t c) const {
	return wcschr(SPLIT_PUNCTUATION, c) != NULL;
}

int UrduSentenceBreaker::getIndexAfterClosingPunctuation(const LocatedString *input, int index) {
	const int len = input->length();
	do {
		index++;
	} while ((index < len) && isClosingPunctuation(input->charAt(index)));
	return index;
}

bool UrduSentenceBreaker::isClosingPunctuation(const wchar_t c) const {
	return wcschr(CLOSING_PUNCTUATION, c) != NULL;
}

bool UrduSentenceBreaker::matchFinalPeriod(const LocatedString *input, int index, int origin) {
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
	if (input->charAt(index) != URDU_PERIOD) {
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

int UrduSentenceBreaker::matchEllipsis(const LocatedString *input, int index) {
	const int len = input->length();

	// We have to be at a period to match.
	if ((index >= len) || (input->charAt(index) != URDU_PERIOD)) {
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
		if ((end >= len) || (input->charAt(end) != URDU_PERIOD)) {
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



void UrduSentenceBreaker::attemptLongSentenceBreak(Sentence **results, int max_sentences, Metadata *metadata) {
	Sentence *thisSentence = results[_cur_sent_no];
	int whitespace_count = countWhiteSpace(thisSentence->getString());
	if (whitespace_count <= _max_whitespace) 
		return;

	std::vector<Sentence*> subSentences;
	int next_sent_no = thisSentence->getSentNumber() + static_cast<int>(subSentences.size());
	int sent_start = 0;
	for (int breakpoint = sent_start; breakpoint < thisSentence->getString()->length(); breakpoint++) {
		if (isSecondaryEOSSymbol(thisSentence->getString()->charAt(breakpoint))) {
			if (!DefaultSentenceBreaker::inRestrictedBreakSpan(metadata, thisSentence->getString(), breakpoint)) {
				subSentences.push_back(DefaultSentenceBreaker::createSubSentence(thisSentence, next_sent_no, sent_start, breakpoint-1, "hyphen"));
				sent_start = breakpoint;
				next_sent_no++;
			}
		}
	}
	subSentences.push_back(DefaultSentenceBreaker::createFinalSubSentence(thisSentence, next_sent_no, sent_start));

	// if no sub-sentences returned, keep this sentence as-is
	if (subSentences.empty())  {
		SessionLogger::warn("long_sentence") << "Could not break up long sentence, sentence number " << _cur_sent_no;
		return;
	}
	if (_cur_sent_no + static_cast<int>(subSentences.size()) > max_sentences) {
		throw UnexpectedInputException("UrduSentenceBreaker::attemptLongSentenceBreak()",
									   "Document contains more than max_sentences sentences");
	}
	
	BOOST_FOREACH(Sentence *sent, subSentences) {
		results[_cur_sent_no++] = sent;
	}

	// decrement sent_no to point to last complete sentence
	_cur_sent_no--; 
	
	delete thisSentence;
}

bool UrduSentenceBreaker::isSecondaryEOSSymbol(const wchar_t c) const {
	return wcschr(SECONDARY_EOS_SYMBOL, c) != NULL;
}

bool UrduSentenceBreaker::isCarriageReturn(const wchar_t c) const {
  return wcschr(CARRIAGE_RETURN, c) != NULL;
}

bool UrduSentenceBreaker::matchEmail(const LocatedString *string, int index) {
  int prev_whitespace = 0;
  int i= 0;
  for (i = index; i>=0; i--) {
    if (iswspace(string->charAt(i))) {
      prev_whitespace = i;
      break;
    }
  }
  int at = string->indexOf(L"@", prev_whitespace);
  if (at > 0 && at < index)
    return true;
  return false;
}
