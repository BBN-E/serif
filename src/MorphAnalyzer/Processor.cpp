// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/tokens/Untokenizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "MorphAnalyzer/Processor.h"
#include <time.h>
#include <boost/scoped_ptr.hpp>

#if defined(WIN32) || defined(WIN64) 
#define snprintf _snprintf 
#endif

#define MAX_NAMES_PER_SENTENCE 100
#define MAX_NAMES_PER_SPACE_SEP_TOK 5

Processor::Processor(char* param_file): _tokenizer(0), _morphAnalyzer(0), _morphSelector(0),
										_n_tokens(0), _n_tokens_adjusted(0)
{
	ParamReader::readParamFile(param_file);

	std::string log_file = ParamReader::getRequiredParam("quicklearn_log");
	std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
	SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(), 0 ,0);

	//intialize tokenization components
	_tokenizer = Tokenizer::build();

	_morphAnalyzer = MorphologicalAnalyzer::build();
	_morphSelector = _new MorphSelector();

}

Processor::~Processor (){
	delete _tokenizer;
	delete _morphAnalyzer;
	delete _morphSelector;
}

wstring Processor::createAnalyzedCorpus(char* corpus_file, char* analyzed_corpus_file){
	// Get start time
	time_t start_time;
	time(&start_time);

	char errmsg[600];
	try {
		UTF8OutputStream outstream;
		outstream.open(analyzed_corpus_file);

		boost::scoped_ptr<UTF8InputStream> corpusstream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& corpusstream(*corpusstream_scoped_ptr);
		corpusstream.open(corpus_file);
		if(corpusstream.fail()){
			snprintf(errmsg, 599, "Processor::ReadCorpus(), Can't read corpus file: %s", corpus_file);
			return getRetVal(false, errmsg);
		}
		//get the root
		UTF8Token tok;
		NameOffset nameInfo[MAX_NAMES_PER_SENTENCE];
		LocatedString* sentenceString;

		int n_cache_docs = 0;
		int n_total_sent = 0;
		while(!corpusstream.eof()){
			Symbol sentId;
			corpusstream >> tok;	//<doc
			int nsent = 0;
			if(wcsncmp(tok.chars(), L"<DOC", 4) ==0){
					corpusstream >> tok; // docid="ID"
					Document* doc = _new Document(tok.symValue());
					int nname = readSavedSentence(corpusstream, sentenceString, sentId, nameInfo);
					while(nname != -1){
						//tokenizer has to be reset fore every new sentence
						_tokenizer->resetForNewSentence(doc, nsent);
						addAnalyzedSentence(outstream, sentId, sentenceString, nameInfo, nname);
						nsent++;
						n_total_sent++;
						delete sentenceString;
						nname = readSavedSentence(corpusstream, sentenceString, sentId, nameInfo);
					}
					delete doc;
					n_cache_docs++;
					if (n_cache_docs > 50) {
						SessionLexicon::getInstance().getLexicon()->clearDynamicEntries();
						n_cache_docs = 1;
					}
			}
		}
		//cout << "\nNumber of tokens adjusted to match name boundaries: " << _n_tokens_adjusted;
		//cout << "\nNumber of total tokens: " << _n_tokens << "\n";

		// Report time
		time_t end_time;
		time(&end_time);
		double total_time = difftime(end_time, start_time);
		std::cout << "Elapsed time " << total_time << " seconds.\n";

		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		return getRetVal(false, e.getMessage());
	}


}

