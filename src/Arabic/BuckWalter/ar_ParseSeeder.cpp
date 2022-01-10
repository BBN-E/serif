// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"
#include "Generic/theories/LexicalToken.h"
#include "Arabic/BuckWalter/ar_BWNormalizer.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/BuckWalter/ar_BuckWalterFunctions.h"

ArabicParseSeeder::ArabicParseSeeder(){
	_use_GALE_tokenization = ParamReader::isParamTrue("use_GALE_tokenization");
}
void ArabicParseSeeder::_reset() {
	int t = _n_lex_entries;
	if(t > MAX_POSSIBILITIES_PER_WORD){
		t = MAX_POSSIBILITIES_PER_WORD;
	}
	for(int i=0; i< t; i++){
		_noVowelNumSeg[i] =0;
		_mergedNumSeg[i] =0;
	}
	_n_lex_entries = 0;
	_num_grouped = 0;

}
void ArabicParseSeeder::processToken(const LocatedString& sentenceString, const Token* t) throw (UnrecoverableException) {
	const LexicalToken* lt = dynamic_cast<const LexicalToken*>(t);
	if (!lt) throw InternalInconsistencyException("ArabicParseSeeder::processToken", 
		"This ArabicParseSeeder requires all tokens to be LexicalTokens.");
	_reset();
	_n_lex_entries = lt->getNLexicalEntries();
	if(_n_lex_entries > MAX_POSSIBILITIES_PER_WORD){
		_n_lex_entries = MAX_POSSIBILITIES_PER_WORD;
	}

	//std::cout<<"Populate"<<std::endl;
	
	/*if((t->getSymbol().to_debug_string()[0] == 'w')){
		std::cout<<"ArabicParseSeeder::Process Token: num lex entries:"<<_n_lex_entries<<" string: "
		<<t->getSymbol().to_debug_string()<<std::endl;	
		_populateNoVowelWordChart(t);
		std::cout<<"Initial No Vowel Chart"<<std::endl;
		dumpNoVowelWordChart(std::cout);
		std::cout<<"----------------"<<std::endl;
		_alignNoVowelWordChart(_n_lex_entries);
		std::cout<<"Aligned No Vowel Chart"<<std::endl;
		dumpNoVowelWordChart(std::cout);
		std::cout<<"----------------"<<std::endl;
		_mergeSegments();
		_addOffsets(t);
		std::cout<<"Merged Charts"<<std::endl;
		dumpBothCharts(std::cout);
		std::cout<<"----------------"<<std::endl;

		_groupParseTokens();
		std::cout<<"Final Charts (Grouped)"<<std::endl;
		std::cout<<"Num Grouped: "<<_num_grouped<<std::endl;
		dumpBothCharts(std::cout);
		std::cout<<"----------------"<<std::endl;
	}
	else{*/
	_populateNoVowelWordChart(lt);
	_alignNoVowelWordChart(_n_lex_entries);
	_mergeSegments();
	_addOffsets(sentenceString, lt);
	_groupParseTokens();
	//}

}
void ArabicParseSeeder::dumpNoVowelWordChart(std::ostream &out){
	for(int i=0; i<_n_lex_entries; i++){
		int n_seg = _noVowelNumSeg[i];
		out<<"NumSeg: "<<n_seg<<"\n ";
		for(int j =0; j<n_seg; j++){
			const NoVowelParseToken &p = _noVowelWordChart[j][i];
			out<<"\tSTART: "<<p.start<<" END: "<<p.end;
			out<<" Word: "<<p.normString.to_debug_string()
				<<" ";
			out.flush();
			out<<p.word_part<<"\n";
		}
		out.flush();
	}
}
void ArabicParseSeeder::dumpNoVowelWordChart(UTF8OutputStream &out){
	out<<"-------------------------------\n";
	for(int i=0; i<_n_lex_entries; i++){
		int n_seg = _noVowelNumSeg[i];
		out<<"NumSeg: "<<n_seg<<"\n ";
		for(int j =0; j<n_seg; j++){
			const NoVowelParseToken &p = _noVowelWordChart[j][i];
			out<<"\t"<<j<<" START: "<<p.start<<" END: "<<p.end<<"\n";
			out<<"\t\tWord: "<<p.normString.to_debug_string()
				<<" "<<p.word_part;
			out<<"\nLEX ENTRY: ";
			p.lex_ent->dump(out);
		}
		out.flush();
	}
	out<<"-------------------------------\n";

}
void ArabicParseSeeder::dumpBothCharts(UTF8OutputStream &out){
	for(int i=0; i<_n_lex_entries; i++){
		out<<"################Word Chart###################\n";
		int n_seg = _noVowelNumSeg[i];
		out<<"NumSeg: "<<n_seg<<"\n ";
		out.flush();
		int j;
		for(j =0; j<n_seg; j++){
			const NoVowelParseToken &p = _noVowelWordChart[j][i];
			out<<"\t"<<j<<" START: "<<p.start<<" END: "<<p.end<<"\n";
			out.flush();
			out<<"\t\tWord: "<<p.normString.to_debug_string()
				<<" "<<p.word_part;
			out.flush();
			out<<"\nLEX ENTRY: ";
			p.lex_ent->dump(out);
			out.flush();
		}
		out<<"################Merged Chart###################\n";
		n_seg = _mergedNumSeg[i];
		out<<"NumSeg: "<<n_seg<<"\n ";
		out.flush();
		for(j =0; j<n_seg; j++){
			ParseToken p = _mergedWordChart[j][i];
			out<<"\t"<<j<<" START: "<<p.start<<" END: "<<p.end<<"\n";
			out.flush();
			out<<"\t\tWord: "<<p.normString.to_debug_string()
				<<" "<<p.word_part<<" "<<"StartOffset: "
				<<p.start_offset.value<CharOffset>()
				<<" EndOffset: "<<p.end_offset.value<CharOffset>()
				<<"\nLEX ENTRY: ";
			out.flush();
			for(size_t k=0; k< p.lex_ents.size(); k++){
				p.lex_ents[k]->dump(out);
				out.flush();
			}
		}
		out<<"###############################################\n";
		out.flush();
	}
	out<<"-------------------------------\n";
}



