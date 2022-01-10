// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "Generic/tokens/Tokenizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"

using namespace std;

class Processor
{
public:
	Processor(char* param_file);
	~Processor();
	wstring createAnalyzedCorpus(char* corpus_file, char* analyzed_corpus_file);
	// Reads a sentence from a stream
	// static since it is also used in QuickLearn
	static std::wstring getSentenceTextFromStream(UTF8InputStream &in); 

private:

	class NameOffset{
	public:
			Symbol type;
			EDTOffset startoffset;
			EDTOffset endoffset;
	};

	wstring getRetVal(bool ok, const wchar_t* txt =L"");
	wstring getRetVal(bool ok, const char* txt );
	
/*
	*   We parse XML like data (display string can have &, >, and < so it is not actually xml)
	*	with out using an XML parser.  
	*	
	*
	*/
	//Read a single <SENTENCE>....<SENTENCE>training sentences from saved file
	//	  (training file or corpus file)
	//    Use this when loading a saved project
	int readSavedSentence(UTF8InputStream &in, LocatedString*& sentString, Symbol& sentId, NameOffset nameInfo[]);

	//Read all sentences from GUI's strings
	static int findNextMatchingChar(const wchar_t* txt, int start, wchar_t c);

	// find the offset of the first  instance of c after start, throw exception if not found
	static int findNextMatchingCharRequired(const wchar_t* txt, int start, wchar_t c);

	// parse <ANNOTATION .... /> into type, start_offset, end_offset.  
	// Return false if string does not start with <ANNOTATION
	bool parseAnnotationLine(const wchar_t* line, Symbol& type, EDTOffset& start_offset, EDTOffset& end_offset);

	// split origTok into 1,2, or 3 tokens as specified by namestart and nameend
	// put the new tokens in newTokens, return the number of new tokens
	int splitNameTokens(const Token* origTok, EDTOffset namestart, EDTOffset nameend, const LocatedString *sentenceString, Token** newTokens);
	int splitNameTokens(const Token* origTok, int inName[], const NameOffset nameInfo[], const LocatedString *sentenceString, Token** newTokens);

	//return the token number that contains offset
	int getTokenFromOffset(const TokenSequence* tok, EDTOffset offset);

	//return the first token number that contains offset
	int getStartTokenFromOffset(const TokenSequence* tok, EDTOffset offset);

	//return the last token number that contains offset
	int getEndTokenFromOffset(const TokenSequence* tok, EDTOffset offset);

	//put substr of str from start to end in substr
	void wcsubstr(const wchar_t* str, int start, int end, wchar_t* substr);

	//adjust the token theory so that name boundaries are one token boundaries, return a new token sequence
	TokenSequence* adjustTokenizationToNames(const TokenSequence* toks, const NameOffset nameInfo[], int nann, const LocatedString* sentenceString);

	void addAnalyzedSentence(UTF8OutputStream &out, Symbol id,	LocatedString* sentenceString, 
											const NameOffset* nameInfo, int nname);

	Tokenizer* _tokenizer;
	class MorphologicalAnalyzer *_morphAnalyzer;
	class MorphSelector *_morphSelector;
	char _corpus_file[500];
	char _analyzed_corpus_file[500];


	int _n_tokens;
	int _n_tokens_adjusted;

};
#endif
