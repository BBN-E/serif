// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/tokens/en_Tokenizer.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/parse/TokenTagTable.h"
#include "Generic/tokens/SymbolList.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Metadata.h"

#include <wchar.h>

#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
using namespace std;

Token* EnglishTokenizer::_tokenBuffer[MAX_SENTENCE_TOKENS+1];
Token* EnglishTokenizer::_tempTokenBuffer[MAX_SENTENCE_TOKENS+1];
wchar_t EnglishTokenizer::_char_buffer[MAX_TOKEN_SIZE+1];

#define SPLIT_PUNCTUATION        L"[](){},\"'`?!:;%|"

/// this  is US dollar, cent, GB pound, currency, Yen, franc, afghani,
static wchar_t PREFIX_CURRENCY_SYMBOLS[] = 
	{L'$',0xa2,0xa3,0xa4,0xa5,0x192,0x60b,
///  bengali rupee, bengali rupee, gujarati rupee, tamil rupee, baht, riel,
	  0x9f2,0x9f3,0xaf1,0xbf9,0xe3f,0x17db,
///  script-big-m, cjk?, cjk?, cjk?, cjk?, rial
      0x2133,0x5143,0x5186,0x5706,0x5713,0xfdfc,0};
/// range is correct as of Unicode v5.1 2008
static wchar_t LOW_UNICODE_CURRENCY_INDEX = 0x20a0;
static wchar_t HIGH_UNICODE_CURRENCY_INDEX = 0x20b5;

static Symbol TELEPHONE_SOURCE_SYM = Symbol(L"telephone");
static Symbol BROADCAST_CONV_SOURCE_SYM = Symbol(L"broadcast conversation");
static Symbol BLOG_SOURCE_SYM = Symbol(L"blog");
static Symbol WEBLOG_SOURCE_SYM = Symbol(L"weblog");
static Symbol USENET_SOURCE_SYM = Symbol(L"usenet");

static Symbol PAREN_URL_PREFIX = Symbol(L"http://bbn.com/serif/names#");
static Symbol SINGLE_PAREN = Symbol(L"SingleParen");
static Symbol DOUBLE_PAREN = Symbol(L"DoubleParen");

// IMPORTANT NOTE:
// Keep in mind that the LocatedString class uses half-open
// intervals [start, end) while the Metadata and Span classes
// use closed intervals [start, end]. This is an easy source
// of off-by-one errors.

EnglishTokenizer::EnglishTokenizer() {
	_substitutionMap = NULL;
	_markupSubstitutionMap = NULL;
	_forceReplacements = NULL;
	_splitEndings = NULL;
	_hyphenatedTokens = NULL;
	_max_paren_markup_string_size = 0;
	
	std::string file_name = ParamReader::getRequiredParam("tokenizer_subst");
	_substitutionMap = _new SymbolSubstitutionMap(file_name.c_str());

	file_name = ParamReader::getParam("paren_markup_subst");
	if (!file_name.empty()) {
  		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
  		UTF8InputStream& in(*in_scoped_ptr);
		in.open(file_name.c_str());
		_markupSubstitutionMap = _new TokenTagTable(in);
		in.close();
		_use_paren_markups = true;
	    _max_paren_markup_string_size = _markupSubstitutionMap->maxKeySize();
	}else{
		_markupSubstitutionMap = _new TokenTagTable();
		_use_paren_markups = false;
	}

	_forceReplacements = _new StringStringMap;
	file_name = ParamReader::getParam("tokenizer_force_replacements");
	if (!file_name.empty()) {
		readReplacementsFile(file_name.c_str(), _forceReplacements);
	}
	
	file_name = ParamReader::getRequiredParam("tokenizer_hyphenated_endings");
	_splitEndings = _new SymbolList(file_name.c_str());
	
	file_name = ParamReader::getRequiredParam("tokenizer_hyphenated_tokens");
	_hyphenatedTokens = _new SymbolList(file_name.c_str());
	
	file_name = ParamReader::getRequiredParam("tokenizer_hyphenated_prefixes");
	_hyphenatedPrefixes = _new SymbolList(file_name.c_str());

	file_name = ParamReader::getParam("tokenizer_always_split_tokens");
	if (!file_name.empty()) {
        _alwaysSplitTokens = _new SymbolList(file_name.c_str());
	} else _alwaysSplitTokens  = _new SymbolList();

	_split_hyphenated_names = ParamReader::getOptionalTrueFalseParamWithDefaultVal("split_hyphenated_names", true);

	_use_itea_tokenization = ParamReader::isParamTrue("use_itea_tokenization");
	_replace_question_marks = ParamReader::isParamTrue("replace_question_marks");
	_use_GALE_tokenization = ParamReader::isParamTrue("use_GALE_tokenization");
	
	_replace_emoticons = ParamReader::isParamTrue("replace_emoticons");
	_remove_excessive_characters = ParamReader::isParamTrue("remove_excessive_characters");

	_do_tokenization = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_tokenization", true);

	_ignore_urls_at_boundaries = ParamReader::isParamTrue("tokenizer_ignore_urls_at_boundaries");

	_ignore_parentheticals = ParamReader::isParamTrue("tokenizer_ignore_parentheticals");

	_ignore_non_english_markup = ParamReader::isParamTrue("tokenizer_ignore_non_english_markup");

	_max_number_of_tokens = ParamReader::getOptionalIntParamWithDefaultValue("tokenizer_maximum_tokens_per_sentence", 99999);


}

EnglishTokenizer::~EnglishTokenizer() {
	if (_substitutionMap != NULL) {
		delete _substitutionMap;
	}
	if (_splitEndings != NULL) {
		delete _splitEndings;
	}
	if (_hyphenatedTokens != NULL) {
		delete _hyphenatedTokens;
	}
	if (_hyphenatedPrefixes != NULL) {
		delete _hyphenatedPrefixes;
	}
	if (_markupSubstitutionMap != NULL) {
		delete _markupSubstitutionMap;
	}
	if (_forceReplacements != NULL) {
		delete _forceReplacements;
	}
	delete _alwaysSplitTokens;
}

/**
 * @param doc the document being tokenized.
 * @param sent_no the index number of the next sentence that will be read.
 */
void EnglishTokenizer::resetForNewSentence(const Document *doc, int sent_no){
	_document = doc;
	_cur_sent_no = sent_no;
	
}


/**
 * Puts an array of pointers to TokenSequences where specified by results arg,
 * and returns its size. Returns <code>0</code> if something goes wrong. The
 * caller is responsible for deleting the array and the TokenSequences.
 *
 * @param results the array of token sequences.
 * @param max_theories the maximum number of sentence theories to produce.
 * @param string the input string of the sentence.
 * @return the number of sentence theories produced; this will always be
 *         <code>1</code> on success and <code>0</code> on failure for this
 *         tokenizer.
 */
