// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Arabic/names/ar_IdFNameRecognizer.h"
#include "Arabic/names/ar_NameRecognizer.h"
#include "Generic/theories/DocTheory.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/common/ar_WordConstants.h"
#include "Generic/common/SymbolHash.h"

#include <boost/algorithm/string.hpp>

using namespace std;

ArabicNameRecognizer::ArabicNameRecognizer() : _idfNameRecognizer(0), _pidfDecoder(0) {
	EntityType(); //initialize the entity type array, initializing it on the fly in getNames() causes a crash
	_knownNames = 0;
	string nameFinderParam = ParamReader::getParam("name_finder");
	if (boost::iequals(nameFinderParam,"idf")) {
		_name_finder = IDF_NAME_FINDER;
	}
	else if (boost::iequals(nameFinderParam,"pidf")) {
		_name_finder = PIDF_NAME_FINDER;
	}
	else {
		throw UnexpectedInputException(
			"ArabicNameRecognizer::ArabicNameRecognizer()",
			"name-finder parameter not specified. (Should be 'idf' or 'pidf'.)");
	}

	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer = _new ArabicIdFNameRecognizer();
	}
	else {
		_pidfDecoder = _new PIdFModel(PIdFModel::DECODE);
		//this is hackish, but the PIdF is combining names that a separated by 'w (and)' 
		//(eg.  Palestinian and Israeli) use a list of names to separate  XX w XX when
		//we know both names
		//std::string buff = ParamReader::getRequiredParam("known_name_list");
		//_knownNames = _new SymbolHash(8000, buff.c_str());
		//std::cout<<"decoder initialized"<<std::endl;
	}
	// This should only be turned on if sentence selection is the only thing you are doing!
	PRINT_SENTENCE_SELECTION_INFO = ParamReader::isParamTrue("print_sentence_selection_info");
	
}


void ArabicNameRecognizer::resetForNewSentence(const Sentence *sentence) {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->resetForNewSentence();
	}
}

void ArabicNameRecognizer::cleanUpAfterDocument() {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->cleanUpAfterDocument();
	}
}

void ArabicNameRecognizer::resetForNewDocument(DocTheory *docTheory) {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->resetForNewDocument(docTheory);
	}
	_docTheory = docTheory; 
}

int ArabicNameRecognizer::getNameTheories(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence)
{
//create a person for every poster
	_sent_no = tokenSequence->getSentenceNumber();

	if (_docTheory->isSpeakerSentence(_sent_no)) {
		//results[0] = _new NameTheory();
		//results[0]->getNNameSpans() = 0;
		//return 1;
		results[0] = _new NameTheory(tokenSequence, 1);
		results[0]->takeNameSpan(_new NameSpan(0, tokenSequence->getNTokens() - 1,
			EntityType::getPERType()));

		// faked
		if (PRINT_SENTENCE_SELECTION_INFO)
			results[0]->setScore(100000);
		//std::cout<<"return 1 name"<<std::endl;

		
		return 1;
	}

	int n_results;
	if (_name_finder == IDF_NAME_FINDER) {
		n_results = _idfNameRecognizer->getNameTheories(results, max_theories,
												   tokenSequence);
	}
	else { // PIDF_NAME_FINDER
		n_results = _pidfDecoder->getNameTheories(results, max_theories,
											 tokenSequence);

		cleanNameTheories(results, n_results, tokenSequence);
	}

	for (int i = 0; i < n_results; i++) 
		postProcessNameTheory(results[i], tokenSequence);

	return n_results;
}

