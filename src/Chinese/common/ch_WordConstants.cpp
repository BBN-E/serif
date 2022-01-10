// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/common/ch_WordConstants.h"
#include "Generic/linuxPort/serif_port.h"

// 1st, 2nd and 3rd Person Pronouns
Symbol ChineseWordConstants::I = Symbol(L"\x6211");						// \346\210\221
Symbol ChineseWordConstants::MY = Symbol(L"\x6211\x7684");					// \346\210\221\347\232\204
Symbol ChineseWordConstants::YOU = Symbol(L"\x4f60");						// \344\275\240
Symbol ChineseWordConstants::YOUR = Symbol(L"\x4f60\x7684");				// \344\275\240\347\232\204
Symbol ChineseWordConstants::WE = Symbol(L"\x6211\x4eec");					// \346\210\221\344\273\254
Symbol ChineseWordConstants::OUR = Symbol(L"\x6211\x4eec\x7684");			// \346\210\221\344\273\254\347\232\204
Symbol ChineseWordConstants::YOU_PL = Symbol(L"\x4f60\x4eec");				// \344\275\240\344\273\254
Symbol ChineseWordConstants::YOUR_PL = Symbol(L"\x4f60\x4eec\x7684");		// \344\275\240\344\273\254\347\232\204
Symbol ChineseWordConstants::HE = Symbol(L"\x4ed6");						// \344\273\226
Symbol ChineseWordConstants::HIS = Symbol(L"\x4edc\x7684");				// \344\273\226\347\232\204
Symbol ChineseWordConstants::SHE = Symbol(L"\x5979");						// \345\245\271
Symbol ChineseWordConstants::HER = Symbol(L"\x5979\x7684");				// \345\245\271\347\232\204
Symbol ChineseWordConstants::IT = Symbol(L"\x5b83");						// \345\256\203
Symbol ChineseWordConstants::ITS = Symbol(L"\x5b83\x7684");				// \345\256\203\347\232\204
Symbol ChineseWordConstants::POSS = Symbol(L"\x5176");                     // her/his/its
Symbol ChineseWordConstants::THEY_INANIMATE = Symbol(L"\x5b83\4eec");		// \345\256\203\344\273\254
Symbol ChineseWordConstants::THEY_FEM = Symbol(L"\x5979\x4eec");			// \345\245\271\344\273\254
Symbol ChineseWordConstants::THEY_MASC = Symbol(L"\x4ed6\x4eec");			// \344\273\226\344\273\254
Symbol ChineseWordConstants::THEIR_INANIMATE = Symbol(L"\x5b83\x4eec\x7684"); // \345\256\203\344\273\254\347\232\204
Symbol ChineseWordConstants::THEIR_FEM = Symbol(L"\x5979\x4eec\x7684");	// \345\245\271\344\273\254\347\232\204
Symbol ChineseWordConstants::THEIR_MASC = Symbol(L"\x4edc\x4eec\x7684");	// \344\273\226\344\273\254\347\232\204

// Other Pronominals
Symbol ChineseWordConstants::YOU_FORMAL = Symbol(L"\x60a8");				// \346\202\250
Symbol ChineseWordConstants::THIRD_PERSON = Symbol(L"\x5176");				// \345\205\266
Symbol ChineseWordConstants::WHO = Symbol(L"\x8c01");						// \350\260\201
Symbol ChineseWordConstants::REFLEXIVE = Symbol(L"\x81ea\x5df1");			// \350\201\252\345\267\261
Symbol ChineseWordConstants::COUNTERPART = Symbol(L"\x5bf9\x65b9");		// \345\257\271\346\226\271
Symbol ChineseWordConstants::HERE = Symbol(L"\x8fd9\x91cc");				// \350\277\231\351\207\214
Symbol ChineseWordConstants::THIS = Symbol(L"\x8fd9");						// \350\277\231
Symbol ChineseWordConstants::EACH = Symbol(L"\x5404\x81ea");				// \345\220\204\350\207\252
Symbol ChineseWordConstants::LITERARY_THIRD_PERSON = Symbol(L"\x4e4b");	// \344\271\213
Symbol ChineseWordConstants::ITSELF = Symbol(L"\x672c\x8eab");				// \346\234\254\350\272\253
Symbol ChineseWordConstants::THERE = Symbol(L"\x90a3\x91cc");				// \351\202\243\351\207\214
Symbol ChineseWordConstants::MYSELF = Symbol(L"\x672c\x4eba");				// \346\234\254\344\272\272
Symbol ChineseWordConstants::ONESELF = Symbol(L"\x81ea\x8eab");			// \350\207\252\350\272\253
Symbol ChineseWordConstants::LATTER = Symbol(L"\x540e\x8005");				// \345\220\216\350\200\205