void ArabicParseSeeder::dumpBothCharts(std::ostream &out){
	for(int i=0; i<_n_lex_entries; i++){
		out<<"################Word Chart###################\n";
		int n_seg = _noVowelNumSeg[i];
		out<<"NumSeg: "<<n_seg<<"\n ";
		out.flush();
		int j;
		for(j =0; j<n_seg; j++){
			const NoVowelParseToken &p = _noVowelWordChart[j][i];
			out<<"\t"<<j<<" START: "<<p.start<<" END: "<<p.end;
			out.flush();
			out<<" Word: "<<p.normString.to_debug_string()
				<<" "<<p.word_part<<"\n";
			out.flush();
			//out<<"\nLEX ENTRY: ";
			//p.lex_ents[0]->dump(out);
		}
		out<<"################Merged Chart###################\n";
		n_seg = _mergedNumSeg[i];
		out<<"NumSeg: "<<n_seg<<"\n ";
		out.flush();
		for(j =0; j<n_seg; j++){
			ParseToken p = _mergedWordChart[j][i];
			out<<"\t"<<j<<" START: "<<p.start<<" END: "<<p.end;
			out<<" Word: "<<p.normString.to_debug_string()
				<<" "<<p.word_part<<" "<<"StartOffset: "
				<<p.start_offset.value<CharOffset>()<<" EndOffset: "<<p.end_offset.value<CharOffset>()<<"\n";
			out.flush();
				//<<"\nLEX ENTRY: ";
			/*
			for(int k=0; k< p.num_lex_ent; k++){
				p.lex_ents[k]->dump(out);
			}
			*/
		}
		out<<"###############################################\n";
		out.flush();
	}
	out<<"-------------------------------\n";
}

