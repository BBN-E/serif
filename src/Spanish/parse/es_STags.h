// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_SPANISH_STAGS_H
#define ES_SPANISH_STAGS_H

#include "Generic/parse/STags.h"
#include "Generic/common/Symbol.h"

class SpanishSTags {
	// Note: this class is intentionally not a subclass of
	// STags.  See STags.h for explanation.
public:

	//static Symbol UNKNOWN;

	//============================================================
	// Clause Tags
	//============================================================
	// (counts show how often each tag is used in the training corpus)
	static Symbol CONJ;               // [ 15465] conjunction phrase
	static Symbol CONJ_SUBORD;        // [ 10220] subordinating conjunction phrase
	static Symbol GERUNDI;            // [  1404] gerund
	static Symbol GRUP_A;             // [ 30140] adjective group
	static Symbol GRUP_ADV;           // [ 12619] adverbial group
	static Symbol GRUP_NOM;           // [140635] nominal group
	static Symbol GRUP_VERB;          // [ 43042] verbal group
	static Symbol INC;                // [   526] inserted element
	static Symbol INFINITIU;          // [ 11965] infinitive
	static Symbol INTERJECCIO;        // [    86] interjection
	static Symbol MORFEMA_PRONOMINAL; // [  3056] pronominal morpheme
	static Symbol MORFEMA_VERBAL;     // [  2283] verbal morpheme
	static Symbol NEG;                // [  3597] negation
	static Symbol PARTICIPI;          // [  6541] participle
	static Symbol PREP;               // [ 79745] preposition
	static Symbol RELATIU;            // [  9619] relative
	static Symbol SENTENCE;           // [ 17347] top-level sentence
	static Symbol S;                  // [  9479] sentence
	static Symbol S_A;                // [ 26635] adjectival phrase as noun complement
	static Symbol S_F_A;              // [  5304] adverbial comparitive clause
	static Symbol S_F_C;              // [ 19934] completive clause
	static Symbol S_F_R;              // [ 10521] relative clause
	static Symbol S_NF_P;             // [  6883] participle
	static Symbol SA;                 // [  2866] argumental adjectival phrase
	static Symbol SADV;               // [ 12788] adverbial phrase
	static Symbol SN;                 // [138009] nominal phrase
	static Symbol SP;                 // [ 81796] prepositional phrase
	static Symbol SPEC;               // [ 78151] specifier
	
	// Special tags not found in the treebank
	static Symbol SNP; // proper noun phrase.
	static Symbol S_STAR; // verb-less sentence (usually coordinated)
	static Symbol S_F_C_STAR; // verb-less complement phrase

	// Aliases   
	static Symbol NPA;    // identical to SpanishSTags::GRUP_NOM
	static Symbol NP;     // identical to SpanishSTags::SN
	static Symbol PP;     // identical to SpanishSTags::SP
	static Symbol COMMA;  // identical to SpanishSTags::POS_FC
	static Symbol DATE;   // identical to SpanishSTags::POS_W

	// Clauses with dropped pronoun subjects
	static Symbol S_DP_SBJ;
	static Symbol S_F_A_DP_SBJ;
	static Symbol S_F_A_J_DP_SBJ;
	static Symbol S_F_C_P_DP_SBJ;
	static Symbol S_F_C_DP_SBJ;
	static Symbol S_F_R_DP_SBJ;
	static Symbol S_J_DP_SBJ;
	static Symbol SENTENCE_DP_SBJ;