void Processor::addAnalyzedSentence(UTF8OutputStream &out, Symbol id, LocatedString* sentenceString,
											const NameOffset* nameInfo, int nname) {
	TokenSequence* fixedTokenSequence;
	TokenSequence* tempTheory;

	_tokenizer->getTokenTheories(&tempTheory, 1, sentenceString);
	_morphAnalyzer->getMorphTheories(tempTheory);
	_morphSelector->selectTokenization(sentenceString, tempTheory);
	fixedTokenSequence = adjustTokenizationToNames(tempTheory, nameInfo, nname, sentenceString);
	
	out << L"<SENTENCE ID=\"" << id.to_string() << L"\">\n";
	for (int i = 0; i < fixedTokenSequence->getNTokens(); i++) {
		const Token * token = fixedTokenSequence->getToken(i);
		out << L"<TOKEN START=\"" << token->getStartEDTOffset() << L"\" END=\"" << token->getEndEDTOffset() << L"\">";
		out << token->getSymbol().to_string();
		out << L"</TOKEN>\n";
	}
	out << L"</SENTENCE>\n";
	
	/*
	// Output in PIdF training format instead
	out << L"( ";
	int cur_name = 0;
	Symbol tag = Symbol(L"NONE");
	Symbol status = Symbol(L"-ST");
	int starttok = -1;
	int endtok = -1;
	if (nname > 0) {
		starttok = getStartTokenFromOffset(fixedTokenSequence, nameInfo[cur_name].startoffset);
		endtok = getEndTokenFromOffset(fixedTokenSequence, nameInfo[cur_name].endoffset);
	}

	for (int i = 0; i < fixedTokenSequence->getNTokens(); i++) {
		const Token * token = fixedTokenSequence->getToken(i);
		if (i == starttok) {
			tag = nameInfo[cur_name].type;
			status = Symbol(L"-ST");
		}
		
		// Added for spacified...
		//Symbol pToken = token->getSymbol();
		//wchar_t str[MAX_TOKEN_SIZE];
		//Untokenizer::untokenize(pToken.to_string(), str, MAX_TOKEN_SIZE);
		//size_t len = wcslen(str);
		//wchar_t buffer[2];
		//buffer[1] = L'\0';
		//for (int k = 0; k < len; k++) {
		//	buffer[0] = str[k];
		//	Symbol sub = Tokenizer::getSubstitutionSymbol(Symbol(buffer));
		//	out << L"(" << sub.to_string() << L" " << tag << status << L") ";
		//	status = Symbol(L"-CO");
		//}
			
		out << L"(" << token->getSymbol().to_string() << L" " << tag << status << L") "; 
		status = Symbol(L"-CO");
		if (i == endtok) {
			tag = Symbol(L"NONE");
			status = Symbol(L"-ST");
			cur_name++;
			if (cur_name < nname) {
				starttok = getStartTokenFromOffset(fixedTokenSequence, nameInfo[cur_name].startoffset);
				endtok = getEndTokenFromOffset(fixedTokenSequence, nameInfo[cur_name].endoffset);
			}
			else {
				starttok = -1;
				endtok = -1;
			}
		}
	}
	out << L")\n";
	*/
	

	delete tempTheory;
	delete fixedTokenSequence;
}