void ArabicParseSeeder::_mergeSegments() throw (UnrecoverableException){
	//std::cout<< "_mergeSegments()"<<std::endl;
	Symbol added = Symbol(L"ADDED");
	int merged_count = 0;
	for(int i =0; i<_n_lex_entries; i++){
		merged_count = 0;
	//special case words that were retokenized during names, all sub parts should be merged!
		if(_noVowelWordChart[0][i].word_part == RETOKENIZED){
			_updateMergedWordChart(merged_count, i, 0, _noVowelNumSeg[i] -1);
			merged_count++;
		}
		else{
			for(int j=0; j< _noVowelNumSeg[i]; j++){ 
				LexicalEntry* le = _noVowelWordChart[j][i].lex_ent;
				FeatureValueStructure* fvs = le->getFeatures();

				Symbol cat = fvs->getCategory();
				Symbol pos = fvs->getPartOfSpeech();

				if(_isPrefInfl(&_noVowelWordChart[j][i])){
					//make new word in chart, next word will attach
					int k = j;
					k++;
					while(k<_noVowelNumSeg[i]){
						if(!_isPrefInfl(&_noVowelWordChart[k][i])){
							break;
						}
						k++;	//fix 11/21
					}
					if(k == _noVowelNumSeg[i]){
						k--;
					}
					_updateMergedWordChart(merged_count,i,j,k);
					merged_count++;

					j = k; //k or k+1?

				}
				else if(_isSuffInfl(&_noVowelWordChart[j][i])){
					//append to previous (merged_count -1) location
					int place =  merged_count -1;
					wcscpy(_word_buffer,_mergedWordChart[place][i].normString.to_string());
					wcscat(_word_buffer, _noVowelWordChart[j][i].normString.to_string());
					_mergedWordChart[place][i].normString = Symbol(_word_buffer);
					_mergedWordChart[place][i].end = _noVowelWordChart[j][i].end;
					_mergedWordChart[place][i].lex_ents.push_back(_noVowelWordChart[j][i].lex_ent);
				}
				else{
	
					_mergedWordChart[merged_count][i].normString = _noVowelWordChart[j][i].normString;
					_mergedWordChart[merged_count][i].start = _noVowelWordChart[j][i].start;
					_mergedWordChart[merged_count][i].end = _noVowelWordChart[j][i].end;
					_mergedWordChart[merged_count][i].lex_ents.reserve(1);
					_mergedWordChart[merged_count][i].lex_ents.resize(0);
					_mergedWordChart[merged_count][i].lex_ents.push_back(
							_noVowelWordChart[j][i].lex_ent);
					_mergedWordChart[merged_count][i].word_part = _noVowelWordChart[j][i].word_part;
					_mergedWordChart[merged_count][i].cat = cat;
					_mergedWordChart[merged_count][i].pos = pos;
					merged_count++;
				}
			}
		}
		_mergedNumSeg[i] = merged_count;
	}
}
void ArabicParseSeeder::_updateMergedWordChart(int m_count, int l_count, int nv_start_index, int nv_end_index){
	wcscpy(_word_buffer,_noVowelWordChart[nv_start_index][l_count].normString.to_string());
	for(int i = nv_start_index+1; i <= nv_end_index; i++){
		if((wcslen(_word_buffer) + wcslen(_noVowelWordChart[i][l_count].normString.to_string())) 
			> MAX_CHAR_PER_WORD)
		{
			throw UnrecoverableException(
				"ArabicParseSeeder::_updateMergedWordChart()",
				"More than MAX_CHAR_PER_WORD characters to be merged");
		}
		wcscat(_word_buffer,_noVowelWordChart[i][l_count].normString.to_string());
	}
	_mergedWordChart[m_count][l_count].normString = Symbol(_word_buffer);
	_mergedWordChart[m_count][l_count].start = 
		_noVowelWordChart[nv_start_index][l_count].start;
	_mergedWordChart[m_count][l_count].end = 
		_noVowelWordChart[nv_end_index][l_count].end;
	//copy lex entries
	if((nv_end_index - nv_start_index) > MAX_POSSIBILITIES_PER_WORD)
		{
			// Is this still a fatal error??
			throw UnrecoverableException(
				"ArabicParseSeeder::_updateMergedWordChart()",
				"More than MAX_POSSIBILITIES_PER_WORD lex entries to be merged");
		}
	_mergedWordChart[m_count][l_count].lex_ents.reserve(nv_end_index-nv_start_index);
	_mergedWordChart[m_count][l_count].lex_ents.resize(0);
	for(int m = nv_start_index; m <= nv_end_index; m++){
		_mergedWordChart[m_count][l_count].lex_ents.push_back(
			_noVowelWordChart[m][l_count].lex_ent);
	}	
	_mergedWordChart[m_count][l_count].word_part = STEM;
}