int EnglishTokenizer::getTokenTheories(TokenSequence **results,
								int max_theories,
								const LocatedString *string, bool beginOfSentence,
								bool endOfSentence)
{
	// Make a local copy of the sentence string
	// MEM: make sure it gets deleted!
	LocatedString  *localString = _new LocatedString(*string);

	if (max_theories < 1)
		return 0;

	if (_do_tokenization) {
		// Perform string replacements:
		int subs = removeHarmfulUnicodes(localString);
		if (subs > 0) {
			SessionLogger::dbg("unicode") << "removed harmful Unicode " << subs << 
				" chars in tokenizer";
		}
		subs = replaceNonstandardUnicodes(localString);
		if (subs > 0) {
			SessionLogger::dbg("unicode") << "replaced non-standard Unicode " << subs << 
				" chars in tokenizer";
		}
		if (_use_paren_markups){
			replaceParentheticalMarkup(localString);
		}

		if (_replace_emoticons) {
			replaceEmoticons(localString);
		}

		if (_use_itea_tokenization) {
			if (_replace_question_marks)
				replaceQuestionMarks(localString);
			splitSlashesSpecial(localString);

			localString->replace(L"((", L" -LDB- ");
			localString->replace(L"))", L" -RDB- ");
			localString->replace(L"( (", L" -LDB- ");
			localString->replace(L") )", L" -RDB- ");
			localString->replace(L"(FNU)", L" FNU ");
			localString->replace(L"(LNU)", L" LNU ");
			localString->replace(L"ALSO KNOWN AS (AKA)", L" AKA ");
			localString->replace(L"ALSO KNOWN AS", L" AKA ");
			localString->replace(L" (at) ", L"@");
			localString->replace(L"(at)", L"@");
			localString->replace(L" (AT) ", L"@");
			localString->replace(L"(AT)", L"@");
			localString->replace(L" (at sign) ", L"@");
			localString->replace(L"(at sign)", L"@");
			localString->replace(L" (AT SIGN) ", L"@");
			localString->replace(L"(AT SIGN)", L"@");
			localString->replace(L" (atsign) ", L"@");
			localString->replace(L"(atsign)", L"@");
			localString->replace(L" (ATSIGN) ", L"@");
			localString->replace(L"(ATSIGN)", L"@");
			localString->replace(L"(-UNDERSCORE-)", L"_");
			localString->replace(L"(-AT-)", L"@");
			localString->replace(L"(-LEFT-BRACKET-)", L"(");
			localString->replace(L"(-RIGHT-BRACKET-)", L")");
			localString->replace(L"(-ACUTE-ACCENT-)", L"'");
			localString->replace(L"(-ILLEGAL-CHARACTER-)S ", L"'S ");
			localString->replace(L"(-ILLEGAL-CHARACTER-)s ", L"'s ");
			localString->replace(L"(-ILLEGAL-CHARACTER-)", L"");
			localString->replace(L"(-VERTICAL-BAR-)", L"|");
			localString->replace(L"(-PLUS-)", L"+");
			localString->replace(L"(-PERCENT-)", L"%");
			localString->replace(L"(-EQUAL-)", L"=");
		}

		// Apply any parameter file forced replacements
		for (StringStringMap::iterator replacement = _forceReplacements->begin(); replacement != _forceReplacements->end(); replacement++) {
			localString->replace(replacement->first, replacement->second);
		}

		localString->replace(L". . .", L"...");

		//correct possibly recursive ampersand markup
		int llen;
		do {
			llen = localString->length();
			localString->replace(L"&amp;", L"&");
		}while( llen > localString->length() );

		if (_use_GALE_tokenization) {
			localString->replace(L"&gt;", L" ");
			localString->replace(L"&lt;", L" ");
			localString->replace(L"&quot;", L"\"");
			localString->replace(L"&apos;", L"\'");
			localString->replace(L" ... @", L"...@");
			localString->replace(L" (at) ",L"@");
			localString->replace(L"(at)",L"@");
			// for MT, this is best, but I'm not going to check it in...
			//localString->replace(L" - ", L"-");
			removeCorruptedText(localString);
			standardizeNonASCII(localString);
			
			// allow no more than three consecutive periods.
			/*int len;
			do{
				len = localString->length();
				localString->replace(L"....", L"...");
				localString->replace(L"----", L"---");
				localString->replace(L"====", L"---");
			}while( len > localString->length() );
			*/
			replaceExcessivePunctuation(localString);

			// HACK
			localString->replace(L" # ", L" ");

			// Handle untranslated tokens from MT
			localString->replace(L"#unk#", L" ");
			localString->replace(L"#UNK#", L" ");
			localString->replace(L"sourceemptyoruseless", L" ");
			localString->replace(L"notranslationavailable", L" ");
			localString->replace(L"nottranslatedpart", L" ");

		}

		if ((!_use_itea_tokenization)&& beginOfSentence) removeLeadingPunctuation(localString);
		
		//cleanUpCurrency(localString);
		splitCurrencyAmounts(localString);
		splitXmlGlyphs(localString);
		eliminateDoubleQuotes(localString);
		recoverOpenQuotes(localString);
		normalizeWhitespace(localString);
		removeStutters(localString);
		//redundant EMB 1/7/04
		//splitHyphenatedNames(localString);
		splitHyphenatedCaps(localString);
		splitHyphenatedEndings(localString);
		splitEllipses(localString);
		splitDashes(localString);
		splitSlashes(localString);
		if (_ignore_non_english_markup)
			removeNonEnglishMarkup(localString);

		if(endOfSentence) splitFinalPeriod(localString);
		replaceHyphenGroups(localString);
	}

	if (_remove_excessive_characters) {
		replaceExcessiveCharacters(localString);
	}

	// Now find the tokens, breaking at whitespace.
	int token_index = 0;
	int string_index = 0;
	while ((string_index < localString->length())) {
		if (token_index == MAX_SENTENCE_TOKENS) {
			std::wstringstream errMsg;
			errMsg << L"Sentence exceeds token limit of " << MAX_SENTENCE_TOKENS << L". The remainder of the sentence will be discarded.\n";
			errMsg << L"Full sentence: " << localString->toWString() << L"\n";
			errMsg << L"Truncated sentence: " << localString->substringAsWString(0, string_index) << L"\n";
			SessionLogger::warn_user("too_many_tokens") << errMsg.str().c_str();				
			break;
		}

		Token *nextToken = getNextToken(localString, &string_index);
		if (nextToken != NULL) {
			_tokenBuffer[token_index] = nextToken;
			token_index++;
		}
	}

	if (_do_tokenization) {
		if (_ignore_parentheticals)
			token_index = cleanUpParentheticals(localString, token_index);
		if (_ignore_urls_at_boundaries)
			token_index = cleanUpURLs(localString, token_index);
		token_index = cleanUpSpeechTokens(token_index);
		cleanUpWrittenTokens(token_index);
	}

	if (token_index > _max_number_of_tokens) {
	  for (int i = 0; i < token_index; i++) {
	    delete _tokenBuffer[i];
	  }
	  results[0] = _new TokenSequence(_cur_sent_no, 0, _tokenBuffer);
	} else {
	  results[0] = _new TokenSequence(_cur_sent_no, token_index, _tokenBuffer);
	}

	delete localString;

	return 1;
}


void EnglishTokenizer::replaceParentheticalMarkup(LocatedString *string){
	// Search the string for matched parens; contents must not be parens
	// replace any (including its parens) with _markupSubstitutionMap
	wchar_t _char_buf[MAX_TOKEN_SIZE];
	int i = 0;
	while (i < (string->length() - 3)) {
		if (string->charAt(i) == L'(') {
			// we will look for at-most max_markup_sym_size
			// and not past end of string for close paren
			for (int j=1; j+i < string->length() - 1; j++){
				if (j > _max_paren_markup_string_size) {
					break;
				}
				if (string->charAt(j+i) == L'('){
					break;
				}
				if (string->charAt(j+i) == L')') {
				  // Copy the characters into the temp buffer 
					int k = 0;
					for (int ii = 1; ii < j; ii++) {
						_char_buf[k++] = string->charAt(i+ii);
					}
					_char_buf[k] = L'\0';
					// test whether there is a Symbol to avoid table bloat
					if (!Symbol::member(_char_buf)){
						break;
					}
					Symbol testKey = Symbol(_char_buf);
					Symbol newSym = _markupSubstitutionMap->lookup(testKey);
					if (!newSym.is_null()) {
						if (newSym == ParserTags::nullSymbol) {	
							string->remove(i,j+i+1);
						} else{
							string->replace(i,k+2,newSym.to_string());
						}
					}
					break;
				}
			}
		}
		i++;
	}
}

//Replace common emoticons and symbols with -EMOT- and -SYM- respectively
//Information about the specific emoticon is lost
//This means that tokenization will not split emoticon into multiple tokens, which is good
void EnglishTokenizer::replaceEmoticons(LocatedString *string){
	static const boost::wregex emoticons(
		L"[:;=]" //eyes
		L"-?" //optional nose
		L"[dDpPoOvVsS\\)\\]\\}\\(\\[\\{]" //mouth
		);
	
	static const boost::wregex symbols(
		L"(&lt;/?3)" //heart (i.e. <3)
		);

	std::wstring as_wstring = string->substringAsWString(0, string->length());
	boost::wsregex_iterator next_emo(as_wstring.begin(), as_wstring.end(), emoticons);
	boost::wsregex_iterator end_emo;
	while (next_emo != end_emo) {
		boost::wsmatch match = *next_emo;
		string->replace(match.str(), L" -EMOT- ");
		next_emo++;
	}
	
	boost::wsregex_iterator next_sym(as_wstring.begin(), as_wstring.end(), symbols);
	boost::wsregex_iterator end_sym;
	while (next_sym != end_sym) {
		boost::wsmatch match = *next_sym;
		string->replace(match.str(), L" -SYMB- ");
		next_sym++;
	}
}

//Replace any letter repeated more than 2 times in a row with just 2 of that letter
//This is helpful with badly formed internet text, where people put excessive letters for emphasis
void EnglishTokenizer::replaceExcessiveCharacters(LocatedString *input) {
	int start = 0;
	int end;
	for(int start = 0; start < input->length(); start++) {
		if(isalpha(input->charAt(start))) {
			wchar_t let = input->charAt(start);
			for(end = start + 1; end < input->length() && input->charAt(end) == let; end++) {}
			int letStrLen = end-start;
			//end is currently 1 past the end of the punctuation string
			if(letStrLen > 2) {
				//replace replaces from pos to pos+len inclusive, so we need to subtract 1 from the length
				input->replace(start, letStrLen-1, wstring(1, let).c_str());
			}
		}
	}
}

