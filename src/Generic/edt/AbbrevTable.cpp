// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/edt/AbbrevTable.h"
#include <boost/scoped_ptr.hpp>

DebugStream AbbrevTable::_debugOut;
SymbolArraySymbolArrayMap *AbbrevTable::_table[2];
bool AbbrevTable::is_initialized = false;

// load in the static abbreviations table from disk, initialize DYNAMIC to empty
void AbbrevTable::initialize() {
	if (is_initialized)
		return;

	std::string filename = ParamReader::getRequiredParam("abbrev_maker_file");
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
	UTF8InputStream& instream(*instream_scoped_ptr);
	FileReader reader(instream);

	const int MAX_SYMBOLS = 16;
	Symbol array[MAX_SYMBOLS];
	int nArray = 0;
	SymbolArray *newKey, *newValue;

	//initialize hash maps
	_table[STATIC] = _new SymbolArraySymbolArrayMap(300);
	_table[DYNAMIC] = _new SymbolArraySymbolArrayMap(300);

	_debugOut.init(Symbol(L"abbrev_table_debug_file"));
	while(reader.hasMoreTokens()) {
        reader.getLeftParen();
		
		nArray = reader.getSymbolArray(array, MAX_SYMBOLS);
		newValue = _new SymbolArray(array, nArray);
		
		_debugOut << "found VALUE: ";
		for(int i=0; i<nArray; i++) 
			_debugOut << array[i].to_debug_string() << " ";
		_debugOut << "\n";
		
		nArray = reader.getSymbolArray(array, MAX_SYMBOLS);
		newKey = _new SymbolArray(array, nArray);
		if(!add(newKey, newValue, STATIC))
			delete newKey;
		_debugOut << "      KEY: ";
		for(int j=0; j<nArray; j++) 
			_debugOut << array[j].to_debug_string() << " ";
		_debugOut << "\n";


		while((nArray = reader.getOptionalSymbolArray(array, MAX_SYMBOLS))!= 0) {
			_debugOut << "      KEY: ";
			for(int i=0; i<nArray; i++) 
				_debugOut << array[i].to_debug_string() << " ";
			_debugOut << "\n";
			
			newValue = _new SymbolArray(*newValue);
			newKey = _new SymbolArray(array, nArray);
			if(!add(newKey, newValue, STATIC)) 
				delete newKey;
		}
		reader.getRightParen();
	}
	
	is_initialized = true;
}

// destroy all objects owned by AbbrevTable
void AbbrevTable::destroy() {

	if (!is_initialized) {
		throw InternalInconsistencyException("AbbrevTable::destroy()",
											 "AbbrevTable has not been initialized.");
	}

	//delete stuff
	SymbolArraySymbolArrayMap::iterator iter;
	for (iter = _table[STATIC]->begin();
		 iter != _table[STATIC]->end();
		 ++iter)
	{
		delete (*iter).first;
		delete (*iter).second;
	}
	delete _table[STATIC];

	for (iter = _table[DYNAMIC]->begin();
		 iter != _table[DYNAMIC]->end();
		 ++iter)
	{
		delete (*iter).first;
		delete (*iter).second;
	}
	delete _table[DYNAMIC];

	is_initialized = false;
}

// Reset dynamic table when we begin a new document; the previous document's context is obviously no longer helpful
void AbbrevTable::cleanUpAfterDocument() {
	if (!is_initialized) {
		throw InternalInconsistencyException("AbbrevTable::cleanUpAfterDocument()",
											 "AbbrevTable has not been initialized.");
	}

	SymbolArraySymbolArrayMap::iterator iter;
	for (iter = _table[DYNAMIC]->begin();
		 iter != _table[DYNAMIC]->end();
		 ++iter)
	{
		delete (*iter).first;
		delete (*iter).second;
	}
	_table[DYNAMIC]->clear();
}

// resolve whole mention bindings, and then word-by-word bindings.
// 7/21/04: Fixed a bug. Now whole-mention bindings aren't the only thing that happen (MRK)
int AbbrevTable::resolveSymbols(const Symbol words[], int nWords, Symbol results[], int max_results) {
	
	if (!is_initialized) {
		throw InternalInconsistencyException("AbbrevTable::resolveSymbols()",
											 "AbbrevTable has not been initialized.");
	}

	//resolve whole-mention bindings first
	int nResults = 0;

	_debugOut << "unresolved: ";
	for(int i=0; i<nWords; i++) 
		_debugOut << words[i].to_debug_string() << " ";
	_debugOut << "\n";

	nResults = lookupPhrase(words, nWords, results, max_results);
	if(nResults > 0) {
		//found a whole-mention binding
		_debugOut << "resolved  : ";
		for(int i=0; i<nResults; i++) 
			_debugOut << results[i].to_debug_string() << " ";
		_debugOut << "\n\n";

		return nResults;
	}
	else {
		for (int i=0; i<nWords && nResults<max_results; i++) {
			// lookup this word, if 0 results then simply add on the word itself
			int thisNResults = lookupPhrase(&words[i], 1, &results[nResults], max_results-nResults);
			if(thisNResults == 0) {
				results[nResults++] = words[i];
			}
			else 
				nResults += thisNResults;
		}
	}
			_debugOut << "resolved  : ";
		for(int j=0; j<nResults; j++) 
			_debugOut << results[j].to_debug_string() << " ";
		_debugOut << "\n\n";


	return nResults;
}