bool ArabicParseSeeder::_isPrefInfl(const NoVowelParseToken* p){
	//this matches what was done in bw script
	FeatureValueStructure* fvs = p->lex_ent->getFeatures();
	Symbol cat = fvs->getCategory();
	Symbol pos = fvs->getPartOfSpeech();
	const wchar_t* pos_str = pos.to_string();
	/*if(cat == Symbol(L"ADDED")){	//this could be inflection marked on a stem
		return false;
	}
	else*/ if(p->word_part != PREFIX){
		if( (wcsncmp(pos_str, L"FUT",3)==0)||
			(wcsncmp(pos_str, L"DET",3)==0)||
			(wcsncmp(pos_str, L"IV", 2)==0)){	//IV are imp. verb prefixes
			return true;
		}
		else{
			return false;
		}
		
	}
	//PREP|SUBJUNC|EMPHATIC_PARTICLE|RESULT_CLAUSE_PARTICLE
	if(( wcsncmp(pos_str, L"CONJ",4)==0) ||
		(wcsncmp(pos_str, L"PREP",4)==0) ||
		(wcsncmp(pos_str, L"SUBJUNC",7)==0) ||
		(wcsncmp(pos_str, L"EMPHATIC_PARTICLE",17)==0) ||
		(wcsncmp(pos_str, L"RESULT_CLAUSE_PARTICLE",22)==0)){
			return false;
		}
	else{
		return true;
	}
}
bool ArabicParseSeeder::_isSuffInfl(const NoVowelParseToken* p){
	FeatureValueStructure* fvs = p->lex_ent->getFeatures();
	Symbol cat = fvs->getCategory();
	Symbol pos = fvs->getPartOfSpeech();
	const wchar_t* pos_str = pos.to_string();
	if(cat == Symbol(L"ADDED")){	//this could be inflection marked on a stem
		if(( wcsncmp(pos_str, L"PVSUFF_SUBJ",11)==0) ||
			(wcsncmp(pos_str, L"IVSUFF_SUBJ",11)==0) ||
			(wcsncmp(pos_str, L"CVSUFF_SUBJ",11)==0) ||
			(wcsncmp(pos_str, L"NSUFF",5)==0)){
				return true;
			}
		else{
			return false;
		}
	}
	else if(p->word_part != SUFFIX){
		if((wcsncmp(pos_str, L"NSUFF",5)==0)||
			(wcsncmp(pos_str, L"PVSUFF_SUBJ",11)==0) ||
			(wcsncmp(pos_str, L"IVSUFF_SUBJ",11)==0) ||
			(wcsncmp(pos_str, L"CVSUFF_SUBJ",11)==0)){
			return true;
		}
		return false;
	}

	if(( wcsncmp(pos_str, L"PVSUFF_DO",9)==0) ||
		(wcsncmp(pos_str, L"IVSUFF_DO",9)==0) ||
		(wcsncmp(pos_str, L"CVSUFF_DO",9)==0) ||
		(wcsncmp(pos_str, L"POSS_PRON_",10)==0)||
		(wcsncmp(pos_str, L"PRON_",5)==0))
		{
			return false;
		}
	else{
		return true;
	}
}


