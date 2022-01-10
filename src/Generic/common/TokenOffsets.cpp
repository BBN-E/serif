// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/TokenOffsets.h"
#include <stdio.h>
#include <algorithm>
#include "Generic/common/InternalInconsistencyException.h"


/// Adds a set of regular expression match offsets to the _token_offsets vector
void TokenOffsets::addRegexOffset(int begin, int end) {	
	offset_object curr_oo;
	
	if(overlapsExist(begin,end)){
		return;
	}

    curr_oo.begin = begin;
    curr_oo.end = end;
    curr_oo.isRegexToken = true;

    _token_offsets.push_back(curr_oo);

    /* Sort the vector so that regular expression offsets are always in order */
	std::sort(_token_offsets.begin(), _token_offsets.end());
}

/* 
 * Debugging function - outputs vector of token offsets, vector of untokenized offsets,
 * and ordered vector of pointers to these two vectors. 
 */
void TokenOffsets::printInfo() {
	offset_object * curr_offset;
	unsigned int i;

	std::ostringstream ostr;
	ostr << "\n\nHere's the vector of tokens: ";
	for (i = 0; i < _token_offsets.size(); i++) {
		ostr << "[" << _token_offsets.at(i).getBegin() << ", " << _token_offsets.at(i).getEnd() << ", " << _token_offsets.at(i).getRegexBool() << "] ";
	}

	ostr << "\nHere's the vector of un-tokens: ";
	for (i = 0; i < _untok_offsets.size(); i++) {
		ostr << "[" << _untok_offsets.at(i).getBegin() << ", " << _untok_offsets.at(i).getEnd() << ", " << _untok_offsets.at(i).getRegexBool() << "] ";
	}

	ostr << "\nHere's the vector of pointers: ";
	for (i = 0; i < _tok_and_untok_offsets.size(); i++) {
		curr_offset = _tok_and_untok_offsets.at(i);

		ostr << "[" << curr_offset->getBegin() << ", " << curr_offset->getEnd() << ", " << curr_offset->getRegexBool() << "] ";
	}
	SessionLogger::info("SERIF") << ostr.str();
}

/* 
 * Returns the size of the _tok_and_untok_offsets vector --
 * a vector of pointers to (regular expression tokens) + (non-tokenized 
 * areas around these tokens).
 */
int TokenOffsets::size() {
	return int (_tok_and_untok_offsets.size());
}

/*
 * "Helper" function for addRegexOffset -- is called separately by the user
 * but may be combined into addRegexOffset if desired in the future.  Determines
 * if the offset_object that is being added overlaps an existing regex token.  If
 * so, then the current set of offsets is not added.
 */
bool TokenOffsets::overlapsExist(int begin, int end) {
	/* check if offsets you're about to add overlap already-existing offsets */
	offset_object curr_oo;
	int curr_beg, curr_end;

	for (int i = 0; i < numRegExpMatches(); i++) {
		curr_oo = _token_offsets.at(i);
		curr_beg = curr_oo.getBegin();
		curr_end = curr_oo.getEnd();

		if (((begin >= curr_beg) && (begin <= (curr_end-1))) ||
			((end >= curr_end) && (end <= (curr_end-1))) ||
			(begin <= curr_beg && (end >= curr_end))) {
			//cout << "I've found an overlap, curr: [" << curr_beg << ", " << curr_end << "] and checking: [" << begin << ", " << end << "]" << endl;
			return true;
		}
	}

	return false;
}

/* Creates a new offset_object */
offset_object TokenOffsets::createOffsetObject(int begin, int end, bool isRegEx) {
	offset_object temp_oo;
	temp_oo.begin = begin;
	temp_oo.end = end;
	temp_oo.isRegexToken = isRegEx;

	return temp_oo;
}
	

/* 
 * Input here is (begin_off, end_off), where begin_off = beginning offset you want to include in your returned
 * vector and end_off = ending offset you want to include in your returned vector.
 * I deliberately wrote this NOT to assume that we were using the whole string and that the beginning offset 
 * starts at 0, just in case we later want to work with subsections of strings.
 */

