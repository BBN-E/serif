// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "English/common/en_WordConstants.h"
#include "English/timex/Strings.h"

#include <string>

Symbol EnglishWordConstants::COULD = Symbol(L"could");
Symbol EnglishWordConstants::SHOULD = Symbol(L"should");
Symbol EnglishWordConstants::MIGHT = Symbol(L"might");
Symbol EnglishWordConstants::MAY = Symbol(L"may");
Symbol EnglishWordConstants::WHERE = Symbol(L"where");
Symbol EnglishWordConstants::WHETHER = Symbol(L"whether");
Symbol EnglishWordConstants::JUST = Symbol(L"just");

Symbol EnglishWordConstants::THE = Symbol(L"the");
Symbol EnglishWordConstants::THIS = Symbol(L"this");
Symbol EnglishWordConstants::THAT = Symbol(L"that");
Symbol EnglishWordConstants::THESE = Symbol(L"these");
Symbol EnglishWordConstants::THOSE = Symbol(L"those");

Symbol EnglishWordConstants::IF = Symbol(L"if");

Symbol EnglishWordConstants::HE = Symbol(L"he");
Symbol EnglishWordConstants::HIM = Symbol(L"him");
Symbol EnglishWordConstants::HIS = Symbol(L"his");
Symbol EnglishWordConstants::SHE = Symbol(L"she");
Symbol EnglishWordConstants::HER = Symbol(L"her");
Symbol EnglishWordConstants::IT = Symbol(L"it"); 
Symbol EnglishWordConstants::ITS = Symbol(L"its");
Symbol EnglishWordConstants::THEY = Symbol(L"they");
Symbol EnglishWordConstants::THEM = Symbol(L"them");
Symbol EnglishWordConstants::THEIR = Symbol(L"their");
Symbol EnglishWordConstants::I = Symbol(L"i");
Symbol EnglishWordConstants::WE = Symbol(L"we");
Symbol EnglishWordConstants::ME = Symbol(L"me");
Symbol EnglishWordConstants::US = Symbol(L"us");
Symbol EnglishWordConstants::YOU = Symbol(L"you");
Symbol EnglishWordConstants::MY = Symbol(L"my");
Symbol EnglishWordConstants::OUR = Symbol(L"our");
Symbol EnglishWordConstants::YOUR = Symbol(L"your");

Symbol EnglishWordConstants::MYSELF = Symbol(L"myself");
Symbol EnglishWordConstants::YOURSELF = Symbol(L"yourself");
Symbol EnglishWordConstants::HIMSELF = Symbol(L"himself");
Symbol EnglishWordConstants::HERSELF = Symbol(L"herself");
Symbol EnglishWordConstants::OURSELVES = Symbol(L"ourselves");
Symbol EnglishWordConstants::YOURSELVES = Symbol(L"yourselves");
Symbol EnglishWordConstants::THEMSELVES = Symbol(L"themselves");

Symbol EnglishWordConstants::WHO = Symbol(L"who");
Symbol EnglishWordConstants::WHOM = Symbol(L"whom");
Symbol EnglishWordConstants::WHOSE = Symbol(L"whose");
Symbol EnglishWordConstants::WHICH = Symbol(L"which");

Symbol EnglishWordConstants::HERE = Symbol(L"here");
Symbol EnglishWordConstants::THERE = Symbol(L"there");
Symbol EnglishWordConstants::ABROAD = Symbol(L"abroad");
Symbol EnglishWordConstants::OVERSEAS = Symbol(L"overseas");
Symbol EnglishWordConstants::HOME = Symbol(L"home");

Symbol EnglishWordConstants::MR = Symbol(L"mr.");
Symbol EnglishWordConstants::MRS = Symbol(L"mrs.");
Symbol EnglishWordConstants::MS = Symbol(L"ms.");
Symbol EnglishWordConstants::MISS = Symbol(L"miss.");

Symbol EnglishWordConstants::OTHER = Symbol(L"other");
Symbol EnglishWordConstants::ADDITIONAL = Symbol(L"additional");
Symbol EnglishWordConstants::EARLIER = Symbol(L"earlier");
Symbol EnglishWordConstants::PREVIOUS = Symbol(L"previous");
Symbol EnglishWordConstants::FORMER = Symbol(L"former");
Symbol EnglishWordConstants::ANOTHER = Symbol(L"another");
Symbol EnglishWordConstants::MANY = Symbol(L"many");
Symbol EnglishWordConstants::FEW = Symbol(L"few");
Symbol EnglishWordConstants::A = Symbol(L"a");
Symbol EnglishWordConstants::AN = Symbol(L"an");
Symbol EnglishWordConstants::SEVERAL = Symbol(L"several");

Symbol EnglishWordConstants::IS = Symbol(L"is");
Symbol EnglishWordConstants::ARE = Symbol(L"are");
Symbol EnglishWordConstants::WAS = Symbol(L"was");
Symbol EnglishWordConstants::WERE = Symbol(L"were");
Symbol EnglishWordConstants::WILL = Symbol(L"will");
Symbol EnglishWordConstants::WOULD = Symbol(L"would");
Symbol EnglishWordConstants::HAS = Symbol(L"has");
Symbol EnglishWordConstants::HAD = Symbol(L"had");
Symbol EnglishWordConstants::HAVE = Symbol(L"have");
Symbol EnglishWordConstants::_S = Symbol(L"\'s");
Symbol EnglishWordConstants::_D = Symbol(L"\'d");
Symbol EnglishWordConstants::_LL = Symbol(L"\'ll");
Symbol EnglishWordConstants::_VE = Symbol(L"\'ve");
Symbol EnglishWordConstants::_re = Symbol(L"\'re");
Symbol EnglishWordConstants::WO_ = Symbol(L"wo"); // wo(n't)