int ArabicNameRecognizer::getNameTheoriesForSentenceBreaking(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence)
{
	if (_name_finder == IDF_NAME_FINDER) {
		return _idfNameRecognizer->getNameTheories(results, max_theories,
												   tokenSequence);
	}
	else { // PIDF_NAME_FINDER
		return _pidfDecoder->getNameTheories(results, max_theories,
											 tokenSequence);
	}
}
void ArabicNameRecognizer::cleanNameTheories(NameTheory** results, int ntheories, TokenSequence *tokenSequence){
	Symbol ignore_start[5];
	Symbol punc[9];
	ignore_start[0] = ArabicSymbol(L"w");
	ignore_start[1] = ArabicSymbol(L"b");
	ignore_start[2] = ArabicSymbol(L"l");
	ignore_start[3] = ArabicSymbol(L"f");
	ignore_start[4] = ArabicSymbol(L"k");
	wchar_t buff[2];
	buff[0] = L'\x60c';
	buff[1] = L'\0';
	punc[0] = Symbol(buff);
	buff[0] = L'\xAB';
	punc[1] = Symbol(buff);
	buff[0] = L'\xBB';
	punc[2] = Symbol(buff);
	buff[0] = L'\xAD';
	punc[3] = Symbol(buff);
	buff[0] = L'\x61F';
	punc[4] = Symbol(buff);
	buff[0] = L'\x61B';
	punc[5] = Symbol(buff);
	punc[6] = Symbol(L"''");
	punc[7] = Symbol(L".");
	punc[8] = Symbol(L"-");
	int i;
	for(i =0; i<ntheories; i++){
		NameTheory* nt = results[i];
		for(int j =0; j< nt->getNNameSpans(); j++){
			int name_start = nt->getNameSpan(j)->start;
			int name_end = nt->getNameSpan(j)->end;
			int adj_name_start = name_start;
			int k = name_start;
			int m;
			while((adj_name_start <= name_end) && (k == adj_name_start)){
				for(m = 0; m < 5; m++){
					if(tokenSequence->getToken(k)->getSymbol() == ignore_start[m]){
						//std::cout<<"matched start: "<<ignore_start[m].to_debug_string()<<std::endl;
						adj_name_start++;
					}
				}
				for(m = 0; m < 9; m++){
					if(tokenSequence->getToken(k)->getSymbol() == punc[m]){
						//std::cout<<"matched start: "<<punc[m].to_debug_string()<<std::endl;
						adj_name_start++;
					}
				}
				k++;
			}
			int adj_name_end = name_end;
			k = name_end;
			while((adj_name_end >= adj_name_start) && (k == adj_name_end)){
				for(m = 0; m < 5; m++){
					if(tokenSequence->getToken(k)->getSymbol() == ignore_start[m]){
						//std::cout<<"matched end: "<<ignore_start[m].to_debug_string()<<std::endl;
						adj_name_end--;
					}
				}
				for(m = 0; m < 9; m++){
					if(tokenSequence->getToken(k)->getSymbol() == punc[m]){
						//std::cout<<"matched end: "<<punc[m].to_debug_string()<<std::endl;
						adj_name_end--;
					}
				}
				k--;
			}
			if((name_start != adj_name_start) || (name_end != adj_name_end)){
				if(adj_name_start <= adj_name_end){
					//std::cout<<"adjusting name boundary"<<std::endl;
					nt->getNameSpan(j)->start = adj_name_start;
					nt->getNameSpan(j)->end = adj_name_end;
				}
			}

		}
	}

	/*
//This problem does not seem to exist!
	//deal with XX w YY (eg. Palestinians and Israelis).  The PIdF frequently finds these
	//as a single name.  If both XX and YY are in known names separate the names;
	Symbol name_separators[3];
	name_separators[0] = ArabicSymbol(L"w");
	name_separators[1] = ArabicSymbol(L"b");
	name_separators[2] = ArabicSymbol(L"l");
	int j, k, m;
	int n_to_split = 0;
	int names_to_split[MAX_SENTENCE_TOKENS];
	int split_tokens[MAX_SENTENCE_TOKENS];
	for(i =0; i<ntheories; i++){
		NameTheory* orig_nt = results[i];


		for(j =0; j< orig_nt->getNNameSpans(); j++){
			int name_start = orig_nt->getNameSpan(j)->start;
			int name_end = orig_nt->getNameSpan(j)->end;
			//print the name:
			std::cout<<"*****Checking name "<<j<<"*****"<<std::endl;
			for(int q = name_start; q<=name_end; q++){
				std::cout<<tokenSequence->getToken(q)->getSymbol().to_debug_string()<<" ";
			}
			std::cout<<std::endl;

			bool found_sep = false;
			bool both_known = false;
			int sep_tok = 0;
			for(k = name_start+1; k < name_end; k++){
				if(found_sep){
					break;
				}
				for(m = 0; m < 3; m++){
					if(tokenSequence->getToken(k)->getSymbol() == name_separators[m]){
						std::cout<<"Found Separator " <<name_separators[m].to_debug_string()<<std::endl;
						found_sep = true;
						sep_tok = k;
					}
				}
			}
			Symbol name1;
			Symbol name2;
			if(found_sep){
				std::wstring word1 = L"";
				
				int nchar = 0;
				word1 += tokenSequence->getToken(name_start)->getSymbol().to_string();
				for(k = (name_start+1); k < sep_tok; k++){
					word1 += L" ";
					word1 += tokenSequence->getToken(k)->getSymbol().to_string();
				}
				name1 = Symbol(word1.c_str());
				std::cout<<"\tname1 -" <<name1.to_debug_string()<<"-"<<std::endl;
				if(_knownNames->lookup(name1)){
					std::wstring word2= L"";
					word2 += tokenSequence->getToken(sep_tok+1)->getSymbol().to_string();
					for(k = (sep_tok+2); k < name_end; k++){
						word2 += L" ";
						word2 += tokenSequence->getToken(k)->getSymbol().to_string();
					}
					name2 = Symbol(word2.c_str());
					std::cout<<"\tname2 -" <<name2.to_debug_string()<<"-"<<std::endl;

					if(_knownNames->lookup(name2)){
						std::cout<<"\t\tboth known!!"<<std::endl;
						both_known = true;

					}
				}
			}


			if(both_known){
				std::cout<<"Found Name To Split: "<<j<<" "
					<<name1.to_debug_string()<<" - "
					<<tokenSequence->getToken(sep_tok)->getSymbol().to_debug_string()<<" - "
					<<name2.to_debug_string()<<
					std::endl;
				names_to_split[n_to_split] = j;
				split_tokens[n_to_split++] = sep_tok;
			}
		
		}
		if(n_to_split > 0){
			NameTheory* fixed_nt = _new NameTheory();
			fixed_nt->getNNameSpans() = orig_nt->getNNameSpans() + n_to_split;
			fixed_nt->nameSpans = _new NameSpan*[fixed_nt->getNNameSpans()];
			int curr_split = 0;
			int curr_nt = 0;
			for(j =0; j< orig_nt->getNNameSpans(); j++){
				if(j != names_to_split[curr_split]){
					fixed_nt->nameSpans[curr_nt] = _new NameSpan();
					fixed_nt->nameSpans[curr_nt]->start = orig_nt->getNameSpan(j)->start;
					fixed_nt->nameSpans[curr_nt]->end = orig_nt->getNameSpan(j)->end;
					fixed_nt->nameSpans[curr_nt]->type = orig_nt->getNameSpan(j)->type;
					curr_nt++;
				}
				else{
					std::cout<<"Splitting name: "<<j<<std::endl;
					fixed_nt->nameSpans[curr_nt] = _new NameSpan();
					fixed_nt->nameSpans[curr_nt]->start = orig_nt->getNameSpan(j)->start;
					fixed_nt->nameSpans[curr_nt]->end = split_tokens[curr_split]-1;
					fixed_nt->nameSpans[curr_nt]->type = orig_nt->getNameSpan(j)->type;
					curr_nt++;
					fixed_nt->nameSpans[curr_nt] = _new NameSpan();
					fixed_nt->nameSpans[curr_nt]->start =  split_tokens[curr_split]+1;
					fixed_nt->nameSpans[curr_nt]->end = orig_nt->getNameSpan(j)->start;
					fixed_nt->nameSpans[curr_nt]->type = orig_nt->getNameSpan(j)->type;
					curr_nt++;
					curr_split++;
				}
			}
			if(curr_split != n_to_split){
				std::cout<<"Error Didn't Split all names"<<std::endl;
			}
			delete orig_nt;
			results[i] = fixed_nt;
		}
	}
	*/
//
	//The ACE2005 annotation disagrees with the BBN Annotation about the annotation of things such as
	//	- city of Boston and company Microsoft Corp.
	//in both cases BBN marks and name and desc, but the LDC marks the entire phrase as a name
	//I am fixing this with a list of words (city, country, company,  that attach to the name that
	//follows them when that name is of the correct type.  



	bool in_name[MAX_SENTENCE_TOKENS];
	int j, k;
	Symbol Af = ArabicSymbol(L"Af");
	Symbol b = ArabicSymbol(L"b");
	for(i =0; i<ntheories; i++)
	{
		NameTheory* nt = results[i];
		//first get all tokens that are in names, we dont want to force a name overlap!!!!
		for(j =0; j< tokenSequence->getNTokens(); j++){
			in_name[j] = false;
		}
		for(j =0; j< nt->getNNameSpans(); j++){
			NameSpan* ns = nt->getNameSpan(j);
			for(k = ns->start; k<= ns->end; k++){
				in_name[k] = true;
			}
		}
		for(j =0; j< nt->getNNameSpans(); j++){
			NameSpan* ns = nt->getNameSpan(j);
			//previous step removes b from Af b (AFP), fix this here
			if((ns->type.matchesORG()) 
				&& (ns->start == ns->end) 
				&& (ns->start+1 < tokenSequence->getNTokens())
				&& !in_name[ns->start+1])
			{
				if((tokenSequence->getToken(ns->start)->getSymbol() == Af))
				{
					if(tokenSequence->getToken(ns->end+1)->getSymbol() == b)
					{
						SessionLogger::logger->reportDebugMessage() <<"reextending Af b"<<"\n";
						ns->end++;
						in_name[ns->end] = true;
					}
				}
			}

			if(ns->start == 0)
				continue;
			if(in_name[ns->start - 1])
				continue;
			int prev_tok = ns->start - 1;
			Symbol prev_word = tokenSequence->getToken(prev_tok)->getSymbol();
			//std::cout<<"prev_word: "<<prev_word.to_debug_string()
			//	<<" first name word: "<<
			//	tokenSequence->getToken(ns->start)->getSymbol().to_debug_string()<<std::endl;
			bool extend_name = false;
			if(ns->type.matchesGPE()){
				if(ArabicWordConstants::matchGPEWord(prev_word)){
					extend_name = true;
				}
			}
			else if(ns->type.matchesORG()){
				if(ArabicWordConstants::matchORGWord(prev_word)){
					extend_name = true;
				}
			}
			else if(ns->type.matchesFAC()){
				if(ArabicWordConstants::matchGPEWord(prev_word)){
					extend_name = true;
				}
			}
			else if(ns->type.matchesLOC()){
				if(ArabicWordConstants::matchGPEWord(prev_word)){
					extend_name = true;
				}
			}
			if(extend_name){
				SessionLogger &logger = *SessionLogger::logger;
				SessionLogger::LogMessageMaker warning = logger.reportWarning().with_id("fix_name");
				warning<<"Warning extending name span to include hw\n\told name: ";
				for(k = ns->start; k <= ns->end; k++){
					warning<<tokenSequence->getToken(k)->getSymbol().to_debug_string()<<" ";
				}
				ns->start--;
				warning<<"\n\tnew start: "<<prev_word.to_debug_string()<<"\n";
				
				in_name[ns->start] = true;
			}
		}
	}
}
