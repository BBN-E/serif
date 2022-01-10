// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/parse/es_STags.h"
#include "Spanish/propositions/es_SemTreeUtils.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"

#include "Generic/common/DebugStream.h"
#include "Generic/common/WordConstants.h"


// known-to-be transitive past participles
const Symbol::SymbolGroup SpanishSemTreeUtils::transitivePastParticiples = Symbol::makeSymbolGroup(
	 L"abatiendo abofeteado  acarreado acollarado acometendo acuchillado acusado agarrado agrediendo aplastado aprehendendo "
	    L"apretado apu\xf1alado arrancado arrebatado arrestado asaltado asesinado asiendo atacado  atrapado"
		L"birlado capturado cargado causado chocado cimentado clavado coger comunicado cortado culpado "
		L"da\xf1" L"ado dado desactivado desalojado descubriendo desfigurado desplazado destrozado detenendo disparado donado "
		L"ejecutado eliminado embargado emponzo\xf1" L"ado encontrado escaldado estellado evenenado flechado fundado fusilado "
		L"golpeado hallado heriendo incapacitado incriminado lesionado llevado "
		L"machacado magullado maltratado martirizado matado mordendo mutilado ocasionado "
		L"pellizcado pescado picado pillado pisoteado prendendo "
		L"raptado rebanado regalado robado rozado  secuestrado sonado "
		L"tajado torturado traendo transferiendo transmitiendo transportado tulliendo vencendo violado ");


const Symbol::SymbolGroup SpanishSemTreeUtils::transitiveInfinitives = Symbol::makeSymbolGroup(
	 L"abatir abofetear  acarrear acollarar acometer acuchillar acusar agarrar agredir aplastar aprehender  apretar apu\xf1alar "
		L"arrancar arrebatar arrestar asaltar asesinar asir atacar  atrapar"
		L"birlar capturar cargar causar chocar cimentar clavar coger comunicar cortar culpar "
		L"da\xf1ar dar desactivar desalojar descubrir desfigurar desplazar destrozar  detener disparar donar "
		L"ejecutar eliminar embargar emponzo\xf1ar encontrar escaldar estellar evenenar flechar fundar fusilar "
		L"golpear hallar herir incapacitar incriminar lesionar llevar "
		L"machacar magullar maltratar martirizar matar morder mutilar ocasionar "
		L"pellizcar pescar picar pillar pisotear prender "
		L"raptar rebanar regalar robar rozar secuestrar sonar "
		L"tajar torturar traer transferir transmitir transportar tullir vencer violar ");

const Symbol::SymbolGroup SpanishSemTreeUtils::temporalWords = Symbol::makeSymbolGroup(
	// add month abrevs??  find some first
	L"abril agosto a\xf1o a\xf1os atardecer atardeceres d\xe9" L"cada diciembre d\xeda d\xedas enero est\xedo febrero "
	 L"hora horas invierno julio junio "
	 L"madrugada ma\xf1ana ma\xf1anas marzo mayo mediod\xeda mediod\xedas medianoche medianoches mes meses minuto minutos"
     L"noche noches noviembre octubre oto\xf1o primavera "
	 L"semana semanas septiembre tarde tardes velada veladas verano "
	 L"1980 1981 1982 1983 1984 1985 1986 1987 1988 1989 "
	 L"1990 1991 1992 1993 1994 1995 1996 1997 1998 1999 "
	 L"2000 2001 2002 2003 2004 2005 2006 2007 2008 2009 "
	 L"2010 2011 2012 2013 2014 2015 ");

const Symbol::SymbolGroup SpanishSemTreeUtils::temporalPeriodPortionWords = Symbol::makeSymbolGroup(
	L"comienzo fin final medio principio t\xe9rmino"); 

const Symbol::SymbolGroup SpanishSemTreeUtils::negativeAdverbs = Symbol::makeSymbolGroup(
	L"nada ni ning\xfan no");

const Symbol SpanishSemTreeUtils::sym_de = Symbol(L"de");
const Symbol SpanishSemTreeUtils::sym_haber = Symbol(L"haber");
const Symbol SpanishSemTreeUtils::sym_por = Symbol(L"por");

const Symbol::SymbolGroup SpanishSemTreeUtils::sym_haberForms = Symbol::makeSymbolGroup(
	L"ha haber hab\xe9is han has he hemos");

const Symbol::SymbolGroup SpanishSemTreeUtils::sym_locativeNominals = Symbol::makeSymbolGroup(
	L"ac\xe1 aqu\xed ah\xed all\xe1 all\xed ");
