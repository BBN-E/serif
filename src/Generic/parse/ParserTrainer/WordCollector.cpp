// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/WordCollector.h"
#include "Generic/parse/ParserTrainer/TrainerVocab.h"
#include "Generic/parse/WordFeatures.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include <cstddef>
#include <string>


void WordCollector::read_from_file (UTF8InputStream& stream) 
{
	UTF8Token token;
	Symbol word_symbol;
	bool isFirstWord = true;
	
			
	stream >> token; // had better be a tag
	if ((wcscmp(token.chars(), L"(") == 0) || (wcscmp(token.chars(), L")") == 0))
		throw UnexpectedInputException("WordCollector::read_from_file",
		"ERROR: ill-formed sentence (parentheses where a tag should be)\n");

	stream >> token;

	while (true) {
		if (wcscmp (token.chars(), L"(") == 0) {
			read_from_file(stream);
		} else if (wcscmp (token.chars(), L")") == 0) {
			return;
		} else {
			word_symbol = Symbol(token.chars());

			vocabularyTable->add(word_symbol);

			//  add the right form to wordFeatTable
			Symbol word_features = word_symbol;
			word_features = wordFeat->features(word_features, isFirstWord);
			Symbol wdft[2];
			wdft[0] = word_symbol;
			wdft[1] = word_features;
			wordFeatTable->add(wdft,1);

			isFirstWord = false;
			stream >> token; // ')'
			return;
		}
		stream >> token;
	}

}
	

void WordCollector::print_all(char *v)
{
	vocabularyTable->print(v);

	// dump the new wordFeatTable
	char headName[501];
	size_t headsiz = strlen(v) - 4;
	if (headsiz > 490) headsiz = 490;
	_snprintf(headName, headsiz ,"%s" ,v);
	headName[headsiz] = '\0';
	char wordFeatTab[501];
	_snprintf(wordFeatTab, 501, "%s.wordfeat" ,headName);

	wordFeatTable->print(wordFeatTab);

}

