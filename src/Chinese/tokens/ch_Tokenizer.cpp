// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/tokens/ch_Tokenizer.h"
//#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Chinese/common/UnicodeGBTranslator.h"
#include "Chinese/common/ch_StringTransliterator.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/names/IdFSentenceTheory.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Metadata.h"

#define MAX_TOKEN_THEORIES 30

using namespace std;
class Metadata;

Token* ChineseTokenizer::_tokenBuffer[MAX_SENTENCE_TOKENS];
wchar_t ChineseTokenizer::_wchar_buffer[MAX_TOKEN_SIZE+1];
SymbolSubstitutionMap* ChineseTokenizer::_substitutionMap = 0;

ChineseTokenizer::ChineseTokenizer() : _POSClassTags(0), _decoder(0), _POSTheories(0), _sentenceChars(0) {

	_do_tokenization = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_tokenization", true);

	if (_do_tokenization) {
		// read and load IDF model parameters		
		std::string model_prefix = ParamReader::getRequiredParam("tokenizer_params");
		std::string pos_file = ParamReader::getRequiredParam("tokenizer_tags");
		
		_POSClassTags = _new NameClassTags(pos_file.c_str());
		_decoder = _new IdFDecoder(model_prefix.c_str(), _POSClassTags);
		_POSTheories = _new IdFSentenceTheory* [MAX_TOKEN_THEORIES];
		_sentenceChars = _new IdFSentenceTokens();
	}

	// Read and load symbol substitution parameters
	if (_substitutionMap == 0) {
		std::string token_subst_file = ParamReader::getRequiredParam("tokenizer_subst");
		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file.c_str());
	}
}

ChineseTokenizer::~ChineseTokenizer() {
	delete _decoder;
	delete _POSClassTags;
	delete [] _POSTheories;
	delete _sentenceChars;

	if (_substitutionMap != 0) 
		delete _substitutionMap;
}

void ChineseTokenizer::resetForNewSentence(const Document *doc, int sent_no) {
	_document = doc;
	_cur_sent_no = sent_no;
}

int ChineseTokenizer::getTokenTheories(TokenSequence **results,
								int max_theories,
								const LocatedString *string, bool beginOfSentence,
								 bool endOfSentence)
{
	SessionLogger &logger = *SessionLogger::logger;

	// Make a local copy of the sentence string
	LocatedString  localString(*string);

	if (max_theories > MAX_TOKEN_THEORIES)
		max_theories = MAX_TOKEN_THEORIES;

	if (max_theories < 1) {
		SessionLogger::warn("tokenizer") << "ChineseTokenizer::getTokenTheories(), max_theories is less than one.\n";
		return 0;
	}

	if (!beginOfSentence || !endOfSentence) {
		SessionLogger::warn("tokenizer") << "ChineseTokenizer::getTokenTheories(), " 
			   << "ChineseTokenizer does not know how to handle incomplete sentences.\n";
	}

	
	removeHarmfulUnicodes(&localString);

	//correct possibly recursive ampersand markup
	//  (must be before all others)
	int llen;
	do {
		llen = localString.length();
		localString.replace(L"&amp;", L"&");
	}while( llen > localString.length() );

	// replace HTML/XML style escape characters
	localString.replace(L"&quot;", L"\"");
	localString.replace(L"&#34;", L"\"");
	///localString.replace(L"&amp;", L"&");
	localString.replace(L"&#38;", L"\"");
	localString.replace(L"&gt;", L">");
	localString.replace(L"&#62;", L"\"");
	localString.replace(L"&lt;", L"<");
	localString.replace(L"&#60;", L"\"");
	localString.replace(L"&apos;", L"'");
	localString.replace(L"&#39;", L"'");
	localString.replace(L"&#8226;", L"\x2022");
	localString.replace(L"&#160;", L"");

	if (_do_tokenization) {
		removeWhitespace(&localString);
		return getIdFTokenTheories(results, max_theories, localString);
	} else {
		return getCharTokenTheory(results, max_theories, localString);
	}
}

