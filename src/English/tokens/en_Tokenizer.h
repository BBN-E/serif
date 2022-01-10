// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/parse/TokenTagTable.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/SymbolList.h"
#include "Generic/tokens/Tokenizer.h"


#include <wchar.h>

#include <stack>

// Back-doors for unit tests:
#ifdef _DEBUG
#define _protected    public
#define _private      public
#else
#define _protected    protected
#define _private      private
#endif

// Containers for tracking parentheticals
typedef std::stack< std::pair<Symbol, int> > ParenStack;
typedef std::vector< std::pair<int, int> > ParenTokenIndices;

// TODO: mark all const methods as such

/**
 * The tokenizer for English-language input. Takes a located string
 * and produces a sequence of symbols representing the individual tokens
 * in a sentence. This implementation is not probabilistic but rather uses
 * a set of rules.
 *
 * @author David A. Herman
 */
class EnglishTokenizer : public Tokenizer {

public:
	/// Constructs a new English tokenizer.
	EnglishTokenizer();
	/// Destroys this tokenizer.
	~EnglishTokenizer();

protected:
	/// The index number of the sentence being tokenized.
	int _cur_sent_no;

	/// The internal buffer of tokens used to construct the token sequence.
	static Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];
	static Token *_tempTokenBuffer[MAX_SENTENCE_TOKENS+1];

	/// The internal buffer of characters used to construct individual tokens.
	static wchar_t _char_buffer[MAX_TOKEN_SIZE+1];

	/// Finds the next token in the string.
	Token* getNextToken(const LocatedString *string, int *index) const;

	// True if you want to use itea specific tokenization rules
	bool _use_itea_tokenization;
	// True if you want to use GALE-specific tokenization rules
	bool _use_GALE_tokenization;

	// True if you want the tokenizer to skip URLs that are strictly the first or last token
	bool _ignore_urls_at_boundaries;

	// True if you want the tokenizer to ignore non-text parentheticals
	bool _ignore_parentheticals;

	// True if you want the tokenizer to skip sentences that appear to be non-English markup
	bool _ignore_non_english_markup;

	// English tokenizer will "zero out" any sentence longer than this length
	int _max_number_of_tokens;

	// The token substitution map.
	SymbolSubstitutionMap *_substitutionMap;

	// The list of forced pre-tokenization replacements (generally used to split tokens)
	StringStringMap* _forceReplacements;