TokenSequence* Processor::adjustTokenizationToNames(const TokenSequence* toks,
													 const NameOffset nameInfo[], int nann,
													 const LocatedString* sentenceString)
{
	int starttok;
	int endtok;

	int inName[MAX_SENTENCE_TOKENS][MAX_NAMES_PER_SPACE_SEP_TOK+1];
	Token* newTokens[MAX_SENTENCE_TOKENS];


	for(int i=0; i<toks->getNTokens(); i++){
		inName[i][0] =0;
	}
	for(int i=0; i<nann; i++){
		starttok= getStartTokenFromOffset(toks, nameInfo[i].startoffset);
		endtok = getEndTokenFromOffset(toks, nameInfo[i].endoffset);
		if(starttok!=-1) {
			inName[starttok][inName[starttok][0]+1] = i;
			inName[starttok][0]++;

			// when the sentence is too long the token might not be found and -1 is returned
			if(starttok != endtok && endtok!=-1) { 
				inName[endtok][inName[endtok][0]+1] = i;
				inName[endtok][0]++;
			}
		}
	}
	int currTok =0;
	for(int i=0; i< toks->getNTokens(); i++){
		//this token isnt in a name, just add it to the newTokens
		if(inName[i][0] == 0){
			newTokens[currTok++] = _new Token(*toks->getToken(i));
		}
		//simple case only one name assigned to this token
		else if(inName[i][0] == 1){
			Token* nameParts[3];
			int nnewtoks = splitNameTokens(toks->getToken(i),
				nameInfo[inName[i][1]].startoffset, nameInfo[(inName[i][1])].endoffset,
				sentenceString, nameParts);
			if (nnewtoks > 1)
				_n_tokens_adjusted++;
			for(int j=0; j<nnewtoks; j++){
				newTokens[currTok++] = nameParts[j];
			}
		}
		//hardest case multiple names assigned to a single token
		else{
			Token* nameParts[2*MAX_NAMES_PER_SPACE_SEP_TOK + 1];
			int nnewtoks = splitNameTokens(toks->getToken(i), inName[i], nameInfo, sentenceString, nameParts);
			for(int j=0; j<nnewtoks; j++){
				newTokens[currTok++] = nameParts[j];
			}
			if (nnewtoks > 1)
				_n_tokens_adjusted++;
			/*
			std::cout<<"problem name: starttoken"<<i
				<<" nnames: "<<inName[i][0]<<std::endl;


			throw UnexpectedInputException("Processor::AdjustTokenizationToNames()",
				"More than one annotation per Token");
			*/

		}
		_n_tokens++;
	}
	return _new TokenSequence(0, currTok, newTokens);
}

void Processor::wcsubstr(const wchar_t* str, int start, int end, wchar_t* substr){
	int j =0;
	for(int i=start ; i< end; i++, j++){
		substr[j] = str[i];
	}
	substr[j] = L'\0';
}

int Processor::readSavedSentence(UTF8InputStream &in,
											 LocatedString*& sentString, Symbol& sentId,
											 NameOffset nameInfo[]){
	UTF8Token tok;
/*
  <SENTENCE ID=sentence_id>
    012345678901234
	<DISPLAY_TEXT>This is the sentence text.</DISPLAY_TEXT>
    <ANNOTATION TYPE=PER START_OFFSET=5 END_OFFSET=8 SOURCE=Example1/>
    <ANNOTATION TYPE=GPE START_OFFSET=21 END_OFFSET=27 SOURCE=Example1/>
  </SENTENCE>
*/
	in >> tok;	//<SENTENCE, ignore this
	if(wcscmp(tok.chars(), L"<SENTENCE") !=0){
		if((wcslen(tok.chars()) < 1) || iswspace(tok.chars()[0])){
			return -2;
		}
		return -1;
	}
	in >> tok;	//id="ID">
	wchar_t idstring[500];
	//static_cast<int>
	int len = static_cast<int>( wcslen(tok.chars()));
	wcsubstr(tok.chars(), 4, len-2, idstring);

	sentId = Symbol(idstring);

	const std::wstring &sent_txt = getSentenceTextFromStream(in);
	sentString = _new LocatedString(sent_txt.c_str());
	std::wstring line;
	in.getLine(line);
	Symbol type;

	int nann = 0;
	while(wcsncmp(line.c_str(), L"</SENTENCE>", 11) != 0){
		if(nann >= MAX_NAMES_PER_SENTENCE){
			return -1;
		}
		if(!parseAnnotationLine(line.c_str(), nameInfo[nann].type, nameInfo[nann].startoffset, nameInfo[nann].endoffset)){
			return -1;
		}
		nann++;
		in.getLine(line);
	}
	return nann;

}
bool Processor::parseAnnotationLine(const wchar_t* line, Symbol& type, EDTOffset& start_offset, EDTOffset& end_offset){
	size_t len = wcslen(line);
//    <ANNOTATION TYPE=PER START_OFFSET=5 END_OFFSET=8 SOURCE=Example1/>
	try{
		int start, end, curr_char = 0;
		wchar_t str_buffer[500];
		int len = static_cast<int>(wcslen(line));
		while((curr_char < len) && iswspace(line[curr_char])){
			curr_char++;
		}
		wcsubstr(line, curr_char+1, curr_char+11, str_buffer);
		if(wcsncmp(str_buffer, L"ANNOTATION",10) !=0){
			return false;
		}
		//TYPE="..."
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		curr_char++;
		start = curr_char;
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		end = curr_char;
		curr_char++;
		wcsubstr(line, start, end, str_buffer);
		type = Symbol(str_buffer);

		//START_OFFSET=..
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		curr_char++;
		start = curr_char;
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		end = curr_char;
		curr_char++;
		wcsubstr(line, start, end, str_buffer);
		start_offset = EDTOffset(_wtoi(str_buffer));

		//END_OFFSET=..
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		curr_char++;
		start = curr_char;
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		end = curr_char;
		curr_char++;
		wcsubstr(line, start, end, str_buffer);
		end_offset = EDTOffset(_wtoi(str_buffer));
		return true;

	}

	catch(UnexpectedInputException& e){
		e.prependToMessage("Processor::parseAnnotationLine() ");
		throw;
	}
}