//static Symbol THE;
//static Symbol A;
Symbol EnglishWordConstants::AND = Symbol(L"and");
Symbol EnglishWordConstants::OF = Symbol(L"of");
Symbol EnglishWordConstants::CORP_ = Symbol(L"corp.");
Symbol EnglishWordConstants::LTD = Symbol(L"ltd");
Symbol EnglishWordConstants::LTD_ = Symbol(L"ltd.");
Symbol EnglishWordConstants::INC = Symbol(L"inc");
Symbol EnglishWordConstants::INC_ = Symbol(L"inc.");
Symbol EnglishWordConstants::CORP = Symbol(L"corp");
Symbol EnglishWordConstants::_COMMA_ = Symbol(L",");
Symbol EnglishWordConstants::_HYPHEN_ = Symbol(L"-");
//static Symbol _S;
Symbol EnglishWordConstants::PLC = Symbol(L"plc");
Symbol EnglishWordConstants::PLC_ = Symbol(L"plc.");

Symbol EnglishWordConstants::OR = Symbol(L"or");

// used by TemporalIdentifier
Symbol EnglishWordConstants::BEGINNING = Symbol(L"beginning");
Symbol EnglishWordConstants::START = Symbol(L"start");
Symbol EnglishWordConstants::END = Symbol(L"end");
Symbol EnglishWordConstants::CLOSE = Symbol(L"close");
Symbol EnglishWordConstants::MIDDLE = Symbol(L"middle");

Symbol EnglishWordConstants::LEFT_BRACKET = Symbol(L"-lrb-");
Symbol EnglishWordConstants::RIGHT_BRACKET = Symbol(L"-rrb-");
Symbol EnglishWordConstants::DOUBLE_LEFT_BRACKET = Symbol(L"-ldb-");
Symbol EnglishWordConstants::DOUBLE_RIGHT_BRACKET = Symbol(L"-rdb-");
Symbol EnglishWordConstants::DOUBLE_LEFT_BRACKET_UC = Symbol(L"-LDB-");
Symbol EnglishWordConstants::DOUBLE_RIGHT_BRACKET_UC = Symbol(L"-RDB-");
Symbol EnglishWordConstants::LEFT_CURLY_BRACKET = Symbol(L"-lcb-");
Symbol EnglishWordConstants::RIGHT_CURLY_BRACKET = Symbol(L"-rcb-");

Symbol EnglishWordConstants::AKA1 = Symbol(L"a.k.a.");
Symbol EnglishWordConstants::AKA2 = Symbol(L"aka");
Symbol EnglishWordConstants::AKA3 = Symbol(L"a.k.a");
Symbol EnglishWordConstants::AKA4 = Symbol(L"currently");

Symbol EnglishWordConstants::FNU = Symbol(L"fnu");
Symbol EnglishWordConstants::LNU = Symbol(L"lnu");

Symbol EnglishWordConstants::CONSIST = Symbol(L"consist");
Symbol EnglishWordConstants::COMPRISE = Symbol(L"comprise");
Symbol EnglishWordConstants::MAKE = Symbol(L"make");
Symbol EnglishWordConstants::CONTAIN = Symbol(L"contain");
Symbol EnglishWordConstants::AS = Symbol(L"as");
Symbol EnglishWordConstants::FOLLOW = Symbol(L"follow");
Symbol EnglishWordConstants::FOLLOWING = Symbol(L"following");
Symbol EnglishWordConstants::ONE = Symbol(L"1");
Symbol EnglishWordConstants::PERIOD = Symbol(L".");
Symbol EnglishWordConstants::UPPER_A = Symbol(L"A");
Symbol EnglishWordConstants::FOR = Symbol(L"for");
Symbol EnglishWordConstants::ORGANIZE = Symbol(L"organize");
Symbol EnglishWordConstants::APPEAR = Symbol(L"appear");

Symbol EnglishWordConstants::ARMY = Symbol(L"army");
Symbol EnglishWordConstants::CORPS = Symbol(L"corps");
Symbol EnglishWordConstants::DIVISION = Symbol(L"division");
Symbol EnglishWordConstants::BRIGADE = Symbol(L"brigade");
Symbol EnglishWordConstants::BATTALION = Symbol(L"battalion");
Symbol EnglishWordConstants::COMPANY = Symbol(L"company");
Symbol EnglishWordConstants::SQUAD = Symbol(L"squad");

