// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/common/ar_WordConstants.h"
#include "Arabic/common/ar_ArabicSymbol.h"

Symbol ArabicWordConstants::I_1 = ArabicSymbol(L">nA");
Symbol ArabicWordConstants::I_2 = ArabicSymbol(L"AnA");

Symbol ArabicWordConstants::YOU_S_1=ArabicSymbol(L">nt"); //sing masc. & fem. differ only in vowels
Symbol ArabicWordConstants::YOU_S_2=ArabicSymbol(L"Ant"); //sing masc. & fem. differ only in vowels

Symbol ArabicWordConstants::YOU_D_1=ArabicSymbol(L">ntmA");
Symbol ArabicWordConstants::YOU_D_2=ArabicSymbol(L"AntmA");

Symbol ArabicWordConstants::YOU_MP_1=ArabicSymbol(L">ntm");
Symbol ArabicWordConstants::YOU_FP_1=ArabicSymbol(L">ntn");
Symbol ArabicWordConstants::YOU_MP_2=ArabicSymbol(L"Antm");
Symbol ArabicWordConstants::YOU_FP_2=ArabicSymbol(L"Antn");
Symbol ArabicWordConstants::HE=ArabicSymbol(L"hw");
Symbol ArabicWordConstants::SHE=ArabicSymbol(L"hy");
Symbol ArabicWordConstants::THEY_M=ArabicSymbol(L"hm");
Symbol ArabicWordConstants::THEY_D=ArabicSymbol(L"hmA");//is both F and M?
Symbol ArabicWordConstants::THEY_F=ArabicSymbol(L"hmn");

//new stuff to deal with clitics
Symbol ArabicWordConstants::MY=ArabicSymbol(L"y");
Symbol ArabicWordConstants::OUR=ArabicSymbol(L"ny");

Symbol ArabicWordConstants::YOUR=ArabicSymbol(L"k");
Symbol ArabicWordConstants::YOUR_P1=ArabicSymbol(L"km");
Symbol ArabicWordConstants::YOUR_P2=ArabicSymbol(L"kn");
Symbol ArabicWordConstants::YOUR_D=ArabicSymbol(L"kmA");

Symbol ArabicWordConstants::HIS=ArabicSymbol(L"h");
Symbol ArabicWordConstants::HER=ArabicSymbol(L"hA");
Symbol ArabicWordConstants::THEIR=ArabicSymbol(L"hn");

Symbol ArabicWordConstants::MASC = Symbol(L":MASC");
Symbol ArabicWordConstants::FEM =Symbol(L":FEM");
Symbol ArabicWordConstants::UNKNOWN =Symbol(L":UNKNOWN");
Symbol ArabicWordConstants::SINGULAR = Symbol(L":SING");
Symbol ArabicWordConstants::PLURAL = Symbol(L":PL");
Symbol ArabicWordConstants::DUAL = Symbol(L":DUAL");

// prefix clitics
Symbol ArabicWordConstants::L = ArabicSymbol(L"l");
Symbol ArabicWordConstants::B = ArabicSymbol(L"b");
Symbol ArabicWordConstants::W = ArabicSymbol(L"w");
Symbol ArabicWordConstants::F = ArabicSymbol(L"f");
Symbol ArabicWordConstants::K = ArabicSymbol(L"k");

Symbol ArabicWordConstants::_nationalityEndings[] = 
{
	ArabicSymbol(L"ytAn"),
	ArabicSymbol(L"ytyn"),
	ArabicSymbol(L"yyn"),
	ArabicSymbol(L"yAn"),
	ArabicSymbol(L"ywn"),
	ArabicSymbol(L"yAt"),
	ArabicSymbol(L"yA"),
	ArabicSymbol(L"yw"),
	ArabicSymbol(L"yy"),
	ArabicSymbol(L"yp"),
	ArabicSymbol(L"y"),
	ArabicSymbol(L"A")
};
Symbol ArabicWordConstants::_gpe_words[] = 
{

	ArabicSymbol(L"mdynp"), //city
	ArabicSymbol(L"qryp"), //village
	ArabicSymbol(L"bldp"), //town
	ArabicSymbol(L"dwlp"), //country
	ArabicSymbol(L"jmhwryp"), //country
	ArabicSymbol(L"mHAfZp"), //county /province
	ArabicSymbol(L"mqATEp"), //county /province
	ArabicSymbol(L"dwlp"), //country
	ArabicSymbol(L"jmhwryp"), //country
	ArabicSymbol(L"wlAyp"), //province
	ArabicSymbol(L"mmlkp"), //kingdom
	ArabicSymbol(L"AmArp"), //emirate
	ArabicSymbol(L"dwlp"), //country
};

