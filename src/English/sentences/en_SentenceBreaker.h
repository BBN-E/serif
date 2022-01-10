#ifndef EN_SENTENCE_BREAKER_H
#define EN_SENTENCE_BREAKER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Sentence.h"
#include "English/zoner/en_TextZoner.h"
#include "English/sentences/WordSet.h"
#include "Generic/sentences/DefaultSentenceBreaker.h"

#include <stack>

typedef std::pair<int, int> BreakBound;

class Metadata;
class Region;
class Zone;
class LocatedString;

class EnglishSentenceBreaker : public DefaultSentenceBreaker {
protected:
	WordSet *_nonFinalAbbreviations;
	WordSet *_noSplitAbbreviations;
	WordSet *_knownWords;
	WordSet *_lowercaseHeadlineWords;
	std::set<std::wstring> _datelineParentheticals;
	std::set<std::wstring> _rarelyCapitalizedWords;
	std::vector<std::wstring> _punctuatedLeads;
	std::set<std::wstring> _days;
	std::set<std::wstring> _months;
	std::vector<std::pair<std::wstring, std::wstring> > _parenPairs;
	std::stack<wchar_t> _quoteLevel;

public:
	~EnglishSentenceBreaker();
	EnglishSentenceBreaker();

private:

	/// Resets the sentence breaker to run on the next new document.
	void resetForNewDocument(const Document *doc);

	/// Breaks a LocatedString into individual Sentence objects.
	int getSentencesRaw(Sentence **results, int max_sentences,
						const Region* const* regions, int num_regions);

	int getSentencesRaw(Sentence **results, int max_sentences,
						const Zone* const* zones, int num_zones);

	/// Tries to match and return a dateline from the beginning of input.
	int getDatelineSentences(Sentence **results, int max_sentences, const LocatedString *input, int start);

	/// Reads the next complete sentence from input.
	Sentence* getNextSentence(int *offset, const LocatedString *input);

	bool _breakLongSentences;
	bool _breakListSentences;
	std::wstring _listSeparators;
	int _minListSeparators;
	bool attemptListSentenceBreak(Sentence **results, int max_sentences);
	bool isAcceptableSentenceString(const LocatedString *string, int start, int end);

	int getIndexBeforeOpeningPunctuation(const LocatedString *input, int index);
	int getIndexAfterClosingPunctuation(const LocatedString *input, int index);
	int getIndexAfterClosingPunctuationHandlingQuotes(const LocatedString *input, int index);
	bool matchSentenceFinalCitation(const LocatedString *input, int index, int origin);
	bool matchFinalPeriod(const LocatedString *input, int index, int origin);
	bool matchPossibleFinalPeriod(const LocatedString *input, int index, int origin);
	Symbol getWordEndingAt(int index, const LocatedString *input, int origin);
	bool matchNonSplittableName(const LocatedString *input, int index, int origin);
	bool matchLikelyAbbreviation(Symbol word);
	bool matchLikelyInitials(Symbol word);
	bool matchLikelySentenceStart(const LocatedString *input, int pos);
	bool matchWashPostDateline(const LocatedString *input, int pos);
	int matchEllipsis(const LocatedString *input, int index);
	bool matchExplicitItemizedListBegin(const LocatedString *input, int index);
	bool matchListItem(const LocatedString *input, int index);
	bool matchBlankLine(const LocatedString *input, int index);
	bool matchCenterOfURL(const LocatedString *input, int index, int ellipsis_len);
	bool isWordChar(const wchar_t c) const;
	bool isSplitChar(const wchar_t c) const;
	bool isOpeningPunctuation(const wchar_t c) const;
	bool isClosingPunctuation(const wchar_t c) const;
	bool isListSeparator(const wchar_t c) const;
	bool isBulletChar(const wchar_t c) const;
	bool isEOSChar(const wchar_t ch) const;
	bool isSecondaryEOSChar(const wchar_t ch) const;
	bool isEOLChar(const wchar_t ch) const;
	int max(int a, int b);
	int min(int a, int b);

	int _short_line_length;

	bool _isAllLowercase;
	bool _skipHeadlines;
	bool _downcaseHeadlines;
	bool _isLikelyHeadline(const LocatedString* locStr);

