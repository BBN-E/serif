// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Korean/sentences/kr_SentenceBreaker.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include <wchar.h>
#include "math.h"

class Metadata;

#define MAX_ABBREV_WORD_LEN      3

const wchar_t NEW_LINE = 0x000A;
const wchar_t CARRIAGE_RETURN = 0x000D;
const wchar_t LATIN_STOP = 0x002E;			
const wchar_t LATIN_EXCLAMATION = 0x0021;   
const wchar_t LATIN_QUESTION = 0x003F;		
const wchar_t FULL_STOP = 0xFF0E;			// \357\274\216
const wchar_t FULL_COMMA = 0xFF0C;
const wchar_t FULL_EXCLAMATION = 0xFF01;	// \357\274\201
const wchar_t FULL_QUESTION = 0xFF1F;		// \357\274\237
const wchar_t FULL_SEMICOLON = 0xFF1B;
const wchar_t LRB = 0x0028;
const wchar_t RRB = 0x0029;
const wchar_t EMAIL_AT = 0x0040;

KoreanSentenceBreaker::KoreanSentenceBreaker() {}

void KoreanSentenceBreaker::resetForNewDocument(class Document *doc) {
	_curDoc = doc;
	_curRegion = 0;
	_cur_sent_no = 0;
	_metadata = _curDoc->getMetadata();
}

int KoreanSentenceBreaker::getSentencesRaw(Sentence **results, int max_sentences, 
									 Region** const regions,
									 int num_regions)
{
	if (num_regions < 1) {
		SessionLogger::logger->beginWarning();
		(*SessionLogger::logger) << "No regions in document. Thus, no sentences.";
		return 0;
	}

	// insert get dateline sentence here

	for (int i = 0; i < num_regions; i++) {
		int j = 0;
		_curRegion = regions[i];
		while ( j < _curRegion->getString()->length()) { 
			if (_cur_sent_no == max_sentences) {
				SessionLogger::logger->beginWarning();
				(*SessionLogger::logger) << "KoreanSentenceBreaker::getSentences(),"
										 << " Truncating Document with more than max_sentences sentences"; 
				break;
			}
			j += getNextSentence(results, j, _curRegion->getString());
		}
	}
	return _cur_sent_no;
}

int KoreanSentenceBreaker::getNextSentence(Sentence **results, int offset, const LocatedString *input) {
	int start,i = 0;
	bool in_paren = false;
	bool in_email = false;

	// skip whitespace
	while ((offset + i) < input->length() && iswspace(input->charAt(offset+i))) {
		i++;
	}

	if (offset + i < input->length()) {
		start = offset + i;
		while (true) {
          
			wchar_t c = input->charAt(offset + i);

			// check for eos 
			if (isEOSChar(c) || matchFinalPeriod(input, offset+i, start))  { 
				// check to see if we're in a restricted span
				if (!inRestrictedBreakSpan(offset+i, input) && !in_paren && !in_email) {
					// If the sentence ended with an ellipsis, skip to the end of it.
					int ellipsis_len = matchEllipsis(input, offset+i);
					if (ellipsis_len > 0) {
						i += ellipsis_len - 1;
					}
					// move past closing puncuation 
					while ((offset + i + 1 < input->length()) && 
       					   (isClosingPunctuation(input->charAt(offset+i+1))))
					{
						i++;
					}
					i++;
					break;
				}
			}
			// check for double newlines:
			// break on '\n\n' and '\r\n\r\n' but NOT '\r\n'.
			if (isEOLChar(c))  {
				// '\n\n'
				if (c == NEW_LINE && offset + i + 1 < input->length()) {
					c = input->charAt(offset + i + 1);
					if (c == NEW_LINE && !inRestrictedBreakSpan(offset+i+1, input)) {
						i += 2;
						break;
					}
				}
				// '\r\n\r\n'
				else if (c == CARRIAGE_RETURN && offset + i + 3 < input->length()) {
					if (input->charAt(offset + i + 1) == NEW_LINE &&
						input->charAt(offset + i + 2) == CARRIAGE_RETURN &&
						input->charAt(offset + i + 3) == NEW_LINE &&
						!inRestrictedBreakSpan(offset+i+3, input))
					{
						i += 4;
						break;
					}
				}
			}
			
			// Hacks to make sure we don't break inside parens or an email address
			if (c == LRB) {
				in_paren = true;
			}
			else if (c == RRB) {
				in_paren = false;
			}
			if (c == EMAIL_AT) {
				in_email = true;
			}
			else if (in_email && !iswascii(c)) {
				in_email = false;
			}

			// check for end of input
			if ((offset + i) >= input->length() - 1) {
				i++;
				break;
			}

			i++;
		}
		LocatedString *sub = input->substring(start, offset+i);
		results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
		delete sub;

		if (results[_cur_sent_no] && results[_cur_sent_no]->getNChars() > getMaxSentenceChars()) 
			attemptSentenceBreak(results);
		else
			_cur_sent_no++;
	}

	return i;
}