Symbol EnglishWordConstants::in = Symbol(L"in");
Symbol EnglishWordConstants::ON = Symbol(L"on");
Symbol EnglishWordConstants::AT = Symbol(L"at");
Symbol EnglishWordConstants::WITH = Symbol(L"with");
Symbol EnglishWordConstants::BY = Symbol(L"by");
Symbol EnglishWordConstants::FROM = Symbol(L"from");
Symbol EnglishWordConstants::ABOUT = Symbol(L"about");
Symbol EnglishWordConstants::INTO = Symbol(L"into");
Symbol EnglishWordConstants::AFTER = Symbol(L"after");
Symbol EnglishWordConstants::OVER = Symbol(L"over");
Symbol EnglishWordConstants::SINCE = Symbol(L"since");
Symbol EnglishWordConstants::UNDER = Symbol(L"under");
Symbol EnglishWordConstants::LIKE = Symbol(L"like");
Symbol EnglishWordConstants::BEFORE = Symbol(L"before");
Symbol EnglishWordConstants::UNTIL = Symbol(L"until");
Symbol EnglishWordConstants::DURING = Symbol(L"during");
Symbol EnglishWordConstants::THROUGH = Symbol(L"through");
Symbol EnglishWordConstants::AGAINST = Symbol(L"against");
Symbol EnglishWordConstants::BETWEEN = Symbol(L"between");
Symbol EnglishWordConstants::WITHOUT = Symbol(L"without");
Symbol EnglishWordConstants::BELOW = Symbol(L"below");

Symbol EnglishWordConstants::AM1 = Symbol(L"am");
Symbol EnglishWordConstants::AM2 = Symbol(L"a.m.");
Symbol EnglishWordConstants::AM3 = Symbol(L"a.m");
Symbol EnglishWordConstants::PM1 = Symbol(L"pm");
Symbol EnglishWordConstants::PM2 = Symbol(L"p.m.");
Symbol EnglishWordConstants::PM3 = Symbol(L"p.m");
Symbol EnglishWordConstants::GMT = Symbol(L"gmt");
Symbol EnglishWordConstants::OLD = Symbol(L"-old");

Symbol EnglishWordConstants::YEAR = Symbol(L"year");
Symbol EnglishWordConstants::MONTH = Symbol(L"month");
Symbol EnglishWordConstants::DAY = Symbol(L"day");
Symbol EnglishWordConstants::HOUR = Symbol(L"hour");
Symbol EnglishWordConstants::MINUTE = Symbol(L"minute");
Symbol EnglishWordConstants::SECOND = Symbol(L"second");

Symbol EnglishWordConstants::TWO = Symbol(L"two");
Symbol EnglishWordConstants::THREE = Symbol(L"three");
Symbol EnglishWordConstants::FOUR = Symbol(L"four");
Symbol EnglishWordConstants::FIVE = Symbol(L"five");
Symbol EnglishWordConstants::SIX = Symbol(L"six");
Symbol EnglishWordConstants::SEVEN = Symbol(L"seven");
Symbol EnglishWordConstants::EIGHT = Symbol(L"eight");
Symbol EnglishWordConstants::NINE = Symbol(L"nine");
Symbol EnglishWordConstants::TEN = Symbol(L"ten");
Symbol EnglishWordConstants::ELEVEN = Symbol(L"eleven");
Symbol EnglishWordConstants::TWELVE = Symbol(L"twelve");


Symbol EnglishWordConstants::DOT = Symbol(L".");

Symbol EnglishWordConstants::JUNIOR = Symbol(L"junior");
Symbol EnglishWordConstants::JR = Symbol(L"jr");
Symbol EnglishWordConstants::SR = Symbol(L"sr");
Symbol EnglishWordConstants::SNR = Symbol(L"snr");
Symbol EnglishWordConstants::SNR_ = Symbol(L"snr.");
Symbol EnglishWordConstants::JR_ = Symbol(L"jr.");
Symbol EnglishWordConstants::SR_ = Symbol(L"sr.");
Symbol EnglishWordConstants::FIRST = Symbol(L"1st");
Symbol EnglishWordConstants::FIRST_ = Symbol(L"1st.");
Symbol EnglishWordConstants::FIRST_I = Symbol(L"i");
Symbol EnglishWordConstants::FIRST_I_ = Symbol(L"i.");
Symbol EnglishWordConstants::SECOND_2ND = Symbol(L"2nd");
Symbol EnglishWordConstants::SECOND_ = Symbol(L"2nd.");
Symbol EnglishWordConstants::SECOND_II = Symbol(L"ii");
Symbol EnglishWordConstants::SECOND_II_ = Symbol(L"ii.");
Symbol EnglishWordConstants::THIRD = Symbol(L"3rd");
Symbol EnglishWordConstants::THIRD_ = Symbol(L"3rd.");
Symbol EnglishWordConstants::THIRD_III = Symbol(L"iii");
Symbol EnglishWordConstants::FORTH = Symbol(L"4th");
Symbol EnglishWordConstants::FORTH_ = Symbol(L"4th.");
Symbol EnglishWordConstants::FORTH_IV = Symbol(L"iv");
Symbol EnglishWordConstants::FIVTH = Symbol(L"v");
Symbol EnglishWordConstants::TENTH = Symbol(L"x");

