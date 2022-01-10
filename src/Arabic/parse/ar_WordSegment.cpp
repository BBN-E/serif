// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ar_WordSegment.h"
#include "Arabic/parse/ar_Segment.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/SessionLogger.h"
using namespace std;

//const size_t ChartToken::max_possibilities = CHART_TOKEN_MAX_POSS;
//const size_t ChartToken::max_segments = CHART_TOKEN_MAX_SEG_PER_WORD;
WordSegment::~WordSegment(){
	for(int i = 0; i<_num_possibilities; i++){
		for(int j = 0; j < _possibilities[i].numSeg; j++){
			delete _possibilities[i].segments[j];
		}
	}
}


WordSegment::WordSegment(const WordSegment::Segmentation* possibilities, int num_poss){
	wchar_t wholeword[100];
	int not_found[50];
	int numNotFound =0;
	int let =0;
	for(int i =0; i<possibilities[0].numSeg; i++){
		const wchar_t* seg = possibilities[0].segments[i].to_string();
		for(int j=0; j< static_cast<int>(wcslen(seg)); j++){
			wholeword[let++] = seg[j];
		}
	}
	wholeword[let++] = L'\0';
	_originalWord = Symbol(wholeword);
//	std::cout <<"Original Word "<<_originalWord.to_debug_string()<<std::endl;
	WordSegment* temp = _new WordSegment();
	temp->getSegments(Symbol(wholeword));
//	std::cout<<"All possibilities "<<std::endl;
	temp->printSegment();
//	std::cout<<std::endl;
	int curr_pos =0;
	for(int j=0; j<num_poss; j++){
		bool found = false;
		for(int i=0; i< temp->getNumPossibilities(); i++){
			if(temp->_possibilities[i].numSeg == possibilities[j].numSeg){
				bool all_same = true;
				for(int k=0; k<possibilities[j].numSeg; k++){
					if(possibilities[j].segments[k]!=temp->_possibilities[i].segments[k]->getText()){
						all_same = false;
						break;
					}
				}
				if(all_same){
					_possibilities[curr_pos].numSeg = temp->_possibilities[i].numSeg;
					for(int k =0; k< _possibilities[curr_pos].numSeg; k++){
						_possibilities[curr_pos].segments[k] = _new Segment(temp->_possibilities[i].segments[k]);
					}
					found = true;
					curr_pos++;
					break;
				}
			}
		} 
		if(!found){
			not_found[numNotFound++] = j;
		}
	}
	if(numNotFound >0){			
		SessionLogger::logger->beginError();
		*SessionLogger::logger<<"WordSegments found by BW by not found by getSegments()\n";

		for(int i =0; i< numNotFound; i++){
			*SessionLogger::logger<<"\t( ";
			for(int j=0; j <possibilities[not_found[i]].numSeg;j++){
				*SessionLogger::logger<<" ( "<<possibilities[not_found[i]].segments[j].to_debug_string()<<" )";
			}
			*SessionLogger::logger<<")\n";
		}
	}
	if(numNotFound == num_poss){
		/*throw UnrecoverableException(
			"WordSegment::WordSegment", 
			"None of BW segments were produced by getSegments()\n");
		*/
		//
		*SessionLogger::logger<<"\tUsing getSegments possibilities, not BW!\n";
		*SessionLogger::logger<<"\tFor "<<temp->getOriginalWord().to_debug_string()<<"\n";
//		std::cout<<"\tUsing getSegments possibilities, not BW!\n";
//		std::cout<<"\tFor "<<temp->getOriginalWord().to_debug_string()<<std::endl;
		
		curr_pos =0;
		for(int i=0; i< temp->getNumPossibilities(); i++){
			_possibilities[curr_pos].numSeg = temp->_possibilities[i].numSeg;
			for(int k =0; k< _possibilities[curr_pos].numSeg; k++){
				_possibilities[curr_pos].segments[k] = _new Segment(temp->_possibilities[i].segments[k]);
			}
			curr_pos++;
		}
		
	}
	
	

	_num_possibilities = curr_pos;
	delete temp;
	//Don't allow one segment words to span more nonexistant tokens
	if(_num_possibilities == 1){
		if(_possibilities[0].numSeg ==1){
			_possibilities[0].segments[0]->setEnd(0);
		}
		else{
			for(int i =0; i < _possibilities[0].numSeg; i++){
				Segment* old_seg = _possibilities[0].segments[i];
				if(old_seg->getStart() != i) {
					_possibilities[0].segments[i]->setStart(i);
				}
				if(old_seg->getEnd() != i){
					_possibilities[0].segments[i]->setEnd(i); 
				}
			}
		}

	}
	


	//error checking to be sure segments are consistent
	int max_num_seg = maxLength();
	for(int i =0; i< _num_possibilities; i++){
		if(_possibilities[i].segments[_possibilities[i].numSeg-1]->getEnd() >
			max_num_seg){
				std::cerr<<"*******Problem with segmentation length"<<std::endl;
				printSegment();
//				std::cout<<std::endl;
		_fixSegmentation();
			//throw UnrecoverableException(
			//	"WordSegment::WordSegment", 
			//	"BW segments _end incorrect\n");

			}
	}





//	std::cout<<"Valid possibilities: \n";
//	printSegment();
//	std::cout<<std::endl;

	
}