void KoreanSentenceBreaker::attemptSentenceBreak(Sentence **results) {

	Sentence* thisSentence = results[_cur_sent_no];
	int origSentLength = thisSentence->getString()->length();
	int numSplitSent = static_cast <int>(ceil((double (origSentLength))/getMaxSentenceChars()));
	int avgNumChars = origSentLength/numSplitSent;
	int currChar = avgNumChars;

	SessionLogger::logger->beginWarning();
	(*SessionLogger::logger) << "KoreanSentenceBreaker::attemptLongSentenceBreak(),"
							 << " Attempting to break sentence with " << origSentLength
							 << " characters into smaller sentences with " << avgNumChars
							 << " characters on average.\n";
	int start = 0;
	
	while ((currChar < origSentLength) && (_cur_sent_no < MAX_DOCUMENT_SENTENCES)) {

		int offset = 0;
		wchar_t c_minus;
		wchar_t c_plus;

		bool found_break = false;

		/* First, lets look to see if there's a comma close by */
		while (!found_break && (offset < 100) && 
			   (currChar + offset < origSentLength) && 
			   (currChar + offset - start + 1 < getMaxSentenceChars()))
		{
			c_minus = thisSentence->getString()->charAt(currChar - offset);
			c_plus = thisSentence->getString()->charAt(currChar + offset);
			
			if (isSecondaryEOSChar(c_minus) && 
				!inRestrictedBreakSpan(currChar - offset, thisSentence->getString())) 
			{
				(*SessionLogger::logger) << "---Substring from: " << start << " to " << currChar - offset << " [punctuation]\n";
				LocatedString* shortString = thisSentence->getString()->substring(start, currChar-offset+1);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
				_cur_sent_no++;
				start = currChar - offset + 1;
				delete shortString;

				// If there are no more subsequences to be found,
				// put the remaining text in a subsentence
				if ((start < origSentLength) && (origSentLength - start < getMaxSentenceChars())) {
					shortString = thisSentence->getString()->substring(start);
					(*SessionLogger::logger) << "Final Substring from: " << start << " to end\n";
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
					_cur_sent_no++;
					currChar = origSentLength + 1;
					start = origSentLength + 1;
					delete shortString;
				} else {
					currChar += (avgNumChars - offset);
				}
				found_break = true;
				break;
			}
			else if (isSecondaryEOSChar(c_plus) &&
					 !inRestrictedBreakSpan(currChar + offset, thisSentence->getString()) )
			{
				(*SessionLogger::logger) << "---Substring from: " << start << " to " << currChar + offset << " [punctuation]\n";
				LocatedString* shortString = thisSentence->getString()->substring(start, currChar+offset+1);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
				_cur_sent_no++;
				start = currChar + offset + 1;
				delete shortString;

				// If there are no more subsequences to be found,
				// put the remaining text in a subsentence
				if ((start < origSentLength) && (origSentLength - start < getMaxSentenceChars())) {
					shortString = thisSentence->getString()->substring(start);
					(*SessionLogger::logger) << "Final Substring from: " << start << " to end\n";
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
					_cur_sent_no++;
					currChar = origSentLength + 1;
					start = origSentLength + 1;
					delete shortString;
				}
				else {
					currChar += (avgNumChars + offset);
				}
				found_break = true;
				break;
			}
			else {
				offset++;
			}
		}

		/* No comma, so just break at a white space point */
		offset = 0;
		while (!found_break && 
			   (currChar + offset < origSentLength) && 
			   (currChar - offset > start) &&
			   (currChar + offset - start + 1 < getMaxSentenceChars()))
		{
			c_minus = thisSentence->getString()->charAt(currChar - offset);
			c_plus = thisSentence->getString()->charAt(currChar + offset);
			
			if (iswspace(c_minus) &&
				!inRestrictedBreakSpan(currChar - offset, thisSentence->getString())) 
			{
				(*SessionLogger::logger) << "---Substring from: " << start << " to " << currChar - offset << " [whitespace]\n";
				LocatedString* shortString = thisSentence->getString()->substring(start, currChar-offset+1);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
				_cur_sent_no++;
				start = currChar - offset + 1;
				delete shortString;

				// If there are no more subsequences to be found,
				// put the remaining text in a subsentence
				if ((start < origSentLength) && (origSentLength - start < getMaxSentenceChars())) {
					shortString = thisSentence->getString()->substring(start);
					(*SessionLogger::logger) << "Final Substring from: " << start << " to end\n";
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
					_cur_sent_no++;
					currChar = origSentLength + 1;
					start = origSentLength + 1;
					delete shortString;
				}
				else {
					currChar += (avgNumChars - offset);
				}
				found_break = true;
				break;
			}
			else if (iswspace(c_plus) &&
					!inRestrictedBreakSpan(currChar + offset, thisSentence->getString())) 
			{
				(*SessionLogger::logger) << "---Substring from: " << start << " to " << currChar + offset << " [whitespace]\n";
				LocatedString* shortString = thisSentence->getString()->substring(start, currChar+offset+1);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
				_cur_sent_no++;
				start = currChar + offset + 1;
				delete shortString;

				// If there are no more subsequences to be found,
				// put the remaining text in a subsentence
				if ((start < origSentLength) && (origSentLength - start < getMaxSentenceChars())) {
					shortString = thisSentence->getString()->substring(start);
					(*SessionLogger::logger) << "Final Substring from: " << start << " to end\n";
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
					_cur_sent_no++;
					currChar = origSentLength + 1;
					start = origSentLength + 1;
					delete shortString;
				}
				else {
					currChar += (avgNumChars + offset);
				}
				found_break = true;
				break;
			}
			else {
				offset++;
			}
		}

		/* Can't find a good break, so find the first non-restricted breakpoint near currChar */
		offset = 0;
		if (!found_break) {
			SessionLogger::logger->beginWarning();
			(*SessionLogger::logger) << "KoreanSentenceBreaker::attemptLongSentenceBreak(),"
									 << " Could not find good break point, breaking at random.";
			
			while ((currChar + offset < origSentLength) &&
				   (currChar + offset - start + 1 < getMaxSentenceChars()) &&
				   (inRestrictedBreakSpan(currChar + offset, thisSentence->getString())))
			{
				offset++;
			}

			
			if ((currChar + offset < origSentLength) &&
				(currChar + offset - start + 1 < getMaxSentenceChars())) 
			{
				(*SessionLogger::logger) << "---Substring from: " << start << " to " << currChar + offset << " [Random]\n";
				LocatedString* shortString = thisSentence->getString()->substring(start, currChar+offset+1);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
				_cur_sent_no++;
			
				start = currChar + offset + 1;
				delete shortString;

				// If there are no more subsequences to be found,
				// put the remaining text in a subsentence
				if ((start < origSentLength) && (origSentLength - start < getMaxSentenceChars())) {
					shortString = thisSentence->getString()->substring(start);
					(*SessionLogger::logger) << "Final Substring from: " << start << " to end\n";
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
					_cur_sent_no++;
					currChar = origSentLength + 1;
					start = origSentLength + 1;
					delete shortString;
				}
				else {
					currChar += (avgNumChars + offset);
				}
			}
			// can't find a good break point after currChar, so look before instead.
			else {
				offset = 0;
				while ((currChar - offset > start) &&
      				   (inRestrictedBreakSpan(currChar - offset, thisSentence->getString())))
				{
					offset++;
				}
				if (currChar - offset > start) {
					(*SessionLogger::logger) << "---Substring from: " << start << " to " << currChar + offset << " [Random]\n";
					LocatedString* shortString = thisSentence->getString()->substring(start, currChar-offset+1);
					results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
					_cur_sent_no++;
				
					start = currChar - offset + 1;
					delete shortString;

					// If there are no more subsequences to be found,
					// put the remaining text in a subsentence
					if ((start < origSentLength) && (origSentLength - start < getMaxSentenceChars())) {
						shortString = thisSentence->getString()->substring(start);
						(*SessionLogger::logger) << "Final Substring from: " << start << " to end\n";
						results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortString);
						_cur_sent_no++;
						currChar = origSentLength + 1;
						start = origSentLength + 1;
						delete shortString;
					}
					else {
						currChar += (avgNumChars - offset);
					}
				}
				// can't find any valid breakpoints within max_sentence_chars range
				else {
					throw InternalInconsistencyException("KoreanSentenceBreaker::attemptSentenceBreak()",
						"Could not find an unrestricted sentence break point.");
				}
			}
		}
	}

	delete thisSentence;
}