	//============================================================
	// POS tags
	//============================================================
	// (counts show how often each tag is used in the training corpus)
	static Symbol POS_AO;             // [  1959] adjective (ordinal)
	static Symbol POS_AQ;             // [ 33976] adjective (qualificative)
	static Symbol POS_CC;             // [ 15023] conjunction (coordinating)
	static Symbol POS_CS;             // [ 12045] conjunction (subordinating)
	static Symbol POS_DA;             // [ 51830] determiner (article)
	static Symbol POS_DD;             // [  2900] determiner (demonstrative)
	static Symbol POS_DE;             // [    12] determiner (exclamative)
	static Symbol POS_DI;             // [ 13539] determiner (indefinite)
	static Symbol POS_DN;             // [  2407] determiner (numeral)
	static Symbol POS_DP;             // [  5392] determiner (possessive)
	static Symbol POS_DT;             // [    56] determiner (interrogative)
	static Symbol POS_FAA;            // [    54] punctuation (open exclamationmark)
	static Symbol POS_FAT;            // [    72] punctuation (clos eexclamationmark)
	static Symbol POS_FC;             // [ 30148] punctuation (comma)
	static Symbol POS_FD;             // [   761] punctuation (colon)
	static Symbol POS_FE;             // [  9308] punctuation (quotation)
	static Symbol POS_FG;             // [  2867] punctuation (hyphen)
	static Symbol POS_FH;             // [     7] punctuation (slash)
	static Symbol POS_FIA;            // [   262] punctuation (open questionmark)
	static Symbol POS_FIT;            // [   378] punctuation (close questionmark)
	static Symbol POS_FP;             // [ 17517] punctuation (period)
	static Symbol POS_FPA;            // [  1823] punctuation (open bracket)
	static Symbol POS_FPT;            // [  1825] punctuation (close bracket)
	static Symbol POS_FS;             // [   114] punctuation (etc)
	static Symbol POS_FX;             // [   305] punctuation (semicolon)
	static Symbol POS_FZ;             // [   107] punctuation (mathsign)
	static Symbol POS_I;              // [    99] interjection
	static Symbol POS_NC;             // [ 92013] noun (common)
	static Symbol POS_NP;             // [ 29122] noun (proper)
	static Symbol POS_PD;             // [   669] pronoun (demonstrative)
	static Symbol POS_PE;             // [     1] pronoun (exclamative)
	static Symbol POS_PI;             // [  2090] pronoun (indefinite)
	static Symbol POS_PN;             // [   471] pronoun (numeral)
	static Symbol POS_PP;             // [  9314] pronoun (personal)
	static Symbol POS_PR;             // [  9667] pronoun (relative)
	static Symbol POS_PT;             // [   420] pronoun (interrogative)
	static Symbol POS_PX;             // [    60] pronoun (possessive)
	static Symbol POS_RG;             // [ 15338] adverb (general)
	static Symbol POS_RN;             // [  3615] adverb (negative)
	static Symbol POS_S;              // [ 79907] preposition
	static Symbol POS_VAG;            // [     6] verb (auxilliary gerund)
	static Symbol POS_VAI;            // [  4335] verb (auxilliary infinitive)
	static Symbol POS_VAM;            // [     5] verb (auxilliary imparitive)
	static Symbol POS_VAN;            // [   250] verb (auxilliary indicative)
	static Symbol POS_VAP;            // [    30] verb (auxilliary past participle)
	static Symbol POS_VAS;            // [   248] verb (auxilliary subjunctive)
	static Symbol POS_VMG;            // [  1330] verb (main gerund)
	static Symbol POS_VMI;            // [ 30683] verb (main infinitive)
	static Symbol POS_VMM;            // [   301] verb (main imparitive)
	static Symbol POS_VMN;            // [ 11180] verb (main indicative)
	static Symbol POS_VMP;            // [  4779] verb (main past participle)
	static Symbol POS_VMS;            // [  2337] verb (main subjunctive)
	static Symbol POS_VSG;            // [    69] verb (semiauxilliary gerund)
	static Symbol POS_VSI;            // [  4911] verb (semiauxilliary infinitive)
	static Symbol POS_VSM;            // [     5] verb (semiauxilliary imparitive)
	static Symbol POS_VSN;            // [   544] verb (semiauxilliary indicative)
	static Symbol POS_VSP;            // [   402] verb (semiauxilliary past participle)
	static Symbol POS_VSS;            // [   284] verb (semiauxilliary subjunctive)
	static Symbol POS_W;              // [  2731] date
	static Symbol POS_Z;              // [  3942] number
	static Symbol POS_ZC;             // [  1421] number (currency)

	// Symbol groups
	static const Symbol::SymbolGroup POS_A_GROUP; // adjective
	static const Symbol::SymbolGroup POS_C_GROUP; // conjunction
	static const Symbol::SymbolGroup POS_D_GROUP; // determiner
	static const Symbol::SymbolGroup POS_F_GROUP; // punctuation
	static const Symbol::SymbolGroup POS_N_GROUP; // noun
	static const Symbol::SymbolGroup POS_P_GROUP; // pronoun
	static const Symbol::SymbolGroup POS_R_GROUP; // adverb
	static const Symbol::SymbolGroup POS_V_GROUP; // verb
	static const Symbol::SymbolGroup POS_Z_GROUP; // number


	
	static void initializeTagList(std::vector<Symbol> tags);
};
#endif
