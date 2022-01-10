// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/morphSelection/Retokenizer.h"
#include "Generic/morphSelection/MorphDecoder.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalTokenSequence.h"

MorphSelector::MorphSelector() : morphDecoder(0) {
	morphDecoder = MorphDecoder::build();
}

MorphSelector::~MorphSelector() {
	delete morphDecoder;
}

int MorphSelector::selectTokenization(const LocatedString *sentenceString, TokenSequence *origTS) {
	int nNewTokens = 
		morphDecoder->getBestWordSequence(*sentenceString, origTS, words, map, startOffsets, 
		endOffsets, MAX_SENTENCE_TOKENS);
	updateTokenSequence(origTS, map, startOffsets, endOffsets, 
		words, nNewTokens);
	return nNewTokens;
}

int MorphSelector::selectTokenization(const LocatedString *sentenceString, TokenSequence *origTS, 
									  CharOffset *constraints, int n_constraints)
{
	int nNewTokens = 
		morphDecoder->getBestWordSequence(*sentenceString, origTS, words, map, startOffsets, 
		endOffsets, constraints, n_constraints, MAX_SENTENCE_TOKENS);
	updateTokenSequence(origTS, map, startOffsets, endOffsets, 
		words, nNewTokens);
	return nNewTokens;
}

void MorphSelector::updateTokenSequence(TokenSequence* origTS, int* map,  
										 OffsetGroup* start, OffsetGroup* end, Symbol* words, 
										 int nwords)
{
	int nOrigTokens = origTS->getNTokens();

	// Is the original token sequence a lexical token sequence?
	bool isLexicalTokenSequence = (dynamic_cast<LexicalTokenSequence*>(origTS) != NULL);

	for (int i = 0; i < nwords; i++) {
		if (isLexicalTokenSequence)
			newTokens[i] = _new LexicalToken(start[i], end[i], words[i], map[i]);

		/*
		const Token* origTok = origTS->getToken(_newTokens[i]->getOriginalTokenIndex());
		Token* newTok = _newTokens[i];
		std::cout << "%%%% TSTest: "
			<< newTok->getSymbol().to_debug_string()
			<< " " << newTok->getStartEDTOffset()
			<< " " < newTok->getStartEDTOffset()
			<< " n lex choice: " << origTok->getNLexicalEntries() << std::endl;
		for (int m = 0; m < origTok->getNLexicalEntries(); m++) {
			std::cout << "    ";
			origTok->getLexicalEntry(m)->dump(std::cout);
			std::cout << std::endl;
		}*/
	}

	Retokenizer::getInstance().Retokenize(origTS, newTokens, nwords);

	if (isLexicalTokenSequence) {
		for (int i = 0; i < nwords; i++)
			delete newTokens[i];
	}
}