// For English PER specific words - honorary
Symbol EnglishWordConstants::DR = Symbol(L"dr");
Symbol EnglishWordConstants::DR_ = Symbol(L"dr.");
Symbol EnglishWordConstants::PRESIDENT = Symbol(L"president");
Symbol EnglishWordConstants::POPE = Symbol(L"pope");
Symbol EnglishWordConstants::RABBI = Symbol(L"rabbi");
Symbol EnglishWordConstants::ST = Symbol(L"st");
Symbol EnglishWordConstants::ST_ = Symbol(L"st.");
Symbol EnglishWordConstants::MISTER = Symbol(L"mister");
Symbol EnglishWordConstants::MR_L = Symbol(L"mr");
Symbol EnglishWordConstants::MR_ = Symbol(L"mr.");
Symbol EnglishWordConstants::MS_L = Symbol(L"ms");
Symbol EnglishWordConstants::MS_ = Symbol(L"ms.");
Symbol EnglishWordConstants::MIS_ = Symbol(L"mis.");
Symbol EnglishWordConstants::MIS_L = Symbol(L"mis");
Symbol EnglishWordConstants::MISS_L = Symbol(L"miss");
Symbol EnglishWordConstants::MISS_L_ = Symbol(L"miss.");
Symbol EnglishWordConstants::MRS_L = Symbol(L"mrs");
Symbol EnglishWordConstants::MRS_ = Symbol(L"mrs.");
Symbol EnglishWordConstants::IMAM = Symbol(L"imam");
Symbol EnglishWordConstants::FATHER = Symbol(L"father");
Symbol EnglishWordConstants::REV = Symbol(L"rev");
Symbol EnglishWordConstants::REV_ = Symbol(L"rev.");
Symbol EnglishWordConstants::SISTER = Symbol(L"sister");
Symbol EnglishWordConstants::AYATOLLAH = Symbol(L"ayatollah");
Symbol EnglishWordConstants::SHAH = Symbol(L"shah");
Symbol EnglishWordConstants::EMPEROR = Symbol(L"emperor");
Symbol EnglishWordConstants::PRIME = Symbol(L"prime");
Symbol EnglishWordConstants::PRIME_ = Symbol(L"pr.");
Symbol EnglishWordConstants::PRIME_MINISTER = Symbol(L"prm.");
Symbol EnglishWordConstants::MINISTER = Symbol(L"minister");
Symbol EnglishWordConstants::MINISTER_ = Symbol(L"minst.");
//Symbol EnglishWordConstants::EMBASSADOR = Symbol(L"embassador");
Symbol EnglishWordConstants::PRINCE = Symbol(L"prince");
Symbol EnglishWordConstants::PRINCESS = Symbol(L"princess");
Symbol EnglishWordConstants::QUEEN = Symbol(L"queen");
Symbol EnglishWordConstants::KING = Symbol(L"king");
Symbol EnglishWordConstants::LORD = Symbol(L"lord");
Symbol EnglishWordConstants::MAJESTY = Symbol(L"majesty");
Symbol EnglishWordConstants::SIR = Symbol(L"sir");
Symbol EnglishWordConstants::SENATOR = Symbol(L"senator");
Symbol EnglishWordConstants::SENATOR_ = Symbol(L"sen.");
Symbol EnglishWordConstants::GOVERNOR = Symbol(L"governor");
Symbol EnglishWordConstants::GOVERNOR_ = Symbol(L"gov.");
Symbol EnglishWordConstants::CONGRESSMAN = Symbol(L"congressman");
Symbol EnglishWordConstants::CONGRESSWOMAN = Symbol(L"congresswoman");

Symbol EnglishWordConstants::GENERAL = Symbol(L"general");
Symbol EnglishWordConstants::COLONEL = Symbol(L"colonel");
Symbol EnglishWordConstants::BRIGADIER = Symbol(L"brigadier");
Symbol EnglishWordConstants::FOREIGN = Symbol(L"foreign");
Symbol EnglishWordConstants::REPUBLICAN = Symbol(L"republican");
Symbol EnglishWordConstants::DEMOCRAT = Symbol(L"democrat");
Symbol EnglishWordConstants::CHIEF = Symbol(L"chief");
Symbol EnglishWordConstants::JUSTICE = Symbol(L"justice");
Symbol EnglishWordConstants::COACH = Symbol(L"coach");
Symbol EnglishWordConstants::VICE = Symbol(L"vice");
Symbol EnglishWordConstants::CHANCELLOR = Symbol(L"chancellor");
Symbol EnglishWordConstants::JUDGE = Symbol(L"judge");
//Symbol EnglishWordConstants::FORMER = Symbol(L"former");
Symbol EnglishWordConstants::AMBASSADOR = Symbol(L"ambassador");
Symbol EnglishWordConstants::REPRESENTATIVE = Symbol(L"representative");
Symbol EnglishWordConstants::ATTORNEY = Symbol(L"attorney");
Symbol EnglishWordConstants::DEPUTY = Symbol(L"deputy");
Symbol EnglishWordConstants::SPOKESMAN = Symbol(L"spokesman");
Symbol EnglishWordConstants::SPOKESWOMAN = Symbol(L"spokeswoman");
Symbol EnglishWordConstants::SHEIKH = Symbol(L"sheikh");
Symbol EnglishWordConstants::HEAD = Symbol(L"head");
Symbol EnglishWordConstants::PROFESSOR = Symbol(L"professor");
Symbol EnglishWordConstants::PROFESSOR_ = Symbol(L"prof.");
Symbol EnglishWordConstants::LATE = Symbol(L"late");
Symbol EnglishWordConstants::OFFICIAL = Symbol(L"official");
Symbol EnglishWordConstants::LEADER = Symbol(L"leader");
Symbol EnglishWordConstants::LEUTENANT = Symbol(L"leutenant");
Symbol EnglishWordConstants::LEUTENANT2 = Symbol(L"lt");
Symbol EnglishWordConstants::LEUTENANT_ = Symbol(L"lt.");
Symbol EnglishWordConstants::SARGENT = Symbol(L"sargent");
Symbol EnglishWordConstants::SARGENT_ = Symbol(L"sgt.");
Symbol EnglishWordConstants::CMDR = Symbol(L"cmdr");
Symbol EnglishWordConstants::CMDR_ = Symbol(L"cmdr.");
Symbol EnglishWordConstants::ADMIRAL = Symbol(L"admiral");
Symbol EnglishWordConstants::SECRETARY = Symbol(L"secretary");
Symbol EnglishWordConstants::HERO = Symbol(L"hero");
Symbol EnglishWordConstants::REPORTER = Symbol(L"reporter");
Symbol EnglishWordConstants::ECONOMIST = Symbol(L"economist");
Symbol EnglishWordConstants::M_ = Symbol(L"m.");