int EnglishTokenizer::cleanUpParentheticals(const LocatedString *string, int token_index) {
	// Digit-only parentheticals are ignored
	static const boost::wregex digits(L"\\d+");

	// First, find all of the complete parentheticals (including nesting)
	ParenStack parenLevel;
	ParenTokenIndices parens;
	int i;
	for (i = 0; i < token_index; i++) {
		if (_tokenBuffer[i]->getSymbol() == Symbol(L"-LRB-") || _tokenBuffer[i]->getSymbol() == Symbol(L"-LCB-") || _tokenBuffer[i]->getSymbol() == Symbol(L"-LDB-"))
			parenLevel.push(ParenStack::value_type(_tokenBuffer[i]->getSymbol(), i));
		else if (!parenLevel.empty() &&
				 ((_tokenBuffer[i]->getSymbol() == Symbol(L"-RRB-") && parenLevel.top().first == Symbol(L"-LRB-")) ||
				 (_tokenBuffer[i]->getSymbol() == Symbol(L"-RCB-") && parenLevel.top().first == Symbol(L"-LCB-")) ||
				 (_tokenBuffer[i]->getSymbol() == Symbol(L"-RDB-") && parenLevel.top().first == Symbol(L"-LDB-")))) {
			parens.push_back(ParenTokenIndices::value_type(parenLevel.top().second, i));
			parenLevel.pop();
		}
	}

	// Decide which parentheticals should be deleted
	ParenTokenIndices deleteParens;
	for (size_t h = 0; h < parens.size(); h++) {
		ParenTokenIndices::value_type paren = parens[h];

		// Remove just the double parentheticals, not their contents, since it's probably a name
		if (_tokenBuffer[paren.first]->getSymbol() == Symbol(L"-LRB-") &&
			h < parens.size() - 1 &&
			_tokenBuffer[parens[h + 1].first]->getSymbol() == Symbol(L"-LRB-") &&
			paren.first == parens[h + 1].first + 1) {
			// Mark just the parentheses for deletion
			deleteParens.push_back(ParenTokenIndices::value_type(parens[h + 1].first, paren.first));
			deleteParens.push_back(ParenTokenIndices::value_type(paren.second, parens[h + 1].second));

			// Add a name span for this parenthetical, if we have that capability
			std::wstringstream tokenString;
			for (int t = paren.first + 1; t < paren.second; t++) {
				tokenString << _tokenBuffer[t]->getSymbol();
				if (t < paren.second - 1)
					tokenString << L" ";
			}
			addMatchingSpans(tokenString.str(), _tokenBuffer[paren.first + 1]->getStartEDTOffset(), DOUBLE_PAREN);

			// Next
			h++;
			continue;
		}
		if (_tokenBuffer[paren.first]->getSymbol() == Symbol(L"-LDB-")) {
			// Mark just the parentheses for deletion
			deleteParens.push_back(ParenTokenIndices::value_type(paren.first, paren.first));
			deleteParens.push_back(ParenTokenIndices::value_type(paren.second, paren.second));

			// Add a name span for this parenthetical, if we have that capability
			std::wstringstream tokenString;
			for (int t = paren.first + 1; t < paren.second; t++) {
				tokenString << _tokenBuffer[t]->getSymbol();
				if (t < paren.second - 1)
					tokenString << L" ";
			}
			addMatchingSpans(tokenString.str(), _tokenBuffer[paren.first + 1]->getStartEDTOffset(), DOUBLE_PAREN);

			// Next
			continue;
		}

		// Parentheticals are filtered differently based on length
		bool keep = false;
		if (paren.second - paren.first > 2) {
			// Keep multi-token parentheticals, unless they start with certain prefixes
			if (!boost::iequals(_tokenBuffer[paren.first + 1]->getSymbol().to_string(), L"tpn")) {
				keep = true;
			}
		} else if (paren.second - paren.first == 2) {
			// Discard single-token parentheticals, unless the contents exists elsewhere in the document
			std::wstring tokenString(_tokenBuffer[paren.first + 1]->getSymbol().to_string());
			if (tokenString.length() > 3 && !boost::regex_match(tokenString, digits)) {
				keep = addMatchingSpans(tokenString, _tokenBuffer[paren.first + 1]->getStartEDTOffset(), SINGLE_PAREN);
			}
		}

		// Find statements or comments and keep their content
		for (i = paren.first + 1; i < paren.second; i++) {
			if (boost::iequals(_tokenBuffer[i]->getSymbol().to_string(), L"comment") || boost::iequals(_tokenBuffer[i]->getSymbol().to_string(), L"statement")) {
				deleteParens.push_back(ParenTokenIndices::value_type(paren.first + 1, i + 1));
				break;
			}
		}

		// Mark any non-matching parentheticals for deletion
		if (!keep) {
			deleteParens.push_back(paren);
		}
	}

	// Delete any matched parentheticals and their contents
	int orig_token_index = token_index;
	int new_token_index = 0;
	for (i = 0; i < orig_token_index; i++) {
		_tempTokenBuffer[i] = _tokenBuffer[i];
		_tokenBuffer[i] = 0;
	}
	for (i = 0; i < orig_token_index;) {
		bool in_parenthetical = false;
		for (size_t j = 0; j < deleteParens.size(); j++) {
			ParenTokenIndices::value_type paren = deleteParens[j];
			if (i >= paren.first && i <= paren.second) {
				in_parenthetical = true;
				break;
			}
		}
		if (in_parenthetical) {
			delete _tempTokenBuffer[i];
			i++;
		} else
			_tokenBuffer[new_token_index++] = _tempTokenBuffer[i++];
	}

	return new_token_index;
}

bool EnglishTokenizer::addMatchingSpans(std::wstring search, EDTOffset search_start, Symbol local_name) {
	SpanCreator* nameCreator = _document->getMetadata()->lookupSpanCreator(Symbol(L"NAME"));
	if (nameCreator == NULL)
		return false;
	std::wstring docString = _document->getOriginalText()->toWString();
	size_t pos = docString.find(search, 0);
	std::vector< std::pair<EDTOffset, EDTOffset> > spanOffsets;
	while (pos != std::wstring::npos) {
		spanOffsets.push_back(std::make_pair(_document->getOriginalText()->startOffsetGroup(static_cast<int>(pos)).value<EDTOffset>(), _document->getOriginalText()->endOffsetGroup(static_cast<int>(pos + search.length()) - 1).value<EDTOffset>()));
		pos = docString.find(search, pos + 1);
	}
	if (spanOffsets.size() > 1) {
		for (size_t i = 0; i < spanOffsets.size(); i++) {
			Symbol entityTypes[3];
			entityTypes[0] = PAREN_URL_PREFIX + local_name;
			entityTypes[1] = Symbol(L"POG");
			entityTypes[2] = local_name;
			if (spanOffsets[i].first != search_start) {
				entityTypes[0] = entityTypes[0] + Symbol(L"Ref");
				entityTypes[2] = entityTypes[2] + Symbol(L"Ref");
			}
			_document->getMetadata()->newSpan(nameCreator->getIdentifier(), spanOffsets[i].first, spanOffsets[i].second, entityTypes);
		}
		return true;
	}

	// No matching spans to keep
	return false;
}

int EnglishTokenizer::cleanUpURLs(const LocatedString *string, int token_index) {
	int i;
	int orig_token_index = token_index;
	int new_token_index = 0;
	for (i = 0; i < orig_token_index; i++) {
		_tempTokenBuffer[i] = _tokenBuffer[i];
		_tokenBuffer[i] = 0;
	}

	for (i = 0; i < orig_token_index;) {
		LocatedString* tokenString = new LocatedString(_tempTokenBuffer[i]->getSymbol().to_string());
		if ((i == 0 || i == orig_token_index - 1) && EnglishTokenizer::matchURL(tokenString, 0)) {
			// Skip an initial or terminal URL token
			delete _tempTokenBuffer[i];
			i++;
		} else {
			_tokenBuffer[new_token_index++] = _tempTokenBuffer[i++];
		}
		delete tokenString;
	}

	return new_token_index;
}

void EnglishTokenizer::cleanUpWrittenTokens(int ntokens) {
	if (_document->getSourceType() != BLOG_SOURCE_SYM &&
		_document->getSourceType() != WEBLOG_SOURCE_SYM &&
		_document->getSourceType() != USENET_SOURCE_SYM)
		return;

	for (int i = 0; i < ntokens; i++) {
		std::wstring wordStr = _tokenBuffer[i]->getSymbol().to_string();
		size_t length = wordStr.length();		
		std::wstring newStr = L"";
		if (length > 2 &&
			iswalpha(wordStr.at(1)) &&
			((wordStr.at(0) == L'_' && wordStr.at(length-1) == L'_') ||
			 (wordStr.at(0) == L'~' && wordStr.at(length-1) == L'~') ||
			 (wordStr.at(0) == L'/' && wordStr.at(length-1) == L'/') ||
			 (wordStr.at(0) == L'*' && wordStr.at(length-1) == L'*')))
		{
			std::wstring newStr = wordStr.substr(1,length-2);
		}
		if (!newStr.empty()) {
			Token *newToken = _new Token(*_tokenBuffer[i], Symbol(newStr.c_str()));
			delete _tokenBuffer[i];
			_tokenBuffer[i] = newToken;
		}
	}
}