void WordSegment::_fixSegmentation(){
//	std::cout<<"###### _fixSegmentation()"<<std::endl;
	int thisWordLen = static_cast<int>(wcslen(_originalWord.to_string()));
	if(thisWordLen >= 500){
		throw UnrecoverableException(
		"WordSegment::_fixSegmentation() word is more than 500 characters");
		return;
	}
	//breaks go in the character preceeding a brea
	int segmentBreaks[500][20];
	//intialize char Matrix
	for(int i =0; i<500; i++){
		for(int j=0; j<20; j++){
			segmentBreaks[i][j] =0;
		}
	}

//[by SRS; unref'ed var]	int breakIndex[20];
//[by SRS; unref'ed var]	int numBreaks;
	
	int n_poss = _num_possibilities;
//[by SRS; unref'ed var]	int n;
	for(int i= 0; i < n_poss; i++){
		int place =0;
		for(int j=0; j< _possibilities[i].numSeg; j++){
			int thisSegLen = static_cast<int>(
				wcslen(_possibilities[i].segments[j]->getText().to_string()));

			if((segmentBreaks[place+thisSegLen][n_poss]<=  
				segmentBreaks[place+thisSegLen+1][n_poss]))
			{
				for(int k=place+thisSegLen; k<=thisWordLen; k++){
					segmentBreaks[k][n_poss]++;
				}
			}
			for(int k=place+thisSegLen; k<thisWordLen; k++){
				segmentBreaks[k][j]++;
			}
			place += thisSegLen;
		}

	}
	//print chart for debugging
//	for(int i= 0; i <= n_poss; i++){
//		for(int j=0; j <= thisWordLen;j++){
//			std::cout<<segmentBreaks[j][i]<<"\t";
//		}
//		std::cout<<"\n";
//	}

	int startChar, endChar, place =0;
	for(int i= 0; i < n_poss; i++){
		place =0;
		for(int j=0; j< _possibilities[i].numSeg; j++){
			int length = static_cast<int>(
				wcslen(_possibilities[i].segments[j]->getText().to_string()));
			startChar = place;
			endChar = length+place-1;
			int startSeg =segmentBreaks[startChar][n_poss];
			int endSeg =segmentBreaks[endChar][n_poss];

			_possibilities[i].segments[j]->setStart(segmentBreaks[startChar][n_poss]);
			_possibilities[i].segments[j]->setEnd(segmentBreaks[endChar][n_poss]);
			place +=length;



		}
	}
	return;

}


