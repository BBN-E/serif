// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKEN_OFFSETS_H
#define TOKEN_OFFSETS_H

#include <vector>
#include <iostream>



/* 
 * Struct representing offset pairs (begin, end) for a portion of a
 * located string.  This struct can be used to represent regular
 * expression matches (in which case, "isRegexToken" is true) or
 * untokenized portions of the locatedString that need to be sent
 * to the "existing" Tokenizer (in which case, "isRegexToken" is false).
 */
struct offset_object {
	int begin;
	int end;
	bool isRegexToken;

	int getBegin() {
		return begin;
	}

	int getEnd() {
		return end;
	}

	void setBegin(int b) {
		begin = b;
	}

	void setEnd(int e) {
		end = e;
	}

	bool getRegexBool() {
		return isRegexToken;
	}

	/* Comparison operators needed for the sort() operation
	 * on the vector of "offset_object" objects. */
    bool operator < (const offset_object &oo) const
    { return (begin < oo.begin);} 

    bool operator > (const offset_object &oo) const
    { return (begin > oo.begin);}

    bool operator == (const offset_object &oo) const
	{ return (begin == oo.begin);}
};

/* 
 * Stores the offsets of both regular expression matches and the text around
 * these matches that have yet to be tokenized.  Defines functions to manipulate
 * this information 
 */
class TokenOffsets {

public:
	TokenOffsets() {};
	~TokenOffsets();

	void addRegexOffset(int begin, int end);
	void printInfo();
	int size();
	bool overlapsExist(int begin, int end);
	void getUntokenizedOffsets(int begin_off, int end_off);
	int numRegExpMatches();
	offset_object * at(int i);
	

private:
	std::vector <offset_object> _token_offsets;
	std::vector <offset_object*> _tok_and_untok_offsets;
	std::vector <offset_object> _untok_offsets;
	offset_object createOffsetObject(int begin, int end, bool isRegEx);
	void combineOffsets();
	
};
#endif