int EnglishTokenizer::cleanUpSpeechTokens(int token_index) {
	int i;

	if (_document->getSourceType() != TELEPHONE_SOURCE_SYM)
		return token_index;
	
	int orig_token_index = token_index;
	int new_token_index = 0;
	for (i = 0; i < orig_token_index; i++) {
		_tempTokenBuffer[i] = _tokenBuffer[i];
		_tokenBuffer[i] = 0;
	}
	for (i = 0; i < orig_token_index; i++) {
		// he he
		if (i + 1 < orig_token_index && 
			_tempTokenBuffer[i]->getSymbol() == _tempTokenBuffer[i+1]->getSymbol()) 
		{
			delete _tempTokenBuffer[i];
			continue;
		}
		// --
		if (_tempTokenBuffer[i]->getSymbol() == Symbol(L"--")) {
			delete _tempTokenBuffer[i];
			continue;
		}
		_tokenBuffer[new_token_index++] = _tempTokenBuffer[i];
	}
	
	// Let's go through again, now.
	orig_token_index = new_token_index;	
	new_token_index = 0;
	for (i = 0; i < orig_token_index; i++) {
		_tempTokenBuffer[i] = _tokenBuffer[i];
		_tokenBuffer[i] = 0;
	}

	for (i = 0; i < orig_token_index; ) {
		// sentence-initial ,
		if (new_token_index == 0 && _tempTokenBuffer[i]->getSymbol() == Symbol(L",")) {
			delete _tempTokenBuffer[i];
			i++; 
			continue;
		}
		// he he
		if (i + 1 < orig_token_index && 
			_tempTokenBuffer[i]->getSymbol() == _tempTokenBuffer[i+1]->getSymbol()) 
		{
			delete _tempTokenBuffer[i];
			i++;
			continue;
		}
		// I am I am
		if (i + 3 < orig_token_index && 
			_tempTokenBuffer[i]->getSymbol() == _tempTokenBuffer[i+2]->getSymbol() &&
			_tempTokenBuffer[i+1]->getSymbol() == _tempTokenBuffer[i+3]->getSymbol())
		{
			delete _tempTokenBuffer[i];
			delete _tempTokenBuffer[i+1];
			i += 2;
			continue;
		}		
		// I do n't I do n't
		if (i + 5 < orig_token_index && 
			_tempTokenBuffer[i]->getSymbol() == _tempTokenBuffer[i+3]->getSymbol() &&
			_tempTokenBuffer[i+1]->getSymbol() == _tempTokenBuffer[i+4]->getSymbol() &&
			_tempTokenBuffer[i+2]->getSymbol() == _tempTokenBuffer[i+5]->getSymbol())
		{
			delete _tempTokenBuffer[i];
			delete _tempTokenBuffer[i+1];
			delete _tempTokenBuffer[i+2];
			i += 3;
			continue;
		}
		// A B C D E , A B C D E
		int comma = i + 1;
		for (; comma < orig_token_index; comma++) {
			if (_tempTokenBuffer[comma]->getSymbol() == Symbol(L","))
				break;
		}
		int length = comma - i;
		if (comma + length < orig_token_index) {
			bool found_repeat = true;
			for (int j = 0; j < length; j++) {
				if (_tempTokenBuffer[i+j]->getSymbol() != _tempTokenBuffer[comma+1+j]->getSymbol()) {
                    found_repeat = false;
					break;
				}
			}
			if (found_repeat) {
				for (int repeat = i; repeat <= comma; repeat++) {
					delete _tempTokenBuffer[repeat];
				}
				i = comma + 1;
				continue;
			}
		}
		// I 'm I
		if (i + 2 < orig_token_index &&
			_tempTokenBuffer[i]->getSymbol() == _tempTokenBuffer[i+2]->getSymbol() &&
			(_tempTokenBuffer[i]->getSymbol() == Symbol(L"I") ||
			_tempTokenBuffer[i]->getSymbol() == Symbol(L"we") ||
			_tempTokenBuffer[i]->getSymbol() == Symbol(L"you") ||
			_tempTokenBuffer[i]->getSymbol() == Symbol(L"they") ||
			_tempTokenBuffer[i]->getSymbol() == Symbol(L"he") ||
			_tempTokenBuffer[i]->getSymbol() == Symbol(L"she")) &&
			(_tempTokenBuffer[i+1]->getSymbol() == Symbol(L"'ve") ||
			 _tempTokenBuffer[i+1]->getSymbol() == Symbol(L"'s") ||
			 _tempTokenBuffer[i+1]->getSymbol() == Symbol(L"'m") ||
			 _tempTokenBuffer[i+1]->getSymbol() == Symbol(L"'d") ||
			 _tempTokenBuffer[i+1]->getSymbol() == Symbol(L"'ll"))) 
		{
			delete _tempTokenBuffer[i];
			delete _tempTokenBuffer[i+1];
			i += 2;
			continue;
		}


		_tokenBuffer[new_token_index++] = _tempTokenBuffer[i++];
	}
	return new_token_index;
}


void EnglishTokenizer::removeLeadingPunctuation(LocatedString *string) {
	int index = 0; 
	int len = string->length();

	bool found_alpha = false;
	for (int i = 0; i < len; i++) {
		if (iswalpha(string->charAt(i)))
			found_alpha = true;
	}

	if (!found_alpha)
		return;

	while (index < len && iswspace(string->charAt(index)))
		index++;

	if (index >= len || !iswpunct(string->charAt(index)))
		return;

	if (string->charAt(index) == L'\'' ||
		string->charAt(index) == L'"' ||
		string->charAt(index) == L'(' ||
		string->charAt(index) == L'.' ||
		string->charAt(index) == L'`')
		return;

	int first_non_punct = index;
	while (first_non_punct < len && iswpunct(string->charAt(first_non_punct)))
		first_non_punct++;
	
	if (first_non_punct < len && 
		!iswspace(string->charAt(first_non_punct)) &&
		string->charAt(first_non_punct-1) != '-')
		return;

	//std::cerr << "Removing " << string->substring(0, first_non_punct)->toSymbol() << "\n";
	//std::cerr << "    from " << string->toSymbol() << "\n";

	string->remove(0, first_non_punct);

}

void EnglishTokenizer::removeCorruptedText(LocatedString *string) {
	while (true) {
		int i = 0;	
		int corrupt_count = 0;
		bool found_digit = false;
		for (i = 0; i < string->length(); i++) {
			if (iswalpha(string->charAt(i)) || string->charAt(i) == L' ') {
				corrupt_count = 0;
				found_digit = false;
			} else corrupt_count++;

			if (iswdigit(string->charAt(i)))
				found_digit = true;

			if (corrupt_count > 20 && found_digit) {
				int start = i - corrupt_count;
				bool found_alpha = false;
				for ( ; start >= 0; start--) {
					if (iswalpha(string->charAt(start)))
						found_alpha = true;
					if (string->charAt(start) == L' ' ||
						(found_alpha && iswspace(string->charAt(start))))
						break;
				}
				start++;
				int end = i;
				found_alpha = false;				
				for ( ; end < string->length(); end++) {					
					if (iswalpha(string->charAt(end)))
						found_alpha = true;
					if (string->charAt(end) == L' ' ||
						(found_alpha && iswspace(string->charAt(end))))
						break;
				}

				LocatedString *substring = string->substring(start,end);
				SessionLogger::warn_user("corrupt_text") 					
					<< "The system has judged the following text to be meaningless (not standard English) and has removed it from processing.\n"
					<< "The text removed is: " << substring->toString() << "\n"
					<< "The text was removed from: " << string->toString() << "\n";
				delete substring;
				string->remove(start, end);
				break;
			}
		}
		if (i == string->length())
			break;
	}

}

void EnglishTokenizer::removeNonEnglishMarkup(LocatedString *string) {
	int start = 0;
	int end = string->length();
	if (matchNonEnglishMarkup(string, start, end)) {
		string->remove(start, end);
	}
}

/**
 * @param string the located string source of the sentence.
 * @param i the position at which to check for metadata.
 * @return <code>true</code> if the document metadata enforces a token break
 *         immediately before the given position; <code>false</code> if not.
 */
bool EnglishTokenizer::enforceTokenBreak(const LocatedString *string, int i) const {
	Metadata *metadata = _document->getMetadata();
	Span *span;

	// Is there a span ending just before this position?
	span = metadata->getEndingSpan(string->end<EDTOffset>(i - 1));
	if (span != NULL &&
		// don't break in middle of replaced token
		// (replaced tokens causes several characters in the string to have the same offset)
		string->end<EDTOffset>(i - 1) != string->end<EDTOffset>(i) &&
		span->enforceTokenBreak())
	{
		return true;
	}

	// Is there a span starting right at this position?
	span = metadata->getStartingSpan(string->start<EDTOffset>(i));
	if (span != NULL &&
		// don't break in middle of replaced token
		// (replaced tokens causes several characters in the string to have the same offset)
		(i == 0 || string->end<EDTOffset>(i - 1) != string->end<EDTOffset>(i)) &&
		span->enforceTokenBreak()) {
		return true;
	}

	return false;
}

/**
 * @param string a pointer to the input text.
 * @param index on input, a pointer to the index into the string to start reading
 *              from; on output, a pointer to the next index into the string to
 *              start reading the next token from.
 * @return a pointer to the next token.
 */
