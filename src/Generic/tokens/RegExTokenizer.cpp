// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include <wchar.h>
#include <iostream>

#include "Generic/common/ParamReader.h"
#include "Generic/common/limits.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Generic/theories/LexicalToken.h"

#include "Generic/tokens/Tokenizer.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"

#include <string>
#include <vector>


 
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>


#include "Generic/common/RegexMatch.h"
#include "Generic/tokens/RegExTokenizer.h"
#include <boost/scoped_ptr.hpp>



// IMPORTANT NOTE:
// Keep in mind that the LocatedString class uses half-open
// intervals [start, end) while the Metadata and Span classes
// use closed intervals [start, end]. This is an easy source
// of off-by-one errors.

RegExTokenizer::RegExTokenizer(Tokenizer* baseTokenizer)
	: _substitutionMap(NULL), _tokenizer(baseTokenizer),
	  _document(0), _cur_sent_no(0), _create_lexical_tokens(false)
{
	//read in the regular expressions from the specified file
	std::string regexFileName = ParamReader::getRequiredParam("tokenizer_regex_file");
	readRegExFile(regexFileName.c_str());

	// check for tokenizer substitution file (will be used here) 
	std::string file_name = ParamReader::getRequiredParam("tokenizer_subst");
	_substitutionMap = _new SymbolSubstitutionMap(file_name.c_str());

	_create_lexical_tokens = ParamReader::isParamTrue("create_lexical_tokens");

}

/// Resets the state of the tokenizer to prepare for a new sentence.
void RegExTokenizer::resetForNewSentence(const Document *doc, int sent_no){
	_document = doc;
	_cur_sent_no = sent_no;

	_tokenizer->resetForNewSentence(doc, sent_no);
}

/// Reads in the regular expressions from a file
void RegExTokenizer::readRegExFile(const char filename[]){
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& in(*in_scoped_ptr);
	int num_exps;
	
	//get number of Expression sets
	in>>num_exps;
	if(num_exps <= 0)
		throw UnexpectedInputException("RegExpTokenizer::readRegExFile()", "Invalid number of expressions.");
	
	//get end of line after numExpressions
	std::wstring temp;
	in.getLine(temp);
	

	//get blank line after numExpressions
	in.getLine(temp);

	//Get all expressions
	for(int i=0; i<num_exps; i++){
		if(!temp.empty()){
			throw UnexpectedInputException("RegExTokenizer::readRegExFile()", 
						"Regular expression file must contain a blank line after the number of expressions and after each expression/label set.");
		}

		//get Regular Expression
		std::wstring reg_ex;
		boost::wregex tempExpression;
		if(!in.eof())
			in.getLine(reg_ex);
		else{
			char message[500];
			sprintf(message, "Reached end of file before all %d expressions were read.\n", num_exps);
			throw UnexpectedInputException("RegExTokenizer::readRegExFile()", message);
		}
		try{
			tempExpression.assign(reg_ex);
		}
		catch(const boost::regex_error& e)
		{
			char message[500];
			sprintf(message, "Regular expression %d is not valid: %s\n",i+1, e.what());
			throw UnexpectedInputException("RegExTokenizer::applyExpression()", message);
		}
		
			
		//get lines with labels and subexpressions 
		in.getLine(temp);
		while(!temp.empty()){
			std::wstring label = L"";
			int start_sub_exp, end_sub_exp = 0;
			int lineSize = int(temp.size());

			//find label
			size_t pos1 = temp.find(L" ",0);
			if(pos1 == std::wstring::npos){
				char message[500];
				sprintf(message, "Error reading label for expression %d.\n", i+1);
				throw UnexpectedInputException("RegExTokenizer::readRegExFile()", message);
			}
			label = temp.substr(0, pos1);

			//find start subexpression and convert to int
			int pos2 = int(temp.find(L" ",pos1+1));
			std::wstring s_sub_exp;
		
			if(int(pos2) == -1){
				s_sub_exp = temp.substr(pos1+1, lineSize-pos1-1);
				if(s_sub_exp.empty()){
					char message[500];
					sprintf(message,"Error reading subexpression for expression %d.\n",i+1);
					throw UnexpectedInputException("RegExTokenizer::readRegExFile()", message);
				}
				end_sub_exp = -1;
			}
			else{
				s_sub_exp = temp.substr(pos1+1,pos2-pos1-1);
			}
			char str[5];
			wcstombs(str, s_sub_exp.c_str(), 5);
			start_sub_exp = atoi(str);
			if(start_sub_exp < 0){
				throw UnexpectedInputException("RegExTokenizer::readRegExFile()", 
					"Subexpressions must be greater than or equal to 0.");
			}

			//find end subexpression (if exists) and convert to int
			if(end_sub_exp == 0){
				std::wstring e_sub_exp = temp.substr(pos2+1, lineSize-pos2);
				wcstombs(str, e_sub_exp.c_str(), 5);
				end_sub_exp = atoi(str);
			}
			
			//store in _regExStrings	
			RegexData tempRed;
			tempRed.regexVal = reg_ex;
			tempRed.expression = tempExpression;
			tempRed.label = label;
			tempRed.ID  = i+1;

			tempRed.subexp.push_back(start_sub_exp);
			if(end_sub_exp != -1)
				tempRed.subexp.push_back(end_sub_exp);

			_regExStrings.push_back(tempRed);
			
			//get next line
			in.getLine(temp);
		}
			
	}
}

