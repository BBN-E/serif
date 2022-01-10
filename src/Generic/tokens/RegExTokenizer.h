// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGEXP_TOKENIZER_H
#define REGEXP_TOKENIZER_H

#include "Generic/common/LocatedString.h"
#include "Generic/common/RegexMatch.h"
#include <wchar.h>
#include <vector>

#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/common/TokenOffsets.h"
#include <string>
#include <boost/regex.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>

using namespace std;

/* Struct representing the input:
 *  regexVal = string representing regular expression pattern
 *  label = label to be applied when this pattern found (currently not used)
 *  subexp = number (must be 1 or more) of the subexpression to be 
 *			 extracted when the pattern is found.  "0" means the whole
 *			 matching expression is to be used.
 */
struct RegexData
{
	int ID;
	wstring regexVal;
	wstring label;
	vector <int> subexp;
	boost::wregex expression;
};

class RegExTokenizer: public Tokenizer {
private:
	/* Vector of all the regular expression patterns for which to search. */
	vector <RegexData> _regExStrings;

public:
	/** Constructs a new tokenizer.  This takes ownership of the given
	 * base tokenzier. */
	RegExTokenizer(Tokenizer *baseTokenizer);

	/// Destroys this tokenizer.
	~RegExTokenizer();

	int getTokenTheories(TokenSequence **results, int max_theories,
						 const LocatedString *string, 
						 bool beginOfSentence = true,
						 bool endOfSentence =true);

	/// Resets the state of the tokenizer to prepare for a new sentence.
	void resetForNewSentence(const Document *doc, int sent_no);

protected:
	/// 'base' tokenizer, used for backoff.
	Tokenizer *_tokenizer;

	/// The token substitution map.
	SymbolSubstitutionMap *_substitutionMap;

	/// The internal buffer of tokens used to construct the token sequence.
	Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];

	bool _create_lexical_tokens;

private:
	const Document *_document;
	int _cur_sent_no;
	static void regexCallback(const boost::wsmatch& what, int sub_string_pos, std::vector<RegexMatch>* matches_p);
	static void printMatchCallback(const RegexMatch& match);
	void applyExpression (const RegexData& expr, const LocatedString * target_text, vector <RegexMatch> &matches);
	void processRegexps (const std::vector<RegexData>& expressions, LocatedString *target_text, TokenOffsets *token_offsets);
	void readRegExFile(const char filename[]);
	
};

#endif
