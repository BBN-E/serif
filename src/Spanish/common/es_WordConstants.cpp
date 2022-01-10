// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Spanish/common/es_WordConstants.h"

#include <boost/regex.hpp>

Symbol SpanishWordConstants::Y = Symbol(L"y");

// Local constants
namespace {

	

	// Union of symbol groups.
	Symbol::SymbolGroup operator+(Symbol::SymbolGroup a, Symbol::SymbolGroup b) {
		Symbol::SymbolGroup result(a.begin(), a.end());
		result.insert(b.begin(), b.end());
		return result;
	}

	/////////////////////////////////////////////////////////////////
	// Pronouns by Type
	/////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup SUBJECT_PRONOUNS = Symbol::makeSymbolGroup
		(L"yo nosotros nosotras t\xfa vosotros vosotras usted ustedes \xe9l ellos ella ellas");

	const Symbol::SymbolGroup PREP_OBJ_PRONOUNS = Symbol::makeSymbolGroup
		(L"m\xed nos nosotros nosotras ti vos vosotros vosotras usted ustedes \xe9l ellos ella ellas s\xed");

	const Symbol::SymbolGroup DIRECT_OBJ_PRONOUNS = Symbol::makeSymbolGroup
		(L"me nos te os lo los la las vos");

	const Symbol::SymbolGroup INDIRECT_OBJ_PRONOUNS = Symbol::makeSymbolGroup
		(L"me nos te os vos le les se");

	const Symbol::SymbolGroup ABS_INTERROGATIVE_PRONOUNS = Symbol::makeSymbolGroup
		(L"qu\xe9 cu\xe1l cu\xe1les"); // "what?", "which?"
	const Symbol::SymbolGroup PER_INTERROGATIVE_PRONOUNS = Symbol::makeSymbolGroup
		(L"qui\xe9n qui\xe9nes cuya cuyas cuyo cuyos"); // "who?", "whose?"
	const Symbol::SymbolGroup INTERROGATIVE_PRONOUNS = (PER_INTERROGATIVE_PRONOUNS +
	     ABS_INTERROGATIVE_PRONOUNS);

	const Symbol::SymbolGroup RELATIVE_PRONOUNS = Symbol::makeSymbolGroup
		(L"que quien quienes cual cuales cuya cuyas cuyo cuyos");

	const Symbol::SymbolGroup DEMONSTRATIVE_DETERMINERS = Symbol::makeSymbolGroup
		(L"este estos esta estas ese esos esa esas aquel aquellos aquella aquellas");
	const Symbol::SymbolGroup DEMONSTRATIVE_PRONOUNS = Symbol::makeSymbolGroup
		(L"\xe9ste \xe9stos \xe9sta \xe9stas esto \xe9se \xe9sos \xe9sa \xe9sas eso aqu\xe9l "
		 L"aqu\xe9llos aqu\xe9lla aqu\xe9llas aquello");
	const Symbol::SymbolGroup ALL_DEMONSTRATIVE_PRONOUNS = (DEMONSTRATIVE_DETERMINERS +
		DEMONSTRATIVE_PRONOUNS);

	// unstressed, always with a noun
	const Symbol::SymbolGroup POSSESSIVE_ADJECTIVES = Symbol::makeSymbolGroup
		(L"mi mis tu tus su sus nuestro nuestra nuestros nuestras vuestra vuestras vuestro vuestos");  
	//pronoun can be adjective too when used for emphasis
	const Symbol::SymbolGroup POSSESSIVE_PRONOUNS = Symbol::makeSymbolGroup  
		(L"mis m\xedo m\xeda m\xedos m\xedas nuestro nuestra nuestros nuestras tuya tuyas tuyo tuyos "
		 L"vuestra vuestras vuestro vuestos suya suyas suyo suyos");
	const Symbol::SymbolGroup ALL_POSSESSIVE_ADJECTIVES = (POSSESSIVE_ADJECTIVES +
		POSSESSIVE_PRONOUNS);

	/////////////////////////////////////////////////////////////////
	// Reflexive pronouns
	/////////////////////////////////////////////////////////////////
	
