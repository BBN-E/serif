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
#include <boost/scoped_ptr.hpp>

// static member initialization
int TargetConnectives::_num_words = 0;
TargetConnectives::ExplicitConnectiveDict* TargetConnectives::_connective_dict = 0;

void TargetConnectives::loadConnDict (const char *filename){
	
	boost::scoped_ptr<UTF8InputStream> wordListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& wordListStream(*wordListStream_scoped_ptr);	
	UTF8Token token;
	
	if (_connective_dict == 0)
	{
		//Allocate Memory for the explicit connective Hash Table
	    _connective_dict   = _new ExplicitConnectiveDict();
		
	}

	while (!wordListStream.eof()) {
		wordListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		
		char wordBuffer[501];
		Symbol lowerCaseWordSymbol=SymbolUtilities::lowercaseSymbol(token.symValue());
		StringTransliterator::transliterateToEnglish(wordBuffer, lowerCaseWordSymbol.to_string(), 500);
		string word = wordBuffer;
		(* _connective_dict)[word]=1;

		_num_words ++ ;
	}
	
}
		
int TargetConnectives::isInConnDict (string word){
	map <string, int>::iterator myIterator = _connective_dict->find(word);

    if(myIterator != _connective_dict->end())
    {
        return true;
	}else{
		return false;
	}
}

void TargetConnectives::finalize (){
	if (_connective_dict != 0 ){
		delete _connective_dict;
		_connective_dict = 0;
	}
}