Symbol EnglishWordConstants::WELL = Symbol(L"well");



/////////////////////////////////////////////////////////////////
// Daily temporal words
////////////////////////////////////////////////////////////////
Symbol::SymbolGroup DAILY_WORDS = Symbol::makeSymbolGroup
		(L"yesterday today tomorrow monday tuesday wednesday thursday friday saturday sunday");

bool EnglishWordConstants::isLocativePreposition(Symbol word) {
	return (word == in ||
		word == ON ||
		word == AT ||
		word == INTO ||
		word == OVER ||
		word == THROUGH ||
		word == BETWEEN ||
		word == BELOW);
}

bool EnglishWordConstants::isUnknownRelationReportedPreposition(Symbol word) {
	return (word == OF ||           
		word == in ||           
		word == FOR ||          
		word == ON ||           
		word == AT ||           
		word == WITH ||         
		word == BY ||           
		word == AS ||           
		word == FROM ||         
		word == ABOUT ||        
		word == INTO ||         
		word == AFTER ||        
		word == OVER ||         
		word == SINCE ||        
		word == UNDER ||        
		word == LIKE ||         
		word == BEFORE ||       
		word == UNTIL ||        
		word == DURING ||       
		word == THROUGH ||      
		word == AGAINST ||      
		word == BETWEEN ||      
		word == WITHOUT ||      
		word == BELOW);
}


Symbol::SymbolGroup COPULAS = Symbol::makeSymbolGroup
(L"am are be being been is was were _re _s");

bool EnglishWordConstants::isCopula(Symbol s){
	return s.isInSymbolGroup(COPULAS);
}
// note that this was the definition of isCopula used since MKayser svn 1066
bool EnglishWordConstants::isTensedCopulaTypeVerb(Symbol word) {
	return (
		word == IS ||
		word == ARE ||
		word == WAS ||
		word == WERE ||
		word == WILL ||
		word == WOULD ||
		word == HAS ||
		word == HAD ||
		word == HAVE ||
		word == _S ||
		word == _D ||
		word == _LL ||
		word == _VE ||
		word == _re);
}
bool EnglishWordConstants::isNameLinkStopWord(Symbol word) {
	return false;
}

bool EnglishWordConstants::isAcronymStopWord(Symbol word) {
	return (
		word == THE ||
		word == A ||
		word == AND ||
		word == OF ||
		word == CORP ||
		word == CORP_ ||
		word == LTD_ ||
		word == LTD ||
		word == INC ||
		word == INC_ ||
		word == _COMMA_ ||
		word == _HYPHEN_ ||
		word == DOT ||
		word == _S ||
		word == PLC ||
		word == PLC_);
}

bool EnglishWordConstants::isSingularPronoun(Symbol word) {
	return (word == HE ||
		    word == HIM ||
			word == HIS ||
			word == SHE ||
			word == HER ||
			word == IT ||
			word == ITS ||
			word == I ||
			word == ME ||
			word == MY);
}

bool EnglishWordConstants::isPluralPronoun(Symbol word) {
	return (word == THEY ||
			word == THEM ||
			word == THEIR ||
			word == WE ||
			word == US ||
			word == OUR );
}

bool EnglishWordConstants::isPossessivePronoun(Symbol word) {
	return (word == MY ||
			word == HIS ||
			word == HER ||
			word == THEIR ||
			word == YOUR ||
			word == OUR ||
			word == ITS 
			);
}

bool EnglishWordConstants::is1pPronoun(Symbol word) {
	return (word == I ||
			word == WE ||
			word == ME ||
			word == US ||
			word == MY ||
			word == OUR);
}

bool EnglishWordConstants::isSingular1pPronoun(Symbol word) {
	return (word == I ||
			word == ME ||
			word == MY);
}

bool EnglishWordConstants::is2pPronoun(Symbol word) {
	return (word == YOU ||
			word == YOUR);
}

bool EnglishWordConstants::isNonPersonPronoun(Symbol word) {
	return (word == IT ||
			word == ITS);
}
bool EnglishWordConstants::isRelativePronoun(Symbol word) {
	// should include "whose" ?? 
	return (word == WHICH ||
			word == WHO ||
			word == WHOM);
}

bool EnglishWordConstants::is3pPronoun(Symbol word) {
	return (word == HE ||
			word == HIM ||
			word == HIS ||
			word == SHE ||
			word == HER || 
			word == IT ||
			word == ITS ||
			word == THEY ||
			word == THEM || 
			word == THEIR ||
			/* these are for EELD '03 eval */
			word == HERE ||
			word == THERE);
}

bool EnglishWordConstants::isLOCTypePronoun(Symbol word) { 
	return	(word == HERE ||
			word == WHERE ||
			word == THERE);
}