	const Symbol::SymbolGroup REFLEXIVE_PRONOUNS = Symbol::makeSymbolGroup
		(L"me nos te os vos se s\xed");
	const Symbol::SymbolGroup CON_REFLEXIVE_PRONOUNS = Symbol::makeSymbolGroup
		(L"conmigo contigo consigo");
	const Symbol::SymbolGroup ALL_REFLEXIVE_PRONOUNS = (REFLEXIVE_PRONOUNS +
		CON_REFLEXIVE_PRONOUNS);

	const Symbol::SymbolGroup OTHER_PRONOUNS = Symbol::makeSymbolGroup
		(L"alguien alg\xfan alg\xfano nadie algo nada quienquiera cualquier cualquiera");
	const Symbol::SymbolGroup OTHER_NON_PER_PRONOUNS = Symbol::makeSymbolGroup
		(L"algo cualquiera nada");
	//   "alguien"        = someone, anyone  
	//   "alg\xfan"		  = someone, anyone
	//   "alg\xfano"	  = someone, anyone
	//   "nadie"          = no one, nobody, not any body
	//   "algo"           = something, anything
	//   "nada"           = nothing
	//   "quienquiera"    = anyone (at all)
	//   "cualquier"      = anyone, whichever
	//   "cualquiera"     = anything (whatsoever)
	//   "cada uno"       = each (one)  [XX] not included -- multiword
	//   "cada una"       = each (one)  [XX] not included -- multiword
	//   "todo el mundo"  = everyone    [XX] not included -- multiword

	/////////////////////////////////////////////////////////////////
	// Pronouns by Person
	/////////////////////////////////////////////////////////////////

	const Symbol::SymbolGroup FIRST_PERSON_PRONOUNS = Symbol::makeSymbolGroup
		(L"yo nosotros nosotras m\xed me nos mi m\xedo nuestra nuestras nuestro nuestros"
		 L"m\xeda m\xedas m\xedos conmigo");

	const Symbol::SymbolGroup SECOND_PERSON_PRONOUNS = Symbol::makeSymbolGroup
		(L"t\xfa vos vosotros vosotras usted ustedes ti te os lo tu tuya vuestra"
		 L"tuyo tuyos tuyas vuestras vuestro vuestros");

	const Symbol::SymbolGroup THIRD_PERSON_PRONOUNS = Symbol::makeSymbolGroup
		(L"\xe9l ello ellos ella ellas lo los la las le les se s\xed su suya"
		 L"suyo suyas suyos consigo usted ustedes");

	/////////////////////////////////////////////////////////////////
	// Pronouns by Gender
	/////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup MASCULINE_PRONOUNS = Symbol::makeSymbolGroup
		(L"nosotros vosotros \xe9l ello ellos lo los tuyo tuyos suyo suyos nuestro nuestros"
		 L"m\xedo m\xedos vuestro vuestros");

	const Symbol::SymbolGroup FEMININE_PRONOUNS = Symbol::makeSymbolGroup
		(L"nosotras vosotras ella ellas la las touya tuyas suya suyas nuestra nuestras vuestra"
		 L"vuestras m\xeda m\xedas");

	const Symbol::SymbolGroup GENDERLESS_PRONOUNS = Symbol::makeSymbolGroup
		(L"yo t\xfa usted ustedes m\xed ti me nos te os le les mi m\xedo nuestro tu su"
		 L"conmigo contigo consigo");

	const Symbol::SymbolGroup ALWAYS_NEUTER_PRONOUNS = Symbol::makeSymbolGroup
		(L"aquello eso esto");
	const Symbol::SymbolGroup NEUTER_PRONOUNS = Symbol::makeSymbolGroup
		(L"aquello eso esto lo");

	/////////////////////////////////////////////////////////////////
	// Pronouns by Number
	/////////////////////////////////////////////////////////////////
	// Note: there is some overlap here for ambiguous pronouns.
	const Symbol::SymbolGroup SINGULAR_PRONOUNS = Symbol::makeSymbolGroup
		(L"yo t\xfa usted \xe9l ella m\xed ti me te lo la le \xe9ste \xe9sta esto \xe9se \xe9sa eso "
		 L"aqu\xe9l aqu\xe9lla aquello mi mis m\xedo tu tus tuya su suya");
	const Symbol::SymbolGroup PLURAL_PRONOUNS = Symbol::makeSymbolGroup
		(L"nosotros nosotras vosotros vosotras ustedes ellos ellas nos os los las les \xe9stos \xe9stas "
		 L"\xe9sos \xe9sas aqu\xe9llos aqu\xe9llas nuestros vuestras sus suyas suyos");