// English equiv  == (here || there ||abroad ||overseas || home)  but last three asre phrases in Spanish

bool SpanishSemTreeUtils::isNegativeAdverb(Symbol word){
	return word.isInSymbolGroup(negativeAdverbs);
}

bool SpanishSemTreeUtils::isKnownTransitivePastParticiple(Symbol word){
	return word.isInSymbolGroup(transitivePastParticiples);
}

bool SpanishSemTreeUtils::isTemporalPeriodPortion(Symbol word){
	return word.isInSymbolGroup(temporalPeriodPortionWords);
}

bool SpanishSemTreeUtils::isKnownTemporalNominal(Symbol word){
	return (WordConstants::isDailyTemporalExpression(word) ||
			word.isInSymbolGroup(temporalWords));
}
bool SpanishSemTreeUtils::looksLikeTemporal(const SynNode *node, const MentionSet* mentionSet, bool no_embedded) {
		
	if (no_embedded && node->getParent() != 0 &&
		node->getParent()->hasMention() &&
		looksLikeTemporal(node->getParent(), mentionSet, no_embedded)) {
		return false;
	}

	if (node->getTag() == SpanishSTags::POS_W ) { //EnglishSTags::DATE
		return true;
	}

	Symbol word = node->getHeadWord();

	if (isTemporalPeriodPortion(word)) {
		const SynNode *PP = 0;
		for (int i = 0; i < node->getNChildren(); i++) {
			if (node->getChild(i)->getTag() == SpanishSTags::SP){ //EnglishSTags::PP
				PP = node->getChild(i);
				break;
			}
		}
		if (PP != 0 &&
			PP->getHeadWord() == sym_de &&
			PP->getNChildren() >= 2 &&
			looksLikeTemporal(PP->getChild(1), mentionSet, no_embedded)) {
			return true;
		}
	}

	return isKnownTemporalNominal(word);
}
Symbol SpanishSemTreeUtils::composeChildSymbols(const SynNode *node, Symbol tag){
	Symbol kid_syms = Symbol();
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == tag){ 
			Symbol kid_sym = child->getHeadWord();
			if (kid_syms.is_null()){
				kid_syms = kid_sym;
			}else{
				kid_syms = kid_syms + kid_sym;
			}
		}
	}
	return kid_syms;
}

bool SpanishSemTreeUtils::isParticle(const SynNode &node) {
	if (node.getNChildren() == 1 &&
		// (node.getTag() == EnglishSTags::PRT // particle
		node.getTag() == SpanishSTags::POS_S)   // EnglishSTags::RP (preposition)
	{
		return true;
	}
	return false;
}

bool SpanishSemTreeUtils::isAdverb(const SynNode &node) {
	Symbol someTag = node.getTag();
	if (someTag == SpanishSTags::SADV || // EnglishSTags::ADVP) {
        someTag == SpanishSTags::GRUP_ADV || // complex adverb) {
        someTag.isInSymbolGroup(SpanishSTags::POS_R_GROUP) ){ // EnglishSTags::RB)
	   return true;
	}
	return false;
}


bool SpanishSemTreeUtils::isTemporalNP(const SynNode &node, const MentionSet *mentionSet) {
	// in 2009 version, temporals within temporals are okay
	return looksLikeTemporal(&node, mentionSet, !USE_2009_TEMPORALS);
}

bool SpanishSemTreeUtils::isHaber(Symbol word, bool includeInfinitive){
	if (!includeInfinitive && word == sym_haber){
		return false;
	}
	return word.isInSymbolGroup(sym_haberForms);
}
int SpanishSemTreeUtils::verbMainScore(Symbol s){
	int score = -1;
	/*if (isInfinitiveOrGerund(s)){
		score = 4;
	}else */
	if (isMainVerbPOS(s)){
		score = 3;
	}else if (isSemiAuxVerbPOS(s)){
		score = 2;
	}else if (isAuxVerbPOS(s)){
		score = 1;
	}
	return score;
}

bool SpanishSemTreeUtils::isInfinitiveOrGerund(Symbol s){
	return (s == SpanishSTags::INFINITIU ||
			s == SpanishSTags::GERUNDI);    
}
		