void ArabicParseSeeder::_addOffsets(const LocatedString& sentenceString, const Token* t){
	const wchar_t* tokstr = t->getSymbol().to_string();
	OffsetGroup tok_start_offset = t->getStartOffsetGroup();
	OffsetGroup tok_end_offset = t->getEndOffsetGroup();
	int tok_start_pos = sentenceString.positionOfStartOffset<CharOffset>(tok_start_offset.value<CharOffset>());
	int tok_end_pos = sentenceString.positionOfEndOffset<CharOffset>(tok_end_offset.value<CharOffset>());
	//mrf, for GALE processing just set all new token offsets to the same offests as the larger tokens, 
	//This will for the wbXYZ; start =0 end = 5 we end up with 3 tokens
	//w start = 0, end = 5
	//b start = 0, end = 5
	//XYZ start = 0, end = 5
	if(_use_GALE_tokenization){
		for(int i =0; i<_n_lex_entries; i++){
			int j = 0;
			for(j=0; j< _mergedNumSeg[i]; j++){ 
				_mergedWordChart[j][i].start_offset = tok_start_offset;
				_mergedWordChart[j][i].end_offset = tok_end_offset;
			}
		}
	}
	else{
		for(int i =0; i<_n_lex_entries; i++){
			int tok_place =0;
			int tok_len = static_cast<int>(wcslen(tokstr));
			if(_mergedNumSeg[i] == 1){
				_mergedWordChart[0][i].start_offset = tok_start_offset;
				_mergedWordChart[0][i].end_offset = tok_end_offset;
			}
			else{
				int j = 0;
				for(j=0; j< _mergedNumSeg[i]; j++){ 
					int start = tok_place;
					const wchar_t* curr_str = _mergedWordChart[j][i].normString.to_string();
					int curr_place =0; 
					int curr_len =static_cast<int>(wcslen(curr_str));
					while((tok_place < tok_len) && (curr_place < curr_len)){
						if( _equivalentChars(tokstr[tok_place], curr_str[curr_place])){
							tok_place++;
							curr_place++;
						}
						else if(((curr_place+1) < curr_len) && 
								_equivalentChars(tokstr[tok_place], curr_str[curr_place+1]))
						{
									curr_place++;
						}
						else if(((tok_place+1) < tok_len) && 
								_equivalentChars(tokstr[tok_place+1], curr_str[curr_place]))
						{
									tok_place++;
						}
						else{
							break;
						}
					}
					if( start < tok_len){
						int start_pos = tok_start_pos + start;
						int end_pos = tok_start_pos + tok_place - 1;
						_mergedWordChart[j][i].start_offset = sentenceString.startOffsetGroup(start_pos);
						//force end to be >= start
						if(end_pos > start_pos){
							_mergedWordChart[j][i].end_offset = sentenceString.endOffsetGroup(end_pos);
						}
						else{
							_mergedWordChart[j][i].end_offset = sentenceString.endOffsetGroup(start_pos);
						}
					}
					else{
						int prev_len = _mergedWordChart[j-1][i].end_offset.value<CharOffset>().value() - _mergedWordChart[j-1][i].start_offset.value<CharOffset>().value();
						//hack to deal with last segments
						if(prev_len >	curr_len){
							int prev_end_pos = sentenceString.positionOfEndOffset<CharOffset>(_mergedWordChart[j-1][i].end_offset.value<CharOffset>()) - curr_len;
							_mergedWordChart[j-1][i].end_offset = sentenceString.endOffsetGroup(prev_end_pos);
							_mergedWordChart[j][i].start_offset = sentenceString.startOffsetGroup(prev_end_pos + 1);
							_mergedWordChart[j][i].end_offset = sentenceString.endOffsetGroup((prev_end_pos + 1) + (curr_len - 1));
						}
						else if(prev_len >1){
							_mergedWordChart[j][i].end_offset = _mergedWordChart[j-1][i].end_offset;
							_mergedWordChart[j][i].start_offset = _mergedWordChart[j-1][i].end_offset;
							// decrement the value of _mergedWordChar[j-1][i].end_offset
							int prev_end_pos = sentenceString.positionOfEndOffset<CharOffset>(_mergedWordChart[j-1][i].end_offset.value<CharOffset>()) - 1;
							_mergedWordChart[j-1][i].end_offset = sentenceString.endOffsetGroup(prev_end_pos);
						}
						else{	//this will lead to overlapping segments!
							_mergedWordChart[j][i].end_offset = _mergedWordChart[j-1][i].end_offset;
							_mergedWordChart[j][i].start_offset = _mergedWordChart[j-1][i].end_offset;
						}
					}

				}

				if(_mergedWordChart[j-1][i].end_offset.value<CharOffset>().value() != tok_end_offset.value<CharOffset>().value()){
					//std::cerr<<"ERROR: wrong token offset! "<<t->getSymbol().to_debug_string()<<std::endl;
					//std::cerr<<"tok_end_offset: "<<tok_end_offset<<std::endl;
					int j = 0;
					for(j=0; j<_mergedNumSeg[i];j++){
						//std::cerr<<"String: "<<_mergedWordChart[j][i].normString.to_debug_string()
						//	<<" StartOffset: "<<_mergedWordChart[j][i].start_offset<<" "
						//	<<" EndOffset: "<<_mergedWordChart[j][i].end_offset
						//	<<std::endl;
					}
					_mergedWordChart[j-1][i].end_offset = tok_end_offset;
				}
			}
		}
	}
}
bool ArabicParseSeeder::_equivalentChars(wchar_t a, wchar_t b){
	if(a==b){
		return true;
	}
	const wchar_t _gt = ArabicSymbol::ArabicChar('>');
	const wchar_t _lt =ArabicSymbol::ArabicChar('<');
	const wchar_t _A =ArabicSymbol::ArabicChar('A');
	const wchar_t _brack =ArabicSymbol::ArabicChar('{');
	const wchar_t _line =ArabicSymbol::ArabicChar('|');
	const wchar_t _p =ArabicSymbol::ArabicChar('p');
	const wchar_t _t =ArabicSymbol::ArabicChar('t');
	const wchar_t _y =ArabicSymbol::ArabicChar('y');
	const wchar_t _Y =ArabicSymbol::ArabicChar('Y');

	if(a == _gt){
			if(b == _A)	return true;
			if(b == _lt) return true;
			if(b == _brack) return true;
			if(b == _line) return true;
			return false;
	}
	if(a == _lt){
			if(b == _A)	return true;
			if(b == _gt) return true;
			if(b == _brack) return true;
			if(b == _line) return true;
			return false;
	}
	if(a == _A){
			if(b == _lt)	return true;
			if(b == _gt) return true;
			if(b == _brack) return true;
			if(b == _line) return true;

			return false;
	}
	if(a ==  _brack){
			if(b == _A)	return true;
			if(b == _gt) return true;
			if(b == _lt) return true;
			if(b == _line) return true;
			return false;
	}
	if(a == _line){
			if(b == _A)	return true;
			if(b == _gt) return true;
			if(b == _brack) return true;
			if(b == _lt) return true;
			return false;
	}
	if(a ==  _p){
			if(b == _t) return true;
			return false;
	}
	if(a ==  _t){
			if(b == _p) return true;
			return false;
	}
	if(a == _y){
			if(b == _Y) return true;
			return false;
	}
	if(a == _Y){
			if(b == _y) return true;
			return false;
	}
    return false;
	
}