	/////////////////////////////////////////////////////////////////
	// Pronouns by referent entity type
	/////////////////////////////////////////////////////////////////

	const Symbol::SymbolGroup LOC_TYPE_PRONOUNS = Symbol::makeSymbolGroup
		(L"all\xed all\xe1 aqu\xed donde d\xf3nde") + DEMONSTRATIVE_PRONOUNS;
	const Symbol::SymbolGroup PER_TYPE_PRONOUNS = 
		(FIRST_PERSON_PRONOUNS + SECOND_PERSON_PRONOUNS + 
		 THIRD_PERSON_PRONOUNS + REFLEXIVE_PRONOUNS +
		 PER_INTERROGATIVE_PRONOUNS + RELATIVE_PRONOUNS);
	const Symbol::SymbolGroup NON_PER_PRONOUNS = (LOC_TYPE_PRONOUNS +
		ALWAYS_NEUTER_PRONOUNS + OTHER_NON_PER_PRONOUNS);

	/////////////////////////////////////////////////////////////////
	// All Pronouns
	/////////////////////////////////////////////////////////////////

	const Symbol::SymbolGroup ALL_PRONOUNS = 
		(SUBJECT_PRONOUNS + PREP_OBJ_PRONOUNS +
		 DIRECT_OBJ_PRONOUNS + INDIRECT_OBJ_PRONOUNS + 
		 INTERROGATIVE_PRONOUNS + ALL_DEMONSTRATIVE_PRONOUNS +
		 ALL_REFLEXIVE_PRONOUNS + NEUTER_PRONOUNS +
		 POSSESSIVE_PRONOUNS + OTHER_PRONOUNS);

	/////////////////////////////////////////////////////////////////
	// Articles/Determiners
	/////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup DEFINITE_ARTICLES = Symbol::makeSymbolGroup
		(L"el la los las");

	const Symbol::SymbolGroup INDEFINITE_ARTICLES = Symbol::makeSymbolGroup
		(L"un unos una unas");

	const Symbol::SymbolGroup MASCULINE_ARTICLES = Symbol::makeSymbolGroup
		(L"el los un unos");

	const Symbol::SymbolGroup FEMININE_ARTICLES = Symbol::makeSymbolGroup
		(L"la las una unas");

	const Symbol::SymbolGroup DETERMINERS = (DEFINITE_ARTICLES + 
		INDEFINITE_ARTICLES + DEMONSTRATIVE_DETERMINERS);

	/////////////////////////////////////////////////////////////////
	// Titles used with names
	/////////////////////////////////////////////////////////////////

	const Symbol::SymbolGroup MASCULINE_TITLES = Symbol::makeSymbolGroup
		(L"se\xf1or se\xf1ores se\xf1orito se\xf1oritos");

	const Symbol::SymbolGroup FEMININE_TITLES = Symbol::makeSymbolGroup
		(L"se\xf1ora se\f1oras se\xf1orita se\xf1oritas");