Symbol ArabicWordConstants::_fac_words[] = 
{
	ArabicSymbol(L"mxym"), //camp
	ArabicSymbol(L"mstwTnp"), //settlement
	ArabicSymbol(L"mTAr"), //airport
	ArabicSymbol(L"mynAG"), //port
	ArabicSymbol(L"msjd"), //mosque
	ArabicSymbol(L"knysp"), //church
	ArabicSymbol(L"jAmE"), //mosque
	ArabicSymbol(L"mjmE"), //compound
	ArabicSymbol(L"qAEdp"), //base
	ArabicSymbol(L"mHTp"), //station
	ArabicSymbol(L"mlEb"), //field
	ArabicSymbol(L"stAd"), //stadium
	ArabicSymbol(L"AstAd"), //stadium
	ArabicSymbol(L"mqbrp"), //cemetery
	ArabicSymbol(L"$ArE"), //street
	ArabicSymbol(L"mntjE"), //resort
	ArabicSymbol(L"bQr"), //well
	ArabicSymbol(L"mEbd"), //temple
	ArabicSymbol(L"mqhY"), //cafe
	ArabicSymbol(L"$sjn"), //jail
	ArabicSymbol(L"fndq"), //hotel
	ArabicSymbol(L"jAmEp"), //university
	ArabicSymbol(L"klyp"), //college
	ArabicSymbol(L"mst$fY"), //hospital
	ArabicSymbol(L"mktbp"), //library
	ArabicSymbol(L"Tryq"), //street
	ArabicSymbol(L"mEhd"), //institution
	ArabicSymbol(L"qnAp"), //channel
	ArabicSymbol(L"$bkp") //network
};

Symbol ArabicWordConstants::_org_words[] = {
	 ArabicSymbol(L"$rkp"), //settlement
	 ArabicSymbol(L"Hzb"), //party (political)
	 ArabicSymbol(L"jmAEp"), //group
	 ArabicSymbol(L"Hrkp"), //movement
	 ArabicSymbol(L"mnZmp"), //organization
	 ArabicSymbol(L"mHTp"), //station
	 ArabicSymbol(L"rAbTp"), //association
	 ArabicSymbol(L"wkAlp"), //agency
	 ArabicSymbol(L"tnZym"), //order
	 ArabicSymbol(L"jmEyp"), //coop
	 ArabicSymbol(L"AtHAd"), //union
	 ArabicSymbol(L"nqAbp"), //union
	 ArabicSymbol(L"nAdy"), //club
	 ArabicSymbol(L"nqAbp"), //union
	 ArabicSymbol(L"ljnp"), //committe
	 ArabicSymbol(L"mjls"), //council
	 ArabicSymbol(L"hyQp"), //comitte
	 ArabicSymbol(L"mEhd"), //institution
	 ArabicSymbol(L"dAr"), //publishing house
	 ArabicSymbol(L"Sndwq"), //fund
	 ArabicSymbol(L"mrkz"), //center
	 ArabicSymbol(L"qnAp"), //channel
	 ArabicSymbol(L"$bkp"), //network
	 ArabicSymbol(L"mLssp"), //foundation
	 ArabicSymbol(L"jAmEp"), //university
	 ArabicSymbol(L"klyp"), //college
	 ArabicSymbol(L"mst$fY") //hospital
};

Symbol ArabicWordConstants::_loc_words[] = {ArabicSymbol(L"Hy")}; //neighborhood
bool ArabicWordConstants::matchWordToArray(Symbol word, Symbol* array, int max){
	for(int i =0; i< max; i++){
		if(word == array[i]){
			return true;
		}
	}
	return false;
}
/*
	static Symbol _gpe_words[13];
	static Symbol _fac_words[29];
	static Symbol _org_words[27];
	static Symbol _loc_words[1];
*/
bool ArabicWordConstants::matchGPEWord(Symbol word){
	if(matchWordToArray(word, _gpe_words, 13))
		return true;
	return false;
}
bool ArabicWordConstants::matchORGWord(Symbol word){
	if(matchWordToArray(word, _org_words, 27))
		return true;
	return false;
}
bool ArabicWordConstants::matchFACWord(Symbol word){
	if(matchWordToArray(word, _fac_words, 29))
		return true;
	return false;
}
bool ArabicWordConstants::matchLOCWord(Symbol word){
	if(matchWordToArray(word, _loc_words, 1))
		return true;
	return false;
}

