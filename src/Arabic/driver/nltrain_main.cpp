// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/nltrain/NameLinkTrainer.h"
#include "Generic/common/UTF8InputStream.h"
#include <wchar.h>
#include <iostream>
#include <string>
//debugging
#include "Generic/common/NGramScoreTable.h"
#include <boost/scoped_ptr.hpp>



const MAX_DOC_LENGTH = 2000; //TODO: remove this

int main(int argc, char **argv) {
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	UTF8XMLToken* toks =_new UTF8XMLToken[MAX_DOC_LENGTH+1];
	NameLinkTrainer* nlTrainer = _new NameLinkTrainer();
	char* infile ="c:\\SERIF\\data\\Arabic\\nltraintests\\arabic-train-0";
	char* outfile ="c:\\SERIF\\data\\Arabic\\nltraintests\\generic-arabic-train-0";
	//char* infile= argv[1];
	//char* outfile = argv[2];
	uis.open(infile);
	int tokCount=0;
	int startTag=-1;
	while(true){
		while((tokCount < MAX_DOC_LENGTH)&& !uis.eof()){
			uis >> toks[tokCount];
			tokCount++;
		}
		startTag = nlTrainer->LearnDoc(toks, tokCount, startTag);
		if(tokCount == MAX_DOC_LENGTH){
			 tokCount=0;
		}
		else{
			break;
		}
	}
	uis.close();
	nlTrainer->PrintTable(outfile);

	//read the table back in for debugging
	/*
	boost::scoped_ptr<UTF8InputStream> uis2_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis2(*uis2_scoped_ptr);
	std::cout<<"READ TABLE IN "<<argv[2]<<std::endl;
	uis2.open(argv[2]);
	NgramScoreTable* tab = new NgramScoreTable(2,uis2 );
	NgramScoreTable::Table::iterator it=  tab->get_start();
	for (it ; it != tab->get_end() ; ++it) {
		std::cout <<(*it).first[0].to_debug_string();
		std::cout<<"  ";
		std::cout << (*it).first[1].to_debug_string()<<" ";
		std::cout << (*it).second;
		std::cout << endl;
	}
	std::cout <<"done";
	uis2.close();
	*/
}
	
	

		

	
