// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/OutputUtil.h"
#include "boost/regex.hpp"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/SymbolUtilities.h"

#include "Generic/discourseRel/TargetConnectives.h"
#include "Generic/discourseRel/StopWordFilter.h"
#include <boost/scoped_ptr.hpp>

// static member initialization
int StopWordFilter::_num_words = 0;
StopWordFilter::StopWordDict* StopWordFilter::_stopword_dict = 0;
StopWordFilter::FilteredWordDict* StopWordFilter::_filteredword_dict = 0;

void StopWordFilter::loadStopWordDict (const char *filename){
	
	boost::scoped_ptr<UTF8InputStream> wordListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& wordListStream(*wordListStream_scoped_ptr);	
	UTF8Token token;
	
	if (_stopword_dict == 0)
	{
		//Allocate Memory for the explicit connective Hash Table
	    _stopword_dict   = _new StopWordDict();
		
	}

	while (!wordListStream.eof()) {
		wordListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		
		char wordBuffer[501];
		Symbol lowerCaseWordSymbol=SymbolUtilities::lowercaseSymbol(token.symValue());
		StringTransliterator::transliterateToEnglish(wordBuffer, lowerCaseWordSymbol.to_string(), 500);
		string word = wordBuffer;
		(* _stopword_dict)[word]=1;

		_num_words ++ ;
	}
	
}
		
int StopWordFilter::isInStopWordDict (string word){
	map <string, int>::iterator myIterator = _stopword_dict->find(word);

    if(myIterator != _stopword_dict->end())
    {
        return true;
	}else{
		// search for words such as 's, n't 
		if (word.find("'") != std::string::npos || word == "not"){
			return true;
		}else{
			return false;
		}
	}
}

void StopWordFilter::initFilteredWordDict(){
	_filteredword_dict = _new map<string, int>;
}

void StopWordFilter::showFilteredWords (UTF8OutputStream& outStream){
	outStream << "words that were filtered out from wordpair features: \n" ;
	map <string, int>::iterator myIterator;
	for (myIterator = (* _filteredword_dict).begin(); myIterator != (* _filteredword_dict).end(); myIterator++){
		outStream << ((* myIterator).first).c_str() << ", " <<  (* myIterator).second << "\n" ;
	}
}

void StopWordFilter::recordFilteredWords (string word){
	map <string, int>::iterator myIterator = _filteredword_dict->find(word);

    if(myIterator == _filteredword_dict->end())
    {
        (* _filteredword_dict)[word] = 1;
	}else{
		(* _filteredword_dict)[word] += 1;
	}

}

void StopWordFilter::finalize (){
	if (_stopword_dict != 0 ){
		delete _stopword_dict;
		_stopword_dict = 0;
	}

	if (_filteredword_dict != 0){
		delete _filteredword_dict;
		_filteredword_dict = 0;
	}
}