	bool _useGALEHeuristics;
	bool _isRemovableGALERegion(const LocatedString *input);
	bool _isRemovableGALESentence(Sentence* sent);
	bool _isBuriedDateline(const LocatedString *input, int start, int end);
	bool _isSentenceEndingWebAddress(const LocatedString *input, int start, int end);
	bool _isBreakableXMLGlyph(const LocatedString *input, int start, int end);

	bool _useRegionContentFlags;
	bool _breakOnDoubleCarriageReturns;
	bool _aggressiveDoubleCarriageReturnsSplit;
	bool _isSafeDoubleCarriageReturn(const LocatedString *input, int start, int end);
	bool _isCarriageReturnBreak(const LocatedString *input, int start, int end);
	bool _isBreakableCenterPeriod(const LocatedString *input, int start, int end);

	bool _ignore_page_breaks;
	static const boost::wregex page_number_re;
	void _removePageBreaks(LocatedString *input);
	
	bool _dont_break_in_parentheticals;
	bool _treatUnknownDocsAsWebText;
	bool _isWebText;
	bool _isFollowedByPunctuationString(const LocatedString* input, int start, int end);
	bool _isBreakableSemicolon(const LocatedString* input, int start, int end);
	bool _isLastInPunctuationString(const LocatedString* input, int start, int end);
	bool _isClosingStartingOpenParen(const LocatedString* input, int start, int end);
	bool _isClosingQuoteLevel(const LocatedString* input, int end);
	
	bool _useITEASentenceBreakHeuristics;
	bool _isFollowedByListItem(const LocatedString *input, int end);
	bool _isAtEndOfListItem(const LocatedString *input, int end);
	bool _isListItemPeriod(const LocatedString *input, int start, int end);
	bool _isListItemParen(const LocatedString *input, int start, int end);
	bool _endsDashLine(const LocatedString *input, int start, int end);
	bool _isFollowedByDashLine(const LocatedString *input, int end);
	bool _alwaysUseBreakableColons;
	bool _isBreakableColon(const LocatedString *input, int start, int end);
	bool _isRoman(wchar_t c);
	bool _thisSentenceIsListItem;
	bool _lastSentenceIsListItem;
	bool _break_on_footnote_numbers;

	bool _useLACSSSentenceBreakHeuristics;

	bool _isExplicitlyLowercase(const wchar_t c) const;
	bool _isExplicitlyUppercase(const wchar_t c) const;

	bool _isLastInStringOfDashes(const LocatedString *input, int start, int end);
	bool _isFollowedByStringOfDashes(const LocatedString *input, int start, int end);
	bool _isLastInStringOfSlashes(const LocatedString *input, int start, int end);

	bool _break_on_portion_marks;
	static const boost::wregex portion_mark_upper_re;
	static const boost::wregex portion_mark_lower_re;
	static const boost::wregex portion_mark_basic_re;
	bool _isPortionMark(const LocatedString *input, int start, int end);
	bool _isFollowedByPortionMark(const LocatedString *input, int start, int end);

	bool isLikelyWebAddress(Symbol word);

	bool _useWholeDocDatelineMode;
	typedef enum {DL_NONE, DL_CONSERVATIVE, DL_AGGRESSIVE, DL_VERY_AGGRESSIVE} dateline_mode_t;
	dateline_mode_t DATELINE_MODE;
	int _breakOffFromDatelineSentences(Sentence **results, int max_sentences, const LocatedString *input, int start, int max_end);
	std::vector<int> _findPunctuatedLeadBreak(const LocatedString *input, int start, int max_end, std::wstring punct);
	std::vector<int> _findOtherLeadBreak(const LocatedString *input, int start, int max_end);
	std::vector<int> _findParentheticalDatelineBreak(const LocatedString *input, int start, int max_end, std::wstring left, std::wstring right);
	std::vector<int> _findBuriedByline(const LocatedString *input, int start, int max_end);
	bool _isAllUppercaseWord(std::wstring word);
	bool _isPossibleNameWord(std::wstring word);
	bool _isNumber(std::wstring word);
	std::wstring _getNextToken(const LocatedString *input, int start, bool letters_only);
	std::wstring _getPrevToken(const LocatedString *input, int start, bool letters_only);


	bool DEBUG;
	UTF8OutputStream _debugStream;

};

#endif