Symbol ArabicWordConstants::guessGender(Symbol word){
	const wchar_t* wordstr = word.to_string();
	int len = static_cast<int>(wcslen(wordstr));
	if(len <3)
		return UNKNOWN;
	if(wordstr[len-1] == L'\x648')	//singular
		return FEM;
	if((wordstr[len-1] == L'\x62A') && (wordstr[len-2] == L'\x627') ) //plural
		return FEM;
	if((wordstr[len-1] == L'\x646') && (wordstr[len-2] == L'\x627') ) { //dual
		if(wordstr[len-3] == L'\x62A') //dual
			return FEM;
		else return MASC;
    }
	if((wordstr[len-1] == L'\x646') && (wordstr[len-2] == L'\x64A') ) {
		if(wordstr[len-3] == L'\x62A') //dual
			return FEM;
		else return MASC;
    }
	return MASC;
}
Symbol ArabicWordConstants::guessNumber(Symbol word){
	const wchar_t* wordstr = word.to_string();
	int len = static_cast<int>(wcslen(wordstr));
	if(len <2)
		return UNKNOWN;
	if(wordstr[len-1] == L'\x648')	//singular fem
		return SINGULAR;
	if((wordstr[len-1] == L'\x62A') && (wordstr[len-2] == L'\x627') ) //plural fem
		return PLURAL;
	if((wordstr[len-1] == L'\x646') && (wordstr[len-2] == L'\x627') ) //dual 
		return DUAL;
	if((wordstr[len-1] == L'\x646') && (wordstr[len-2] == L'\x64A') ) //dual 
		return DUAL;
	return UNKNOWN;
}



bool ArabicWordConstants::is1pPronoun(Symbol word) {
	return ((word == I_1)||
			(word == I_2) ||
			//new
			(word == MY) ||
			(word == OUR)	);
}
bool ArabicWordConstants::isSingular1pPronoun(Symbol word) {
	return ((word == I_1) || (word == I_2) || (word == MY));
}
bool ArabicWordConstants::is2pPronoun(Symbol word) {
	return ((word == YOU_S_1)||
			(word == YOU_S_2)||
			(word == YOU_D_1)||
			(word == YOU_S_2)||
			(word == YOU_MP_1)||
			(word == YOU_MP_2)||
			(word == YOU_FP_1)||
			(word == YOU_FP_2)  || 
			//new
			(word == YOUR)||
			(word == YOUR_P1)||
			(word == YOUR_P2)||
			(word == YOUR_D));
}

