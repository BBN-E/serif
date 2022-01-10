// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/sentences/ch_SentenceBreaker.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Metadata.h"
#include <wchar.h>
#include <boost/foreach.hpp> 
#include "math.h"

class Metadata;

const wchar_t CH_SPACE = 0x3000;			// \343\200\200
const wchar_t CH_COMMA = 0x3001;
const wchar_t NEW_LINE = 0x000A;
const wchar_t CARRIAGE_RETURN = 0x000D;
const wchar_t EOS_MARK = 0x3002;			// \343\200\202
const wchar_t LATIN_STOP = 0x002E;			
const wchar_t LATIN_EXCLAMATION = 0x0021;   
const wchar_t LATIN_QUESTION = 0x003F;		
const wchar_t FULL_STOP = 0xFF0E;			// \357\274\216
const wchar_t FULL_COMMA = 0xFF0C;
const wchar_t FULL_EXCLAMATION = 0xFF01;	// \357\274\201
const wchar_t FULL_QUESTION = 0xFF1F;		// \357\274\237
const wchar_t FULL_SEMICOLON = 0xFF1B;

const wchar_t FULL_RRB = 0xFF09;		    // \357\274\211

const wchar_t CLOSE_PUNCT_1 = 0x3009;		// \343\200\211
const wchar_t CLOSE_PUNCT_2 = 0x300b;		// \343\200\213
const wchar_t CLOSE_PUNCT_3 = 0x300d;		// \343\200\215
const wchar_t CLOSE_PUNCT_4 = 0x300f;       // \343\200\217
const wchar_t CLOSE_PUNCT_5 = 0x201d;		// \342\200\235

ChineseSentenceBreaker::ChineseSentenceBreaker() {
	_max_desirable_sentence_len = ParamReader::getOptionalIntParamWithDefaultValue("max_desirable_sentence_len", 108);
}

void ChineseSentenceBreaker::resetForNewDocument(const Document *doc) {
	_curDoc = doc;
	_curRegion = 0;
	_cur_sent_no = 0;
	_metadata = _curDoc->getMetadata();
}

int ChineseSentenceBreaker::getSentencesRaw(Sentence **results, int max_sentences, 
									 const Region* const* regions,
									 int num_regions)
{
	if (num_regions < 1) {
		SessionLogger::warn_user("no_document_regions") << "No readable content in this document.";
		return 0;
	}

	// insert get dateline sentence here

	for (int i = 0; i < num_regions; i++) {
		int j = 0;
		_curRegion = regions[i];
		while ( j < _curRegion->getString()->length()) { 
			if (_cur_sent_no == max_sentences) {
				SessionLogger::warn_user("too_many_sentences") << "ChineseSentenceBreaker::getSentences(),"
										 << " Truncating Document with more than max_sentences sentences"; 
				break;
			}
			j += getNextSentence(results, j, _curRegion->getString());
			if (results[_cur_sent_no]) {
				if (results[_cur_sent_no]->getNChars() > _max_desirable_sentence_len) 
					attemptLongSentenceBreak(results, max_sentences);
				_cur_sent_no++;
			}
		}
	}
	return _cur_sent_no;
}