bool SpanishSemTreeUtils::isMainVerbPOS(Symbol s){
	return (s == SpanishSTags::POS_VMG ||    // verb (main gerund)
			s == SpanishSTags::POS_VMI ||	// verb (main infinitive)
			s == SpanishSTags::POS_VMM ||    // verb (main imperative)
			s == SpanishSTags::POS_VMN ||    // verb (main indicative)
			s == SpanishSTags::POS_VMP ||    // verb (main past participle)
			s == SpanishSTags::POS_VMS);     // verb (main subjunctive)
}
bool SpanishSemTreeUtils::isSemiAuxVerbPOS(Symbol s){
	return (s == SpanishSTags::POS_VSG ||                        // verb (semiauxilliary gerund)
			s == SpanishSTags::POS_VSI ||                        // verb (semiauxilliary infinitive)
			s == SpanishSTags::POS_VSM ||                        // verb (semiauxilliary imperative)
			s == SpanishSTags::POS_VSN ||                        // verb (semiauxilliary indicative)
			s == SpanishSTags::POS_VSP ||                        // verb (semiauxilliary past participle)
			s == SpanishSTags::POS_VSS);                         // verb (semiauxilliary subjunctive)
}
bool SpanishSemTreeUtils::isAuxVerbPOS(Symbol s){
	return (s == SpanishSTags::POS_VAG ||               // verb (auxilliary gerund)
			s == SpanishSTags::POS_VAI ||               // verb (auxilliary infinitive)
			s == SpanishSTags::POS_VAM ||               // verb (auxilliary imperative)
			s == SpanishSTags::POS_VAN ||               // verb (auxilliary indicative)
			s == SpanishSTags::POS_VAP ||               // verb (auxilliary past participle)
			s == SpanishSTags::POS_VAS);                // verb (auxilliary subjunctive)
}
Symbol SpanishSemTreeUtils::primaryVerbSym(const SynNode *synode){
	const SynNode *head = synode->getHead();
	Symbol headSym = head->getHeadWord();
	// set vsym in case there is a funky parse with NO head verb (got (INFINITIU of (pr^ que) (pr ver)) once
	Symbol vsym = headSym;
	int main_ness = -1;
	for (int i = 0; i < synode->getNChildren(); i++) {
		const SynNode *child = synode->getChild(i);
		int vms = verbMainScore(child->getTag());
		if ( vms > main_ness){
			main_ness = vms;
			vsym = child->getHeadWord();
		}
	}
	return vsym;
}

bool SpanishSemTreeUtils::isGerundPOS(Symbol s){
	return (s == SpanishSTags::POS_VAG ||   // verb (auxilliary gerund)
			s == SpanishSTags::POS_VMG ||  // verb (main gerund)
			s == SpanishSTags::POS_VSG);   // verb (semiauxilliary gerund)
}
bool SpanishSemTreeUtils::isPastParticiplePOS(Symbol s){
	return (s == SpanishSTags::POS_VAP ||   // verb (auxilliary past participle)
			s == SpanishSTags::POS_VMP ||      // verb (main past participle)
			s == SpanishSTags::POS_VSP);  // verb (semiauxilliary past participle)
}

bool SpanishSemTreeUtils::isLocativeNominal(Symbol word){
	return word.isInSymbolGroup(sym_locativeNominals);
}
bool SpanishSemTreeUtils::isConjunctionPOS(Symbol word) {
	return (word.isInSymbolGroup(SpanishSTags::POS_C_GROUP)||
		word == SpanishSTags::CONJ ||
		word == SpanishSTags::CONJ_SUBORD);
}
bool SpanishSemTreeUtils::isVerbPOS(Symbol word) {
	return (word.isInSymbolGroup(SpanishSTags::POS_V_GROUP));
}
bool SpanishSemTreeUtils::isInfinitivePOS(Symbol s) {
	return (s == SpanishSTags::POS_VAI ||   // verb (auxilliary infinitive)
			s == SpanishSTags::POS_VMI ||      // verb (main infintive)
			s == SpanishSTags::POS_VSI);  // verb (semiauxilliary infinitive)
}
bool SpanishSemTreeUtils::isVerbPhrase(Symbol s){
	return (s == SpanishSTags::S_F_C ||   // completive clause
			s == SpanishSTags::S_F_R ||   // relative clause
			s == SpanishSTags::S_F_C_DP_SBJ ||   // completive clause
			s == SpanishSTags::S_F_R_DP_SBJ ||   // relative clause
			s == SpanishSTags::S_NF_P);   // participle clause
}
bool SpanishSemTreeUtils::isPunctuationPOS(Symbol s){
	return s.isInSymbolGroup(SpanishSTags::POS_F_GROUP);
}