	/////////////////////////////////////////////////////////////////
	// Copula Verbs
	/////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup ESTAR_NON_FINITE = Symbol::makeSymbolGroup
		(L"estar estado estando");
	const Symbol::SymbolGroup ESTAR_PRESENT_INDICATIVE = Symbol::makeSymbolGroup
		(L"estoy est\xe1s est\xe1 estamos est\xe1is est\xe1n");
	const Symbol::SymbolGroup ESTAR_PRETERITE_INDICATIVE = Symbol::makeSymbolGroup
		(L"estuve estuviste estuvo estuvimos estuvisteis estuvieron");
	const Symbol::SymbolGroup ESTAR_IMPERFECT_INDICATIVE = Symbol::makeSymbolGroup
		(L"estaba estabas est\xe1bamos estabais estaban");
	const Symbol::SymbolGroup ESTAR_FUTURE_INDICATIVE = Symbol::makeSymbolGroup
		(L"estar\xe9 estar\xe1s estar\xe1 estaremos estar\xe9is estar\xe1n");
	const Symbol::SymbolGroup ESTAR_PRESENT_SUBJUNCTIVE = Symbol::makeSymbolGroup
		(L"est\xe9 est\xe9s estemos est\xe9is est\xe9n");
	const Symbol::SymbolGroup ESTAR_IMPERFECT_SUBJUNCTIVE = Symbol::makeSymbolGroup
		(L"estuviera estuviese estuvieras estuvieses estuviera estuviese"
		 L"estuvi\xe9ramos estuvi\xe9semos estuvierais estuvieseis estuvieran estuviesen");
	const Symbol::SymbolGroup ESTAR_FUTURE_SUBJUNCTIVE = Symbol::makeSymbolGroup
		(L"estuviere estuvieres estuvi\xe9remos estuviereis estuvieren");
	const Symbol::SymbolGroup ESTAR_CONDITIONAL = Symbol::makeSymbolGroup
		(L"estar\xeda estar\xedas estar\xedamos estar\xedais estar\xedan");
	const Symbol::SymbolGroup ESTAR_IMPERATIVE = Symbol::makeSymbolGroup
		(L"est\xe1 est\xe9 estemos estad est\xe9");
	const Symbol::SymbolGroup ESTAR_INDICATIVE = (ESTAR_PRESENT_INDICATIVE +
		ESTAR_PRETERITE_INDICATIVE + ESTAR_IMPERFECT_INDICATIVE +
		ESTAR_FUTURE_INDICATIVE);
	const Symbol::SymbolGroup ESTAR_SUBJUNCTIVE = (ESTAR_PRESENT_SUBJUNCTIVE +
		ESTAR_IMPERFECT_SUBJUNCTIVE + ESTAR_FUTURE_SUBJUNCTIVE);
	const Symbol::SymbolGroup ESTAR_TENSED = (ESTAR_INDICATIVE + 
		ESTAR_SUBJUNCTIVE + ESTAR_CONDITIONAL + ESTAR_IMPERATIVE);
	const Symbol::SymbolGroup ALL_ESTAR = (ESTAR_NON_FINITE + ESTAR_TENSED);

	const Symbol::SymbolGroup SER_NON_FINITE = Symbol::makeSymbolGroup
		(L"ser sido siendo");
	const Symbol::SymbolGroup SER_PRESENT_INDICATIVE = Symbol::makeSymbolGroup
		(L"soy eres es somos sois son");
	const Symbol::SymbolGroup SER_PRETERITE_INDICATIVE = Symbol::makeSymbolGroup
		(L"fui fuiste fue fuimos fuisteis fueron");
	const Symbol::SymbolGroup SER_IMPERFECT_INDICATIVE = Symbol::makeSymbolGroup
		(L"era eras  \xe9ramos erais eran");
	const Symbol::SymbolGroup SER_FUTURE_INDICATIVE = Symbol::makeSymbolGroup
		(L"ser\xe9 ser\xe1s ser\xe1 seremos ser\xe9is ser\xe1n");
	const Symbol::SymbolGroup SER_PRESENT_SUBJUNCTIVE = Symbol::makeSymbolGroup
		(L"sea seas seamos se\xe1is sean");
	const Symbol::SymbolGroup SER_IMPERFECT_SUBJUNCTIVE = Symbol::makeSymbolGroup
		(L"\fuere fuese fureas fueses fuera fuese fu\xe9ramos fu\xe9semos"
		 L"fuerais fueseis fueran fuesen");
	const Symbol::SymbolGroup SER_FUTURE_SUBJUNCTIVE = Symbol::makeSymbolGroup
		(L"fuere fueres fu\xe9remos fuereis fueren");
	const Symbol::SymbolGroup SER_CONDITIONAL = Symbol::makeSymbolGroup
		(L"ser\xeda ser\xedas ser\xedamos ser\xedais ser\xedan");
	const Symbol::SymbolGroup SER_IMPERATIVE = Symbol::makeSymbolGroup
		(L"s\xe9 seamos sed");
	const Symbol::SymbolGroup SER_INDICATIVE = (SER_PRESENT_INDICATIVE +
		SER_PRETERITE_INDICATIVE + SER_IMPERFECT_INDICATIVE +
		SER_FUTURE_INDICATIVE);
	const Symbol::SymbolGroup SER_SUBJUNCTIVE = (SER_PRESENT_SUBJUNCTIVE +
		SER_IMPERFECT_SUBJUNCTIVE + SER_FUTURE_SUBJUNCTIVE);
	const Symbol::SymbolGroup SER_TENSED = (SER_INDICATIVE + 
		SER_SUBJUNCTIVE + SER_CONDITIONAL + SER_IMPERATIVE);
	const Symbol::SymbolGroup ALL_SER = (SER_NON_FINITE + SER_TENSED);

