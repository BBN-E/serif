// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/SessionLogger.h"

CorrectMention::CorrectMention() { 
	nameSpan = 0;
	sentence_number = -1;
	printed = false;
	start = EDTOffset();
	end = EDTOffset();
	head_start = EDTOffset();
	head_end = EDTOffset();
}

void CorrectMention::loadFromSexp(Sexp* mentionSexp) 
{
	size_t num_children = mentionSexp->getNumChildren();
	if (num_children != 7) 
		throw UnexpectedInputException("CorrectMention::loadFromSexp()",
		"mentionSexp must have exactly 7 children in:", mentionSexp->to_debug_string().c_str());

	Sexp* typeSexp = mentionSexp->getNthChild(0);
	Sexp* roleSexp = mentionSexp->getNthChild(1);
	Sexp* startSexp = mentionSexp->getNthChild(2);
	Sexp* endSexp = mentionSexp->getNthChild(3);
	Sexp* headStartSexp = mentionSexp->getNthChild(4);
	Sexp* headEndSexp = mentionSexp->getNthChild(5);
	Sexp* idSexp = mentionSexp->getNthChild(6);

	if (!typeSexp->isAtom() || !startSexp->isAtom() || !endSexp->isAtom() ||
		!headStartSexp->isAtom() || !headEndSexp->isAtom() || !idSexp->isAtom() ||
		!roleSexp->isAtom()) 
			throw UnexpectedInputException("CorrectMention::loadFromSexp()",
									       "Didn't find mention atoms in correctAnswerSexp");

	correctMentionType = typeSexp->getValue();

	if (isName()) 
		mentionType = Mention::NAME;
	else if (isPronoun())
		mentionType = Mention::PRON;
	else if (isDateTime())
		mentionType = Mention::NAME;
	else if (isNominal())
		mentionType = Mention::DESC;
	else if (isUndifferentiatedPremod())
		mentionType = Mention::NONE;
	else mentionType = Mention::DESC;
	

	role = roleSexp->getValue();

	start = EDTOffset(_wtoi(startSexp->getValue().to_string()));
	end = EDTOffset(_wtoi(endSexp->getValue().to_string()));
	head_start = EDTOffset(_wtoi(headStartSexp->getValue().to_string()));
	head_end = EDTOffset(_wtoi(headEndSexp->getValue().to_string()));
	annotationID = idSexp->getValue();

}