void WordSegment::printSegment(){
	std::cout<< "Possibilities: "<<_num_possibilities<<"\n";
	for(int i= 0; i<_num_possibilities;i++){
		std::cout<< "\tSegments: "<< _possibilities[i].numSeg<<"\t";
		for(int j=0; j < _possibilities[i].numSeg;j++){
			Segment* t = _possibilities[i].segments[j];
			std::cout 
				<< " ("
				<<t->getText().to_debug_string()
				<<" "
				<<t->getStart()
				<<" "
				<<t->getEnd()
				<<") ";
		}
		std::cout<<"\n";
	}
}
void WordSegment::getSegments(Symbol token){
	_num_possibilities = 0;
	_originalWord = token;
	Symbol sub[5];
	int type[5];
	int start[5];
	//always put the word in by itself
	sub[0] = token;
	type[0] = Segment::STEM;
	start[0] =0;
	_addPossibility(sub, start, type, 1);
	const wchar_t* word = token.to_string();
	size_t len = wcslen(word);
	if(len < 2){
		_possibilities[0].segments[0]->setEnd(0);		
		return;
	}
	//look for affixes
	int p1 = _prefix(word, 0);
	int p2 = 0;
	if((p1 == CONJ) && (len >2)){	//mrf - can never have conj+prep as full word
		p2 = _prefix(word, 1);
		if(p2 == CONJ) p2 = 0;
	}
	int suff = _suffix(word);
	//look for possible errors in small words
	size_t suffSize = suff;
	size_t p1Size = p1 ? 1 : 0;
	size_t p2Size = p2 ? 1 : 0;
	if(suff == 6 || suff ==5) suffSize = 3;
	bool tooGrainy =false;
	if(len <= (p1Size + p2Size + suffSize) ){	
		tooGrainy = true;

	}
	

	if(p1){
		//Add prefix and word
		type[0] = Segment::PREFIX;
		type[1] = Segment::STEM;
		start[1] = 1;
		sub[0] = _subWord(word, 0,1);	
		sub[1] = _subWord(word, 1,len); 
		_addPossibility(sub, start, type, 2);
		if(p2){
			//add conj, second prefix, word
			type[1] = Segment::PREFIX;
			type[2] = Segment::STEM;
			start[2] = 2;
			sub[1] = _subWord(word, 1, 2);
			sub[2] = _subWord(word, 2,len);
			_addPossibility(sub,start,type, 3);
			if(suff && !tooGrainy){
				start[3] = 3;
				//add conj, second prefix, word, suffix
				if( (suff ==5) || (suff ==6)){
					_addComplexPossibility(sub, start, type, suff, 3);
				}
				else{
					type[2] = Segment::STEM;
					type[3] = Segment::SUFFIX;
					sub[2] = _subWord(word, 2, len-suff);
					sub[3] = _subWord(word, len - suff, len);
					_addPossibility(sub, start, type, 4);
				}
			}
		}
		if(suff && (p2 || !tooGrainy) ){
			//add prefix, word, suffix
			start[1] = 1;
			type[1] = Segment::STEM;
			start[2] = p2 ? 3 : 2;
			if(tooGrainy) start[2]--;
			if( (suff ==5) || (suff ==6)){
				sub[1] = _subWord(word, 1, len);
				_addComplexPossibility(sub, start, type, suff, 2);
			}
			else{
				type[2] = Segment::SUFFIX;
				sub[1] = _subWord(word, 1, len-suff);
				sub[2] = _subWord(word, len - suff, len);
				_addPossibility(sub, start, type, 3);
			}
		}
	}
	if(suff && (p1 || !tooGrainy)){
		//add word, suffix
		start[0] = 0;
		type[0] = Segment::STEM;
		if(p1){
			start[1] = p2 ? 3 :2;
			if(tooGrainy) start[1]--;
		}
		else start[1] = 1;	
		if( (suff ==5) || (suff ==6)){
			sub[0] = _subWord(word, 0, len);
			_addComplexPossibility(sub, start, type, suff, 1);
		}
		else{
			sub[0] = _subWord(word, 0, len-suff);
			sub[1] = _subWord(word, len - suff, len);
			type[1] = Segment::SUFFIX;
			_addPossibility(sub, start,type, 2);

		}
	}
	//put end values in segments
	int maxSeg = maxLength();
	int i = 0;
	for(i = 0; i<_num_possibilities; i++){
		int numSeg = _possibilities[i].numSeg;
		int j = 0;
		for(j =0; j < numSeg -1; j++){
			_possibilities[i].segments[j]->setEnd( 
				_possibilities[i].segments[j+1]->getStart()-1);
		}
		if(_emptyLeaf)
			_possibilities[i].segments[j]->setEnd(maxSeg);
		else
			_possibilities[i].segments[j]->setEnd(maxSeg-1);
	}
}
void WordSegment::_addComplexPossibility(const Symbol* s, const int* places,
										 const int* type, int suffix, int numStr)
{
	//need to make an empty leaf cell in the chart
	_emptyLeaf = true;
	Symbol wholeWord = s[numStr-1];
	const wchar_t* wordStr = wholeWord.to_string();
	size_t len = wcslen(wordStr);
	Symbol word1;
	Symbol word2;
	Symbol suff1;
	Symbol suff2;
	if(suffix == 5){	//ny, y
		word1 = _subWord(wordStr, 0, len-1);
		suff1 = _subWord(wordStr, len-1, len);
		word2 = _subWord(wordStr, 0, len-2);
		suff2 = _subWord(wordStr, len-2, len);
	}
	else if(suffix == 6){	//hmA, mA
		word1 = _subWord(wordStr, 0, len-2);
		suff1 = _subWord(wordStr, len-2, len);
		word2 = _subWord(wordStr, 0, len-3);
		suff2 = _subWord(wordStr, len-3, len);
	}
	for(int i = 0; i< numStr-1; i++){
		_possibilities[_num_possibilities].segments[i] = 
			_new Segment(s[i],places[i], type[i]);
	}
	//since XXXss -> (XXXs)(s) (XXX)(ss) , ((XXX)(s))(s), (XXX)((s)(s))
	_possibilities[_num_possibilities].segments[numStr-1] = 
		_new Segment(word1,places[numStr-1], Segment::STEM);
	_possibilities[_num_possibilities].segments[numStr] = 
		_new Segment(suff1, places[numStr]+1, Segment::SUFFIX);	//places[numStr-1]+2
	_possibilities[_num_possibilities].numSeg = numStr+1;
	_num_possibilities++;
	for(int i = 0; i< numStr-1; i++){
		_possibilities[_num_possibilities].segments[i] = 
			_new Segment(s[i],places[i], type[i]);
	}
	_possibilities[_num_possibilities].segments[numStr-1] = 
		_new Segment(word2,places[numStr-1], Segment::STEM);
	_possibilities[_num_possibilities].segments[numStr] = 
		_new Segment(suff2, places[numStr], Segment::SUFFIX); //places[numStr-1]+1
	_possibilities[_num_possibilities].numSeg = numStr+1;	
	_num_possibilities++;
}