Token* EnglishTokenizer::getNextToken(const LocatedString *string, int *index) const {
	int start = *index;

	// Skip past any initial whitespace.
	while ((start < string->length()) &&
		   (iswspace(string->charAt(start))))
	{
		start++;
	}

	// If we're at the end of the string, there was no token.
	if (start == string->length()) {
		*index = start;
		return NULL;
	}

	// Search for the end of the token.
	int end = start + 1;
	while ((end < string->length()) &&
		   (!iswspace(string->charAt(end))) &&
		   (!enforceTokenBreak(string, end)))
	{
		end++;
	}
	if ((end - start) > MAX_TOKEN_SIZE) {
		SessionLogger::warn_user("token_too_big")
			<< "The number of characters in the following token (word) exceeds the system limit of " << MAX_TOKEN_SIZE << " and the token will be split into pieces for processing: " << string->substringAsWString(start, end);
		end = start + MAX_TOKEN_SIZE;
	}

	// Copy the characters into the temp buffer and create a Symbol.
	int j = 0;
	for (int i = start; i < end; i++) {
		_char_buffer[j] = string->charAt(i);
		j++;
	}
	_char_buffer[j] = L'\0';
	Symbol sym = _substitutionMap->replace(Symbol(_char_buffer));

	// Set the index out-parameter to one beyond the end of the token.
	*index = end;
	// EDT-HACK
	return _new Token(string, start, end-1, sym);

}

bool EnglishTokenizer::isHyphenatedName(const wchar_t *string, int len) const {
	int i = 0;

	while (i < len) {
		if (!iswupper(string[i])) {
			return false;
		}
		while ((i < len) && (string[i] != L'-')) {
			i++;
		}
		i++;
	}

	// A hyphenated name can't end with a hyphen. This prevents
	// the system from thinking a stutter after a capitalized
	// word is a hyphenated name (e.g., "I- I don't know").
	return (string[len - 1] != L'-');
}

void EnglishTokenizer::splitHyphenatedCaps(LocatedString *string) {
	int string_index = 0;

	// Search the entire string.
	while (string_index < string->length()) {
		// Find the next hyphen.
		while ((string_index < string->length()) && (string->charAt(string_index) != L'-')) {
			string_index++;
		}

		int hyphen_index = string_index;

		// Move past the hyphen.
		string_index++;

		if (string_index < string->length()) {
			// Find the beginning of the word.
			int start_index = string_index - 2;
			while ((start_index >= 0) && !iswspace(string->charAt(start_index))) {
				start_index--;
			}
			start_index++;

			// Find the end of the word.
			int end_index = string_index;
			while ((end_index < string->length()) && !iswspace(string->charAt(end_index))) {
				end_index++;
			}

			// Get the symbol representing the entire hyphenated word.
			LocatedString *sub = string->substring(start_index, end_index);
			const Symbol found_sym = sub->toSymbol();
			const wchar_t *found = found_sym.to_string();
			int found_len = sub->length();
			delete sub;

			bool first_piece_special = false;
			LocatedString *firstPiece = string->substring(start_index, hyphen_index);
			if (_hyphenatedPrefixes->contains(firstPiece->toSymbol()))
			{
				first_piece_special = true;
			}
			delete firstPiece;

			// Don't split apart any hyphenated tokens that begin or end
			// with "-" or that contain the substring "--" within them.
			if (!first_piece_special &&
				!_hyphenatedTokens->contains(found_sym) &&
				(found_len > 1) &&
				(found[0] != L'-') &&
				(found[found_len - 1] != L'-') &&
				(wcsstr(found, L"--") == NULL))
			{
				int piece_start_index = start_index;
				int piece_end_index = start_index;

				// Check each piece of the hyphenated word. If any piece
				// is capitalized, split it apart from the word.
				while (piece_start_index < end_index) {
					// Find the next piece.
					piece_end_index = piece_start_index;
					while ((piece_end_index < end_index) &&
						   (string->charAt(piece_end_index) != L'-'))
					{
						piece_end_index++;
					}
					// Get the symbol representing the piece.
					LocatedString *sub = string->substring(piece_start_index, piece_end_index);
					const Symbol piece_sym = sub->toSymbol();
					delete sub;

					// If we're splitting on hyphens and the piece is capitalized, split it off.
					// Otherwise, only split it if it's in the _alwaysSplitTokens list.
					if ((_split_hyphenated_names && iswupper(string->charAt(piece_start_index))) ||
						_alwaysSplitTokens->contains(piece_sym))
					{
						int shift = 0;
						// If it's not the last piece, put a space after it.
						if (piece_end_index < end_index) {
							string->insert(L" ", piece_end_index);
							shift++;
						}
						// If it's not the first piece, put a space before it.
						if (piece_start_index > start_index) {
							string->insert(L" ", piece_start_index);
							shift++;
						}
						piece_end_index += shift;
						end_index += shift;
					}
					piece_start_index = piece_end_index + 1;
				}
			}
			string_index = end_index;
		}
	}
}

void EnglishTokenizer::splitHyphenatedEndings(LocatedString *string) {
	int string_index = 0;

	// Search the entire string.
	while (string_index < string->length()) {
		// Find the next hyphen.
		while ((string_index < string->length()) && (string->charAt(string_index) != L'-')) {
			string_index++;
		}

		// Move past the hyphen.
		string_index++;

		if (string_index < string->length()) {
			// Find the end of the word.
			int end_index = string_index;
			while ((end_index < string->length()) && !iswspace(string->charAt(end_index))) {
				end_index++;
			}

			// Get the symbol representing the right half of the hyphenated word.
			LocatedString *sub = string->substring(string_index, end_index);
			Symbol found = sub->toSymbol();
			int found_len = sub->length();
			delete sub;

			// See if it matches any of the endings we are supposed to split.
			if (_splitEndings->contains(found)) {
				string->insert(L" ", string_index - 1);
				// Move past the new space, the ending, and the space after.
				string_index += (found_len + 2);
			}
		}
	}
}

void EnglishTokenizer::splitCurrencyAmounts(LocatedString *string) {
	// Search the string for dollar signs and other similar marks for popular
	// currencies.   We will only insert a space
	// if the following char is a digit, so we can stop at the
	// next-to-last character. This means peeking at the next char is
	// always safe.

	int i = 0;
	while (i < (string->length() - 1)) {
		wchar_t wch = string->charAt(i);
		if (iswdigit(string->charAt(i + 1)) &&
			(((wch >= LOW_UNICODE_CURRENCY_INDEX) &&
			  (wch <= HIGH_UNICODE_CURRENCY_INDEX))  ||
			 (wcschr(PREFIX_CURRENCY_SYMBOLS, wch) != NULL)))
		{
			string->insert(L" ", i + 1);
		}
		i++;
	}
}

void EnglishTokenizer::splitXmlGlyphs(LocatedString *string) {
	int i = 0;
	while(i < (string->length() - 5)) {
		if (string->charAt(i) == L'&' && 
			(matchXmlGlyph(string, i + 3) || matchXmlGlyph(string, i + 4)))
		{
			string->insert(L" ", i);
			i++;
		}
		i++;
	}
}

/**
 * Replaces double-quotation marks (<code>'"'</code>) with two open-quote marks
 * (<code>"``"</code>) when followed by another open-quote mark (either
 * <code>'\''</code> or <code>'`'</code>), and otherwise with two close-quote
 * marks (<code>"''"</code>). The next pass, EnglishTokenizer::recoverOpenQuotes, will
 * convert close-quote marks to open-quote marks where appropriate.
 *
 * @param string the string to modify.
 */
void EnglishTokenizer::eliminateDoubleQuotes(LocatedString *string) {
	for (int i = 0; i < string->length(); i++) {
		if (string->charAt(i) == L'"') {
			int currentLength = string->length();

			// Count the double quotes in a row, and any single quotes after
			int doubleQuotes = matchCharacter(string, i, L'"');
			int singleQuotes = matchCharacter(string, i + doubleQuotes, L'\'');

			// Decide whether these are opening or closing quotes
			bool replaceWithCloseQuotes;
			int replacements = 0;
			if (i >= (currentLength - doubleQuotes)) {
				// If these are the last characters, replace with close quotes
				replaceWithCloseQuotes = true;
			} else if (i < currentLength - doubleQuotes && string->charAt(i + doubleQuotes) == L'`') {
				// Before an open quote, replace with open quotes
				replaceWithCloseQuotes = false;
			} else if (i < currentLength - doubleQuotes && singleQuotes) {
				// Before a single quote, replace all with open quotes, unless followed by an s (i.e. possessive)
				if  (i + doubleQuotes + singleQuotes < currentLength && string->charAt(i + doubleQuotes + singleQuotes) == L's') {
					// This comes up with quoted publication titles, e.g. "Wall Street Journal"'s recent issue
					replaceWithCloseQuotes = true;
				} else {
					replaceWithCloseQuotes = false;
					for (int q = 0; q < singleQuotes; q++) {
						string->replace(i + doubleQuotes + q, 1, L"`");
						replacements++;
					}
				}
			} else if (i > 0 && string->charAt(i - 1) == L'\'') {
				// After a close quote, replace with close quotes
				replaceWithCloseQuotes = true;
			} else if (i == 0 || iswspace(string->charAt(i - 1)) || string->charAt(i - 1) == L'`') {
				// After a space or open quote, or if these are the first characters, replace with open quotes
				replaceWithCloseQuotes = false;
			} else {
				// Default to replacing with close quotes
				replaceWithCloseQuotes = true;
			}

			// Perform the actual replacement
			for (int q = 0; q < doubleQuotes; q++) {
				string->replace(i + 2*q, 1, replaceWithCloseQuotes ? L"''" : L"``");
				replacements += 2;
			}
			i += replacements - 1;
		}
	}
}