int ChineseSentenceBreaker::getNextSentence(Sentence **results, int offset, const LocatedString *input) {
	int start,i = 0;
	results[_cur_sent_no] = 0; // initialize the result to null

	// skip whitespace
	while ((offset + i) < input->length() && iswspace(input->charAt(offset+i))) {
		i++;
	}

	if (offset + i < input->length()) {
		start = offset + i;
		while (true) {
          
			wchar_t c = input->charAt(offset + i);
		
			// check for eos 
			if (isEOSChar(c) ||
			   ((c == FULL_RRB || c == L')') && 
			   (_cur_sent_no == 0 || (_cur_sent_no > 0 && results[_cur_sent_no-1]->getRegion()->getRegionTag() == L"HEADLINE"))))  // break after nwire opening
			{
				// check to see if we're in a restricted span
				if (offset + i + 1 < input->length() && !inRestrictedBreakSpan(_metadata, input, offset+i+1)) {		 
					// move past closing puncuation 
					while ((offset + i + 1 < input->length()) && 
       					   (isClosingPunctuation(input->charAt(offset+i+1))))
					{
						i++;
						c = input->charAt(offset + i);
					}
					i++;
					break;
				}
			}
			// check for double newlines:
			// break on '\n\n' and '\r\n\r\n' but NOT '\r\n'.
			if (isEOLChar(c))  {
				// '\n\n'
				if (c == NEW_LINE && offset + i + 2 < input->length()) {
					c = input->charAt(offset + i + 1);
					if (c == NEW_LINE && !inRestrictedBreakSpan(_metadata, input, offset+i+2)) {
						i += 2;
						break;
					}
				}
				// '\r\n\r\n'
				else if (c == CARRIAGE_RETURN && offset + i + 4 < input->length()) {
					if (input->charAt(offset + i + 1) == NEW_LINE &&
						input->charAt(offset + i + 2) == CARRIAGE_RETURN &&
						input->charAt(offset + i + 3) == NEW_LINE &&
						!inRestrictedBreakSpan(_metadata, input, offset+i+4))
					{
						i += 4;
						break;
					}
				}
			}
			// check for end of input
			if ((offset + i) >= input->length() - 1) {
				i++;
				break;
			}

			i++;
		}

		// Optionally trim off any whitespace at the end of the sentence
		int pretrim_i = i;
		while ((offset + i > start) && (offset + i > input->length() || iswspace(input->charAt(offset + i - 1)))) {
			i--;
		}

		// Create the value of the sentence output parameter with the substring.
		LocatedString *sub;
		if (offset + i == start) {
			// Use the pre-trim sentence end
			sub = input->substring(start, offset + pretrim_i);
		} else {
			// Use the trimmed sentence end
			sub = input->substring(start, offset + i);
			i = pretrim_i;
		}

		results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
		delete sub;

	}

	return i;
}

void ChineseSentenceBreaker::attemptLongSentenceBreak(Sentence **results, int max_sentences) {

	Sentence* thisSentence = results[_cur_sent_no];
	int orig_sent_length = thisSentence->getString()->length();
	int num_split_sents = static_cast <int>(ceil((double (orig_sent_length))/_max_desirable_sentence_len));

	std::vector<Sentence*> subSentences = DefaultSentenceBreaker::attemptLongSentenceBreak(thisSentence, num_split_sents);

	// if no sub-sentences returned, keep this sentence as is
	if (subSentences.empty()) 
		return;

	if (_cur_sent_no + static_cast<int>(subSentences.size()) > max_sentences) {
		throw UnexpectedInputException("ChineseSentenceBreaker::attemptLongSentenceBreak()",
									   "Document contains more than max_sentences sentences");
	}
	
	BOOST_FOREACH(Sentence *sent, subSentences) {
		results[_cur_sent_no++] = sent;
	}

	// decrement sent_no to point to last complete sentence
	_cur_sent_no--;
	
	delete thisSentence;

}

// Returns true if substring [start, end) is an acceptable sub-sentence for
// use in attemptLongSentenceBreak
bool ChineseSentenceBreaker::isAcceptableSentenceString(const LocatedString *sentence, int start, int end) { 
	return (end - start < _max_desirable_sentence_len); 
}

bool ChineseSentenceBreaker::isEOSChar(const wchar_t ch) const {
	if (ch == EOS_MARK || 
		ch == CH_SPACE ||
		ch == LATIN_EXCLAMATION ||
		ch == LATIN_QUESTION ||
//      ch == LATIN_STOP ||
//		ch == FULL_STOP ||
		ch == FULL_EXCLAMATION ||
		ch == FULL_QUESTION)
		return true;
	return false;
}

bool ChineseSentenceBreaker::isSecondaryEOSChar(const wchar_t ch) const {
	if (ch == CH_COMMA ||
		ch == FULL_COMMA || 
		ch == FULL_SEMICOLON)
		return true;
	return false;
}

bool ChineseSentenceBreaker::isEOLChar(const wchar_t ch) const {
	if (ch == NEW_LINE || ch == CARRIAGE_RETURN)
		return true;
	return false;
}

bool ChineseSentenceBreaker::isClosingPunctuation(const wchar_t ch) const {
	if (ch == CLOSE_PUNCT_1 ||
		ch == CLOSE_PUNCT_2 ||
		ch == CLOSE_PUNCT_3 ||
		ch == CLOSE_PUNCT_4 ||
		ch == CLOSE_PUNCT_5)
		return true;
	return false;
}