void WordSegment::_addPossibility(const Symbol* s, const int* places,  
								  const int* type, int numStr){
	//hack only puts in valid possibilities, if any segments are empty,
	//do nothing
	for(int i = 0; i<numStr; i++){
		if(wcslen(s[i].to_string())<1)
			return;
	}
	for(int i = 0; i< numStr; i++){
		_possibilities[_num_possibilities].segments[i] = 
			_new Segment(s[i],places[i], type[i]);
	}
	_possibilities[_num_possibilities].numSeg = numStr;
	_num_possibilities++;
}

Symbol WordSegment::_subWord(const wchar_t* str, size_t start, size_t end){
	int j = 0;
	for(size_t i = start; i<end; i++, j++)
		_str_buff[j] = str[i];
	_str_buff[j] = L'\0';
	return Symbol(_str_buff);
}
/*Look for a clitic suffix, return the number of characters in the 
* suffix.  Return 5 for suffix ny, which must be treated as both
* XXXXn+y and XXXX+ny 
* Return 6 for hmA, which must be treated as both
* XXXXh+mA and XXXX+hmA
*/
int WordSegment::_suffix(const wchar_t* word){
	size_t wordLength = wcslen(word);
	wchar_t lc = word[wordLength-1];
	wchar_t lc2 = 0;
	wchar_t lc3 = 0;
	if(wordLength > 2) lc2 =word[wordLength -2];
	if(wordLength > 3) lc3 = word[wordLength -3];
	if(lc == ArabicSymbol::ArabicChar('k')||
		lc==ArabicSymbol::ArabicChar('h'))
		return 1;
	if(lc== ArabicSymbol::ArabicChar('y')){
		if(lc2 ==ArabicSymbol::ArabicChar('n'))
			return 5;
		return 1;
	}
	//hmA	
	if((lc ==ArabicSymbol::ArabicChar('A'))&&
		(lc2==ArabicSymbol::ArabicChar('m'))&&
		(lc3 == ArabicSymbol::ArabicChar('h'))) return 6;
	//kmA added for BW
	if((lc ==ArabicSymbol::ArabicChar('A'))&&
		(lc2==ArabicSymbol::ArabicChar('m'))&&
		(lc3 == ArabicSymbol::ArabicChar('k'))) return 6;

	//nA, hA,mA 
	if((lc == ArabicSymbol::ArabicChar('A'))&&
		((lc2 ==ArabicSymbol::ArabicChar('n'))||
			(lc2==ArabicSymbol::ArabicChar('h'))||
			(lc2==ArabicSymbol::ArabicChar('m')))) return 2;
	//km, hm
	if((lc == ArabicSymbol::ArabicChar('m'))&& 
		((lc2 ==ArabicSymbol::ArabicChar('h'))||
		(lc2 == ArabicSymbol::ArabicChar('k')))) return 2;
	//hn, //kn added for BW
	if((lc == ArabicSymbol::ArabicChar('n')) && 
		((lc2 ==ArabicSymbol::ArabicChar('h')) ||
		(lc2 ==ArabicSymbol::ArabicChar('k')))) return 2;
	//nhn
	if((lc ==ArabicSymbol::ArabicChar('n'))&&
		(lc2==ArabicSymbol::ArabicChar('h'))&&
		(lc3 == ArabicSymbol::ArabicChar('n'))) return 3;
	return 0;
}
/*Look for a clitic prefix in a word return the type of prefix
*
*/