/**
 * <p>
 * This method substitutes open-quotes for close-quotes along the lines
 * of these decision tables:
 * </p>
 *
 * <pre>
 *  Two consecutive close-quotes ('')
 *  -------+------------------------------
 *         '''         =>       '''
 *         ''90s       =>       ''90s
 *         ''4 people  =>       ``4 people
 *         ''_hello    =>       ''_hello
 *  -------+------------------------------
 *
 *  One single close-quote (')
 *  -------+-----------------------------
 *         '`          =>       '`
 *         '90s        =>       '90s
 *      don't know     =>    don't know
 *   F.B.I.'s          => F.B.I.'s
 *         '4 people   =>       `4 people
 *         '_hello     =>       '_hello
 *  -------+-----------------------------
 * </pre>
 *
 * @todo more comments here.....
 * @param string the string to modify.
 */
void EnglishTokenizer::recoverOpenQuotes(LocatedString *string) {
	// No string replacements in this method affect the string length.
	const int length = string->length();

	for (int i = 0; i < length; i++) {
		if (string->charAt(i) == L'\'') {
			int lastIndex = length - 1;

			// Two consecutive close-quotes ('')
			if ((i < lastIndex) && (string->charAt(i + 1) == L'\'')) {
				if (((i + 1) < lastIndex)                     &&
					(string->charAt(i + 2) != L'\'')          &&
					(!matchDecadeAbbreviation(string, i + 1)) &&
					(iswalnum(string->charAt(i + 2))))
				{
					string->replace(i, 1, L"`");
					string->replace(i + 1, 1, L"`");
				}

				// Skip past all consecutive close quotes.
				i = skipCloseQuotes(string, i + 1);
			}

			// One single close-quote (')
			else if (i < lastIndex) {
				if (((i + 1) < lastIndex)                      &&
					// Not really needed since '`' isn't alpha-numeric:
					(string->charAt(i + 1) != L'`')            &&
					(!matchDecadeAbbreviation(string, i))      &&
					(!matchValidMidWordApostrophe(string, i))  &&
					(!matchContractionOrPossessive(string, i)) &&
					(!matchEndingPossessive(string, i))        &&
					(!matchAbbreviationPossessive(string, i))  &&
					(iswalnum(string->charAt(i + 1))))
				{
					string->replace(i, 1, L"`");
				}
			}
		}
	}
}

// +---------------+------------------+------------------+----------------+
// |  Don't split  |  Split 2 before  |   Split before   |   Split both   |
// +---------------+------------------+------------------+----------------+
// |  1,000.65     |   should_n't     | friend_'s        |      _,_       |
// |  O'Brien      |       is_n't     | F.B.I._'s        |      _!_       |
// |               |                  |                  |      _:_       |
// +---------------+------------------+------------------+----------------+
void EnglishTokenizer::normalizeWhitespace(LocatedString *string) {
	// NOTE: We only ever modify the string *after* the current position.
	for (int i = 0; i < string->length() - 1; i++) {
		wchar_t curr_char = string->charAt(i);

		// Collapse whitespace.
		if (iswspace(curr_char)) {
			int end = skipWhitespace(string, i);
			if (end > (i + 1)) {
				string->remove(i + 1, end);
			}
		}

		// ````` ->  ``_``_`_    '''''  -> '_''_''_
		// ````  ->  ``_``_      ''''   -> ''_''_
		// ```   ->  ``_`_       '''    -> '_''_
		// ``    ->  ``_         ''     -> ''_
		// `     ->  `_          '      -> '_

		// The current character is a punctuation mark, so we
		// may need to insert a space after it.
		else if (isSplitChar(curr_char) &&
				 !matchNumberSeparator(string, i) &&
				 !matchValidMidWordApostrophe(string, i) &&
				 !matchURL(string, i) &&
				 !matchContractionOrPossessive(string, i) &&
				 !matchAbbreviationPossessive(string, i) &&
				 !matchEndingPossessive(string, i) &&
				 !matchEmoticon(string, i) &&
				 !iswspace(string->charAt(i + 1)))
		{
			// Count opening and closing quotes, if any
			int openQuotes = matchOpenQuotes(string, i);
			int closeQuotes = matchCloseQuotes(string, i);

			// We always insert a space after, but we also want one every two quotes
			if (openQuotes > 1) {
				// Jump each pair of opening quotes
				int insertions = 0;
				for (int j = i + 2; j < i + openQuotes; j += 2) {
					string->insert(L" ", j + insertions);
					insertions++;
				}
				i += openQuotes + insertions;
				string->insert(L" ", i);
			} else if (closeQuotes > 1) {
				// Jump each pair of closing quotes, backwards to handle odd count
				int insertions = 0;
				for (int j = i + closeQuotes; j > i; j -= 2) {
					string->insert(L" ", j);
					insertions++;
				}
				i += closeQuotes + insertions;
			} else {
				string->insert(L" ", i + 1);
				i++;
			}
		}

		// x`````->  x_``_``_`_  x''''' -> x_'_''_''_
		// x```` ->  x_``_``_    x''''  -> x_''_''_
		// x```  ->  x_``_`_     x'''   -> x_'_''_
		// x``   ->  x_``_       x''    -> x_''_
		// x`    ->  x_`_        x'a    -> x_'a
		//                       x'#    -> x_'_#
		//                       .'a    -> ._'a
		//                       x:x    -> x_:_x

		// The next character is a punctuation mark, so we
		// may need to insert a space before and/or after it.
		else if (isSplitChar(string->charAt(i + 1)) &&
				 !matchNumberSeparator(string, i + 1) &&
				 !matchValidMidWordApostrophe(string, i + 1) &&
				 !matchURL(string, i + 1) &&
				 !matchXmlGlyph(string, i + 1) &&
				 !iswspace(string->charAt(i)))
		{
			// Count opening and closing quotes, if any
			int openQuotes = matchOpenQuotes(string, i + 1);
			int closeQuotes = matchCloseQuotes(string, i + 1);

			// Check for possessive after quotes, and let it get broken on its own
			if (closeQuotes > 1 && matchEndingPossessive(string, i + closeQuotes)) {
				closeQuotes--;
			}

			// We always insert a space before and after, but we also want one every two quotes
			if (openQuotes > 1) {
				// Jump each pair of opening quotes
				int insertions = 0;
				for (int j = i + 1; j < i + openQuotes; j += 2) {
					string->insert(L" ", j + insertions);
					insertions++;
				}
				i += openQuotes + insertions + 1;
				string->insert(L" ", i); // Insert final space
			} else if (closeQuotes > 1) {
				// Jump each pair of closing quotes, backwards to handle odd count
				int insertions = 0;
				for (int j = i + closeQuotes + 1; j > i + 1; j -= 2) {
					string->insert(L" ", j);
					insertions++;
				}
				string->insert(L" ", i + 1); // Insert initial space
				i += closeQuotes + insertions + 1;
			}
			// Insert a space before the quote mark in a possessive
			// or contraction, then advance past the quote mark, so
			// in the next iteration, another space will NOT be 
			// inserted after.
			else if (matchContractionOrPossessive(string, i + 1) ||
					 matchAbbreviationPossessive(string, i + 1) ||
					 matchEndingPossessive(string, i + 1))
			{
				string->insert(L" ", i + 1);
				i += 2;
			}
			// Any other kind of punctuation gets preceded by a space.
			// Don't advance beyond the quote mark, so in the next
			// iteration, another space will be inserted.
			else {
				string->insert(L" ", i + 1);
				i++;
			}
		}

		// xn't_   ->  x_n't_
		// xn't#   ->  x_n't_#

		// The next characters are "n't" so we need to put a space before
		// and possibly after them.
		else if (matchNegativeContraction(string, i + 1)) {
			// If there's not already a trailing space, add before and after.
			if (((i + 4) < string->length()) &&
				(!iswspace(string->charAt(i + 4))))
			{
				string->insert(L" ", i + 4);
				string->insert(L" ", i + 1);
				i += 5;
			}
			// Otherwise just add a space before.
			else {
				string->insert(L" ", i + 1);
				i += 4;
			}
		}
	}
}