RegExTokenizer::~RegExTokenizer() {
	delete _substitutionMap;
	delete _tokenizer;
	_substitutionMap = 0;
	_tokenizer = 0;
}

/* 
 * When a match is found, this procedure extracts and stores the details 
 *  needed for future processing. 
 */
void RegExTokenizer::regexCallback(const boost::wsmatch& what,	// input
					int sub_string_pos,							// input
					vector<RegexMatch>* matches_p)			// output
{
	/* what[0] contains the whole string */
	/* add found substring, position, and length to matches */
    int position = int(what.position(sub_string_pos));
	
	int length   = int (what[sub_string_pos].str().length());
	wstring value = what[sub_string_pos].str();
	RegexMatch curr_match (value, position, length);
	matches_p->push_back(curr_match);
}

/*
 * Prints debugging information for each match.
 */
void RegExTokenizer::printMatchCallback(const RegexMatch& match)
{
	SessionLogger::info("SERIF") << "\n" << "++++Found value: "<< match.getString()<< " at position: "<< match.getPosition()<< 
	  " with length: "<< match.getLength()<< "."<< endl;
}
/*
 * Applies the given regular expression to the given target text.  Returns a vector of RegexMatch objects.
 */
void RegExTokenizer::applyExpression (const RegexData& expr, const LocatedString * target_text, vector <RegexMatch> &matches)
{
	boost::wregex expression = expr.expression;

    /* get subexpression number(s) */    
	int subExp = expr.subexp.at(0);



	/*get input string from located string */
	wstring lstext = target_text -> toString();

	//Find matches for each desired subexpression 
	//AES 7/10/06: For now just look at first subexpression number not the range. 
	//TODO: Make loop to find all subexpressions within the specified range
	int j = 0;
	// construct our iterators:
	boost::wsregex_iterator regex_it(lstext.begin(), lstext.end(), expression);
	boost::wsregex_iterator regex_end;
	if(regex_it != regex_end){
		if (subExp < 0 || (size_t)subExp >= regex_it->size()) {
			char message[500];
			sprintf(message, "Invalid subexpression index (%d) for expression %d\n",subExp,expr.ID);
			throw UnexpectedInputException("RegExTokenizer::applyExpression()", message);
		}
		for_each(regex_it, regex_end, boost::bind(&regexCallback, _1, subExp, &matches));
	}
}

/*
 * Inputs: - Vector of RegexData objects (representing regular expressions, subexpression numbers, and
 *         labels for tagging)
 *		   - Text over which to search for the regular expression patterns.
 *		   - Vector in which to store offsets of matches.
 *
 * Outputs: - Vector of TokenOffsets (each TokenOffsets object represents a pattern match)
 */
void RegExTokenizer::processRegexps (const vector<RegexData>& expressions, LocatedString *target_text, TokenOffsets *token_offsets)
{
	vector <RegexMatch> matches;

	/* cycle through regular expressions */
	for (unsigned int i = 0; i < expressions.size(); i++) {
		//wcout<<expressions.at(i).regexVal<<expressions.at(i).label<<endl;
		matches.clear();
	    applyExpression(expressions.at(i), target_text, matches);

		/* print out each match for this particular regular expression */
	    //for_each(matches.begin(), matches.end(), boost::bind(&printMatchCallback, _1));

	    /* now we have all the matches for one expression */
	    /* cycle through each instance found for this reg exp */
	    /* record begin/end offsets */
	    int currPos, currLen;
	    for (unsigned int j = 0; j < matches.size(); j++) {
		    currPos = matches.at(j).getPosition();
		    currLen = matches.at(j).getLength();

		    //if (i == 0) {
  				/* immediately store if first regex (since there shouldn't be any stored at this point, so no overlap) */
				token_offsets->addRegexOffset(currPos, currPos+currLen);
			//} 
			//else if (!(token_offsets->overlapsExisting(currPos, currPos+currLen))) {
				/* only store if it doesn't already exist, and isn't first regexp */
				//token_offsets->addRegexOffset(currPos, currPos+currLen);
			//}
			
		}
	}
}

/*
 * Input: LocatedString representing an entire sentence, sentence number to initialize the TokenSequence,
 *        plus other variables that are needed to pass on to the existing Tokenizer.
 * 
 * Output: TokenSequence
 * 
 * Functionality: This function gets a complete sentence as input and returns a new TokenSequence that
 *                combines:
 *					1) Tokens found using user-supplied regular expression patterns
 *				    2) Tokens found by sending the remaining text (after applying 1) through the
 *					   "existing" (rules + "break on whitespace") Tokenizer.
 *
 */