/* Also creates a vector of pointers to _both_ tokenized and untokenized offsets.  The range
 * spans from the "begin_off" and "end_off" specified in the "getUntokenizedOffsets" 
 * procedure */
void TokenOffsets::getUntokenizedOffsets(int begin_off, int end_off) {
	offset_object *curr_pt_oo;

	/* if there previously was an _untok_offsets vector, clear it since
	 * we are creating a new one (presumably something has changed) */
	_untok_offsets.clear();
	
	int last_end = begin_off;
	
	for (int i = 0; i < numRegExpMatches(); i++) {
		curr_pt_oo = &_token_offsets.at(i);
		
		if (curr_pt_oo->getBegin() > last_end) {
			//offset_object *curr_untok;
			offset_object dummy_untok;
			dummy_untok = createOffsetObject(last_end, curr_pt_oo->getBegin(), false);
		
			//cout << "\nUntok Pushing: [" << dummy_untok.begin << ", " << dummy_untok.end << ", " << dummy_untok.isRegexToken << "]" << endl;
			_untok_offsets.push_back(dummy_untok);
		}

        last_end = curr_pt_oo->getEnd();

		/* debug */
		//cout << "In Loop: ";
		//printInfo();
		/* end debug */
	}

	/* look for any untokenized parts between last regex_offset and end of string */
	if (last_end <= end_off) {
		offset_object dummy_untok;
		//offset_object *curr_untok;

		dummy_untok = createOffsetObject(last_end, end_off+1, false);

		//cout << "\nUntok Pushing: [" << dummy_untok.begin << ", " << dummy_untok.end << ", " << dummy_untok.isRegexToken << "]" << endl;
		_untok_offsets.push_back(dummy_untok);

	}

	/* debug */
	//cout << "\nAfter Loop: ";
	//printInfo();
	/* end debug */

	combineOffsets();
}

/* Combines the regex_token offsets and the offsets for the untokenized portions of the sentence.
 * These two vectors are combined into a vector of pointers, and need to be pushed onto the vector
 * in the correct sequential order. */
void TokenOffsets::combineOffsets() {
	std::vector <offset_object> *last_entry_vector;
	std::vector <offset_object> *other_vector;
    offset_object currI, currJ;
	int i = 0, j = 0;

	_tok_and_untok_offsets.clear();

	//check that both vectors have items
	if (_token_offsets.empty()){
		throw InternalInconsistencyException("TokenOffsets::combineOffsets()", "Sentence should contain at least one special token.");
	}
	else if(_untok_offsets.empty()){	
		last_entry_vector = &_token_offsets;
		for (i = 0; i < int(last_entry_vector->size()); i++) {
			_tok_and_untok_offsets.push_back(&last_entry_vector->at(i));
		}
		//std::cout<<"Sentence only contains a special token.\n";
	}
	/* figure out which vector has the last entry */
	else{
		if (_untok_offsets.back() > _token_offsets.back()) {
			last_entry_vector = &_untok_offsets;
			other_vector = &_token_offsets;
		} 
		else {
			last_entry_vector = &_token_offsets;
			other_vector = &_untok_offsets;
		}
	
		currJ = other_vector->at(j);

		/* Start by traversing the vector with the last entry and add pointers one-by-one
		* to each entry of this vector (currI).  Meanwhile, add pointers pointing to members 
		* of the other vector (currJ) if less than currI. */
		for (i = 0; i < int(last_entry_vector->size()); i++) {
			currI = last_entry_vector->at(i);
			while (currJ < currI) {
				_tok_and_untok_offsets.push_back(&other_vector->at(j));

				if (j >= int(other_vector->size()-1))
					break;

				j++;
				currJ = other_vector->at(j);
			}

			_tok_and_untok_offsets.push_back(&last_entry_vector->at(i));
		}
	}
}

/* Returns the number of regular expression matches found. */
int TokenOffsets::numRegExpMatches() {
	return int(_token_offsets.size());
}

/* Returns a pointer to the offset_object at index i in the tok_and_untok_offsets vector */
offset_object * TokenOffsets::at(int i) {
	return _tok_and_untok_offsets.at(i);
}

TokenOffsets::~TokenOffsets() {
}