wstring Processor::getRetVal(bool ok, const wchar_t* txt){
	wstring retval(L"<RETURN>\n");
	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";
	retval += txt;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}
wstring Processor::getRetVal(bool ok, const char* txt){
	wstring retval(L"<RETURN>\n");
	if(strlen(txt) >999){
		char errbuffer[1000];
		strcpy(errbuffer, "Invalid Conversion, String too long\nFirst 900 characters: ");
		strncat(errbuffer, txt, 900);
		return getRetVal(false, errbuffer);
	}
	wchar_t conversionbuffer[1000];
	mbstowcs(conversionbuffer, txt, 1000);

	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";

	retval += conversionbuffer;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}



int Processor::getTokenFromOffset(const TokenSequence* tok, EDTOffset offset){
	int midtoken = tok->getNTokens()/2;
	EDTOffset midoffset = tok->getToken(midtoken)->getStartEDTOffset();
	if(offset == midoffset){
		return midtoken;
	}
	if(offset < midoffset){
		for(int i=0; i< midtoken; i++){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	if(offset > midoffset){
		for(int i = midtoken; i< tok->getNTokens(); i++){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	return -1;
}

int Processor::getStartTokenFromOffset(const TokenSequence* tok, EDTOffset offset){
	int midtoken = tok->getNTokens()/2;
	EDTOffset midoffset = tok->getToken(midtoken)->getStartEDTOffset();

	if(offset == midoffset){
		while (midtoken > 0 && tok->getToken(midtoken-1)->getEndEDTOffset() == offset)
			midtoken--;
		return midtoken;
	}
	if(offset < midoffset){
		for(int i=0; i< midtoken; i++){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	if(offset > midoffset){
		for(int i = midtoken; i< tok->getNTokens(); i++){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	return -1;
}

int Processor::getEndTokenFromOffset(const TokenSequence* tok, EDTOffset offset){
	int midtoken = tok->getNTokens()/2;
	EDTOffset midoffset = tok->getToken(midtoken)->getStartEDTOffset();

	if(offset == midoffset){
		while (midtoken < tok->getNTokens() && tok->getToken(midtoken+1)->getStartEDTOffset() == offset)
			midtoken++;
		return midtoken;
	}
	if(offset < midoffset){
		for(int i = midtoken - 1; i >= 0; i--){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	if(offset > midoffset){
		for(int i = tok->getNTokens() - 1; i >= midtoken; i--){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	return -1;
}

int Processor::findNextMatchingChar(const wchar_t* txt, int start, wchar_t c){
	int length = static_cast<int>(wcslen(txt));
	int curr_char = start;
	while((curr_char < length) && txt[curr_char] != c){
		curr_char++;
	}
	if(txt[curr_char] == c){
		return curr_char;
	}
	else{
		return -1;
	}
}
int Processor::findNextMatchingCharRequired(const wchar_t* txt, int start, wchar_t c){
	int retval = findNextMatchingChar(txt, start, c);
	if(retval >= 0){
		return retval;
	}
	else{
		char buffer[500];
		snprintf(buffer, 500, "Couldn't find character: %c after char # %d ", c, start);
		throw UnexpectedInputException("PIdFActiveLearning:findNextMatchingCharRequired()",
					buffer);
		return -1;
	}
}

int Processor::splitNameTokens(const Token* origTok, int inName[], const NameOffset nameInfo[], const LocatedString *sentenceString, Token** newTokens) {
	int nnames = inName[0];
	int sorted_names[MAX_NAMES_PER_SPACE_SEP_TOK + 1];
	sorted_names[0] = nnames;
	EDTOffset min_offset(9999);
	EDTOffset prev_offset;
	int token_number = -1;
	// sort the names by their start offsets
	for (int j = 1; j <= nnames; j++) {
		for (int i = 1; i <= nnames; i++) {
			EDTOffset offset = nameInfo[inName[i]].startoffset;
			if ((offset < min_offset) && (offset > prev_offset)) {
				min_offset = offset;
				token_number = i;
			}
		}
		sorted_names[j] = inName[token_number];
		prev_offset = min_offset;
		min_offset = EDTOffset(9999);
	}

	Token * token_to_split = _new Token(*origTok);
	int currTok = 0;
	for (int i = 1; i <= nnames; i++) {
		Token* nameParts[3];
		int nnewtoks = splitNameTokens(token_to_split,
			nameInfo[sorted_names[i]].startoffset, nameInfo[(sorted_names[i])].endoffset,
			sentenceString, nameParts);
		for(int j = 0; j < nnewtoks-1; j++){
			newTokens[currTok] = nameParts[j];
			currTok++;
		}
		token_to_split = nameParts[nnewtoks - 1];
	}
	newTokens[currTok] = token_to_split;

	return currTok+1;
}

int Processor::splitNameTokens(const Token* origTok, EDTOffset namestart,
										EDTOffset nameend, const LocatedString *sentenceString, Token** newTokens)
{
	if((namestart <= origTok->getStartEDTOffset()) && (nameend >= origTok->getEndEDTOffset())){
		newTokens[0] = _new Token(*origTok);
		return 1;
	}
	const wchar_t* toktxt = origTok->getSymbol().to_string();

	if((namestart <= origTok->getStartEDTOffset()) && (nameend <= origTok->getEndEDTOffset())){
		int start_pos = sentenceString->positionOfStartOffset(origTok->getStartEDTOffset());
		int split_pos = sentenceString->positionOfEndOffset(nameend);
		int end_pos = sentenceString->positionOfEndOffset(origTok->getEndEDTOffset());
		newTokens[0] = _new Token(sentenceString, start_pos, split_pos);
		newTokens[1] = _new Token(sentenceString, split_pos+1, end_pos);
		return 2;
	}
	if((namestart >= origTok->getStartEDTOffset()) && (nameend >= origTok->getEndEDTOffset())){
		int start_pos = sentenceString->positionOfStartOffset(origTok->getStartEDTOffset());
		int split_pos = sentenceString->positionOfEndOffset(namestart)-1;
		int end_pos = sentenceString->positionOfEndOffset(origTok->getEndEDTOffset());
		newTokens[0] = _new Token(sentenceString, start_pos, split_pos);
		newTokens[1] = _new Token(sentenceString, split_pos+1, end_pos);
		return 2;
	}
	if((namestart >= origTok->getStartEDTOffset()) && (nameend <= origTok->getEndEDTOffset())){
		int start_pos = sentenceString->positionOfStartOffset(origTok->getStartEDTOffset());
		int split1_pos = sentenceString->positionOfEndOffset(namestart)-1;
		int split2_pos = sentenceString->positionOfEndOffset(nameend);
		int end_pos = sentenceString->positionOfEndOffset(origTok->getEndEDTOffset());
		newTokens[0] = _new Token(sentenceString, start_pos, split1_pos);
		newTokens[1] = _new Token(sentenceString, split1_pos+1, split2_pos);
		newTokens[2] = _new Token(sentenceString, split2_pos+1, end_pos);
		return 3;
	}
	return 0;
}

std::wstring Processor::getSentenceTextFromStream(UTF8InputStream &in) {
	std::wstring sent_txt, line;
	bool add_new_line = false;

	in.getLine(line);
	const wchar_t* linestr = line.c_str();
	int start = findNextMatchingCharRequired(linestr, 0, L'>');
	start++;
	int end = findNextMatchingChar(line.c_str(),start, L'<');
	if(end!=-1) { // in case of <DISPLAY_TEXT>text...</DISPLAY_TEXT>
		sent_txt += line.substr(start, end-start);
	}else { // in case of <DISPLAY_TEXT>\n text...\n </DISPLAY_TEXT>
		while(end==-1) {
			in.getLine(line);
			end = findNextMatchingChar(line.c_str(),0, L'<');
			if(end==-1) {
				if(add_new_line)
					sent_txt += L'\n';
				sent_txt += line;
				add_new_line = true;
			}
		}
	}
	return sent_txt;
}

/*
int Processor::splitNameTokens(const Token* origTok, int namestart,
										int nameend, Token** newTokens)
{
	if((namestart <= origTok->getStartEDTOffset()) && (nameend >= origTok->getEndEDTOffset())){
		newTokens[0] = _new Token(*origTok);
		return 1;
	}
	const wchar_t* toktxt = origTok->getSymbol().to_string();
	wchar_t tokbuff[500];

	if((namestart <= origTok->getStartEDTOffset()) && (nameend <= origTok->getEndEDTOffset())){
		wcsubstr(toktxt, origTok->getStartEDTOffset(), nameend, tokbuff);
		Symbol part1sym = Symbol(tokbuff);
		wcsubstr(toktxt, nameend+1, origTok->getEndEDTOffset(), tokbuff);
		Symbol part2sym = Symbol(tokbuff);

		Token* part1 = _new Token(origTok->getStartEDTOffset(), nameend, part1sym);
		Token* part2 = _new Token(nameend+1, origTok->getEndEDTOffset(),  part2sym);
		newTokens[0] = part1;
		newTokens[1] = part2;
		return 2;
	}
	if((namestart >= origTok->getStartEDTOffset()) && (nameend >= origTok->getEndEDTOffset())){
		wcsubstr(toktxt, origTok->getStartEDTOffset(), namestart-1, tokbuff);
		Symbol part1sym = Symbol(tokbuff);
		wcsubstr(toktxt, namestart, origTok->getEndEDTOffset(), tokbuff);
		Symbol part2sym = Symbol(tokbuff);
		Token* part1 = _new Token(origTok->getStartEDTOffset(), namestart-1, part1sym);
		Token* part2 = _new Token(namestart, origTok->getEndEDTOffset(),  part2sym);
		newTokens[0] = part1;
		newTokens[1] = part2;
		return 2;
	}
	if((namestart >= origTok->getStartEDTOffset()) && (nameend <= origTok->getEndEDTOffset())){
		wcsubstr(toktxt, origTok->getStartEDTOffset(), namestart-1, tokbuff);
		Symbol part1sym = Symbol(tokbuff);
		wcsubstr(toktxt, namestart, nameend, tokbuff);
		Symbol part2sym = Symbol(tokbuff);
		wcsubstr(toktxt, nameend+1, origTok->getEndEDTOffset(), tokbuff);
		Symbol part3sym = Symbol(tokbuff);

		Token* part1 = _new Token(origTok->getStartEDTOffset(), namestart-1, part1sym);
		Token* part2 = _new Token(namestart, nameend, part2sym);
		Token* part3 = _new Token(nameend+2, origTok->getEndEDTOffset(),  part3sym);
		newTokens[0] = part1;
		newTokens[1] = part2;
		newTokens[2] = part3;
		return 3;
	}
	return 0;
}

*/