int RegExTokenizer::getTokenTheories(TokenSequence **results, int max_theories,
									 const LocatedString *string, 
									 bool beginOfSentence,
									 bool endOfSentence)
{
	// Make a local copy of the sentence string
	// MEM: make sure it gets deleted!
	LocatedString  *localString = _new LocatedString(*string);
	TokenOffsets *token_offsets = _new TokenOffsets();

	
	

	processRegexps(_regExStrings, localString, token_offsets);

	/* Get untokenized text only if regular expression matches were found */
	if (token_offsets->numRegExpMatches() > 0) {

		int	token_index = 0;

		token_offsets->getUntokenizedOffsets(0, localString->length()-1);

		/*// debug 
		cout << "\nFinally, here's the whole sequence: ";
		token_offsets->printInfo();
		// end debug*/
		
		
		/* So now we have access to a vector of pointers to all portions of the sentence */
		/* Either create tokens or send the untokenized portion to the existing tokenizer */
		offset_object *tempOO;
		
		for (int i = 0; i < token_offsets->size(); i++) {
			tempOO = token_offsets->at(i);

			/* if the current offset_object was found during regexp processing */
			if (tempOO->getRegexBool()) {

				/* make a token out of the regex match */

				/* First, check the token size */
				if ((tempOO->getEnd() - tempOO->getBegin()) > MAX_TOKEN_SIZE) {
					SessionLogger::warn("token_too_big") 
						<< "Number of characters in token exceeds limit of " << MAX_TOKEN_SIZE;

					/* Used MAX_TOKEN_SIZE-1 because when you just use MAX_TOKEN_SIZE, the last
					 * character will get cut of.  I think this is because during Token assignment,
					 * there are only (MAX_TOKEN_SIZE-1) slots available because of the last "\0". */
					tempOO->setEnd(tempOO->getBegin() + (MAX_TOKEN_SIZE-1));

					/* need to readjust untokenized offsets here */
					token_offsets->getUntokenizedOffsets(0, localString->length()-1);
					//token_offsets->printInfo();
				}

				/* temporary buffer to get a symbol for each character */
				wchar_t chars[2];
				chars[1] = L'\0';

				/* Resulting buffer that exists after all symbols have been replaced */
				/* We do 10 * the max size because we can substitute characters with multiple 
				   characters through the _substitutionMap */
				wchar_t temp_buffer[10 * MAX_TOKEN_SIZE + 1]; 
				temp_buffer[0] = L'\0';

				/* Copy each character into a temp buffer and create a Symbol. */
				for (int j = tempOO->getBegin(); j < tempOO->getEnd(); j++) {
					chars[0] = localString->charAt(j);

					Symbol sub = _substitutionMap->replace(Symbol(chars));
	
					/* concatenates existing "symbol-ized" buffer with new symbol */
					wcscat(temp_buffer, sub.to_string());
				}

				Symbol sym = Symbol(temp_buffer);

				/* Create a token */
				Token *nextToken = 0;
				
				if (_create_lexical_tokens) 
					nextToken = _new LexicalToken(localString, tempOO->getBegin(), tempOO->getEnd()-1, sym);
				else
					nextToken = _new Token(localString, tempOO->getBegin(), tempOO->getEnd()-1, sym);
				//Token *nextToken = _new Token(localString->start<EDTOffset>(tempOO->getBegin()), localString->end<EDTOffset>(tempOO->getEnd()-1),
				//	localString->start<ASRTime>(tempOO->getBegin()), localString->end<ASRTime>(tempOO->getEnd()-1), sym);

				/* Add token to the token buffer */
				if (nextToken != NULL) {
					_tokenBuffer[token_index] = nextToken;
					token_index++;
				}
			} else {
				/* the current offset_object is still untokenized; send the text to the existing tokenizer */
				LocatedString *tempsubstring = localString->substring(tempOO->getBegin(), tempOO->getEnd());
				
				/* If the text is empty, skip to the next offset_object */
				tempsubstring->trim();
				if (tempsubstring->length() == 0)
					continue;

				/* Booleans indicate whether this portion of the sentence starts with the beginning of the
				 * sentence or ends with the end of the sentence. */
				bool isBegin = false, isEnd = false;

				if (tempOO->getBegin() == 0) {
					isBegin = true;
				}

				if (tempOO->getEnd() == localString->length()) {
					isEnd = true;
				}

				/* Call modified version of the current Tokenizer */
				_tokenizer->getTokenTheories(results, max_theories, tempsubstring, isBegin, isEnd);
				
				
				int numTokens = (*results)->getNTokens();
				Token *addToken;

				/* Add all found tokens to the token buffer */
				for (int k = 0; k < numTokens; k++) {
					addToken = const_cast<Token *>((*results)->getToken(k));

					if (addToken != NULL) {
						_tokenBuffer[token_index] = addToken;
						token_index++;
					}
				}
			}
		}

		/* after for loop, copy the finished TokenSequence to results */
		if (_create_lexical_tokens)
			results[0] = _new LexicalTokenSequence(_cur_sent_no, token_index, _tokenBuffer);
		else
			results[0] = _new TokenSequence(_cur_sent_no, token_index, _tokenBuffer);

		delete localString;
		return 1;
	} else {
		/* process as normal if regex matches were not found */
		delete localString;
		return _tokenizer->getTokenTheories(results, max_theories, string, true, true);
	}
}