void EnglishTokenizer::splitEllipses(LocatedString *string) {
	for (int i = 1; i < (string->length() - 3); i++) {
		if ((string->charAt(i) == L'.') &&
			(string->charAt(i + 1) == L'.') &&
			(string->charAt(i + 2) == L'.') &&
			(!iswspace(string->charAt(i - 1))))
		{
			// for email addresses in weblogs, e.g. "larouse...@yahoo.com"
			if (i + 3 < string->length() &&	string->charAt(i + 3) == L'@')
				continue;

			if (matchURL(string, i)) 
				continue;

			// insert space before the ellipses
			string->insert(L" ", i);
	
			// we should also insert one after... why not?
			int last_period = i + 2;
			for (int j = i + 3; j < string->length(); j++) {
				if (string->charAt(j) == L'.')
					last_period = j;
				else break;
			}
			string->insert(L" ", last_period + 1);

		}
	}
}

void EnglishTokenizer::splitDashes(LocatedString *string) {

	for (int i = 0; i < (string->length() - 1); i++) {
		if ((string->charAt(i) == L'-') &&
			(string->charAt(i + 1) == L'-'))
		{
			if (i > 0 && !iswspace(string->charAt(i - 1)))
				string->insert(L" ", i);

			int lastdash = i + 1;
			while (string->length() > lastdash + 1 && string->charAt(lastdash + 1) == L'-')
				lastdash++;
			if (string->length() > lastdash + 1 && !iswspace(string->charAt(lastdash + 1)))
				string->insert(L" ", lastdash + 1);
			i = lastdash - 1;
		}
	}
}

void EnglishTokenizer::splitSlashes(LocatedString *string) {
	// split on "//", or on "[a-zA-Z]/[a-zA-Z]" or on
	// "\s/[a-zA-Z]" or on [a-zA-Z]/\s"
	// This is to prevent e.g. 1/2 from splitting.

	for (int i = 0; i < (string->length() - 3); i++) {

		if ((string->charAt(i) == L'/' &&
			 string->charAt(i + 1) == L'/') ||
			
			(string->charAt(i) == L'/' &&
			 iswalpha(string->charAt(i + 1)) &&
			 i != 0 && iswalpha(string->charAt(i - 1))) ||
			
			(string->charAt(i) == L'/' &&
			 iswalpha(string->charAt(i + 1)) &&
			 i != 0 && iswspace(string->charAt(i - 1))) ||

		    (string->charAt(i) == L'/' && 
			 i != 0 && iswalpha(string->charAt(i - 1)) &&
			 iswspace(string->charAt(i + 1))))
		{
			//wprintf (L"%s\n\n%d\n\n\n", string->toString(), i);
			// don't split on slashes if we're within something beginning with "http" or "www."
			if (matchURL(string, i))
				continue;

			int lastslash = i;
			if (i > 0 && !iswspace(string->charAt(i - 1))) {
				string->insert(L" ", i);
				lastslash++;
			}

			while (string->length() > lastslash + 1 && string->charAt(lastslash + 1) == L'/')
				lastslash++;
			if (string->length() > lastslash + 1 && !iswspace(string->charAt(lastslash + 1))) {
				//wprintf(L"Inserting %d\n\n\n", lastslash + 1);
				string->insert(L" ", lastslash + 1);
			}
			i = lastslash - 1;
		}
	}
}


void EnglishTokenizer::splitSlashesSpecial(LocatedString *string) {
	for (int i = 0; i < (string->length() - 7); i++) {

		if (string->charAt(i) == L'/') {

			// back up, make sure we have a possible name before the slash
			bool found_alpha = false;
			int j = i - 1;
			while (j >= 0) {
				wchar_t c = string->charAt(j);
				j--;
				if (iswspace(c))
					break;
				if (c == L'(' || c == L')' || c == '\'')
					continue;
				if (iswalpha(c)) {
					found_alpha = true;
					continue;
				}
				return;
			}
			if (!found_alpha)
				return;

			// look forward, make sure we have a possible phone number after the slash
			bool found_digit;
			j = i + 1;
			while (j < string->length()) {
				wchar_t c = string->charAt(j);
				j++;
				if (j == i + 2 && c == L'+')
					continue;
				if (iswspace(c) || c == L'.')
					break;
				if (iswdigit(c)) {
					found_digit = true;
					continue;
				}
				return;
			}
			if (!found_digit) 
				return;

			// we have a Smith/7819239123 string
			string->insert(L" ", i + 1);
			string->insert(L" ", i);
		}
	}
}


void EnglishTokenizer::splitFinalPeriod(LocatedString *string) {
	// Skim from the end to the left of any trailing whitespace or punctuation.
	int index = string->length() - 1;
	while ((index > 0) &&
		   (iswspace(string->charAt(index)) || isSplitChar(string->charAt(index))))
	{
		index--;
	}

	bool matched_ellipsis = false;

	// If the character is a period, make sure it's preceded by a space.
	if ((index > 0) && (string->charAt(index) == L'.')) {
		// If it's an ellipsis, the space goes before the ellipsis.
		if (matchEllipsis(string, index)) {
			matched_ellipsis = true;
			index -= 2;
		}
		if ((index > 0) && !iswspace(string->charAt(index - 1))) {
			string->insert(L" ", index);
		}
	}

	// If we ended with an ellipsis, check to see if that was in turn
	// preceded by a final period.
	if (matched_ellipsis) {
		index--;
		// Skim again to the left of trailing whitespace or punctuation.
		while ((index > 0) &&
			   (iswspace(string->charAt(index)) || isSplitChar(string->charAt(index))))
		{
			index--;
		}

		// If the character is a period, make sure it's preceded by a space.
		if ((index > 0) && (string->charAt(index) == L'.')) {
			// If it's an ellipsis, the space goes before the ellipsis.
			if (matchEllipsis(string, index)) {
				index -= 2;
			}
			if ((index > 0) && !iswspace(string->charAt(index - 1))) {
				string->insert(L" ", index);
			}
		}
	}
}

void EnglishTokenizer::removeStutters(LocatedString *input) {
	const int len = input->length();

	// Search right-to-left for fastest removal.
	int end = len - 1;
	while (end > 0) {
		// Find the next token.
		while ((end > 0) && iswspace(input->charAt(end))) {
			end--;
		}

		if (end > 0) {
			int start = end - 1;
			while ((start >= 0) && !iswspace(input->charAt(start))) {
				start--;
			}
			start++;

			// If the word ends in a hyphen and the hyphen is preceded
			// by an alphabetic character, it's a stuttered word.
			// If the word starts with a hyphen, though, it's a special
			// sequence (such as "-LRB-"), so it's not a stutter.
			// This is dangerous, and is currently only enabled if we
			// are dealing with telephone speech.
			if (_document->getSourceType() == TELEPHONE_SOURCE_SYM &&
				(input->charAt(end) == L'-') &&
				(input->charAt(start) != L'-') &&
				(((end + 1) == len) || iswspace(input->charAt(end + 1))) &&
				(iswalpha(input->charAt(end - 1))))
			{
				input->remove(start, end + 1);
			}
			// If the word is "um" or "uh", it's a stuttered word.
			else if (_document->getSourceType() == TELEPHONE_SOURCE_SYM ||
					 _document->getSourceType() == BROADCAST_CONV_SOURCE_SYM)
			{
				LocatedString *sub = input->substring(start, end + 1);
				sub->toLowerCase();
				Symbol word = sub->toSymbol();
				delete sub;
				if ((word == Symbol(L"um")) || (word == Symbol(L"uh")) || 
					(word == Symbol(L"eh")) || (word == Symbol(L"ah"))) 
				{
					input->remove(start, end + 1);
				}
			}

			end = start - 1;
		}
	}
}

void EnglishTokenizer::cleanUpCurrency(LocatedString *string) {

	// Remove the "US" from statements of numeric currency
	string->replace(L" US $", L" $");
	string->replace(L" US$", L" $");

	// Remove "US" from particularly evil things like $50US,000 
	bool in_dollar_sign = false;
	for (int i = 0; i < (string->length() - 2); i++) {
		if (string->charAt(i) == L'$') {
			in_dollar_sign = true;
			continue;
		}

		if (iswdigit(string->charAt(i)))
			continue;

		if (iswspace(string->charAt(i))) {
			in_dollar_sign = false;
			continue;
		}

		if (in_dollar_sign && string->charAt(i) == L'U' && string->charAt(i+1) == L'S') {
			string->remove(i,i+2);
			in_dollar_sign = false;
		}
	}
}


bool EnglishTokenizer::matchDecadeAbbreviation(const LocatedString *string, int index) const {
	return ((index + 3) < string->length())      &&
		   (string->charAt(index) == L'\'')      &&
		   (iswdigit(string->charAt(index + 1))) &&
		   (iswdigit(string->charAt(index + 2))) &&
		   (towlower(string->charAt(index + 3)) == L's');
}

