// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEXPREADER_H
#define SEXPREADER_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include <boost/scoped_ptr.hpp>

// bare bones class for reading data from a Sexp file. See underneath this class definition for
// recommended usage notes.
class SexpReader {
public:
	SexpReader();
	~SexpReader();
	SexpReader(const char *file_name);
	void openFile(const char *file_name);
	void closeFile();

	// get...() functions: each of these functions reads one token from the stream; if that token is not
	// of the desired type (e.g. left-parenthesis), an exception is thrown. This largely frees the client 
	// from error-checking the Sexp file.
	void getLeftParen() throw(UnexpectedInputException);
	void getRightParen() throw(UnexpectedInputException);
	UTF8Token getWord() throw(UnexpectedInputException);
	void getEOF() throw(UnexpectedInputException);
	UTF8Token getNonEOF() throw(UnexpectedInputException);
	int getSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException);

	// getToken(): inputs an OR'd bunch of flags representing valid input token types.
	UTF8Token getToken(int specifier);

	// returns true if there is more than just whitespace between here and EOF
	bool hasMoreTokens();
	
	void pushBack(UTF8Token token);
	
	// this function exists in case the client wishes to handle exceptions differently (e.g. to print the 
	// name of a derived reader class rather than simply SexpReader). 
	virtual void throwUnexpectedInputException(char *funcName, char *errDescrip);

	
	//the following functions allow us to make multi-step reads transactional
	void startRead();
	void readFailed();
	void readSucceeded();

	// NOTE: this method replaced by a param read of "sexp-read-debug"
//	void setDebugStream(char *file) { _debugOut.openFile(file); _debugOut << "hi!"; }

protected:
	boost::scoped_ptr<UTF8InputStream> _file;
	enum TokenType {
		LPAREN = 1 << 0,
		RPAREN = 1 << 1,
		EOFTOKEN = 1 << 2,
		WORD   = 1 << 3};
	GrowableArray <UTF8Token> _tokenCache;

	//the following two structures allow us to roll back failed transactions
	GrowableArray <UTF8Token> _rollbackCache;
	GrowableArray <int> _rollbackIndices;
	DebugStream _debugOut;

};

// Recommended usage:
//		- break down your parsing task into a hierarchy of subtasks, so that each subtask is responsible for
//		  populating a data structure according to data from the stream. Some subtasks may be designated
//		  "optional."
//		- associate each subtask to a method named getFoo() (Foo being whatever structure the subtask aims
//		  to populate), stick these methods in a reader class derived from SexpReader. These functions should
//		  *throw an exception* if a complete Foo is not read in.
//		- have one master getFoo() method that reads in the whole document and returns the full data structure.
//		- for optional subtasks, have a getOptionalFoo() method, which returns whether it read a Foo() as 
//		  well as the Foo it read (if it exists). These should generally not throw any exceptions since 
//		  the empty string "fulfills" the function.


#endif