bool EnglishWordConstants::isPERTypePronoun(Symbol word) { 
	return (word == HE ||
			word == HIM ||
			word == HIS ||
			word == SHE ||
			word == HER || 
			word == THEY ||
			word == THEM || 
			word == THEIR ||
			word == I ||
			word == WE ||
			word == ME ||
			word == US ||
			word == MY ||
			word == OUR ||
			word == YOU ||
			word == YOUR ||
			word == WHO ||
			word == WHOSE ||
			word == WHOM ||
			word == MYSELF ||
			word == YOURSELF ||
			word == HIMSELF ||
			word == HERSELF ||
			word == OURSELVES ||
			word == YOURSELVES ||
			word == THEMSELVES);
}


bool EnglishWordConstants::isPunctuation(Symbol word) {
	if (word == LEFT_BRACKET ||
		word == RIGHT_BRACKET ||
		word == DOUBLE_LEFT_BRACKET ||
		word == DOUBLE_RIGHT_BRACKET)
		return true;
	std::wstring str = word.to_string();
	size_t length = str.length();
	for (size_t i = 0; i < length; ++i) {
		if (!iswpunct(str[i]))
			return false;
	}
	return true;
}

bool EnglishWordConstants::isAlphabetic(Symbol word) {
	std::wstring str = word.to_string();
	size_t length = str.length();
	for (size_t i = 0; i < length; ++i) {
		if (!iswalpha(str[i]))
			return false;
	}
	return true;
}

bool EnglishWordConstants::isNumeric(Symbol word) {
	std::wstring str = word.to_string();
	size_t length = str.length();
	for (size_t i = 0; i < length; ++i) {
		if (!iswdigit(str[i]))
			return false;
	}
	return true;
}

bool EnglishWordConstants::isSingleCharacter(Symbol word) {
	std::wstring str = word.to_string();
	return (str.length() == 1);
}

bool EnglishWordConstants::startsWithDash(Symbol word) {
	std::wstring str = word.to_string();
	return (str.length() > 0 && str[0] == L'-');
}

bool EnglishWordConstants::isOrdinal(Symbol word) {
	std::wstring str = word.to_string();
	if (str.length() < 3) return false;

	std::wstring last2 = str.substr(str.length() - 2);
	if (last2.compare(L"ST") &&
		last2.compare(L"st") &&
		last2.compare(L"ND") &&
		last2.compare(L"nd") &&
		last2.compare(L"RD") &&
		last2.compare(L"rd") &&
		last2.compare(L"TH") &&
		last2.compare(L"th"))
		return false;

	for (size_t i = 0; i < str.length() - 2; i++) 
		if (!iswdigit(str[i])) return false;

	return true;
}

Symbol EnglishWordConstants::getNumericPortion(Symbol word)
{
	std::wstring str = word.to_string();
	size_t i = 0;
	for (i = 0; i < str.length(); i++)
		if (!iswdigit(str[i])) break;

	if (i == 0) return Symbol();
	if (i == str.length()) i = str.length();

	return Symbol(str.substr(0, i).c_str());

}
		
bool EnglishWordConstants::isMilitaryWord(Symbol word) {
	return 
	   (word == ARMY ||
		word == CORPS ||
		word == DIVISION ||
		word == BRIGADE ||
	    word == BATTALION ||
		word == COMPANY ||
		word == SQUAD);
}

bool EnglishWordConstants::isWHQPronoun(Symbol word) {
	return 
		(word == WHO ||
		 word == WHOM ||
		 word == WHICH ||
		 word == WHOSE ||
		 word == THAT ||
		 word == WHERE);
}

bool EnglishWordConstants::isTimeExpression(Symbol word) 
{
	std::wstring str = word.to_string();

	int num_digits = 0;
	int num_separators = 0;
	int num_t = 0;
	int num_alpha = 0;

	for (size_t i = 0; i < str.length(); i++) {
		wchar_t c = str.at(i);
		if (iswdigit(c)) num_digits++;
		if (iswalpha(c)) num_alpha++;
		if (c == L'T' || c == L't') num_t++;
		if (c == L':') num_separators++;
	}

	if (!(num_digits > 1 && num_t < 2 && num_t == num_alpha && num_separators > 0)) 
		return false;

	str = stringReplace(str, L"t", L"");
	str = stringReplace(str, L"T", L"");

	size_t index = str.find(L':');
	wstring hours = str.substr(0, index);
	if (Strings::parseInt(hours) == -1 && Strings::parseInt(hours) > 24)
		return false;

	std::wstring minutes = L"";
	size_t index2 = str.find(L':', index + 1);
	if (index2 == std::wstring::npos)
		minutes = str.substr(index + 1);
	else 
		minutes = str.substr(index + 1, index2 - index + 1);	

	return (minutes.length() == 2 && Strings::parseInt(minutes) > -1 && Strings::parseInt(minutes) < 60);
}

bool EnglishWordConstants::isTimeModifier(Symbol word) 
{
	std::wstring lcStr = toLower(word.to_string());
	Symbol lcWord = Symbol(lcStr.c_str());

	return 
		lcWord == AM1 ||
		lcWord == AM2 ||
		lcWord == AM3 ||
		lcWord == PM1 ||
		lcWord == PM2 ||
		lcWord == PM3 ||
		lcWord == GMT;
}

