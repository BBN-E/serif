// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/nltrain/NameLinkTrainer.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"
NameLinkTrainer::NameLinkTrainer(){
	_name_counts=_new NgramScoreTable(2, INITIAL_TABLE_SIZE);
	_type_counts=_new NgramScoreTable(1, INITIAL_TABLE_SIZE);
	_uniq_counts=_new NgramScoreTable(1, INITIAL_TABLE_SIZE);
	_tags[PER] = Symbol(L"<ENAMEX TYPE=\"PERSON\">");
	_labels[PER] = Symbol(L"PER");
	_tags[ORG] = Symbol(L"<ENAMEX TYPE=\"ORGANIZATION\">");
	_labels[ORG] = Symbol(L"ORG");
	_tags[LOC] = Symbol(L"<ENAMEX TYPE=\"LOCATION\">");
	_labels[LOC] = Symbol(L"LOC");
	_tags[FAC] = Symbol(L"<ENAMEX TYPE=\"FACILITY\">");
	_labels[FAC] = Symbol(L"FAC");
	_tags[GPE] = Symbol(L"<ENAMEX TYPE=\"GPE\">");
	_labels[GPE] = Symbol(L"GPE");
	_end_tag = Symbol(L"</ENAMEX>");

	
}

int NameLinkTrainer::LearnDoc(UTF8XMLToken* tokens, int numTok){
	return LearnDoc(tokens, numTok, -1);
}


//if you stopped reading a document in the middle!

int NameLinkTrainer::LearnDoc(UTF8XMLToken* tokens, int numTok, int startLab){

	int i, j;
	int currLab =startLab;
	int isWord;
	Symbol thisWord;
	Symbol* tabEntry = _new Symbol[2];
	Symbol* typeEntry = _new Symbol[1];
	for(i=0; i<numTok; i++){
		thisWord = tokens[i].SubstSymValue(); 
		isWord =1;
		for(j=0; j<6; j++){
			if(thisWord ==_tags[j]){
				currLab = j;
				isWord =0;
				break;
			}
		}
		if(thisWord == _end_tag){
			currLab = -1;
			isWord = 0;
		}
		if(isWord && currLab >=0){

			tabEntry[0]=_labels[currLab];
			tabEntry[1]=thisWord;

			typeEntry[0]=_labels[currLab];
			_type_counts->add(typeEntry);
			if(!_name_counts->lookup(tabEntry))
				_uniq_counts->add(typeEntry);			
			_name_counts->add(tabEntry);
		}	
	}
	return currLab;
};

void NameLinkTrainer::PrintTable(char* filename){
	UTF8OutputStream out;
	out.open(filename);
	//_name_counts->print(filename);
	int n = 2;
	NgramScoreTable::Table::iterator iter;
	int size = _name_counts->get_size();
	Symbol* typeGram = _new Symbol[1];
	out << size;
	out << "\n";
	int numType;
	int numUniq;

	for (iter = _name_counts->get_start() ; iter != _name_counts->get_end() ; ++iter) {
		
		out << "((";
		out << (*iter).first[0].to_string();
		out << " ";
		out << (*iter).first[1].to_string();
		out << ") ";
		out << (*iter).second;
		Symbol type = (*iter).first[0];
		typeGram[0]=type;
		numType = _type_counts->lookup(typeGram);
		numUniq = _uniq_counts->lookup(typeGram);
        out <<" ";
		out <<numType;
		out <<" ";
		out <<numUniq;
		out << ")";
		out << "\n";
	}
};
