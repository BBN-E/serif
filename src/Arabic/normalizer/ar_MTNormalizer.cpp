// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/normalizer/ar_MTNormalizer.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/morphSelection/Retokenizer.h"


void ArabicMTNormalizer::normalize(TokenSequence* ts){
	LexicalTokenSequence *lts = dynamic_cast<LexicalTokenSequence*>(ts);
	if (!lts) throw InternalInconsistencyException("ArabicMTNormalizer::normalize",
		"This ArabicMTNormalizer requires a LexicalTokenSequence.");
	int num_tokens = lts->getNTokens();
	int new_index = 0;
	int new_num_tokens = num_tokens; //This will be the number of tokens after normalization.
	for (int i = 0; i < num_tokens; i++) {
		const LexicalToken *origToken = lts->getToken(i);

		bool token_is_originally_end_of_word = true;
		if(i+1 < num_tokens){
			const LexicalToken *nextToken = lts->getToken(i+1);
			if (origToken->getOriginalTokenIndex() == nextToken->getOriginalTokenIndex())
				token_is_originally_end_of_word = false;
		}

		LocatedString *locatedString = _new LocatedString(origToken->getSymbol().to_string());
		normalizeLocatedString(locatedString, token_is_originally_end_of_word);
		
		if(locatedString->length()==0){ //This occaisionally happens when a token is only a diacritic.
			new_num_tokens--;
			delete locatedString;
			continue;
		}
		Symbol newSymbol(locatedString->toString());
		
		_tokens[new_index] = _new LexicalToken(*origToken, newSymbol);
		delete locatedString;
		new_index++;
	}
	
	lts->retokenize(new_num_tokens, _tokens);
}