// Titles
Symbol ChineseWordConstants::MASC_TITLE_1 = Symbol(L"\x5148\x751f");		// \345\205\210\347\224\237
Symbol ChineseWordConstants::MASC_TITLE_2 = Symbol(L"\x4e08\x592b");		// \344\270\210\345\244\253
Symbol ChineseWordConstants::MASC_TITLE_3 = Symbol(L"\x516c\x5b50");		// \345\205\254\345\255\220
Symbol ChineseWordConstants::FEM_TITLE_1 = Symbol(L"\x5973\x58eb");		// \345\245\263\345\243\253
Symbol ChineseWordConstants::FEM_TITLE_2 = Symbol(L"\x592a\x592a");		// \345\244\252\345\244\252
Symbol ChineseWordConstants::FEM_TITLE_3 = Symbol(L"\x592b\x4eba");		// \345\244\253\344\272\272
Symbol ChineseWordConstants::FEM_TITLE_4 = Symbol(L"\x516c\x4e3b");		// \345\205\254\344\270\273
Symbol ChineseWordConstants::FEM_TITLE_5 = Symbol(L"\x5c0f\x59d0");		// \345\260\217\345\247\220	

// Partitives
Symbol ChineseWordConstants::ONE_OF = Symbol(L"\x4e4b\x4e00");				// \344\271\213\344\270\200

// Generic Signals
Symbol ChineseWordConstants::MANY1 = Symbol(L"\x5f88\x591a");				// \345\276\210\345\244\232
Symbol ChineseWordConstants::MANY2 = Symbol(L"\x8bb8\x591a");				// \350\256\270\345\244\232
Symbol ChineseWordConstants::QUITE_LARGE =	Symbol(L"\x4e0d\x5c11");		// \344\270\215\345\260\221
Symbol ChineseWordConstants::SOME = Symbol(L"\x4e00\x4e9b");				// \344\270\200\344\272\233
Symbol ChineseWordConstants::APPROXIMATELY = Symbol(L"\x7ea6");			// \347\272\246
Symbol ChineseWordConstants::MOST = Symbol(L"\x5927\x591a\x6570");			// \345\244\247\345\244\232\346\225\260
Symbol ChineseWordConstants::NUMBER =	Symbol(L"\x4e3a\x6570");			// \344\270\272\346\225\260
Symbol ChineseWordConstants::PLURAL =	Symbol(L"\x4eec");					// \344\273\254

// Punctuation
Symbol ChineseWordConstants::CH_SPACE = Symbol(L"\x3000");					// \343\200\200
Symbol ChineseWordConstants::CH_COMMA = Symbol(L"\x3001");
Symbol ChineseWordConstants::ASCII_COMMA = Symbol(L",");
Symbol ChineseWordConstants::NEW_LINE = Symbol(L"\x000A");
Symbol ChineseWordConstants::CARRIAGE_RETURN = Symbol(L"\x000D");
Symbol ChineseWordConstants::EOS_MARK = Symbol(L"\x3002");					// \343\200\202
Symbol ChineseWordConstants::LATIN_STOP = Symbol(L"\x002E");		
Symbol ChineseWordConstants::LATIN_EXCLAMATION = Symbol(L"\x0021");   
Symbol ChineseWordConstants::LATIN_QUESTION = Symbol(L"\x003F");		
Symbol ChineseWordConstants::FULL_STOP = Symbol(L"\xFF0E");				// \357\274\216
Symbol ChineseWordConstants::FULL_COMMA = Symbol(L"\xFF0C");
Symbol ChineseWordConstants::FULL_EXCLAMATION = Symbol(L"\xFF01");			// \357\274\201
Symbol ChineseWordConstants::FULL_QUESTION = Symbol(L"\xFF1F");			// \357\274\237
Symbol ChineseWordConstants::FULL_SEMICOLON = Symbol(L"\xFF1B");
Symbol ChineseWordConstants::FULL_RRB = Symbol(L"\xFF09");					// \357\274\211

