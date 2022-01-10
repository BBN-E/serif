// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Arabic/partOfSpeech/ar_PartOfSpeechRecognizer.h"

ArabicPartOfSpeechRecognizer::ArabicPartOfSpeechRecognizer(){
}
void ArabicPartOfSpeechRecognizer::resetForNewSentence(){
};
int ArabicPartOfSpeechRecognizer::getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories, 
												TokenSequence* tokenSequence)
{
	LexicalTokenSequence *lexicaTokenSequence = dynamic_cast<LexicalTokenSequence*>(tokenSequence);
	if (!lexicaTokenSequence) throw InternalInconsistencyException("ArabicPartOfSpeechRecognizer::getPartOfSpeechTheories",
		"This ArabicPartOfSpeechRecognizer requires a LexicalTokenSequence.");
	int morpheme1 =0;
	results[0] = _new PartOfSpeechSequence(lexicaTokenSequence);
	wchar_t pos_buffer[1000];
	Symbol pos_tags[500];
	int npos = 0;
	//add the parts of speech note:  while dependencies do exist between the pos for the subparts
	//of an 'original tok' (for instance a preposition could not be attached to a verb), there isn't
	//a clean way to mark these in the theory.  For now assume the parse will pick correctly
	//Do prevent duplicate part of speeches from being added to the theory

	while(morpheme1 <lexicaTokenSequence->getNTokens()){
		const LexicalToken* t = lexicaTokenSequence->getToken(morpheme1);
		int nle = t->getNLexicalEntries();
		/*std::cout<<"getting pos for token: "<<t->getSymbol().to_debug_string()<<" from: "<<
			(lexicaTokenSequence->getOriginalToken(t->getOriginalTokenIndex()))->getSymbol().to_debug_string()<<std::endl;
		if(nle == 0){
			std::cout<<"getting pos for token: "<<t->getSymbol().to_debug_string()<<std::endl;
			std::cout<<"\t\tskipping word with 0 lexical entries: ";
			t->dump(std::cout);
			std::cout<<std::endl;
		}
		*/
		npos = 0;
		int n = 0;
		for(n = 0; n< nle; n++){
			wcscpy(pos_buffer, L"\0");
			LexicalEntry* le = t->getLexicalEntry(n);
			getPOSFromLE(pos_buffer, le, 500);
			//ignore POS that contain "NULL", these were added as unknown stems.
			bool found_null = false;
			for(size_t p = 0; p < wcslen(pos_buffer); p++){
				if(pos_buffer[p] == 'N' && (p+3)< wcslen(pos_buffer)){
					if((pos_buffer[p+1] == 'U') && 
						(pos_buffer[p+2] == 'L') &&
						(pos_buffer[p+3] == 'L')) {
							found_null = true;
							break;
						}
				}
			}
			if(found_null){
				//std::cout<<"\t\tskipping POS with NULL: "<<(static_cast<int>(le->getID()))<<" "
				//	<< Symbol(pos_buffer).to_debug_string()<<std::endl;
				continue;
			}
			if(wcslen(pos_buffer) == 0){
				//std::cout<<"\t\tskipping Empty POS: "<<(static_cast<int>(le->getID()))<<std::endl;
				continue;
			}
			Symbol pos = Symbol(pos_buffer);
			bool matched = false;
			for(int q = 0; q<npos; q++){
				if(pos == pos_tags[q]){
					matched = true;
				}
			}
			if(!matched){
				pos_tags[npos++] = pos;
			}
		}
		for(n = 0; n< npos; n++){
			//std::cout<<"\tadd pos: "<<pos_tags[n].to_debug_string()<<std::endl;
			results[0]->addPOS(pos_tags[n], 1, morpheme1);
		}
		
		morpheme1++;

	}
	
	return 1;
}
void ArabicPartOfSpeechRecognizer::getPOSFromLE(wchar_t* pos_buffer, LexicalEntry *le, int max_len){
	if(le->getNSegments() == 0){
		Symbol pos = (le->getFeatures())->getPartOfSpeech();
		if(pos == Symbol(L"NONE")){
			return;
		}
		if(wcslen(pos_buffer) > 0){
			wcscat(pos_buffer, L"+");
		}
		wcscat(pos_buffer, pos.to_string());
		return;
	}
	
	for(int i =0; i< le->getNSegments(); i++){
		getPOSFromLE(pos_buffer, le->getSegment(i), max_len);
	}
}