void ArabicMTNormalizer::normalizeLocatedString(LocatedString *locatedString, bool token_is_originally_end_of_word){

	/*** Replace extended arabic characters with basic ones ***/
	locatedString->replace(L"\xfe80", L"\x0621");
	locatedString->replace(L"\xfe81", L"\x0622");
	locatedString->replace(L"\xfe82", L"\x0622");
	locatedString->replace(L"\xfe83", L"\x0623");
	locatedString->replace(L"\xfe84", L"\x0623");
	locatedString->replace(L"\xfe85", L"\x0624");
	locatedString->replace(L"\xfe86", L"\x0624");
	locatedString->replace(L"\xfe87", L"\x0625");
	locatedString->replace(L"\xfe88", L"\x0625");
	locatedString->replace(L"\xfe89", L"\x0626");
	locatedString->replace(L"\xfe8a", L"\x0626");
	locatedString->replace(L"\xfe8b", L"\x0626");
	locatedString->replace(L"\xfe8c", L"\x0626");
	locatedString->replace(L"\xfe8d", L"\x0627");
	locatedString->replace(L"\xfe8e", L"\x0627");
	locatedString->replace(L"\xfe8f", L"\x0628");
	locatedString->replace(L"\xfe90", L"\x0628");
	locatedString->replace(L"\xfe91", L"\x0628");
	locatedString->replace(L"\xfe92", L"\x0628");
	locatedString->replace(L"\xfe93", L"\x0629");
	locatedString->replace(L"\xfe94", L"\x0629");
	locatedString->replace(L"\xfe95", L"\x062a");
	locatedString->replace(L"\xfe96", L"\x062a");
	locatedString->replace(L"\xfe97", L"\x062a");
	locatedString->replace(L"\xfe98", L"\x062a");
	locatedString->replace(L"\xfe99", L"\x062b");
	locatedString->replace(L"\xfe9a", L"\x062b");
	locatedString->replace(L"\xfe9b", L"\x062b");
	locatedString->replace(L"\xfe9c", L"\x062b");
	locatedString->replace(L"\xfe9d", L"\x062c");
	locatedString->replace(L"\xfe9e", L"\x062c");
	locatedString->replace(L"\xfe9f", L"\x062c");
	locatedString->replace(L"\xfea0", L"\x062c");
	locatedString->replace(L"\xfea1", L"\x062d");
	locatedString->replace(L"\xfea2", L"\x062d");
	locatedString->replace(L"\xfea3", L"\x062d");
	locatedString->replace(L"\xfea4", L"\x062d");
	locatedString->replace(L"\xfea5", L"\x062e");
	locatedString->replace(L"\xfea6", L"\x062e");
	locatedString->replace(L"\xfea7", L"\x062e");
	locatedString->replace(L"\xfea8", L"\x062e");
	locatedString->replace(L"\xfea9", L"\x062f");
	locatedString->replace(L"\xfeaa", L"\x062f");
	locatedString->replace(L"\xfeab", L"\x0630");
	locatedString->replace(L"\xfeac", L"\x0630");
	locatedString->replace(L"\xfead", L"\x0631");
	locatedString->replace(L"\xfeae", L"\x0631");
	locatedString->replace(L"\xfeaf", L"\x0632");
	locatedString->replace(L"\xfeb0", L"\x0632");
	locatedString->replace(L"\xfeb1", L"\x0633");
	locatedString->replace(L"\xfeb2", L"\x0633");
	locatedString->replace(L"\xfeb3", L"\x0633");
	locatedString->replace(L"\xfeb4", L"\x0633");
	locatedString->replace(L"\xfeb5", L"\x0634");
	locatedString->replace(L"\xfeb6", L"\x0634");
	locatedString->replace(L"\xfeb7", L"\x0634");
	locatedString->replace(L"\xfeb8", L"\x0634");
	locatedString->replace(L"\xfeb9", L"\x0635");
	locatedString->replace(L"\xfeba", L"\x0635");
	locatedString->replace(L"\xfebb", L"\x0635");
	locatedString->replace(L"\xfebc", L"\x0635");
	locatedString->replace(L"\xfebd", L"\x0636");
	locatedString->replace(L"\xfebe", L"\x0636");
	locatedString->replace(L"\xfebf", L"\x0636");
	locatedString->replace(L"\xfec0", L"\x0636");
	locatedString->replace(L"\xfec1", L"\x0637");
	locatedString->replace(L"\xfec2", L"\x0637");
	locatedString->replace(L"\xfec3", L"\x0637");
	locatedString->replace(L"\xfec4", L"\x0637");
	locatedString->replace(L"\xfec5", L"\x0638");
	locatedString->replace(L"\xfec6", L"\x0638");
	locatedString->replace(L"\xfec7", L"\x0638");
	locatedString->replace(L"\xfec8", L"\x0638");
	locatedString->replace(L"\xfec9", L"\x0639");
	locatedString->replace(L"\xfeca", L"\x0639");
	locatedString->replace(L"\xfecb", L"\x0639");
	locatedString->replace(L"\xfecc", L"\x0639");
	locatedString->replace(L"\xfecd", L"\x063a");
	locatedString->replace(L"\xfece", L"\x063a");
	locatedString->replace(L"\xfecf", L"\x063a");
	locatedString->replace(L"\xfed0", L"\x063a");
	locatedString->replace(L"\xfed1", L"\x0641");
	locatedString->replace(L"\xfed2", L"\x0641");
	locatedString->replace(L"\xfed3", L"\x0641");
	locatedString->replace(L"\xfed4", L"\x0641");
	locatedString->replace(L"\xfed5", L"\x0642");
	locatedString->replace(L"\xfed6", L"\x0642");
	locatedString->replace(L"\xfed7", L"\x0642");
	locatedString->replace(L"\xfed8", L"\x0642");
	locatedString->replace(L"\xfed9", L"\x0643");
	locatedString->replace(L"\xfeda", L"\x0643");
	locatedString->replace(L"\xfedb", L"\x0643");
	locatedString->replace(L"\xfedc", L"\x0643");
	locatedString->replace(L"\xfedd", L"\x0644");
	locatedString->replace(L"\xfede", L"\x0644");
	locatedString->replace(L"\xfedf", L"\x0644");
	locatedString->replace(L"\xfee0", L"\x0644");
	locatedString->replace(L"\xfee1", L"\x0645");
	locatedString->replace(L"\xfee2", L"\x0645");
	locatedString->replace(L"\xfee3", L"\x0645");
	locatedString->replace(L"\xfee4", L"\x0645");
	locatedString->replace(L"\xfee5", L"\x0646");
	locatedString->replace(L"\xfee6", L"\x0646");
	locatedString->replace(L"\xfee7", L"\x0646");
	locatedString->replace(L"\xfee8", L"\x0646");
	locatedString->replace(L"\xfee9", L"\x0647");
	locatedString->replace(L"\xfeea", L"\x0647");
	locatedString->replace(L"\xfeeb", L"\x0647");
	locatedString->replace(L"\xfeec", L"\x0647");
	locatedString->replace(L"\xfeed", L"\x0648");
	locatedString->replace(L"\xfeee", L"\x0648");
	locatedString->replace(L"\xfeef", L"\x0649");
	locatedString->replace(L"\xfef0", L"\x0649");
	locatedString->replace(L"\xfef1", L"\x064a");
	locatedString->replace(L"\xfef2", L"\x064a");
	locatedString->replace(L"\xfef3", L"\x064a");
	locatedString->replace(L"\xfef4", L"\x064a");
	locatedString->replace(L"\xfef5", L"\x0644\x0627");
	locatedString->replace(L"\xfef6", L"\x0644\x0627");
	locatedString->replace(L"\xfef7", L"\x0644\x0627");
	locatedString->replace(L"\xfef8", L"\x0644\x0627");
	locatedString->replace(L"\xfef9", L"\x0644\x0627");
	locatedString->replace(L"\xfefa", L"\x0644\x0627");
	locatedString->replace(L"\xfefb", L"\x0644\x0627");
	locatedString->replace(L"\xfefc", L"\x0644\x0627");

	/*** Replace Arabic punctuation with English punctuation ***/
	//Arabic percent sign
	locatedString->replace(L"\x066A", L"%");
	//Arabic decimal separator
	locatedString->replace(L"\x066B", L".");
	//Arabic thousand separator
	locatedString->replace(L"\x066C", L",");
	//Arabic comma
	locatedString->replace(L"\x060C", L",");
	//Arabic date separator
	locatedString->replace(L"\x060D", L"/");
	//Arabic semicolon
	locatedString->replace(L"\x061B", L";");
	//Arabic question mark
	locatedString->replace(L"\x060C", L"?");
	//Arabic five pointed star
	locatedString->replace(L"\x066D", L"*");
	
	/*** Replace Arabic digits with English digits ***/
	//Arabic-Indic digit 0
	locatedString->replace(L"\x0660", L"0");
	//Arabic-Indic digit 1
	locatedString->replace(L"\x0661", L"1");
	//Arabic-Indic digit 2
	locatedString->replace(L"\x0662", L"2");
	//Arabic-Indic digit 3
	locatedString->replace(L"\x0663", L"3");
	//Arabic-Indic digit 4
	locatedString->replace(L"\x0664", L"4");
	//Arabic-Indic digit 5
	locatedString->replace(L"\x0665", L"5");
	//Arabic-Indic digit 6
	locatedString->replace(L"\x0666", L"6");
	//Arabic-Indic digit 7
	locatedString->replace(L"\x0667", L"7");
	//Arabic-Indic digit 8
	locatedString->replace(L"\x0668", L"8");
	//Arabic-Indic digit 9
	locatedString->replace(L"\x0669", L"9");

	/*** Replace compound Arabic characters with single Arabic characters ***/
	//alef + madda => alef with madda above
	locatedString->replace(L"\x0627\x0653", L"\x0622");
	//alef + hamza above => alef with hamza above
	locatedString->replace(L"\x0627\x0654", L"\x0623");
	//waw + hamza above => waw with hamza above
	locatedString->replace(L"\x0648\x0654", L"\x0624");
	//alef + hamza below => alef with hamza below
	locatedString->replace(L"\x0627\x0655", L"\x0625");
	//yeh + hamza above => yeh with hamza above
	locatedString->replace(L"\x064A\x0654", L"\x0626");
	//alef-maksura + hamza => yeh with hamza below
	locatedString->replace(L"\x0649\x0621", L"\x0626");
	//yeh + hamza => yeh with hamza above
	locatedString->replace(L"\x064A\x0621", L"\x0626");

	/*** Remove Arabic diacritics ***/
	locatedString->remove(L"\x064B");
	locatedString->remove(L"\x064C");
	locatedString->remove(L"\x064D");
	locatedString->remove(L"\x064E");
	locatedString->remove(L"\x064F");
	locatedString->remove(L"\x0650");
	locatedString->remove(L"\x0651");
	locatedString->remove(L"\x0652");
	//These two below don't look like diacritics. They seem harmless to remove anyway.
	locatedString->remove(L"\xF508");
	locatedString->remove(L"\xF509");

	wchar_t wch;
	
	//word-final yeh => alef maksura
	if(token_is_originally_end_of_word){
		if(locatedString->length() > 0){
			wch = locatedString->charAt(locatedString->length()-1);
			if(wch == 0x064A)
				locatedString->replace(locatedString->length()-1, 1, L"\x0649");
		}
	}

	//non-word-final alef maksura => alef
	for (int index = 0; index + (token_is_originally_end_of_word? 1 : 0) < locatedString->length(); index++) {
		wch = locatedString->charAt(index);
		if(wch == 0x0649){
			locatedString->replace(index, 1, L"\x0627");
		}
	}

	//alef with madda above => alef
	locatedString->replace(L"\x0622", L"\x0627");
	//alef with hamza above => alef
	locatedString->replace(L"\x0623", L"\x0627");
	//alef with hamza below => alef
	locatedString->replace(L"\x0625", L"\x0627");
}
