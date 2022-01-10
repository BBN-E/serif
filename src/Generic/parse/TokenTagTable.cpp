// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/parse/TokenTagTable.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/parse/ParserTags.h"

const float TokenTagTable::targetLoadingFactor = static_cast<float>(0.7);


TokenTagTable::TokenTagTable(UTF8InputStream& in)
{
    int numEntries;
    int numBuckets;
    UTF8Token token;
    Symbol word;
	Symbol tag;
	Symbol afterTwo;
	int keySize;
	widestKey = 0;
	int seenEntries = 0;

    in >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
    table = _new Table(numBuckets);
	for (int i = 0; i < numEntries; i++) {
        in >> token;
        if (in.eof()) break;
        if (token.symValue() != ParserTags::leftParen)
			throw UnexpectedInputException("TokenTagTable::()",
                "ERROR: missing left paren when reading token tag table");
        in >> token;
        word = token.symValue();
        in >> token;
        tag = token.symValue();  
        in >> token;
		afterTwo = token.symValue();
		while (afterTwo != ParserTags::rightParen){
           // must build a compound key
			wchar_t char_buf[MAX_TOKEN_SIZE+1];
			wchar_t wc;
			const wchar_t * wstring = word.to_string();
			int ic = 0;
			int jsiz = 0;
			while ((wc = wstring[ic++]) != 0){
				char_buf[jsiz++] = wc;
				if (jsiz > MAX_TOKEN_SIZE-3) 
					 throw UnexpectedInputException("TokenTagTable::()",
						"ERROR: compound token too big while reading token tag table");
			}
			char_buf[jsiz++] = L' ';
			wstring = tag.to_string();
			ic = 0;
			while ((wc = wstring[ic++]) != 0){
				char_buf[jsiz++] = wc;
				if (jsiz >= MAX_TOKEN_SIZE) 
					 throw UnexpectedInputException("TokenTagTable::()",
						"ERROR: compound token too big while reading token tag table");
			}
			char_buf[jsiz] = L'\0';
			word = Symbol(char_buf);
			tag = afterTwo;
			in >> token;
			if (in.eof()) 	
				 throw UnexpectedInputException("TokenTagTable::()",
                "ERROR: missing right paren when reading token tag table");
			afterTwo = token.symValue();
		}

		keySize = (int)wcslen(word.to_string());
		if (widestKey < keySize) widestKey = keySize;
        (*table)[word] = tag;
		seenEntries++;
	}
	///std::cerr<<"Loaded TokenTagTable from stream; got "<<seenEntries<<" of reserved "<<numEntries<<"\n";
}

const Symbol TokenTagTable::lookup(Symbol word) const
{
    Table::iterator iter;

    iter = table->find(word);
    if (iter == table->end()) 
        return Symbol();
    else  return (Symbol)((*iter).second);
}

TokenTagTable::~TokenTagTable(){
	if (table != NULL) delete table;
}
