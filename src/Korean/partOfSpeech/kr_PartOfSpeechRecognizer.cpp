// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/Symbol.h"
#include "theories/TokenSequence.h"
#include "partOfSpeech/KoreanPartOfSpeechRecognizer.h"

int KoreanPartOfSpeechRecognizer::getPartOfSpeechTheories(PartOfSpeechSequence **results, 
													int max_theories, 
													TokenSequence* tokenSequence)
{
	wchar_t pos_buffer[500];
	Symbol pos_tags[500];
	int morph_num = 0;

	results[0] = _new PartOfSpeechSequence(tokenSequence->getSentenceNumber(), 
											tokenSequence->getNTokens());

	//add the parts of speech note:  while dependencies do exist between the pos for the subparts
	//of an 'original tok' (for instance a preposition could not be attached to a verb), there isn't
	//a clean way to mark these in the theory.  For now assume the parse will pick correctly
	//Do prevent duplicate part of speeches from being added to the theory

	while (morph_num < tokenSequence->getNTokens()) {
		const Token* t = tokenSequence->getToken(morph_num);
		int nle = t->getNLexicalEntries(); 
		int npos = 0;
		/*
		std::cout << "getting pos for token: " << t->getSymbol().to_debug_string() << " from: " 
			<< (tokenSequence->getOriginalToken(t->getOriginalTokenIndex()))->getSymbol().to_debug_string() << std::endl;
		if (nle == 0) {
			std::cout << "getting pos for token: " << t->getSymbol().to_debug_string() << std::endl;
			std::cout << "\t\tskipping word with 0 lexical entries: ";
			t->dump(std::cout);
			std::cout<<std::endl;
		}
		*/
		
		for (int n = 0; n < nle; n++) {
			LexicalEntry* le = t->getLexicalEntry(n);
			
			// We've already determined the full tokenization, 
			// so there can't be >1 part of speech per token
			if (le->getNSegments() > 1) continue;  

			wcscpy(pos_buffer, L"\0");
			getPOSFromLexicalEntry(pos_buffer, le, 500);
			
			//ignore POS that contain "NULL", these were added as unknown stems.
			bool found_null = false;
			for (size_t p = 0; p < wcslen(pos_buffer); p++) {
				if (pos_buffer[p]   == 'N' &&  (p + 3) < wcslen(pos_buffer)&&
					pos_buffer[p+1] == 'U' && 
					pos_buffer[p+2] == 'L' &&
					pos_buffer[p+3] == 'L' ) 
				{
					found_null = true;
					break;
				}
			}
			if (found_null) {
				//std::cout << "\t\tskipping POS with NULL: " << (static_cast<int>(le->getID())) << " "
				//	<< Symbol(pos_buffer).to_debug_string() << std::endl;
				continue;
			}
			if (wcslen(pos_buffer) == 0) {
				//std::cout << "\t\tskipping Empty POS: " << (static_cast<int>(le->getID())) << std::endl;
				continue;
			}

			Symbol pos = Symbol(pos_buffer);
			bool matched = false;
			for (int q = 0; q < npos; q++) {
				if (pos == pos_tags[q]) 
					matched = true;
			}
			if (!matched) 
				pos_tags[npos++] = pos;
		}
		for (int m = 0; m < npos; m++) {
			//std::cout << "\tadd pos: " << pos_tags[m].to_debug_string() << std::endl;
			results[0]->addPOS(pos_tags[m], (float)1 / npos, morph_num);
		}
		morph_num++;
	}
	return 1;
}

void KoreanPartOfSpeechRecognizer::getPOSFromLexicalEntry(wchar_t* pos_buffer, 
													LexicalEntry *le, 
													int max_len)
{
	if (le->getNSegments() == 0) {
		Symbol pos = (le->getFeatures())->getPartOfSpeech();
		int len = static_cast<int>(wcslen(pos_buffer));
		if (pos == Symbol(L"NONE"))
			return;
		if (len > 0 && len < (max_len - 1)) {
			wcscat(pos_buffer, L"+");
			len++;
		}
		if (max_len - len - 1 > 0)
			wcsncat(pos_buffer, pos.to_string(), (max_len - len - 1));
		return;
	}
	
	for (int i = 0; i < le->getNSegments(); i++)
		getPOSFromLexicalEntry(pos_buffer, le->getSegment(i), max_len);
}