bool KoreanSentenceBreaker::inRestrictedBreakSpan(int index, const LocatedString *input) const {
	bool restrict_break = false;
	if (_metadata->get_span_count() > 0) {
		Metadata::SpanList *list = _metadata->getCoveringSpans(input->lastEdtOffsetEndingAt(index));
		for (int j = 0; j < list->length(); j++) {
			Span *coveringSpan = (*list)[j];
			if (coveringSpan->restrictSentenceBreak() && 
				input->lastEdtOffsetEndingAt(index) != input->lastEdtOffsetEndingAt(input->length()-1) &&
				coveringSpan->covers(input->firstEdtOffsetStartingAt(index+1)))
			{
				restrict_break = true;
				break;
			}
		}
		delete list;
	}
	return restrict_break;
}

bool KoreanSentenceBreaker::isEOSChar(const wchar_t ch) const {
	if (ch == LATIN_EXCLAMATION ||
		ch == LATIN_QUESTION ||
//	    ch == LATIN_STOP ||
		ch == FULL_STOP ||
		ch == FULL_EXCLAMATION ||
		ch == FULL_QUESTION)
		return true;
	return false;
}

bool KoreanSentenceBreaker::isSecondaryEOSChar(const wchar_t ch) const {
	if (ch == FULL_COMMA || 
		ch == FULL_SEMICOLON)
		return true;
	return false;
}