/*void CorrectMention::loadFromAdept(EntityMention* mentionAdept) 
{
	size_t num_children = mentionSexp->getNumChildren();
	if (num_children != 7) 
		throw UnexpectedInputException("CorrectMention::loadFromSexp()",
		"mentionSexp must have exactly 7 children in:", mentionSexp->to_debug_string().c_str());

	Sexp* typeSexp = mentionSexp->getNthChild(0);
	Sexp* roleSexp = mentionSexp->getNthChild(1);
	Sexp* startSexp = mentionSexp->getNthChild(2);
	Sexp* endSexp = mentionSexp->getNthChild(3);
	Sexp* headStartSexp = mentionSexp->getNthChild(4);
	Sexp* headEndSexp = mentionSexp->getNthChild(5);
	Sexp* idSexp = mentionSexp->getNthChild(6);

	if (!typeSexp->isAtom() || !startSexp->isAtom() || !endSexp->isAtom() ||
		!headStartSexp->isAtom() || !headEndSexp->isAtom() || !idSexp->isAtom() ||
		!roleSexp->isAtom()) 
			throw UnexpectedInputException("CorrectMention::loadFromSexp()",
									       "Didn't find mention atoms in correctAnswerSexp");

	correctMentionType = Symbol(string_to_wstring(mentionAdept.mentionType.type));
    long id = mentionAdept.mentionId;
	
	std::string idString;
    std::stringstream strstream;
    strstream << id;
    strstream >> idString;
	
	annotationID =Symbol(string_to_wstring(idString));

	setCharOffset();

	if (isName_ERE()) 
		mentionType = Mention::NAME;
	else if (isPronoun_ERE())
		mentionType = Mention::PRON;
	else if (isNominal_ERE())
		mentionType = Mention::DESC;
	else if (isUndefined_ERE())
		mentionType = Mention::NONE;

	role = Symbol(L"NONE");

	IdentifyMentionHead()


}
*/
/*void CorrectMention::setCharOffset(){

	    int begin = mentionAdept.tokenOffset.beginIndex;
		int charBegin = tokenOffsetBegin_to_CharOffsetBegin(begin);
        int end = mentionAdept.tokenOffset.endIndex;
        int charEnd = tokenOffsetEnd_to_CharOffsetEnd(end);


		start = EDTOffset(charBegin);
	    end = EDTOffset(charEnd);
}

int CorrectMention::tokenOffsetBegin_to_CharOffsetBegin(int tokenOffsetBegin){
    TokenStream tokenStream= mentionAdept.tokenStream;
	vector<Token> tokens = tokenStream.tokenList;
	Token tokenBegin = tokens[tokenOffsetBegin];
	return tokenBegin.charOffset.beginIndex;
}

int CorrectMention::tokenOffsetEnd_to_CharOffsetEnd(int tokenOffsetEnd){
    TokenStream tokenStream= mentionAdept.tokenStream;
	vector<Token> tokens = tokenStream.tokenList;
	Token tokenEnd = tokens[tokenOffsetEnd];
	return tokenEnd.charOffset.beginEnd;
}

void CorrectMention::IdentifyMentionHead(EntityMention* mentionAdept){
	if (isName_ERE() || end-start==1 ) {
	 head_start = start;
	 head_end = end;
	}
	
	else{
	setNoneNameHeadOffset(mentionAdept)
	}
} 


bool CorrectMention::setNoneNameHeadOffset(EntityMention* mentionAdept){
     

	 TokenStream tokenStream= mentionAdept.tokenStream;
	 vector<Token> tokens = tokenStream.tokenList;
	
     int begin = mentionAdept.tokenOffset.beginIndex;
	 int end = mentionAdept.tokenOffset.endIndex;
	 for(int i=begin;i<end;i++){
		 if(props.find(tokens[i].value)){ //find the proposition, the word before it should be the head
			 int charBegin=tokenOffsetBegin_to_CharOffsetBegin(i-1)
			 int charEnd=tokenOffsetEnd_to_CharOffsetEnd(i-1)

		
			head_start = EDTOffset(charBegin);
	        head_end = EDTOffset(charEnd);
		   break;
		 }
	 }

}
*/
bool CorrectMention::setStartAndEndTokens(TokenSequence *tokenSequence) 
{
	if (!CorrectAnswers::isWithinTokenSequence(tokenSequence, head_start, head_end)) return false;

	int new_head_start_token, new_head_end_token, new_start_token, new_end_token;

	bool matched_head = 
		CorrectAnswers::findTokens(tokenSequence, head_start, head_end, new_head_start_token, new_head_end_token);

	/*
	int num_tokens = tokenSequence->getNTokens();
	std::cout << "XX sent:" 
			  << tokenSequence->getSentenceNumber() << " head:"
			  << head_start << "-" << head_end << " extent:" 
			  << start << "-" << end << " sent:" 
			  << tokenSequence->getToken(0)->getStartEDTOffset() << "-"
			  << tokenSequence->getToken(num_tokens - 1)->getEndEDTOffset()
			  << std::endl;
	*/

	bool matched_extent = 
		CorrectAnswers::findTokens(tokenSequence, start, end, new_start_token, new_end_token);

	// Only update our variables if we get a good match.
	if (matched_head && matched_extent) {
		start_token = new_start_token;
		end_token = new_end_token;
		head_start_token = new_head_start_token;
		head_end_token = new_head_end_token;
		return true;
	} else {
		return false;
	}
}

void CorrectMention::addSystemMentionId(Mention::UID id) {
	system_mention_ids.push_back(id);
}

int CorrectMention::getNSystemMentionIds() {
	return static_cast<int>(system_mention_ids.size());
}

Mention::UID CorrectMention::getSystemMentionId(int index) {
	if (index < 0 || index >= getNSystemMentionIds())
		throw InternalInconsistencyException::arrayIndexException("CorrectMention::getSystemMentionId()",
																getNSystemMentionIds(),
																index);
	return system_mention_ids.at(index);
}

