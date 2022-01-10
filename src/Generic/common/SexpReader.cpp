// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SexpReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolConstants.h"

SexpReader::SexpReader() : _tokenCache(100), _rollbackCache(100),
	_rollbackIndices(16), _debugOut(Symbol(L"sexp_read_debug")), _file(UTF8InputStream::build()) { }

SexpReader::SexpReader(const char *file_name) : _file(UTF8InputStream::build(file_name)), _tokenCache(100), _rollbackCache(100),
	_rollbackIndices(16), _debugOut(Symbol(L"sexp_read_debug")) { }

void SexpReader::openFile(const char *file_name) {
	// first we need to clean up from a possible already-open file
	closeFile();
	_file->open(file_name);
}

void SexpReader::closeFile() {
	if (_file->is_open()) {
		_file->close();
		_file->clear();
	}
	_tokenCache.setLength(0);
	_rollbackIndices.setLength(0);
	_rollbackCache.setLength(0);
}

SexpReader::~SexpReader() {
	_file->close();
	_file->clear();
	if(_rollbackIndices.length() != 0) {
		//should be a warning
	}
}

void SexpReader::getLeftParen() throw(UnexpectedInputException) {
	/*
	UTF8Token token = getToken();
	if(token.symValue != SymbolConstants::leftParen)
		throw UnexpectedInputException("SexpReader::getLeftParen()", "left paren not found");
	*/
	getToken(LPAREN);
}

void SexpReader::getRightParen() throw(UnexpectedInputException) {
	/*
	UTF8Token token = getToken();
	if(token.symValue != SymbolConstants::rightParen)
		throw UnexpectedInputException("SexpReader::getRightParen()", "right paren not found");
		*/
	getToken(RPAREN);
}

UTF8Token SexpReader::getNonEOF() throw(UnexpectedInputException) {
	/*
	if(!hasMoreTokens())
		throw UnexpectedInputException("SexpReader::getNonEOF()", "EOF encountered");
	*/
	return getToken(LPAREN | RPAREN | WORD);
}

UTF8Token SexpReader::getWord() throw(UnexpectedInputException) {
	return getToken(WORD);
}

void SexpReader::getEOF() throw(UnexpectedInputException) {
	getToken(EOF);
}

UTF8Token SexpReader::getToken(int specifier) {
	UTF8Token token;
	if(_tokenCache.length() > 0)
		token = _tokenCache.removeLast();
	else (*_file) >> token;
	
	_debugOut << "<" << token.chars() << ">   ";

	if(token.symValue() == SymbolConstants::leftParen) {
		if(!(specifier & LPAREN)) {
			pushBack(token);
			throw UnexpectedInputException("SexpReader::getToken()", "\'(\' not expected.");
		}
	} 
	else if(token.symValue() == SymbolConstants::rightParen) {
		if(!(specifier & RPAREN)) {
			pushBack(token);
			throw UnexpectedInputException("SexpReader::getToken()", "\')\' not expected.");
		} 
	}
	else if(token.symValue() == Symbol(L"")) {
		if(!(specifier & EOFTOKEN)) {
			pushBack(token);
			throw UnexpectedInputException("SexpReader::getToken()", "EOF not expected.");
		}
	}
	else if(!(specifier & WORD)) {
		pushBack(token);
		throw UnexpectedInputException("SexpReader::getToken()", "Word not expected.");
	}

    if(_rollbackIndices.length()!= 0)
		_rollbackCache.add(token);
	return token;
}

bool SexpReader::hasMoreTokens() {
	if(_tokenCache.length() > 0) return true;
	UTF8Token token;
	(*_file) >> token;
	if(token.symValue() == Symbol(L""))
		return false;
	else {
		pushBack(token);
		return true;
	}
}

void SexpReader::pushBack(UTF8Token token) {
	_tokenCache.add(token);
	_debugOut << "\n" << "      [Push " << token.chars() << "]  ";
}

void SexpReader::startRead() {
	//_debugOut << "\n" << "   startRead():  " << "\n" << "   ";
	_rollbackIndices.add(_rollbackCache.length());
}

void SexpReader::readFailed() {
	//_debugOut << "\n" << "   readFailed():   " << "\n";
	if(_rollbackIndices.length() == 0)
		throw InternalInconsistencyException("SexpReader::readFailed()", "No matching startRead() called.");
	int desiredLength = _rollbackIndices.removeLast();
	while(_rollbackCache.length() != desiredLength)
		pushBack(_rollbackCache.removeLast());
}

void SexpReader::readSucceeded() {
	//_debugOut << "\n" << "   readSucceeded():   " << "\n";
	if(_rollbackIndices.length() == 0)
		throw InternalInconsistencyException("SexpReader::readSucceeded()", "No matching startRead() called.");
	_rollbackIndices.removeLast();
	if(_rollbackIndices.length() == 0)
		_rollbackCache.setLength(0);
}

int SexpReader::getSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException)
{
	UTF8Token token = getToken(LPAREN | WORD);
	int nResults = 0;
	if(token.symValue() == SymbolConstants::leftParen)
		while((token = getToken(RPAREN | WORD)).symValue() != SymbolConstants::rightParen && nResults<max_results)
			results[nResults++] = token.symValue();
	else results[nResults++] = token.symValue();
	return nResults;
}

void SexpReader::throwUnexpectedInputException(char *funcName, char *errDescrip) {
	throw UnexpectedInputException(funcName, errDescrip);
}