void ArabicParseSeeder::_groupParseTokens(){
	int num_grouped = 0;
	for(int i =0; i<_n_lex_entries; i++){
		for(int j=0; j< _mergedNumSeg[i]; j++){ 
			int found_match =0;
			int k = 0;
			for(k=0; k <num_grouped; k++){

				if( (_mergedWordChart[j][i].start == _groupedWordChart[k].start)&&
					(_mergedWordChart[j][i].end == _groupedWordChart[k].end)&&
					(_mergedWordChart[j][i].normString == _groupedWordChart[k].nvString)){
						_groupedWordChart[k].postags.push_back(_mergedWordChart[j][i].pos);
						found_match =1;
					}
			}
			if(!found_match){

				_groupedWordChart[num_grouped].start = _mergedWordChart[j][i].start;
				_groupedWordChart[num_grouped].end = _mergedWordChart[j][i].end;
				_groupedWordChart[num_grouped].nvString = _mergedWordChart[j][i].normString;
				_groupedWordChart[k].postags.reserve(1);
				_groupedWordChart[k].postags.resize(0);
				_groupedWordChart[k].postags.push_back(_mergedWordChart[j][i].pos);

				num_grouped++;
			}
		}
	}
	_num_grouped = num_grouped;
}

void ArabicParseSeeder::_alignNoVowelWordChart(int nposs) throw (UnrecoverableException){

	//std::cout<< "_alignNoVowelWordChart()"<<std::endl;

	int maxWordLen = 0;
	int minWordLen = MAX_CHAR_PER_WORD;
	for(int n =0; n< nposs; n++){
		if(_noVowelWordLength[n] > maxWordLen){
			maxWordLen =_noVowelWordLength[n];
		}
		if(_noVowelWordLength[n] < minWordLen){
			minWordLen =_noVowelWordLength[n];
		}
	}
	if(maxWordLen >= MAX_CHAR_PER_WORD){
		throw UnrecoverableException(
		"ArabicParseSeeder::_alignNoVowelWordChart() word is more than MAX_WORD_PER_CHAR characters");
		return;
	}

	//breaks go in the character preceeding a break
	//maximum number of words +1 for composit!
	int segmentBreaks[MAX_CHAR_PER_WORD][MAX_POSSIBILITIES_PER_WORD+1];
	//intialize char Matrix
	for(int j =0; j<MAX_CHAR_PER_WORD; j++){
		for(int k=0; k<=MAX_POSSIBILITIES_PER_WORD; k++){
			segmentBreaks[j][k] =0;
		}
	}

	
	int n_poss = nposs;


	int place =0;
	int add_to_stem = 0; //number of characters to add to stem, make all words equal lenght
	int i;
	for(i =0; i<n_poss; i++){
		place = 0;
		add_to_stem =  maxWordLen - _noVowelWordLength[i];
		for(int j=0; j< _noVowelNumSeg[i]; j++){ 
			int thisSegLen = static_cast<int>(wcslen(_noVowelWordChart[j][i].normString.to_string()));
			if(_noVowelWordChart[j][i].word_part ==STEM){
				thisSegLen+=add_to_stem;
				add_to_stem =0;	//only do this once!
			}
			//std::cout<<"Segment: "<<j<<" length: "<<thisSegLen<<", ";
			if((segmentBreaks[place+thisSegLen-1][n_poss]>=  
				segmentBreaks[place+thisSegLen][n_poss]))
			{
					for(int k=place+thisSegLen; k<= maxWordLen; k++){
						segmentBreaks[k][n_poss]++;
					}
			}
			for(int k=place+thisSegLen; k<= maxWordLen; k++){
				segmentBreaks[k][i]++;
			}
			place+=thisSegLen;

		}
//		std::cout<<std::endl;
	}
	//print chart for debbuging
	/*
	for(int i= 0; i <= n_poss; i++){
		for(int j=0; j <=  maxWordLen;j++){
			std::cout<<segmentBreaks[j][i]<<"\t";
		}
		std::cout<<"\n";
	}
	*/

	
	
	int startChar, endChar  = 0;
	add_to_stem = 0;
	place =0;
	for(i= 0; i < n_poss; i++){
		place =0;
		add_to_stem =  maxWordLen - _noVowelWordLength[i];
		for(int j=0; j<  _noVowelNumSeg[i]; j++){
			int length = static_cast<int>(wcslen((_noVowelWordChart[j][i]).normString.to_string()));
			if(_noVowelWordChart[j][i].word_part ==STEM){
				length+=add_to_stem;
				add_to_stem =0;	//only do this once!
			}
			startChar = place;
			endChar = length+place-1;
			//std::cout<<"start char: "<<startChar<<" end char: "<<endChar<<std::endl;
			int startSeg =segmentBreaks[startChar][n_poss];
			int endSeg =segmentBreaks[endChar][n_poss];
			//std::cout<<"start seg: "<<startSeg<<" end seg: "<<endSeg<<std::endl;

			_noVowelWordChart[j][i].start = (segmentBreaks[startChar][n_poss]);
			_noVowelWordChart[j][i].end = (segmentBreaks[endChar][n_poss]);
			place +=length;
		}
	}
}