int ChineseTokenizer::getIdFTokenTheories(TokenSequence **results, int max_theories,
                                   const LocatedString& string) 
{
	SessionLogger &logger = *SessionLogger::logger;
	int token_count, begin, end; 
	int begin_list[MAX_SENTENCE_TOKENS];
	int end_list[MAX_SENTENCE_TOKENS];

	int i, j;
	Symbol sym;

	wchar_t chars[2];
	chars[1] = L'\0';

	if (string.length() > MAX_SENTENCE_TOKENS) {
		SessionLogger::warn("too_many_tokens")
			<< "ChineseTokenizer::getIdFTokenTheories(), string is longer than MAX_SENTENCE_TOKENS -- truncating.\n";
	}
	
	for (i = 0; i < string.length() && i < MAX_SENTENCE_TOKENS; i++) {
		chars[0] = string.charAt(i);
		_sentenceChars->setWord(i, Symbol(chars));
		if (isConstraintBeginning(&string, i) || isConstraintEnding(&string, i-1)
			 || isASCIIBeginBreak(&string, i) || isASCIIEndBreak(&string, i-1))
		{
			_sentenceChars->constrainToBreakBefore(i, true);
		}
		else
			_sentenceChars->constrainToBreakBefore(i, false);
	}
	_sentenceChars->setLength(string.length() < MAX_SENTENCE_TOKENS ? string.length() : MAX_SENTENCE_TOKENS);
	int num_theories = 0;
	if (_sentenceChars->getLength() != 0)
		num_theories = _decoder->decodeSentenceNBest(_sentenceChars, _POSTheories, max_theories);
	
	if (num_theories == 0) {
		// should never happen, but...
		_POSTheories[0] = new IdFSentenceTheory(_sentenceChars->getLength(),
			_POSClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}
	
	for (int theorynum = 0; theorynum < num_theories; theorynum++) {
		token_count = _POSClassTags->getNumNamesInTheory(_POSTheories[theorynum]);
	
		j = 0;
		begin_list[0] = 0;
		for (i = 0; i < _sentenceChars->getLength(); i++) { 
			int tag = _POSTheories[theorynum]->getTag(i);
			if (_POSClassTags->isStart(tag)) {
				if (i == 0) continue; // already recorded start of first token
				end_list[j] = i - 1;
				j++;
				begin_list[j] = i;
			}
		} 
		end_list[j] = i - 1;

		for (i = 0; i < token_count; i++ ) {
			begin = begin_list[i];
			end = end_list[i];

			int k;
			for (k = 0; k <= end - begin; k++) {
				if (k >= MAX_TOKEN_SIZE) {
					SessionLogger::warn("tokenizer") << "ChineseTokenizer::getIdFTokenTheories(),"
						   << " Truncating Token longer than MAX_TOKEN_SIZE. (" << end << ")";
					break;
				}
				_wchar_buffer[k] = string.charAt(begin + k);
			}
			_wchar_buffer[k++] = L'\0';

			if (constraintViolation(string.start<EDTOffset>(begin), string.end<EDTOffset>(end))) {
				SessionLogger::warn("tokenizer") << "ChineseTokenizer::getIdFTokenTheories(), "
					   << "Encountered constraint violation between " << begin << " and " << end << " : ignoring.\n";
			}

			//Symbol sym = _substitutionMap->replace(Symbol(_wchar_buffer));

			// substitute for any reserved characters
			wchar_t temp_buffer[MAX_TOKEN_SIZE + 1];
			temp_buffer[0] = '\0';
			int len = static_cast<int>(wcslen(_wchar_buffer));
			int temp_len = 0;
			for (int a = 0; a < len; a++) {
				chars[0] = _wchar_buffer[a];
				Symbol sub = _substitutionMap->replace(Symbol(chars));
				temp_len += static_cast<int>(wcslen(sub.to_string()));
				if (temp_len > MAX_TOKEN_SIZE) {
					SessionLogger::warn("tokenizer") << "ChineseTokenizer::getIdFTokenTheories(),"
						   << " Truncating Token longer than MAX_TOKEN_SIZE.";
					break;
				}
				wcsncat(temp_buffer, sub.to_string(), 6);
			}

			Symbol sym = Symbol(temp_buffer);

			_tokenBuffer[i] = _new Token(&string, begin, end, sym);

		}

		TokenSequence *pTokenSequence = _new TokenSequence(_cur_sent_no, token_count, _tokenBuffer);
		pTokenSequence->setScore(
			(float) _POSTheories[theorynum]->getBestPossibleScore());
		
		results[theorynum] = pTokenSequence;

	}
	for (int theory_num = 0; theory_num < num_theories; theory_num++) 
		delete _POSTheories[theory_num];	
	
	return num_theories;
}

int ChineseTokenizer::getCharTokenTheory(TokenSequence **results, int max_theories,
                                  const LocatedString& string) 
{
	SessionLogger &logger = *SessionLogger::logger;
	wchar_t chars[2];
	chars[1] = L'\0';

	int i;
	int token_count = 0;
	for (i = 0; i < string.length() && token_count < MAX_SENTENCE_TOKENS; i++) {
		if (iswascii(string.charAt(i))) { // group together any ascii chars that aren't separated by whitespace
			while (i < string.length() && iswspace(string.charAt(i))) { i++; }
			int start = i;
			while (i + 1 < string.length() && iswascii(string.charAt(i+1)) && !iswspace(string.charAt(i+1))) { i++; }
			Symbol sub = _substitutionMap->replace(Symbol(string.substringAsWString(start, i+1)));
			_tokenBuffer[token_count++] = _new Token(&string, start, i, sub);
		} else {
			chars[0] = string.charAt(i);
			Symbol sub = _substitutionMap->replace(Symbol(chars));
			_tokenBuffer[token_count++] = _new Token(&string, i, i, sub);
		}
	}
	if (i < string.length()) {
		SessionLogger::warn("tokenizer") << "ChineseTokenizer::getCharTokenTheory(), number of tokens found is greater than MAX_SENTENCE_TOKENS -- truncating.\n";
	}
	results[0] = _new TokenSequence(_cur_sent_no, token_count, _tokenBuffer);
	return 1;
}

void ChineseTokenizer::removeWhitespace(LocatedString *text) {
	int i = 0;
	while (i < text->length()) {
		if (iswspace(text->charAt(i)))
			text->remove(i, i + 1);
		else
			i++;
	}

}

// Note: this only looks at the smallest span starting/ending at
// a given offset. To do this correctly, need to look at every span.
bool ChineseTokenizer::constraintViolation(EDTOffset begin, EDTOffset end) {
	Metadata *metadata = _document->getMetadata();
	Span *span;
	
	// No spans exist, so skip check
	if (metadata->get_span_count() == 0)
		return false;

	// Is there a span ending in middle of token? 
	for (EDTOffset i = begin; i < end; ++i) {	 
		span = metadata->getEndingSpan(i);
		if ((span != NULL) && span->enforceTokenBreak()) {
			return true;
		}
	}
	// Is there a span starting in middle of token?
	for (EDTOffset j = EDTOffset(begin.value() + 1); j <= end; ++j) { 
		span = metadata->getStartingSpan(j);
		if ((span != NULL) && span->enforceTokenBreak()) {
			return true;
		}
	}
	return false;
}

bool ChineseTokenizer::isConstraintBeginning(const LocatedString* string, int index) {

	if (_document->getMetadata()->get_span_count() == 0)
		return false;

	EDTOffset this_edt = string->firstStartOffsetStartingAt<EDTOffset>(index);
	// first EDT offset after the last token
	EDTOffset last_edt = (index > 0) ? EDTOffset(string->lastEndOffsetEndingAt<EDTOffset>(index-1).value() + 1): this_edt;
    
	
 	bool result = false;
	Metadata::SpanList *list = _document->getMetadata()->getCoveringSpans(this_edt);
	for (int i = 0; i < list->length(); i++) {
		Span *span = (*list)[i];
		if ((span != NULL) && 
			(span->enforceTokenBreak()) &&
			(span->getStartOffset() >= last_edt)) 
		{
			result = true;
			break;
		}
	}
	delete list;
	return result;
}

bool ChineseTokenizer::isConstraintEnding(const LocatedString* string, int index) {
	if (_document->getMetadata()->get_span_count() == 0 || index < 0)
		return false;

	EDTOffset this_edt = string->lastEndOffsetEndingAt<EDTOffset>(index);
	// last EDT offset before next token
	EDTOffset next_edt = (index < string->length() - 1) ? EDTOffset(string->firstStartOffsetStartingAt<EDTOffset>(index+1).value() - 1): this_edt;
    
 	bool result = false;
	Metadata::SpanList *list = _document->getMetadata()->getCoveringSpans(this_edt);
	for (int i = 0; i < list->length(); i++) {
		Span *span = (*list)[i];
		if ((span != NULL) && 
			(span->enforceTokenBreak()) &&
			(span->getEndOffset() <= next_edt)) 
		{
			result = true;
			break;
		}
	}
	delete list;
	return result;

}

bool ChineseTokenizer::isASCIIBeginBreak(const LocatedString* string, int index) {
	if (index > 0 && iswascii(string->charAt(index)) && !iswascii(string->charAt(index-1)))
		return true;
	else if (index > 0 && iswascii(string->charAt(index)) &&
			 iswpunct(string->charAt(index)) && !iswpunct(string->charAt(index-1)))
	{	
		// Don't break up numbers like 16.9
		if (index < string->length()-1 && string->charAt(index) == L'.' && 
			iswdigit(string->charAt(index-1)) && iswdigit(string->charAt(index+1)))
		{
			return false;
		}
		return true;
	}
	return false;
}

bool ChineseTokenizer::isASCIIEndBreak(const LocatedString* string, int index) {
	if (index >= 0 && index < string->length()-1 && 
		iswascii(string->charAt(index)) && !iswascii(string->charAt(index+1))) 
	{
		if (iswdigit(string->charAt(index)) && 
			(isChineseNumberChar(string->charAt(index+1)) || 
			 isChineseTimeChar(string->charAt(index+1))))
		{ 
			return false;
		}
		return true;
	}
	else if (index >= 0 && index < string->length()-1 && iswascii(string->charAt(index)) &&
		iswpunct(string->charAt(index)) && !iswpunct(string->charAt(index+1)))
	{
		// Don't break up numbers like 16.9
		if (index > 0 && string->charAt(index) == L'.' && 
			iswdigit(string->charAt(index-1)) && iswdigit(string->charAt(index+1)))
		{
			return false;
		}	
		return true;
	}
	return false;
}

// borrowed from ch_IdFWordFeatures
bool ChineseTokenizer::isChineseNumberChar(wchar_t c_char) const {
	if((c_char == 0xff10) ||
       (c_char == 0xff11) ||
       (c_char == 0xff12) ||
       (c_char == 0xff13) ||
       (c_char == 0xff14) ||
       (c_char == 0xff15) ||
       (c_char == 0xff16) ||
       (c_char == 0xff17) ||
       (c_char == 0xff18) ||
       (c_char == 0xff19) ||
       (c_char == 0x4ebf) ||
       (c_char == 0x4e07) ||
       (c_char == 0x5343) ||
       (c_char == 0x767e) ||
       (c_char == 0x5341) ||
       (c_char == 0x4e00) ||
       (c_char == 0x4e8c) ||
       (c_char == 0x4e09) ||
       (c_char == 0x56db) ||
       (c_char == 0x4e94) ||
       (c_char == 0x516d) ||
       (c_char == 0x4e03) ||
       (c_char == 0x516b) ||
       (c_char == 0x4e5d))
		return true;
	return false;

}

bool ChineseTokenizer::isChineseTimeChar(wchar_t c_char) const {
	if((c_char == 0x5e74) ||		// year
       (c_char == 0x6708) ||		// month
       (c_char == 0x65e5))			// day
		return true;
	return false;
}

Symbol ChineseTokenizer::getSubstitutionSymbol(Symbol sym) {
	// Read and load symbol substitution parameters, if needed
	if (_substitutionMap == 0) {
		std::string token_subst_file = ParamReader::getRequiredParam("tokenizer_subst");
		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file.c_str());
	}

	return _substitutionMap->replace(sym);
}