bool ArabicWordConstants::is3pPronoun(Symbol word) {
	return ((word == HE)||
			(word == SHE)||
			(word == THEY_M)||
			(word == THEY_D)||
			(word == THEY_F)  ||
			//new
			(word == HIS) ||
			(word == THEIR) ||
			(word == HER)
			);
}
bool ArabicWordConstants::isDiacritic(wchar_t c){
	bool ret_val = ((c == L'\x64E') ||
				(c == L'\x64F') ||
				(c == L'\x650') ||
				(c == L'\x651') ||
				(c == L'\x652'));
	return ret_val;
}
bool ArabicWordConstants::isPrefixClitic(Symbol word) {
	return (word == L ||
			word == B ||
			word == W ||
			word == F ||
			word == K);
}
bool ArabicWordConstants::isNonKeyCharacter(wchar_t c){
	bool ret_val = (isDiacritic(c) || 
				(c == L'\x670') ||	//these don't have win encoding
				(c == L'\x671') ||
				(c == L'\x6A4') ||
				(c == L'\x640'));		//removed separately from diacritics
	return ret_val;
}
bool ArabicWordConstants::isEquivalentChar(wchar_t a, wchar_t b){
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

Symbol ArabicWordConstants::removeAl(Symbol word)
{
		wchar_t buffer[500];
		buffer[0] = L'\x627';	//alef
		buffer[1] = L'\x644';	//lam
		buffer[2] = L'\0';
		if(wcslen(word.to_string()) <=2){
			return word;
		}
		if(wcsncmp(word.to_string() ,buffer,2) == 0){
			int sz = static_cast <int>(wcslen(word.to_string()));
			sz = sz <498 ? sz : 498;
			int i = 0;
			for(i =2; i<sz; i++){
				buffer[i-2] = word.to_string()[i];
			}
			buffer[i-2] =L'\0';
			return Symbol(buffer);
		}
		return word;
}

int ArabicWordConstants::getFirstLetterAlefVariants(Symbol word, Symbol* variants, int max){
	int nvariants = 0;
	/*
		_mapping['|']  = L'\x0622';
		_mapping['>']  = L'\x0623';
		_mapping['<']  = L'\x0625';
		_mapping['}']  = L'\x0626';
	*/
	wchar_t firstletter = word.to_string()[0];
	wchar_t alef = L'\x627';
	wchar_t alef_hamza_above = L'\x623';
	wchar_t alef_hamza_below = L'\x625';
	wchar_t alef_madda = L'\x622';

	if(! ((firstletter ==  alef) || (firstletter == alef_hamza_above) 
		|| (firstletter == alef_hamza_below) || (firstletter == alef_madda)))
	{
			return 0;
	}
	if(max <4 ){
			return 0; 
	}
	wchar_t buffer[500];
	wcscpy(buffer, word.to_string());
	buffer[0] = alef;
	variants[0] = Symbol(buffer);
	buffer[0] = alef_hamza_above;
	variants[1] = Symbol(buffer);
	buffer[0] = alef_hamza_below;
	variants[2] = Symbol(buffer);
	buffer[0] = alef_madda;
	variants[3] = Symbol(buffer);
	return 4;

}

int ArabicWordConstants::getLastLetterAlefVariants(Symbol word, Symbol* variants, int max){
	int nvariants = 0;
	/*
		_mapping['|']  = L'\x0622';
		_mapping['>']  = L'\x0623';
		_mapping['<']  = L'\x0625';
		_mapping['}']  = L'\x0626';
	*/
	int length = static_cast<int>(wcslen(word.to_string()));
	int last = length -1;
	wchar_t lastletter = word.to_string()[length-1];
	wchar_t alef = L'\x627';
	wchar_t alef_hamza_above = L'\x623';
	wchar_t alef_hamza_below = L'\x625';
	wchar_t alef_madda = L'\x622';

	if(! ((lastletter ==  alef) || (lastletter == alef_hamza_above) 
		|| (lastletter == alef_hamza_below) || (lastletter == alef_madda)))
	{
			return 0;
	}
	if(max <4 ){
			return 0; 
	}
	wchar_t buffer[500];
	wcscpy(buffer, word.to_string());
	buffer[length-1] = alef;
	variants[0] = Symbol(buffer);
	buffer[length-1] = alef_hamza_above;
	variants[1] = Symbol(buffer);
	buffer[length-1] = alef_hamza_below;
	variants[2] = Symbol(buffer);
	buffer[length-1] = alef_madda;
	variants[3] = Symbol(buffer);
	return 4;

}
Symbol ArabicWordConstants::getNationalityStemVariant(Symbol word){

	//endings only occur once, and per inspection don't end names, 
	//no need for double search
	const wchar_t* wordstr = word.to_string();
	size_t endIndex = wcslen(wordstr);
	size_t strlen = wcslen(wordstr);

	for(size_t i =0; i< (size_t)_nEndings; i++){
		if(endsWith(wordstr, _nationalityEndings[i], endIndex)){
			size_t len = wcslen(_nationalityEndings[i].to_string());
			endIndex-=(len);
			break;
		}
	}
	wchar_t buffer[500];
	if (endIndex > 499){
		return word;
	}
	if(endIndex == strlen){
		return word;
	}
	if(endIndex == 0){
		return word;
	}

	size_t j = 0;
	for(j = 0; j<endIndex; j++){
		buffer[j] = wordstr[j];
	}
	buffer[j] =L'\0';
	return Symbol(buffer);

}
int ArabicWordConstants::getPossibleStems(Symbol word, Symbol* variants, int max){
	int nvar =0;
	Symbol tempword = removeAl(word);	
	if(tempword != word){
		variants[nvar++] = tempword;
	}
	if(nvar > max)
		return nvar;
	Symbol base = getNationalityStemVariant(tempword);
	if(base != tempword){
		variants[nvar++] = tempword;
	}
	if(nvar > max)
		return nvar;

	Symbol buffer[5];
	int nalt = getFirstLetterAlefVariants(word, buffer, 5);
	if((nalt + nvar) > max)
		return nvar;
	for(int i=0; i<nalt; i++){
		variants[nvar++] = buffer[i];
	}
	nalt = getLastLetterAlefVariants(word, buffer, 5);
	if((nalt + nvar) > max)
		return nvar;
	for(int j=0; j<nalt; j++){
		variants[nvar++] = buffer[j];
	}
	return nvar;

}
bool ArabicWordConstants::endsWith(const wchar_t* word, Symbol end, size_t index){
	const wchar_t* suf =  end.to_string();
	size_t sufLen = wcslen(suf);
	size_t wordLen = wcslen(word);
	if(index < sufLen)
		return false;
	else{
		for(size_t i = 0; i<sufLen; i++){
			if(suf[i] !=  word[index -sufLen+i]){
				return false;
			}
		}
		Symbol temp = Symbol(word);
		return true;
	}
}