bool EnglishWordConstants::isLowNumberWord(Symbol word)
{
	return 
		word == ONE ||
		word == TWO ||
		word == THREE ||
		word == FOUR ||
		word == FIVE ||
		word == SIX ||
		word == SEVEN ||
		word == EIGHT ||
		word == NINE ||
		word == TEN ||
		word == ELEVEN ||
		word == TWELVE;
}

bool EnglishWordConstants::isFourDigitYear(Symbol word) {
	std::wstring wordStr = word.to_string();
	return 
		wordStr.length() == 4 && 
		1801 <= Strings::parseInt(wordStr) &&  // Note another place where years are defined is en_IdFWordFeatures::getTimexFeature()
		Strings::parseInt(wordStr) <= 2019;  
}

bool EnglishWordConstants::isDashedDuration(Symbol word)
{
	std::wstring wordStr = word.to_string();
	size_t length = wordStr.length();
	if (length == 0) return false;
	size_t index = wordStr.find(L'-');
	if (index == std::wstring::npos) return false;
	size_t index2 = wordStr.find(L'-', index + 1);
	if (index2 != std::wstring::npos) return false;

	std::wstring possNumber = toLower(wordStr.substr(0, index));
	std::wstring possDuration = wordStr.substr(index + 1);

	Symbol beforeDashWord = Symbol(possNumber.c_str());
	Symbol afterDashWord = Symbol(possDuration.c_str());

	return 
		(Strings::parseInt(possNumber) > 0 || 
		 isLowNumberWord(beforeDashWord)) &&
		
		(afterDashWord == YEAR ||
		 afterDashWord == MONTH ||
		 afterDashWord == DAY ||
		 afterDashWord == HOUR ||
		 afterDashWord == MINUTE ||
		 afterDashWord == SECOND);
}

bool EnglishWordConstants::isDateExpression(Symbol word)
{
	int slash_count = 0;
	int dash_count = 0;
	int digit_count = 0;

	std::wstring str = word.to_string();

	for (size_t i = 0; i < str.length(); i++) {
		wchar_t c = str.at(i);
		if (!iswdigit(c) && c != '/' && c != '-') return false;
		if (c == L'/') slash_count++;
		if (c == L'-') dash_count++;
		if (iswdigit(c)) digit_count++;
	}

	//Figure out which separator (/ or -) is in use. Don't allow both.
	if (dash_count > 0 && slash_count > 0) return false;
	
	wchar_t sep;
	int sep_count;
	if (dash_count > 0) {
		sep = L'-';
		sep_count = dash_count;
	} else {
		sep = L'/';
		sep_count = slash_count;
	}
	
	if (sep_count < 1 || sep_count > 2 || digit_count < 2 || digit_count > 8) return false;

	size_t sep_pos = str.find(sep);
	int first_number = Strings::parseInt(str.substr(0, sep_pos));

	size_t second_sep_pos = str.find(sep, sep_pos + 1);
	if (second_sep_pos == std::wstring::npos) second_sep_pos = str.length();
	int second_number = Strings::parseInt(str.substr(sep_pos + 1, second_sep_pos - (sep_pos + 1) ));

	int third_number;
	if (sep_count == 2)
		third_number = Strings::parseInt(str.substr(second_sep_pos + 1, str.length() - (second_sep_pos + 1) ));
	else third_number = -1;

	if (sep == L'/') { //Slashes are less general-purpose than dahses, so more contexts are probably dates.
		if (third_number < 0) { //cases for MM/YYYY, YYYY/MM, or MM/DD
			return (first_number >= 1 && first_number <= 12 && second_number > 1800 && second_number < 2020) ||
				   (second_number >= 1 && second_number <= 12 && first_number > 1800 && first_number < 2020) ||
				   (first_number >= 1 && first_number <= 12 && second_number >= 1 && second_number <= 31);
		} else { //cases for MM/DD/YY(YY) and (YY)YY/MM/DD
			return (first_number >= 1 && first_number <= 12 && second_number >= 1 && second_number <= 31) ||
				   (second_number >= 1 && second_number <= 12 && third_number >= 1 && third_number <= 31);
		}
	} else {
		if (third_number < 0) { //cases for MM-YYYY, YYYY-MM
			return (first_number >= 1 && first_number <= 12 && second_number > 1800 && second_number < 2020) ||
				   (second_number >= 1 && second_number <= 12 && first_number > 1800 && first_number < 2020);
		} else { //cases for MM-DD-YYYY and YYYY-MM-DD
			return (first_number >= 1 && first_number <= 12 && second_number >= 1 && second_number <= 31 && 
						third_number > 1800 && third_number < 2020) ||
				   (second_number >= 1 && second_number <= 12 && third_number >= 1 && third_number <= 31 && 
						first_number > 1800 && first_number < 2020);
		}
	}
}

bool EnglishWordConstants::isDecade(Symbol word) 
{
	// 1930s, 30s e.g.
	std::wstring wordStr = word.to_string();
	size_t length = wordStr.length();
	if (length != 5 && length != 3) return false;

	if (wordStr.at(length - 1) != L's') return false;
	if (wordStr.at(length - 2) != L'0') return false;
	int year = Strings::parseInt(wordStr.substr(0, length - 1));
	return (year >= 1800 && year < 2001) ||
		   (year >= 0 && year < 91);
}

bool EnglishWordConstants::isForReasonPreposition(Symbol s){
	return (s == FOR);
}
bool EnglishWordConstants::isOfActionPreposition(Symbol s){
	return (s == OF);
}
bool EnglishWordConstants::isDailyTemporalExpression(Symbol s){
	return s.isInSymbolGroup(DAILY_WORDS);
}