int WordSegment::_prefix(const wchar_t* word, int loc){
	wchar_t c = word[loc];
	if(c == ArabicSymbol::ArabicChar('w') || 
		c ==ArabicSymbol::ArabicChar('f'))
		return CONJ;
	else if(c == ArabicSymbol::ArabicChar('b') || 
			c == ArabicSymbol::ArabicChar('k') || 
			c == ArabicSymbol::ArabicChar('l'))
		return PREP;
	/* This particle only occurs once in training so ignore it for now
	else if(wstrlen(word)>loc+1)
		if(c == ArabicSymbol::ArabicChar('m') && 
			word[loc+1]==ArabicSymbol::ArabicChar('A'))
			return PART;
	*/
	return 0;
}
int WordSegment::maxLength() const {
	int max = 0;
    for (int i = 0; i < _num_possibilities; i++) {
		if(_possibilities[i].numSeg > max)
			max = _possibilities[i].numSeg;
	}
	return max;
}
WordAnalysis WordSegment::getLongest()const {
	int max = 0;
	int index = 0;
    for (int i = 0; i < _num_possibilities; i++) {
		if(_possibilities[i].numSeg > max){
			max = _possibilities[i].numSeg;
			index = i;
		}
	}
	return _possibilities[index];
}

int WordSegment::lengthOf(int index) const {
	if((index < 0) || (index > _num_possibilities))
		return 0;
	return _possibilities[index].numSeg;
}

WordAnalysis WordSegment::getPossibility(int index) const {
	if (index >= 0 && index < _num_possibilities) {
		return _possibilities[index];
    }
	WordAnalysis empty;
	empty.numSeg =0;
	return empty;
}
int WordSegment::getNumPossibilities() const{
	return _num_possibilities;
}

int WordSegment::getMaxSegments(const wchar_t *word) {
	int result = 1;

	// first count prefix segments
	int prefix = _prefix(word, 0);
	if (prefix == CONJ) {
		result++;
		prefix = _prefix(word, 1);
		if (prefix != NONE)
			result++;
	}
	else if (prefix != NONE) {
		result++;
	}

	// now count suffix segments
	int suffix = _suffix(word);
	if (suffix >= 5)
		result += 2;
	else if (suffix > 0)
		result += 1;

	return result;
}



/*
*********************
		if(p1 && p2 && suff){
			//add just the prefix1, word
			type[0] = Segment::PREFIX;
			type[1] = Segment::STEM;
			start[1] = 1;
			sub[0] = _subWord(word, 0,1);	
			sub[1] = _subWord(word, 1,len); 
			_addPossibility(sub, start, type, 2);
			//add conj, second prefix, word
			type[1] = Segment::PREFIX;
			type[2] = Segment::STEM;
			start[2] = 2;
			sub[1] = _subWord(word, 1, 2);
			sub[2] = _subWord(word, 2,len);
			_addPossibility(sub, start, type, 3);
			//conj, prefix, stem, suffix does not exist!
			//add conj, word , suffix
			//add prefix, word, suffix
			start[1] = 1;
			type[1] = Segment::STEM;
			if( (suff ==5) || (suff ==6)){
				sub[1] = _subWord(word, 1, len);
				_addComplexPossibility(sub, start, type, suff, 2);
			}
			else{
				start[2] = 2;
				type[2] = Segment::SUFFIX;
				sub[1] = _subWord(word, 1, len-suff);
				sub[2] = _subWord(word, len - suff, len);
				_addPossibility(sub, start, type, 3);
			}
			//add word, suffix
			start[0] = 0;
			type[0] = Segment::STEM;
			if( (suff ==5) || (suff ==6)){
				sub[0] = _subWord(word, 0, len);
				_addComplexPossibility(sub, start, type, suff, 1);
			}
			else{
				sub[0] = _subWord(word, 0, len-suff);
				sub[1] = _subWord(word, len - suff, len);
				type[1] = Segment::SUFFIX;
				start[1]
			}
			else start[1] = 1;	
			//start[1] = 1;
			_addPossibility(sub, start,type, 2);
	

		_possibilities[0].segments[0]->setEnd(0);		
		return;
	}
	*/

/*
		if(p1){
			Symbol prefix= _subWord(word, 0,1);
			if(sub[0] == prefix) start[1] = 1;
			else {
				prefix = _subWord(word,1,1);
				if(sub[0]==prefix) start[1] = 2; 
				else {
					start[1] = p2 ?  3 : 2;
					if(tooGrainy) start[1]--;
				}
			}
		}
		else start[1] = 1;	
	*/