	const Symbol::SymbolGroup COPULAS = (ALL_ESTAR + ALL_SER);
	const Symbol::SymbolGroup TENSED_COPULAS = (ESTAR_TENSED + 
		SER_TENSED); 

	/////////////////////////////////////////////////////////////////
	//  Prepositions
	/////////////////////////////////////////////////////////////////
	const Symbol DE = Symbol(L"de");
	const Symbol POR = Symbol(L"por");
	const Symbol::SymbolGroup LOCATIVE_PREPOSITIONS = Symbol::makeSymbolGroup
		(L"lado cerca debajo delante detras encima lejos");

	/////////////////////////////////////////////////////////////////
	// Daily temporal words
	////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup DAILY_WORDS = Symbol::makeSymbolGroup
		// (L"anteayer antier ayer hoy ma\xf1ana lunes martes mi\xe9rcoles jueves viernes s\x00e1bado domingo");
	// note that 'sababdo' with accent on first 'a' must be split (leading zeros fail) to avoid compiler treating '\xe1ba' as a LONG unicode 
		(L"anteayer antier ayer hoy ma\xf1ana lunes martes mi\xe9rcoles jueves viernes s\xe1" L"bado domingo");

	/////////////////////////////////////////////////////////////////
	// some Partitive words (may need to be replaced with multi-word phrases?)
	const Symbol::SymbolGroup PARTITIVE_WORDS = Symbol::makeSymbolGroup(
		L"alguno algunos ambos cada mayor\xeda minor\xeda muchas muchos ni ninguno pocos");
	/////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////
	// Numbers
	/////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup LOW_CARDINALS = Symbol::makeSymbolGroup
		(L"uno dos tres quatro cinco seis siete ocho nueve diez once doce"
		 L"trece catorce quince diecis\xe9is diecisiete dieciocho diecinueve");
	const Symbol::SymbolGroup LOW_ORDINALS = Symbol::makeSymbolGroup
		(L"primero segundo tercero cuarto quinto sexto s\xe9ptimo s\xe9timo"
		 L"octavo noveno d\xe9cimo und\xe9cimo duod\xe9cimo decimotercero"
		 L"decimocuarto decimoquinto decimosexto decimos\xe9ptimo decimoctavo"
		 L"decimonoveno");
	// ordinal abbreviations are <digit(s)>.super{er|o|a}
	// super a == \xaa )SPANISH FEMINIE ORDINAL INDICATOR}
	// super o == \xba {SPANISH MASCULINE ORDINAL INDICATOR}


	/////////////////////////////////////////////////////////////////
	// Special punctuation
	/////////////////////////////////////////////////////////////////
	const Symbol::SymbolGroup PAREN_OR_BRACKET = Symbol::makeSymbolGroup
		(L"-lrb- -rrb- -ldb- -rdb- -LDB- -RDB- -lcb- -rcb-");

	/////////////////////////////////////////////////////////////////
	// Other Symbols
	/////////////////////////////////////////////////////////////////
	Symbol DOUBLE_LEFT_BRACKET(L"-ldb-");
	Symbol DOUBLE_RIGHT_BRACKET(L"-rdb-");
	Symbol DOUBLE_LEFT_BRACKET_UC(L"-LDB-");
	Symbol DOUBLE_RIGHT_BRACKET_UC(L"-RDB-");
	Symbol LEFT_BRACKET(L"-lrb-");
	Symbol RIGHT_BRACKET(L"-rrb-");
	Symbol LEFT_BRACKET_UC(L"-LRB-");
	Symbol RIGHT_BRACKET_UC(L"-RRB-");

