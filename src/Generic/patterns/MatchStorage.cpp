// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/patterns/MatchStorage.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/ValueMention.h"

#include "common/Sexp.h"

MatchStorage::MatchStorage(int num_patterns) 
{
	
	_num_patterns = num_patterns;
	_patternsArray = _new MatchData*[_num_patterns];
	_currentMatches = _new MatchData*[_num_patterns];

	for (int i = 0; i < _num_patterns; i++) {
		_patternsArray[i] = 0;
	}

	_highest = -1;
	_finished = false;
}

MatchStorage::~MatchStorage()
{
	//std::cout << "deleting ms\n"; std::cout.flush();
	for (int i = 0; i < _num_patterns; i++) {
		MatchData *ptr = _patternsArray[i];
		while (ptr != 0) {
			MatchData *next = ptr->next;
			delete ptr;
			ptr = next;
		}
	}
	delete [] _patternsArray;
	delete [] _currentMatches;
	//std::cout << "done deleting ms\n"; std::cout.flush();
}

void MatchStorage::resetCurrentMatches() 
{
	for (int i = 0; i < _num_patterns; i++) 
		_currentMatches[i] = _patternsArray[i];
}

void MatchStorage::storeMatch(int pattern_number, Mention *ment, ValueMention *vm, PatternFeatureSet_ptr sfs, bool head)
{	
	if (pattern_number > _highest) 
		_highest = pattern_number;

	MatchData *newMatch = _new MatchData();
	newMatch->mention = ment;
	newMatch->valueMention = vm;
	newMatch->patternFeatureSet = sfs;
	newMatch->head = head;
	newMatch->next = 0;

	if (ment != 0 && !head) {
		newMatch->start_token = ment->getNode()->getStartToken();
		newMatch->end_token = ment->getNode()->getEndToken();
	} else if (ment != 0 && head) {
		newMatch->start_token = ment->getHead()->getStartToken();
		newMatch->end_token = ment->getHead()->getEndToken();
	} else if (vm != 0) {
		newMatch->start_token = vm->getStartToken();
		newMatch->end_token = vm->getEndToken();
	} else {
		throw InternalInconsistencyException("MatchStorage::storeMatch", "No Mention or ValueMention");
	}

	if (_patternsArray[pattern_number] == 0) {
		_patternsArray[pattern_number] = newMatch;
		_currentMatches[pattern_number] = newMatch;
	} else {
		MatchData *ptr = _patternsArray[pattern_number];
		while (ptr->next != 0) 
			ptr = ptr->next;
		ptr->next = newMatch;
	}
}

bool MatchStorage::matchesAreInOrder() {
	int last_end = -1;
	for (int i = 0; i <= _highest; i++) {
		if (_patternsArray[i] == 0) continue; // text pattern
		MatchData *md = _currentMatches[i];
		if (md->start_token <= last_end)
			return false;
		last_end = md->end_token;
	}
	return true;
}

// Take the next combination of MatchData, and create a sentence out of it.
// Store a sentence offset to Token map
bool MatchStorage::getNextSentence(TokenSequence *ts, int search_start_token, int search_end_token, 
								   std::wstring &result, MatchData **currentMatchData, 
								   int* offsetToStartTokenMap, int* offsetToEndTokenMap)
{
	if (_finished) return false;

	// check to make sure current matches pointed to are in right order and don't overlap
	while (!matchesAreInOrder()) {
		advance();
		if (_finished) return false;
	}

	// current matches do not overlap, create sentence
	int token_number = 0;
	int offset_pos = 0;
	result = L"";

	// We put a space at the beginning of a regex pattern, so we don't start matching in the middle of a word.
	// This requires us to put a space in here as well. 
	appendAndRecord(result, L" ", offset_pos, 0, 0, offsetToStartTokenMap, offsetToEndTokenMap);
	for (int i = 0; i <= _highest; i++) {
		currentMatchData[i] = _currentMatches[i];
		if (_patternsArray[i] == 0) continue; // text pattern
		MatchData *md = _currentMatches[i];

		while (token_number < md->start_token) {
			if (token_number >= search_start_token && token_number <= search_end_token) {
				const wchar_t* token_str = ts->getToken(token_number)->getSymbol().to_string();
				appendAndRecord(result, token_str, offset_pos, token_number, token_number, offsetToStartTokenMap, offsetToEndTokenMap);
				appendAndRecord(result, L" ", offset_pos, token_number + 1, token_number, offsetToStartTokenMap, offsetToEndTokenMap);
			}
			token_number++;
		}
		
		if (token_number >= search_start_token && token_number <= search_end_token) {
			appendAndRecord(result, L"!@RP_MATCH@! ", offset_pos, token_number, md->end_token, offsetToStartTokenMap, offsetToEndTokenMap);
		}
		token_number = md->end_token + 1;
	}

	while (token_number < ts->getNTokens()) {
		if (token_number >= search_start_token && token_number <= search_end_token) {
			const wchar_t* token_str = ts->getToken(token_number)->getSymbol().to_string();
			appendAndRecord(result, token_str, offset_pos, token_number, token_number, offsetToStartTokenMap, offsetToEndTokenMap);
			
			int space_start_token = token_number + 1;
			if (space_start_token >= ts->getNTokens())
				space_start_token = ts->getNTokens() - 1;
			appendAndRecord(result, L" ", offset_pos, space_start_token, token_number, offsetToStartTokenMap, offsetToEndTokenMap);
		}
		token_number++;
	}

	if (offset_pos == 1) {
		// Nothing was added to string. This could happen if there are no subpatterns in the RegexPattern.
		return false;
	}	

	// make the space at the beginning that we put in earlier match the earliest token
	if (offset_pos >= 2) {
		offsetToStartTokenMap[0] = offsetToStartTokenMap[1];
		offsetToEndTokenMap[0] = offsetToEndTokenMap[1];
	}

	advance();
	return true;
}

void MatchStorage::appendAndRecord(std::wstring &result, const wchar_t *str, int &offset_pos, int start_token_number,
								   int end_token_number, int* offsetToStartTokenMap, int* offsetToEndTokenMap) 
{
	result.append(str);

	const size_t len = wcslen(str);
	for (size_t j = 0; j < len; j++) {
		offsetToStartTokenMap[offset_pos] = start_token_number;
		offsetToEndTokenMap[offset_pos] = end_token_number;
		offset_pos++;
	}
}

// Advance to the next combination of mention matches
void MatchStorage::advance() 
{
	for (int i = 0; i <= _highest; i++) {
		if (_patternsArray[i] == 0) continue; // text pattern
		_currentMatches[i] = _currentMatches[i]->next;
		if (_currentMatches[i] != 0) return;
		
		// We moved off the end of this pattern's list, reset to start and advance next pattern's list
		_currentMatches[i] = _patternsArray[i];
	}

	// We moved off the end of the last list. This means we've looked at all combinations of MatchData
	_finished = true;
	return;
}