bool KoreanSentenceBreaker::isEOLChar(const wchar_t ch) const {
	if (ch == NEW_LINE || ch == CARRIAGE_RETURN)
		return true;
	return false;
}

bool KoreanSentenceBreaker::isClosingPunctuation(const wchar_t ch) const {
	//if ()
	//	return true;
	return false;
}

bool KoreanSentenceBreaker::matchFinalPeriod(const LocatedString *input, 
									   int index, int origin) const 
{
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
	if (ellipsis_len > 0 && ellipsis_len <= 3) {
		// should we consider more than three periods an ellipsis + a period?
		return false;
	}

	if (matchDecimalPoint(input, index, origin))
		return false;

	Symbol finalWord = getWordEndingAt(index, input, origin);

	// If the punctuation (i.e., the period or the period and subsequent closing punctuation
	// marks) is followed by non-whitespace, then it's not the end of a sentence.
	int break_pos = getIndexAfterClosingPunctuation(input, index);
	if (break_pos >= len) {
		return true;
	}
	else if (!iswspace(input->charAt(break_pos))) {
		return false;
	}

	// If the final word is likely to be an abbreviation, then this is probably
	// not the end of a sentence.
	if (matchLikelyAbbreviation(finalWord))
	{
		return false;
	}

	// If we survived all the possible negative criteria, assume it's a final period.
	return true;
}

bool KoreanSentenceBreaker::matchPossibleFinalPeriod(const LocatedString *input, 
											   int index, int origin) const 
{
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
	if ((pos < len) && !iswspace(input->charAt(pos))) {
		return false;
	}

	// Otherwise, it's possible.
	return true;
}

/**
 * @param input the input string.
 * @param index the index at which to search.
 * @return the length of the ellipsis substring, or <code>0</code>
 *         if an ellipsis was not matched.
 */
int KoreanSentenceBreaker::matchEllipsis(const LocatedString *input, int index) const {
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

int KoreanSentenceBreaker::getIndexAfterClosingPunctuation(const LocatedString *input, int index) const {
	const int len = input->length();
	do {
		index++;
	} while ((index < len) && isClosingPunctuation(input->charAt(index)));
	return index;
}

bool KoreanSentenceBreaker::matchLikelyAbbreviation(Symbol word) const {
	const wchar_t *s = word.to_string();
	const int len = (int)wcslen(s);

	// It must either begin with an uppercase letter or be
	// a potential abbreviated word.
	if (!iswupper(s[0])) 
		return false;

	Symbol withoutPeriod = Symbol(s, 0, len - 1);


	// If there is more than one period in the word, then it's probably an
	// abbreviation such as K.G.B. or F.B.I. 
	if (wcsstr(withoutPeriod.to_string(), L".") != NULL) {
		return true;
	}

	// Otherwise, assume anything over the maximum length isn't an abreviation.
	return (len - 1) <= MAX_ABBREV_WORD_LEN;
}

Symbol KoreanSentenceBreaker::getWordEndingAt(int index, const LocatedString *input, int origin) const {
	int end = index + 1;
	int start = index - 1;

	// [wordchar or ']* -- so that for "doesn't", we get "doesn't" rather than "t",
	// even though isWordChar(') = false
	// also, don't allow -
	while (start >= origin) {
		if (iswspace(input->charAt(start))) {
			break;
		} else start--;
	}

	LocatedString *sub = input->substring(start + 1, end);
	Symbol ret = sub->toSymbol();
	delete sub;

	return ret;
}

bool KoreanSentenceBreaker::matchDecimalPoint(const LocatedString *input, int index, int origin) const {
	int end = index + 1;
	int start = index - 1;

	while (start >= origin) {
		if (!iswspace(input->charAt(start))) {
			break;
		} else start--;
	}

	if (start < origin || !iswdigit(input->charAt(start)))
		return false;

	while (end < input->length()) {
		if (!iswspace(input->charAt(end))) {
			break;
		} else end++;
	}

	if (end == input->length() || !iswdigit(input->charAt(end)))
		return false;

	return true;
}