	const Symbol::SymbolGroup AKA = Symbol::makeSymbolGroup
		(L"a.k.a. aka a.k.a alias tcc t.c.c. t.c.c");

	const Symbol::SymbolGroup ACRONYM_STOPWORDS = 
		Symbol::makeSymbolGroup(L", - . & ")
		+ DEFINITE_ARTICLES
		+ INDEFINITE_ARTICLES;

	/////////////////////////////////////////////////////////////////
	// Regular Expressions
	/////////////////////////////////////////////////////////////////
	const boost::wregex NUMERIC_RE(L"\\d+", boost::wregex::perl);
	const boost::wregex ALPHABETIC_RE(L"\\w+", boost::wregex::perl);

}

bool SpanishWordConstants::isPronoun(Symbol s) {
	return s.isInSymbolGroup(ALL_PRONOUNS);
}

bool SpanishWordConstants::isReflexivePronoun(Symbol s) {
	return s.isInSymbolGroup(ALL_REFLEXIVE_PRONOUNS);
}

bool SpanishWordConstants::is1pPronoun(Symbol s) {
	return s.isInSymbolGroup(FIRST_PERSON_PRONOUNS);
}

bool SpanishWordConstants::isMasculinePronoun(Symbol s) {
	return s.isInSymbolGroup(MASCULINE_PRONOUNS);
}

bool SpanishWordConstants::isFemininePronoun(Symbol s) {
	return s.isInSymbolGroup(FEMININE_PRONOUNS);
}

bool SpanishWordConstants::isNeuterPronoun(Symbol s) {
	return s.isInSymbolGroup(NEUTER_PRONOUNS);
}
bool SpanishWordConstants::isNonPersonPronoun(Symbol s) {
	return s.isInSymbolGroup(NON_PER_PRONOUNS);
}

bool SpanishWordConstants::isSingular1pPronoun(Symbol s) {
	return (s.isInSymbolGroup(FIRST_PERSON_PRONOUNS) &&
			s.isInSymbolGroup(SINGULAR_PRONOUNS));
}

bool SpanishWordConstants::is2pPronoun(Symbol s) {
	return s.isInSymbolGroup(SECOND_PERSON_PRONOUNS);
}

bool SpanishWordConstants::is3pPronoun(Symbol s) {
	return s.isInSymbolGroup(THIRD_PERSON_PRONOUNS);
}

bool SpanishWordConstants::isSingularPronoun(Symbol s) { 
	return s.isInSymbolGroup(SINGULAR_PRONOUNS);
}

bool SpanishWordConstants::isPluralPronoun(Symbol s) {
	return s.isInSymbolGroup(PLURAL_PRONOUNS);
}

bool SpanishWordConstants::isOtherPronoun(Symbol s) {
	return s.isInSymbolGroup(OTHER_PRONOUNS);
}

bool SpanishWordConstants::isWHQPronoun(Symbol s) {
	return s.isInSymbolGroup(INTERROGATIVE_PRONOUNS);
}

bool SpanishWordConstants::isRelativePronoun(Symbol s) {
	return s.isInSymbolGroup(RELATIVE_PRONOUNS);
}

bool SpanishWordConstants::isPossessivePronoun(Symbol s) {
	return s.isInSymbolGroup(POSSESSIVE_PRONOUNS);
}

bool SpanishWordConstants::isLinkingPronoun(Symbol word) { 
	return (is3pPronoun(word) || isWHQPronoun(word)); 
}

bool SpanishWordConstants::isLOCTypePronoun(Symbol s) {
	return s.isInSymbolGroup(LOC_TYPE_PRONOUNS);
}

bool SpanishWordConstants::isPERTypePronoun(Symbol s) {
	return s.isInSymbolGroup(PER_TYPE_PRONOUNS);
}

bool SpanishWordConstants::isFeminineTitle(Symbol s) {
	return s.isInSymbolGroup(FEMININE_TITLES);
}
bool SpanishWordConstants::isMasculineTitle(Symbol s){
	return s.isInSymbolGroup(MASCULINE_TITLES);
}

