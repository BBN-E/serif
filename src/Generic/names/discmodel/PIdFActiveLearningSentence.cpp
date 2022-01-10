// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/names/discmodel/PIdFActiveLearningSentence.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"




void PIdFActiveLearningSentence::populate(Symbol id, LocatedString* sentenceString, 
									 TokenSequence* tokens, PIdFSentence* sentence )
{
	_sentId = id;
	const wchar_t* idstr = id.to_string();
	int len = static_cast<int>(wcslen(idstr));
	int i;
	for(i =len-1; i>=0; i--){

		if(idstr[i] ==L'-'){
			break;
		}
	}
	if(i == -1){
		throw UnexpectedInputException(
			"PIdFActiveLearningSentence::populate",
			"Sentence id does not have docid-sentid format.");
	}
	int dash = i;
	wchar_t buffer[500];
	for(i=0; (i < dash) && (i<499); i++){
		buffer[i] = idstr[i];
	}
	buffer[i] =L'\0';
	_docId = Symbol(buffer);
	int j=0;
	for(i = dash+1; (i < len) && (j <499) ; i++){
		buffer[j++] = idstr[i];
		if(!iswdigit(idstr[i])){
			throw UnexpectedInputException(
				"PIdFActiveLearningSentence::populate",
				"sentence number portion of sentenceid is not a number");
		}

	}
	buffer[j] = L'\0';

	_sentNum = _wtoi(buffer);



	_sentenceString = sentenceString;
	_tokens = tokens;
	_sentence = sentence;
	_isFirstSent = false;
	_isLastSent = false;
	_in_training_set = false;

}
void PIdFActiveLearningSentence::populate(const PIdFActiveLearningSentence& other ){

	_sentId = other._sentId;
	_docId = other._docId;
	_sentNum = other._sentNum;
	_sentenceString = other._sentenceString;

	_tokens = other._tokens;

	_sentence = other._sentence;
	_isFirstSent = other._isFirstSent;
	_isLastSent = other._isLastSent;
	_in_training_set = other._in_training_set;

}

void PIdFActiveLearningSentence::deleteMembers(){
	_sentId = Symbol();
	_docId = Symbol();
	_sentNum = -1;

	delete _sentenceString;
	delete _tokens;
	delete _sentence;
	_sentenceString = 0;
	_tokens = 0;
	_sentence = 0;
	_isFirstSent = false;
	_isLastSent = false;
	_in_training_set = false;
}

void PIdFActiveLearningSentence::WriteTokenSentence(UTF8OutputStream& uos, Symbol id, TokenSequence* tokens) {
	uos << L"<SENTENCE ID=\"" << id.to_string() << L"\">\n";
	for (int i = 0; i < tokens->getNTokens(); i++) {
		const Token * token = tokens->getToken(i);
		uos << L"<TOKEN START=\"" << token->getStartEDTOffset() << L"\" END=\"" << token->getEndEDTOffset() << L"\">";
		uos << token->getSymbol().to_string();
		uos << L"</TOKEN>\n";
	}
	uos << L"</SENTENCE>\n";
}

void PIdFActiveLearningSentence::WriteSentence(UTF8OutputStream& uos, DTTagSet* tagSet){
	/*
	  <SENTENCE ID=”sentence_id”>
	<DISPLAY_TEXT>This is the sentence text.</DISPLAY_TEXT>
    <ANNOTATION TYPE=”PER” START_OFFSET=”5” END_OFFSET=”8” SOURCE=”Example1”/>
    <ANNOTATION TYPE=”GPE” START_OFFSET=”21” END_OFFSET=”27” SOURCE=”Example1”/>
  </SENTENCE>
	*/
	int nonest = tagSet->getNoneTagIndex();
	int noneco = tagSet->getTagIndex(tagSet->getNoneCOTag());
	int starttag = tagSet->getStartTagIndex();
	int endtag = tagSet->getEndTagIndex();
	uos<<"<SENTENCE ID=\""<<_sentId.to_string()<<"\">\n";
	uos<<"  <DISPLAY_TEXT>"<<_sentenceString->toString()<<"</DISPLAY_TEXT>\n";
	int thistok =0;
	while(thistok < _sentence->getLength()){
		if(!((_sentence->getTag(thistok) == noneco) ||
			(_sentence->getTag(thistok) == nonest)||
			(_sentence->getTag(thistok) == starttag) ||
			(_sentence->getTag(thistok) == endtag)))
		{
				int startoff = _tokens->getToken(thistok)->getStartEDTOffset().value();
				int endoff = _tokens->getToken(thistok)->getEndEDTOffset().value();
				int starttok = thistok;
				Symbol thisredtype = tagSet->getReducedTagSymbol(_sentence->getTag(thistok));

				thistok++;
				while((thistok < _sentence->getLength()) &&
					(thisredtype == tagSet->getReducedTagSymbol(_sentence->getTag(thistok))) &&
					tagSet->isCOTag(_sentence->getTag(thistok)))
				{
					endoff = _tokens->getToken(thistok)->getEndEDTOffset().value();
					thistok++;
					
				}
				uos<<"  <ANNOTATION TYPE=\""<<thisredtype.to_string()
					<<"\" START_OFFSET=\""<<startoff<<"\" END_OFFSET=\""<<endoff<<"\"/>\n";
		}
		else{
			thistok++;
		}
	}
	uos<<"</SENTENCE>\n";
}