std::wstring EnglishWordConstants::toLower(std::wstring str) 
{
	std::wstring lcStr = L"";

	for (size_t i = 0; i < str.length(); i++) {
		lcStr += towlower(str.at(i));
	}
	return lcStr;
}

std::wstring EnglishWordConstants::stringReplace(std::wstring str, std::wstring before, std::wstring after) {
	size_t index;
	while ((index = str.find(before)) != std::wstring::npos) {
		str.replace(index, before.length(), after);
	}
	return str;

}

bool EnglishWordConstants::isReflexivePronoun(Symbol word) {
	return word == MYSELF ||
		word == YOURSELF ||
		word == HIMSELF ||
		word == HERSELF ||
		word == OURSELVES ||
		word == YOURSELVES ||
		word == THEMSELVES;
}


bool EnglishWordConstants::isNameSuffix(Symbol sym) {
	return 	sym == JUNIOR
		|| sym == JR 
		|| sym == JR_ 
		|| sym == SR 
		|| sym == SR_
		|| sym == SNR
		|| sym == SNR_
		|| sym == FIRST
		|| sym == FIRST_
		|| sym == FIRST_I
		|| sym == FIRST_I_
		|| sym == SECOND
		|| sym == SECOND_2ND
		|| sym == SECOND_
		|| sym == SECOND_II
		|| sym == SECOND_II_
		|| sym == THIRD
		|| sym == THIRD_
		|| sym == THIRD_III
		|| sym == FORTH
		|| sym == FORTH_
		|| sym == FORTH_IV
		|| sym == FIVTH
		|| sym == TENTH;
}

bool EnglishWordConstants::isHonorificWord(Symbol sym) {
	return 
		sym == DOT // this is because the dot can be split from a tokensuch as 'mr. because of annotation
		|| sym == DR
		|| sym == DR_
		|| sym == PRESIDENT
		|| sym == POPE
		|| sym == RABBI
		|| sym == ST
		|| sym == ST_
		|| sym == MISTER
		|| sym == MR_L
		|| sym == MR_
		|| sym == MS_L
		|| sym == MS_
		|| sym == MIS_L
		|| sym == MIS_
		|| sym == MISS_L
		|| sym == MISS_L_
		|| sym == MRS_L
		|| sym == MRS_
		|| sym == IMAM
		|| sym == FATHER
		|| sym == REV
		|| sym == REV_
		|| sym == SISTER
		|| sym == AYATOLLAH
		|| sym == SHAH
		|| sym == EMPEROR
		|| sym == PRIME
		|| sym == PRIME_
		|| sym == PRIME_MINISTER
		|| sym == MINISTER
		|| sym == MINISTER_
//		|| sym == EMBASSADOR
		|| sym == PRINCE
		|| sym == PRINCESS
		|| sym == KING
		|| sym == LORD
		|| sym == MAJESTY
		|| sym == SIR
		|| sym == SENATOR
		|| sym == SENATOR_
		|| sym == GOVERNOR
		|| sym == GOVERNOR_
		|| sym == CONGRESSMAN
		|| sym == CONGRESSWOMAN
		|| sym == GENERAL
		|| sym == COLONEL
		|| sym == BRIGADIER
		|| sym == FOREIGN
		|| sym == REPUBLICAN
		|| sym == DEMOCRAT
		|| sym == CHIEF
		|| sym == JUSTICE
		|| sym == COACH
		|| sym == VICE
		|| sym == CHANCELLOR
		|| sym == JUDGE
		|| sym == FORMER
		|| sym == AMBASSADOR
		|| sym == REPRESENTATIVE
		|| sym == ATTORNEY
		|| sym == DEPUTY
		|| sym == SPOKESMAN
		|| sym == SPOKESWOMAN
		|| sym == SHEIKH
		|| sym == HEAD
		|| sym == QUEEN
		|| sym == PROFESSOR
		|| sym == PROFESSOR_
		|| sym == LATE
		|| sym == OFFICIAL
		|| sym == LEADER
		|| sym == LEUTENANT
		|| sym == SARGENT
		|| sym == SARGENT_
		|| sym == LEUTENANT2
		|| sym == LEUTENANT_
		|| sym == CMDR
		|| sym == CMDR_
		|| sym == ADMIRAL
		|| sym == SECRETARY
		|| sym == HERO
		|| sym == REPORTER
		|| sym == ECONOMIST
		|| sym == M_;

}

bool EnglishWordConstants::isDeterminer(Symbol word) {
	// [xx] this should also include "AN".  Test if it changes scores?
	return word == THE || word == A;
}

bool EnglishWordConstants::isDefiniteArticle(Symbol word) {
	return word==THE || word==THIS || word==THAT || word==THESE || word==THOSE;
}

bool EnglishWordConstants::isIndefiniteArticle(Symbol word) {
	return word == A || word == AN;
}

bool EnglishWordConstants::isOpenDoubleBracket(Symbol word) {
	return word == DOUBLE_LEFT_BRACKET || word == DOUBLE_LEFT_BRACKET_UC;
}

bool EnglishWordConstants::isClosedDoubleBracket(Symbol word) {
	return word == DOUBLE_RIGHT_BRACKET || word == DOUBLE_RIGHT_BRACKET_UC;
}
