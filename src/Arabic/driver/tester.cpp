// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Arabic/sentences/ar_SentenceBreaker.h"
#include "Generic/theories/Sentence.h"
#include "Arabic/tokens/ar_Tokenizer.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include <wchar.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/scoped_ptr.hpp>
const MAX_DOC_LENGTH = 20000; //TODO: remove this
const MAX_SENTENCES = 100;
const THIS_EOF=0; 
void debugMsg(std::string s){
	std::cout << s <<std::endl;
}
int main(int argc, char **argv) {
	std::cout<<"version 1"<<std::endl;
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	if(argc <1){
		std::cerr<<"ArabicSerif.exe should be called with 1 arguments- the param file";
		return 0;
	}
	std::cout<<argv[1]<<std::endl;
	try {
		ParamReader::readParamFile(argv[1]);
		char inputFile[100];
		char outputFile[100];
		if(!ParamReader::getNarrowParam(inputFile, Symbol(L"test_input"),100)){
											std::cerr<<"Can't Find Input File";
											return -1;
										}
		
		if(!ParamReader::getNarrowParam(outputFile, Symbol(L"test_output"),100)){
											std::cerr<<"Can't Find Output File";
											return -1;
										}
	
		//char* inputFile ="c:\\Serif/data/test1/EngSent.sgm";
		//char* outputFile ="c:\\Serif/data/test1/EngSent.out";
		uis.open(inputFile);
		wchar_t text[MAX_DOC_LENGTH+1];
		wchar_t ch;
		int cnt = 0;
		while (((ch = uis.get()) != THIS_EOF) && (cnt < MAX_DOC_LENGTH)) {
			text[cnt++] = ch;	
		}
		std::cout<< cnt<< " characters"<<" MAX IS: "<<MAX_DOC_LENGTH<<std::endl;
		uis.close();


		text[cnt] = L'\0';

		LocatedString *lString = _new LocatedString(text, 0);
		ArabicSentenceBreaker breaker;
		Sentence** sentenceSequence = _new Sentence*[MAX_SENTENCES];

		breaker.resetForNewDocument(0);
		int n_sentences = breaker.getSentences(sentenceSequence, MAX_SENTENCES, &lString, 1);
		std::cout<< "got all sentences"<<std::endl;;
		UTF8OutputStream uos;
		uos.open(outputFile);
		std::cout << n_sentences <<std::endl;
		LocatedString* sent;
		ArabicTokenizer* tk = _new ArabicTokenizer();
		TokenSequence** toks = _new TokenSequence*[1];	//only one token theory!
		std::cout<<"Tokenization\n";
		for (int i = 0; i < n_sentences; i++) {
			sent = sentenceSequence[i]->getString();
			tk->resetForNewSentence(0, i);
			tk->getTokenTheories(toks,1,sent);
			int numToks = toks[0]->getNTokens();
			for(int j=0; j<numToks; j++){
				const wchar_t* str = toks[0]->getToken(j)->getSymbol().to_string();
				int k=0;
				int len =(int)wcslen(str);	
				uos.write(str, len);
				uos.put(L' ');
			}
			uos.put(L'\n');
		}
		uos.close();
		return 0;
	}
	catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << "\n";
	//	HeapChecker::checkHeap("main(); About to exit due to error");
	}

}