Symbol ChineseWordConstants::CLOSE_PUNCT_1 = Symbol(L"\x3009");			// \343\200\211
Symbol ChineseWordConstants::CLOSE_PUNCT_2 = Symbol(L"\x300b");			// \343\200\213
Symbol ChineseWordConstants::CLOSE_PUNCT_3 = Symbol(L"\x300d");			// \343\200\215
Symbol ChineseWordConstants::CLOSE_PUNCT_4 = Symbol(L"\x300f");			// \343\200\217
Symbol ChineseWordConstants::CLOSE_PUNCT_5 = Symbol(L"\x201d");			// \342\200\235


bool ChineseWordConstants::isPossessivePronoun(Symbol word) {
	return (word == HIS ||
			word == HER || 
			word == ITS ||
			word == THEIR_INANIMATE ||
			word == THEIR_FEM ||
			word == THEIR_MASC);
}

bool ChineseWordConstants::is1pPronoun(Symbol word) {
	return (word == I ||
		    word == MY ||
			word == WE ||
			word == OUR);
}

bool ChineseWordConstants::is2pPronoun(Symbol word) {
	return (word == YOU ||
			word == YOUR ||
			word == YOU_PL ||
			word == YOUR_PL);
}

bool ChineseWordConstants::is3pPronoun(Symbol word) {
	return (word == HE ||
			word == HIS ||
			word == SHE ||
			word == HER || 
			word == IT ||
			word == ITS ||
			word == THEY_INANIMATE ||
			word == THEY_FEM ||
			word == THEY_MASC ||
	        word == THEIR_INANIMATE ||
			word == THEIR_FEM ||
			word == THEIR_MASC);
}

bool ChineseWordConstants::isSingular1pPronoun(Symbol word) {
	return (word == I ||
		    word == MY);
}

bool ChineseWordConstants::isOtherPronoun(Symbol word) {
	return (word == YOU_FORMAL ||
			word == THIRD_PERSON ||
			word == WHO ||
			word == REFLEXIVE ||
			word == COUNTERPART ||
			word == HERE ||
			word == THIS ||
			word == LITERARY_THIRD_PERSON ||
			word == ITSELF ||
			word == THERE ||
			word == MYSELF ||
			word == ONESELF ||
			word == LATTER);
}

bool ChineseWordConstants::isSingularPronoun(Symbol word) {
	return (word == I ||
			word == MY ||
			word == YOU ||
			word == YOUR ||
			word == HE ||
			word == HIS ||
			word == SHE ||
			word == HER ||
			word == IT ||
			word == ITS);
}

bool ChineseWordConstants::isPluralPronoun(Symbol word) {
	return (word == WE ||
		    word == OUR ||
			word == YOU_PL ||
			word == YOUR_PL ||
			word == THEY_INANIMATE ||
			word == THEY_FEM ||
			word == THEY_MASC ||
			word == THEIR_INANIMATE ||
			word == THEIR_FEM ||
			word == THEIR_MASC);
}



// disallow linking of 1P and 2P pronouns - JSM 10/25/05
bool ChineseWordConstants::isLinkingPronoun(Symbol word) {
	return !(is1pPronoun(word) || is2pPronoun(word));
}
// Let the pronoun linker make the decision - JSM 10/11/05
//bool ChineseWordConstants::isLinkingPronoun(Symbol word) { return true; }
//bool ChineseWordConstants::isLinkingPronoun(Symbol word) { return isPronoun(word); }

bool ChineseWordConstants::isPartitiveWord(Symbol word) {
	return (word == ONE_OF);
}

bool ChineseWordConstants::isURLCharacter(Symbol word) {
	const wchar_t *ch = word.to_string();
	if (wcslen(ch) != 1)
		return false;
	return (iswascii(ch[0]) != 0);
}

bool ChineseWordConstants::isPhoneCharacter(Symbol word) {
	const wchar_t *ch = word.to_string();
	size_t len = wcslen(ch);
	if (len == 1) 
		return (iswdigit(ch[0]) || ch[0] == L'-');
	else
		return (word == Symbol(L"-LRB-") ||
			    word == Symbol(L"-RRB-"));
}

bool ChineseWordConstants::isASCIINumericCharacter(Symbol word) {
	const wchar_t *ch = word.to_string();
	if (wcslen(ch) != 1)
		return false;
	return (iswdigit(ch[0]) != 0);
}