_protected:
	/// replace large range of markup inside parens; e.g.  " sold for (pounds sterling)2000 "
	void replaceParentheticalMarkup(LocatedString *string);

	/// Replaces emoticons and symbols with tags to group during tokenization
	/// e.g. ":)" -> "-EMOT-" and '<3' - "-SYMB-"
	void replaceEmoticons(LocatedString *string);

	/// Replaces letters repeated more than twice with two repetitions of the letter
	/// e.g. "heyyyyy" -> "heyy"
	void replaceExcessiveCharacters(LocatedString *string);

	/// Finds hyphenated phrases with capitalized sub-words and splits them at the hyphens.
	void splitHyphenatedCaps(LocatedString *string);

	/// Finds hyphenated phrases with recognized common endings and splits off the endings.
	void splitHyphenatedEndings(LocatedString *string);

	/// Finds currency symbols (such as U.S. dollar signs, Euros, GB pounds, Yen)
	///   and splits them off from their corresponding numbers.
	void splitCurrencyAmounts(LocatedString *string);

	/// puts a space before "&amp;", "&lt;" and "&gt;"
	void splitXmlGlyphs(LocatedString *string);

	/// Removes leading punctuation (e.g. "-- This is a list")
	void removeLeadingPunctuation(LocatedString *string);

	/// Removes corrupted text (e.g. "The competitors so far range from a }0952:3407:/:42'0:47706
	//									997140:47773)21:94:429"206299;4414923:60:299:7151975299/:77:42999:1409:
	//									42*74;299ity of Houston.")
	void removeCorruptedText(LocatedString *string);

	// Skips any sentences that contain junk, like "xxx tdl rta"
	//   We probably want this to apply to prefixes and suffixes, but hard to find boundary if we didn't break
	void removeNonEnglishMarkup(LocatedString *string);

	/// Eliminates occurrences of double-quotation marks.
	void eliminateDoubleQuotes(LocatedString *string);

	/// Attempts to convert close-quotes to open-quotes where appropriate.
	void recoverOpenQuotes(LocatedString *string);

	/// Splits off punctuation from adjacent text and collapses extra whitespace.
	void normalizeWhitespace(LocatedString *string);

	/// Finds ellipses and splits them off from adjacent text.
	void splitEllipses(LocatedString *string);

	/// Finds "--" or longer ("---") and splits on either side
	void splitDashes(LocatedString *string);

	/// Finds "//" or longer ("///") and splits on either side
	void splitSlashes(LocatedString *string);

	/// Finds "/" in between word and number "Smith/7182421234" 
	/// Often name/phone number in ATEA
	void splitSlashesSpecial(LocatedString *localString);

	/// Splits off a final period character if one is found.
	void splitFinalPeriod(LocatedString *string);

	/// Remove unnecessary information generated by stutters in speech.
	void removeStutters(LocatedString *string);

	/// Remove the word "US" from mentions of currency, e.g. 'US $8 million' or '$50US,000'
	void cleanUpCurrency(LocatedString *string);

	/// Ignore non-text parentheticals inside sentences
	int cleanUpParentheticals(const LocatedString *string, int token_index);

	/// Ignore URLs at the beginning or end of a sentence
	int cleanUpURLs(const LocatedString *string, int token_index);

	/// Remove repeated words and punctuation generated by stutters in speech.
	int cleanUpSpeechTokens(int token_index);

	/// Clean up things that are typical of Internet typing (e.g. *emphasis*)
	void cleanUpWrittenTokens(int ntokens);

	/// Find and add spans that match specified text
	bool addMatchingSpans(std::wstring search, EDTOffset search_start, Symbol local_name);

	/// Replace suspicious question marks (which can occur by
	/// copy and pasting unicode with appropriate string
	void replaceQuestionMarks(LocatedString *string);
	bool looksLikeContraction(LocatedString *string, int pos);
	bool inMiddleOfWord(LocatedString *string, int pos);
	bool replaceEndWordQuestionMark(LocatedString *string, int pos);

	/// Moves an index counter beyond closing quotation marks in input text.
	//-generic    int skipCloseQuotes(const LocatedString *string, int index) const;

	/// Optional markup tokens substitution map.
	TokenTagTable *_markupSubstitutionMap;

	/// flag true if using parenthetical markup substitutions
	bool _use_paren_markups;

	/// flag true if you want to replace emoticons with tags
	bool _replace_emoticons;

	/// flag true if you want to remove characters repeated more than twice
	bool _remove_excessive_characters;

	/// The list of recognized endings that should be split off hyphenated phrases.
	SymbolList *_splitEndings;

	/// The list of hyphenated phrases that should be treated as single tokens.
	SymbolList *_hyphenatedTokens;

	/// The list of prefixes to hyphenated words that should make the word be treated as a single token.
	SymbolList *_hyphenatedPrefixes;

	/// The list of tokens that should always be split off when next to hyphens.
	SymbolList *_alwaysSplitTokens;

	/// True if splitHyphenatedCaps are to be called. False otherwise
	bool _split_hyphenated_names;


	/// True if your data is basically already pretokenized and you want it to
	//   respect those boundaries -- it REALLY ought to have been pretokenized
	//   by this tokenizer!
	bool _do_tokenization;

private:

	/// Resets the state of the tokenizer to prepare for a new sentence.
	void resetForNewSentence(const Document *doc, int sent_no);

	/// Splits an input string into a sequence of tokens.
	int getTokenTheories(TokenSequence **results, int max_theories,
			             const LocatedString *string, bool beginOfSentence = true,
								 bool endOfSentence= true );

	// Perform forced replacements before tokenization
	void forceReplacements(void);

	// These all match at the index of the punctuation mark.
	bool matchDecadeAbbreviation(const LocatedString *string, int index) const;
	bool matchContractionOrPossessive(const LocatedString *string, int index) const;
	bool matchAbbreviationPossessive(const LocatedString *string, int index) const;
	bool matchEndingPossessive(const LocatedString *string, int index) const;
	bool matchNumberSeparator(const LocatedString *string, int index) const;
	bool matchValidMidWordApostrophe(const LocatedString *string, int index) const;
	bool matchEmoticon(const LocatedString *string, int index) const;
	bool matchNonEnglishMarkup(const LocatedString *string, int start, int end) const;
	
	bool matchNegativeContraction(const LocatedString *string, int index) const;

	bool matchEllipsis(const LocatedString *string, int index) const;
	// bool matchURL(const LocatedString *string, int index) const;
	bool isSplitChar(wchar_t c) const;
	bool enforceTokenBreak(const LocatedString *string, int i) const;
	bool matchXmlGlyph(const LocatedString *string, int i) const;

	const Document *_document;
	int _max_paren_markup_string_size;
	bool _replace_question_marks;
	
_private:
	bool isHyphenatedName(const wchar_t *string, int len) const;


};

#undef _protected
#undef _private