void PIdFActiveLearningSentence::AppendSentenceBody(std::wstring& str, DTTagSet* tagSet, bool includeannotation){
		/*
	<DISPLAY_TEXT>This is the sentence text.</DISPLAY_TEXT>
    <ANNOTATION TYPE=”PER” START_OFFSET=”5” END_OFFSET=”8” SOURCE=”Example1”/>
    <ANNOTATION TYPE=”GPE” START_OFFSET=”21” END_OFFSET=”27” SOURCE=”Example1”/>
	*/

	str+= L"  <DISPLAY_TEXT>";
	str+= _sentenceString->toString();
	str+= L"</DISPLAY_TEXT>\n";
	if(!includeannotation)
		return;

	//append the annotation as well
	int nonest = tagSet->getNoneTagIndex();
	int noneco = tagSet->getTagIndex(tagSet->getNoneCOTag());
	int starttag = tagSet->getStartTagIndex();
	int endtag = tagSet->getEndTagIndex();


	int thistok =0;
	wchar_t numbuff[500];
	while(thistok < _sentence->getLength()){
		if(!((_sentence->getTag(thistok) == noneco) ||
			(_sentence->getTag(thistok) == nonest)||
			(_sentence->getTag(thistok) == starttag) ||
			(_sentence->getTag(thistok) == endtag) ||
			(_sentence->getTag(thistok) == -1)))
		{
				int startoff = _tokens->getToken(thistok)->getStartEDTOffset().value();
				int endoff = _tokens->getToken(thistok)->getEndEDTOffset().value();
				int starttok = thistok;
				Symbol thisredtype = tagSet->getReducedTagSymbol(_sentence->getTag(thistok));
				thistok++;

				while((thistok < _sentence->getLength()) &&
					(thisredtype == tagSet->getReducedTagSymbol(_sentence->getTag(thistok))) &&
					tagSet->isCOTag(_sentence->getTag(thistok)))
				{
					endoff = _tokens->getToken(thistok)->getEndEDTOffset().value();
					thistok++;
				}
				str+= L"  <ANNOTATION TYPE=\"";
				str+= thisredtype.to_string();
				str+= L"\" START_OFFSET=\"";
#ifdef _WIN32
				_itow(startoff, numbuff, 10);
#else
				swprintf (numbuff, sizeof(numbuff)/sizeof(numbuff[0]), L"%d", startoff);
#endif
				str+= numbuff;
				str+= L"\" END_OFFSET=\"";
#ifdef _WIN32
				_itow(endoff, numbuff, 10);
#else
				swprintf (numbuff, sizeof(numbuff)/sizeof(numbuff[0]), L"%d", endoff);
#endif
				str+= numbuff;
				str+= L"\"/>\n";
		}
		else{
			thistok++;
		}
	}
}

void PIdFActiveLearningSentence::AppendSentenceStart(std::wstring& str){
/*	  <SENTENCE ID=”sentence_id”>
*/
	str+= L"<SENTENCE ID=\"";
	str+= _sentId.to_string();
	str+= L"\">\n";
}

void PIdFActiveLearningSentence::AppendSentenceEnd(std::wstring& str){
/*	  <SENTENCE ID=”sentence_id”>
*/
	str+= L"</SENTENCE>\n"; 
}
void PIdFActiveLearningSentence::AppendSentence(std::wstring& str, 
												DTTagSet* tagSet, bool includeannotation)
{
	AppendSentenceStart(str);
	AppendSentenceBody(str, tagSet, includeannotation);
	AppendSentenceEnd(str);
}
void PIdFActiveLearningSentence::AppendSentenceWithContext(std::wstring& str, 
												DTTagSet* tagSet, PIdFActiveLearningSentence** context, int ncontext, 
												bool includeannotation)
{
	AppendSentenceStart(str);
	AppendSentenceBody(str, tagSet, includeannotation);
	int half = ncontext/2;
	str+= L"<CONTEXT>\n";
	str+= L"<PREV>";

	int i;
	for(i=0; i< half; i++){
		if(context[i] == 0)
			continue;
		else{
			str+= context[i]->_sentenceString->toString();
		}
	}
	str+= L"</PREV>\n";
	str+= L"<POST>";

	for(i=half; i< ncontext; i++){
		if(context[i] == 0)
			continue;
		else{
			str+= context[i]->_sentenceString->toString();
		}
	}
	str+= L"</POST>\n";

	str+= L"</CONTEXT>\n";

	AppendSentenceEnd(str);
}