// attempt to resolve the whole phrase as a unit, or else return 0
int AbbrevTable::lookupPhrase(const Symbol words[], int nWords, Symbol results[], int max_results){
	int nResults = 0;
	SymbolArray *key, *value, **result;
	key = _new SymbolArray(words, nWords);
	result = (*_table[STATIC]).get(key);
	if(result == NULL) 
		result = (*_table[DYNAMIC]).get(key);
	if(result != NULL) {
		value = *result;
		for(int i=0; i<value->getLength() && nResults<max_results; i++)
			results[nResults++] = value->getArray()[i];
	}
	delete key;
	return nResults;
}

//this is viewable from the outside, so it must be a dynamic add
void AbbrevTable::add(Symbol key[], int nKey, Symbol value[], int nValue) {
	if (!is_initialized) {
		throw InternalInconsistencyException("AbbrevTable::add()",
											 "AbbrevTable has not been initialized.");
	}

	SymbolArray *keyArray, *valueArray;
	keyArray = _new SymbolArray(key, nKey);
	valueArray = _new SymbolArray(value, nValue);
	if(!add(keyArray, valueArray, DYNAMIC)) 
		delete keyArray;
}

// This is a private function which can add to either table
bool AbbrevTable::add(SymbolArray *key, SymbolArray *value, TIndex index) {
//	if((*_table[index])[key]== NULL) {
	if((*_table[index]).get(key) == NULL) {
		(*_table[index])[key] = value;
		return true;
	}
	else {
		SymbolArray *oldValue = (*_table[index])[key];
		(*_table[index])[key] = value;
		delete oldValue;
		return false;
	}
}

//FILE READING FUNCTIONS
AbbrevTable::FileReader::FileReader(UTF8InputStream & file) : _file(file), _nTokenCache(0) { }

void AbbrevTable::FileReader::getLeftParen() throw(UnexpectedInputException) {
	/*
	UTF8Token token = getToken();
	if(token.symValue != SymbolConstants::leftParen)
		throw UnexpectedInputException("AbbrevTable::FileReader::getLeftParen()", "left paren not found");
	*/
	getToken(LPAREN);
}

void AbbrevTable::FileReader::getRightParen() throw(UnexpectedInputException) {
	/*
	UTF8Token token = getToken();
	if(token.symValue != SymbolConstants::rightParen)
		throw UnexpectedInputException("AbbrevTable::FileReader::getRightParen()", "right paren not found");
		*/
	getToken(RPAREN);
}

UTF8Token AbbrevTable::FileReader::getNonEOF() throw(UnexpectedInputException) {
	/*
	if(!hasMoreTokens())
		throw UnexpectedInputException("AbbrevTable::FileReader::getNonEOF()", "EOF encountered");
	*/
	return getToken(LPAREN | RPAREN | WORD);
}

int AbbrevTable::FileReader::getSymbolArray(Symbol results[], int max_results) 
throw(UnexpectedInputException)
{
	UTF8Token token = getToken(LPAREN | WORD);
	int nResults = 0;
	if(token.symValue() == SymbolConstants::leftParen)
		while((token = getToken(RPAREN | WORD)).symValue() != SymbolConstants::rightParen)
			results[nResults++] = token.symValue();
	else results[nResults++] = token.symValue();
	return nResults;
}

int AbbrevTable::FileReader::getOptionalSymbolArray(Symbol results[], int max_results) 
throw(UnexpectedInputException)
{
	UTF8Token token = getToken(LPAREN | WORD | RPAREN);
	int nResults = 0;
	if(token.symValue() == SymbolConstants::rightParen) 
		pushBack(token);
	else if(token.symValue() == SymbolConstants::leftParen)
		while((token = getToken(RPAREN | WORD)).symValue() != SymbolConstants::rightParen)
			results[nResults++] = token.symValue();
	else results[nResults++] = token.symValue();
	return nResults;
}

UTF8Token AbbrevTable::FileReader::getToken(int specifier) {
	UTF8Token token;
	if(_nTokenCache > 0)
		token = _tokenCache[--_nTokenCache];
	else _file >> token;

	if(token.symValue() == SymbolConstants::leftParen) {
		if(!(specifier & LPAREN))
			throw UnexpectedInputException("AbbrevTable::FileReader::getToken()", "\'(\' not expected.");
	} 
	else if(token.symValue() == SymbolConstants::rightParen) {
		if(!(specifier & RPAREN))
			throw UnexpectedInputException("AbbrevTable::FileReader::getToken()", "\')\' not expected.");
	}
	else if(token.symValue() == Symbol(L"")) {
		if(!(specifier & EOFTOKEN))
			throw UnexpectedInputException("AbbrevTable::FileReader::getToken()", "EOF not expected.");
	}
	else if(!(specifier & WORD))
		throw UnexpectedInputException("AbbrevTable::FileReader::getToken()", "Word not expected.");

	return token;
}

bool AbbrevTable::FileReader::hasMoreTokens() {
	if(_nTokenCache > 0) return true;
	UTF8Token token;
	_file >> token;
	if(token.symValue() == Symbol(L""))
		return false;
	else {
		if(_nTokenCache < MAX_CACHE)
			_tokenCache[_nTokenCache++] = token;
		return true;
	}
}

void AbbrevTable::FileReader::pushBack(UTF8Token token) {
	if(_nTokenCache < MAX_CACHE)
		_tokenCache[_nTokenCache++] = token;
}