void ArabicParseSeeder::_populateNoVowelWordChart(const LexicalToken* t)throw (UnrecoverableException){
	int num_lex = t->getNLexicalEntries();
	if(num_lex > MAX_POSSIBILITIES_PER_WORD){
		num_lex = MAX_POSSIBILITIES_PER_WORD;
		//TODO: Log this!
	}
	
	for(int i =0; i<num_lex; i++){
		LexicalEntry* le = t->getLexicalEntry(i);
		int nseg = le->getNSegments();	
		int count =0;
		int word_length =0;
		//if it is a retokenized word, don't allow merging
		if(nseg == 0){
			//non arabic special case, or possibly a clitic that was separted in names
			_noVowelWordChart[count][i].normString = t->getSymbol();
			_noVowelWordChart[count][i].word_part = STEM;
			_noVowelWordChart[count][i].lex_ent = le;
			word_length+=static_cast<int>(wcslen(t->getSymbol().to_string()));
			count++;
		}
		else if(le->getFeatures()->getCategory() == BuckwalterFunctions::RETOKENIZED){
			//std::cout<<"Retokenized word: "<<std::endl;
			ParseToken results[MAX_SEGMENTS_PER_WORD];
			//for(int i =0; i< le->getNSegments(); i++){
			//	LexicalEntry* sub = le->getSegment(i);
			int n_pref = _getVowelessEntries(results,MAX_SEGMENTS_PER_WORD,le);
			for(int j =0; j< n_pref; j++){
				_noVowelWordChart[count][i] = results[j];
				_noVowelWordChart[count][i].word_part = RETOKENIZED;
				word_length+=
					static_cast<int>(
					wcslen(_noVowelWordChart[count][i].normString.to_string()));
				count++;
			}
							
		}
		else if(nseg !=3){
			char message[500];
//			std::cout<<"Problem: nseg:"<<nseg<<std::endl;
//			std::cout<<"ID: "<<static_cast<int>(le->getID())<<std::endl;
//			std::cout<<"Key: "<<le->getKey().to_debug_string()<<std::endl;
			sprintf(message, "Lexical Entry: %lu : %s has %d != 3 segments", 
				le->getID(),le->getKey().to_debug_string()	, nseg);

			throw UnrecoverableException("ArabicParseSeeder::_populateNoVowelWordChart()", message);
		}
		else{
			ParseToken results[MAX_SEGMENTS_PER_WORD];
			//prefixes
			LexicalEntry* prefix = le->getSegment(0);
			int n_pref = _getVowelessEntries(results,MAX_SEGMENTS_PER_WORD,prefix);
			//std::cout<<"Add prefixes: "<<n_pref<<std::endl;
			int j;
			for(j =0; j< n_pref; j++){
				_noVowelWordChart[count][i] = results[j];
				_noVowelWordChart[count][i].word_part = PREFIX;
				word_length+=static_cast<int>(wcslen(_noVowelWordChart[count][i].normString.to_string()));
				//std::cout<<"\t"<<count<<" "<<i<<" "
				//	<<_noVowelWordChart[count][i].normString.to_debug_string()
				//	<<" word length "<<word_length<<std::endl;
				count++;
			}

			//stems
			LexicalEntry* stem = le->getSegment(1);

			int n_stem = _getVowelessEntries(results,MAX_SEGMENTS_PER_WORD,stem);
			//std::cout<<"Add stems: "<<n_stem<<std::endl;
			for(j =0; j< n_stem; j++){
				_noVowelWordChart[count][i] = results[j];
				_noVowelWordChart[count][i].word_part = STEM;
				word_length+=static_cast<int>(wcslen(_noVowelWordChart[count][i].normString.to_string()));
				//std::cout<<"\t"<<count<<" "<<i<<" "
				//	<<_noVowelWordChart[count][i].normString.to_debug_string()
				//	<<" word length "<<word_length<<std::endl;
				count++;

			}

			//suffixes
			LexicalEntry* suffix = le->getSegment(2);
			int n_suffix = _getVowelessEntries(results,MAX_SEGMENTS_PER_WORD,suffix);
			//std::cout<<"Add suffixes: "<<n_suffix<<std::endl;

			for(j =0; j< n_suffix; j++){
				_noVowelWordChart[count][i] = results[j];
				_noVowelWordChart[count][i].word_part = SUFFIX;
				word_length+=static_cast<int>(wcslen(_noVowelWordChart[count][i].normString.to_string()));
			//	std::cout<<"\t"<<count<<" "<<i<<" "
			//		<<_noVowelWordChart[count][i].normString.to_debug_string()
			//		<<" word length "<<word_length<<std::endl;

				count++;
			}
		}
		_noVowelNumSeg[i]= count;
		_noVowelWordLength[i] = word_length;
		
	}
}


