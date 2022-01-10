// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef MATCH_STORAGE_H
#define MATCH_STORAGE_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <map>

class TokenSequence;
class PatternFeatureSet;
class Mention;
class ValueMention;
class MatchData;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;

class MatchStorage 
{
public:
	struct MatchData
	{
		Mention *mention;
		ValueMention *valueMention;
		MatchData *next;
		PatternFeatureSet_ptr patternFeatureSet;
		bool head;

		int start_token;
		int end_token;
	};


	MatchStorage(int num_patterns);
	~MatchStorage();

	void storeMatch(int pattern_number, Mention *ment, ValueMention *vm, PatternFeatureSet_ptr sfs, bool head);
	bool getNextSentence(TokenSequence *ts, int search_start_token, int search_end_token, 
		std::wstring &result, MatchData **currentMatchData, int* offsetToStartTokenMap, int* offsetToEndTokenMap);
	void resetCurrentMatches();

private:


	int _num_patterns;

	int _highest; // highest non-text pattern
	bool _finished; // turns to true when all combinations of non-text patterns have been looked at

	// each entry in _patternsArray is a linked list of matches for that pattern
	MatchData **_patternsArray;
	MatchData **_currentMatches;

	// move to next possible combination of MatchData, set _finished to false, if no more combinations
	void advance();
	void appendAndRecord(std::wstring &result, const wchar_t *str, int &offset_pos, int start_token_number, int end_token_number, 
		int* offsetToStartTokenMap, int* offsetToEndTokenMap);

	bool matchesAreInOrder();
};

#endif