// Matches "n't" from the index one before the apostrophe
bool EnglishTokenizer::matchNegativeContraction(const LocatedString *string, int index) const {
	return ((index + 2) < string->length()) &&
		   // Could be "n't" or "N'T", etc:
		   (towlower(string->charAt(index)) == L'n') &&
		   (string->charAt(index + 1) == L'\'') &&
		   (towlower(string->charAt(index + 2)) == L't') &&
		   // The *entire* contraction must be "n't":
		   ((index + 3 >= string->length()) || (!iswalpha(string->charAt(index + 3))));
}

// Matches from the index of the apostrophe
bool EnglishTokenizer::matchContractionOrPossessive(const LocatedString *string, int index) const {
	// Check for apostrophe followed by at least one other 
	// char and preceded by an alpha-numeric.
	if ((index > 0)                               &&
		((index + 1) < string->length())          &&
		(string->charAt(index) == L'\'')          &&
		(iswalnum(string->charAt(index - 1))))
	{
		// 'm 'd 's
		if ((index + 2 == string->length() || !iswalnum(string->charAt(index + 2))) &&
			(towlower(string->charAt(index + 1)) == L'm' ||
			 towlower(string->charAt(index + 1)) == L'd' ||
			 towlower(string->charAt(index + 1)) == L's'))
			return true;

		if ((index + 2) < string->length() &&
			(index + 3 == string->length() || !iswalnum(string->charAt(index + 3))))
		{
			// 've
			if (towlower(string->charAt(index + 1)) == L'v' &&
				towlower(string->charAt(index + 2)) == L'e')
				return true;
			
			// 'll
			if (towlower(string->charAt(index + 1)) == L'l' &&
				towlower(string->charAt(index + 2)) == L'l')
				return true;

			// 're
			if (towlower(string->charAt(index + 1)) == L'r' &&
				towlower(string->charAt(index + 2)) == L'e')
				return true;

			// 'em
			if (towlower(string->charAt(index + 1)) == L'e' &&
				towlower(string->charAt(index + 2)) == L'm')
				return true;
		}
	}
	return false;
}

bool EnglishTokenizer::matchEndingPossessive(const LocatedString *string, int index) const {
	return ((index > 0)                           &&
		    ((index + 2) < string->length())      &&
			(string->charAt(index) == L'\'')      &&
			(string->charAt(index + 1) == L's')   &&
			(iswspace(string->charAt(index + 2))));
}

bool EnglishTokenizer::matchEmoticon(const LocatedString *string, int index) const {
	if (string->charAt(index) != L':' || index + 1 >= string->length())
		return false;

	if (string->charAt(index + 1) == L'p' || string->charAt(index + 1) == L'P') {
		// :p
		if (index + 2 == string->length() || iswspace(string->charAt(index + 2)))
			return true;
		// :p:
		if (index + 3 < string->length() && string->charAt(index + 2) == L':' && iswspace(string->charAt(index + 3)))
			return true;
	}

	return false;
}

// This is a really lazy unigram classifier; we probably should use something
bool EnglishTokenizer::matchNonEnglishMarkup(const LocatedString *string, int start, int end) const {
	// Count character distribution
	int vowels = 0;
	int consonants = 0;
	for (int i = start; i < end && i < string->length(); i++) {
		if (wcschr(L"aeiouAEIOU", string->charAt(i)) != NULL)
			vowels++;
		else if (iswalpha(string->charAt(i)))
			consonants++;
	}

	// Calculate vowel to consonant ratio
	float ratio = 0.0;
	if (consonants > 0)
		ratio = static_cast<float>(vowels)/consonants;

	// Be conservative and look for very low vowel ratio
	if (ratio < 0.2)
		return true;
	else
		return false;
}

// I.B.M.'s
bool EnglishTokenizer::matchAbbreviationPossessive(const LocatedString *string, int index) const {
	return ((index > 0)                         &&
		    ((index + 1) < string->length())    &&
			(string->charAt(index) == L'\'')    &&
			(string->charAt(index - 1) == L'.') &&
			(towlower(string->charAt(index + 1)) == L's'));
}

// 00:12:00 OR 1,000 OR 34.21
bool EnglishTokenizer::matchNumberSeparator(const LocatedString *string, int index) const {
	return (index > 0) &&
		   ((index + 1) < string->length()) &&
		   ((string->charAt(index) == L',') || 
		   (string->charAt(index) == L':') ||
			(string->charAt(index) == L'.')) &&
		   (iswdigit(string->charAt(index - 1))) &&
		   (iswdigit(string->charAt(index + 1)));
}

// apostrophe surround by alpha characters, but not part of contraction/possessive
bool EnglishTokenizer::matchValidMidWordApostrophe(const LocatedString *string, int index) const {
	return (index > 0) &&
		   ((index + 1) < string->length()) &&
		   (string->charAt(index) == L'\'')    &&
		   (iswalpha(string->charAt(index - 1))) &&
		   (iswalpha(string->charAt(index + 1))) &&
		   (!matchContractionOrPossessive(string, index));
}		   

// ..., but not ...@ (as in "larouse...@yahoo.com")
bool EnglishTokenizer::matchEllipsis(const LocatedString *string, int index) const {
	return ((index - 2) > 0) &&
		   (string->charAt(index) == L'.') &&
		   (string->charAt(index - 1) == L'.') &&
		   (string->charAt(index - 2) == L'.') &&
		   (index + 1 >= string->length() || string->charAt(index + 1) != L'@');
}

bool EnglishTokenizer::matchXmlGlyph(const LocatedString *string, int index) const {
// &amp;, &lt;, &gt; only
	if (string->charAt(index) != L';') 
		return false;
	if (index >= 4 && 
		string->charAt(index-4) == L'&' &&
		string->charAt(index-1) == L'p' &&
		string->charAt(index-2) == L'm' &&
		string->charAt(index-3) == L'a' ) {	
			return true;
	}
	if (index >= 3 && 
		string->charAt(index-3) == L'&' &&
		string->charAt(index-1) == L't' &&
		(  string->charAt(index-2) == L'l' ||
		   string->charAt(index-2) == L'g' )) {	
			return true;
	}
	return false;
}

bool EnglishTokenizer::isSplitChar(wchar_t c) const {
	// TODO: could make this more efficient if it mattered
	return wcschr(SPLIT_PUNCTUATION, c) != NULL;
}

void EnglishTokenizer::replaceQuestionMarks(LocatedString *input) {
	wchar_t last_char = L' ';
	for (int i = 0; i < input->length(); i++) {
		wchar_t c = input->charAt(i);
		wchar_t next_char = L' ';
		if (i + 1 < input->length())
			next_char = input->charAt(i + 1);

		if (c == L'?') {
			if (iswspace(last_char) && iswspace(next_char))
				input->replace(i, 1, L"-");
			else if (iswspace(last_char) && replaceEndWordQuestionMark(input, i))
				input->replace(i, 1, L"'");
			else if (iswspace(last_char)) 
				input->replace(i, 1, L" ");
			else if (looksLikeContraction(input, i))
				input->replace(i, 1, L"'");
			else if (inMiddleOfWord(input, i))
				input->replace(i, 1, L"-");
		}
		last_char = input->charAt(i);
	}
}

bool EnglishTokenizer::replaceEndWordQuestionMark(LocatedString *input, int pos) {
	int end_pos = pos + 2;
	if (end_pos >= input->length())
		return false;

	while (end_pos < input->length() - 1 && !iswspace(input->charAt(end_pos)))
		end_pos++;

	if (iswspace(input->charAt(end_pos)))
		end_pos--;

	// end_pos is end of containing pos
	if (input->charAt(end_pos) == L'?') {
		input->replace(end_pos, 1, L"'");
		return true;
	} 

	return false;
}

bool EnglishTokenizer::looksLikeContraction(LocatedString *input, int pos) {
	// Replace ? with apostrophe and use use previously written method
	input->replace(pos, 1, L"'");
	bool answer = matchContractionOrPossessive(input, pos);
	input->replace(pos, 1, L"?");
	return answer;
}

bool EnglishTokenizer::inMiddleOfWord(LocatedString *input, int pos) {
	if (pos == 0 || pos == input->length() - 1)
		return false;

	wchar_t prev = input->charAt(pos - 1);
	wchar_t next = input->charAt(pos + 1);

	if (iswspace(prev) || iswspace(next)) 
		return false;

	if (next == L'\'' || next == L'"' || next == L',' || next == L';' || next == L':')
		return false;

	// check to make sure we aren't in a web address
	int start_pos = pos - 1;
	while (start_pos > 0 && !iswspace(input->charAt(start_pos)))
		start_pos--;

	if (iswspace(input->charAt(start_pos)))
		start_pos++;

	// start_pos is start of word containing pos
	if (towlower(input->charAt(start_pos)) == L'w' &&
		towlower(input->charAt(start_pos + 1)) == L'w' &&
		towlower(input->charAt(start_pos + 2)) == L'w') 
	{
		return false;
	}

	if (towlower(input->charAt(start_pos)) == L'h' &&
		towlower(input->charAt(start_pos + 1)) == L't' &&
		towlower(input->charAt(start_pos + 2)) == L't' &&
		towlower(input->charAt(start_pos + 3)) == L'p') 
	{
		return false;
	}

	return true;
}