bool SpanishWordConstants::isCopula(Symbol s) { 
	return s.isInSymbolGroup(COPULAS); 
}
bool SpanishWordConstants::isCopulaForPassive(Symbol s) { 
	return s.isInSymbolGroup(ALL_SER); 
}
bool SpanishWordConstants::isTensedCopulaTypeVerb(Symbol s) { 
	return s.isInSymbolGroup(TENSED_COPULAS); 
}
bool SpanishWordConstants::isLocativePreposition(Symbol s){
	return s.isInSymbolGroup(LOCATIVE_PREPOSITIONS);
}
bool SpanishWordConstants::isForReasonPreposition(Symbol s){
	return (s == POR);
}
bool SpanishWordConstants::isOfActionPreposition(Symbol s){
	return (s == DE);
}
bool SpanishWordConstants::isDailyTemporalExpression(Symbol s){
	return s.isInSymbolGroup(DAILY_WORDS);
}
bool SpanishWordConstants::isPartitiveWord(Symbol s){
	return s.isInSymbolGroup(PARTITIVE_WORDS);
}
bool SpanishWordConstants::isLowNumberWord(Symbol s){
	return s.isInSymbolGroup(LOW_CARDINALS);
}
bool SpanishWordConstants::isNumeric(Symbol s) { 
	return boost::regex_match(s.to_string(), NUMERIC_RE);
}

bool SpanishWordConstants::isAlphabetic(Symbol s) { 
	return boost::regex_match(s.to_string(), ALPHABETIC_RE);
}

bool SpanishWordConstants::isSingleCharacter(Symbol s) {
	return (std::wstring(s.to_string()).length() == 1);
}

bool SpanishWordConstants::startsWithDash(Symbol word) {
	std::wstring str = word.to_string();
	return (str.length() > 0 && str[0] == L'-');
}

bool SpanishWordConstants::isOpenBracket(Symbol word) {
	return word == LEFT_BRACKET || word == LEFT_BRACKET_UC;
}

bool SpanishWordConstants::isClosedBracket(Symbol word) {
	return word == RIGHT_BRACKET || word == RIGHT_BRACKET_UC;
}

bool SpanishWordConstants::isOpenDoubleBracket(Symbol word) {
	return word == DOUBLE_LEFT_BRACKET || word == DOUBLE_LEFT_BRACKET_UC;
}

bool SpanishWordConstants::isClosedDoubleBracket(Symbol word) {
	return word == DOUBLE_RIGHT_BRACKET || word == DOUBLE_RIGHT_BRACKET_UC;
}

bool SpanishWordConstants::isPunctuation(Symbol word) {
	if (word.isInSymbolGroup(PAREN_OR_BRACKET)) return true;

	std::wstring str(word.to_string());
	size_t length = str.length();
	for (size_t i = 0; i < length; ++i) {
		if (!iswpunct(str[i]))
			return false;
	}
	return true;
}

bool SpanishWordConstants::isParenOrBracket(Symbol word) {
	return word.isInSymbolGroup(PAREN_OR_BRACKET);
}

bool SpanishWordConstants::isAKA(Symbol s) {
	return s.isInSymbolGroup(AKA);
}

bool SpanishWordConstants::isDefiniteArticle(Symbol s) {
	return s.isInSymbolGroup(DEFINITE_ARTICLES);
}

bool SpanishWordConstants::isIndefiniteArticle(Symbol s) {
	return s.isInSymbolGroup(INDEFINITE_ARTICLES);
}

bool SpanishWordConstants::isMasculineArticle(Symbol s) {
	return s.isInSymbolGroup(MASCULINE_ARTICLES);
}

bool SpanishWordConstants::isFeminineArticle(Symbol s) {
	return s.isInSymbolGroup(FEMININE_ARTICLES);
}
bool SpanishWordConstants::isDeterminer(Symbol s) {
	return s.isInSymbolGroup(DETERMINERS);
}

bool SpanishWordConstants::isAcronymStopWord(Symbol s) {
	return s.isInSymbolGroup(ACRONYM_STOPWORDS);
}