namespace {
    // Symbol constants used by _getVowelessEntries:
    Symbol noneSymbol;
    Symbol emptySymbol(L"");
    Symbol unknownSymbol(L"UNKNOWN");
    Symbol nullSymbol(L"NULL");
}

int ArabicParseSeeder::_getVowelessEntries(ParseToken* results, int size, LexicalEntry* initialEntry) throw (UnrecoverableException){
	LexicalEntry* temp_array[MAX_SEGMENTS_PER_WORD];
	int n =0;
	if(initialEntry->getNSegments() == 0){
		temp_array[0] = initialEntry;
		n = 1;
	}
	else{
		//std::cout<<"ArabicParseSeeder:: Call _pullLexEntriesUp()"<<std::endl;

		n =BuckwalterFunctions::pullLexEntriesUp(temp_array, MAX_SEGMENTS_PER_WORD, initialEntry);
		if(n > size){
			throw UnrecoverableException("_getVowelessEntries()","More than 'size' lexical entries");
		}

		//std::cout<<"found "<<n << " entries"<<std::endl;
		for(int i =0; i<n; i++){
		//	temp_array[i]->dump(std::cout);
		//	std::cout<<"-----------------"<<std::endl;
		}
	}
	int count =0;
	for(int j =0; j < n; j++){
		if(temp_array[j] ==NULL){
			throw UnrecoverableException("_getVowelessEntries()","null lexical entry");
		}
		Symbol word = temp_array[j]->getKey();
		Symbol key = word;
		if(temp_array[j]->getFeatures()!= NULL){
			FeatureValueStructure* fvs = temp_array[j]->getFeatures();
			if(fvs != NULL && fvs->getCategory()!=unknownSymbol){
				word = temp_array[j]->getFeatures()->getVoweledString();
			}
		}
		
		Symbol nvstr = BWNormalizer::normalize(word, key);
		if((nvstr == emptySymbol) || (nvstr== noneSymbol) ||nvstr == nullSymbol){
			;
		}
		else{
			results[count].normString = nvstr;
			results[count].lex_ents.reserve(1);
			results[count].lex_ents.resize(0);
			results[count].lex_ents.push_back(temp_array[j]);
			count++;
		}

	}
	return count;
}
