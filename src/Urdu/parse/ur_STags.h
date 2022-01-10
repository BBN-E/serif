// Copyright 2013 Raytheon BBN Technologies 
// All Rights Reserved.

#ifndef UR_URDU_STAGS_H
#define UR_URDU_STAGS_H

#include "Generic/parse/STags.h"
#include "Generic/common/Symbol.h"

class UrduSTags {
	// Note: this class is intentionally not a subclass of
	// STags.  See STags.h for explanation.
public:
	//============================================================
	// POS tags
	// HTB-v2.5, including all compound types found in hindi train & devel data
	// as defined in HPST-ver0.51-2012/docs/Appendix-10.3-Chunk-POS-Annotaion-Guidelines.doc
	//============================================================
	static Symbol POS_CC;	// conjuncts, coordinating and subordinating 
	static Symbol POS_CCC;	// conjunct compoud (non-final component)
	static Symbol POS_CL;	// classifiers 
	static Symbol POS_DEM;	// demonstratives
	static Symbol POS_ECH;	// echo words (nonsense words that have some context-dependent sense)
	static Symbol POS_INJ;	// interjection, also 'yes'
	static Symbol POS_INTF;	// intensifier
	static Symbol POS_JJ;	// adjective, comparative & superlative
	static Symbol POS_JJC;	// adjective when non-final component of a compound
	static Symbol POS_NEG;	// negative
	static Symbol POS_NN;	// common noun, singular & plural
	static Symbol POS_NNC;	// common noun when non-final component of a compound
	static Symbol POS_NNP;	// proper noun, singular & plural
	static Symbol POS_NNPC;	// proper noun when non-final component of a compound
	static Symbol POS_NST;	// noun denoting spatial and temporal relations, closed set
	static Symbol POS_NSTC;	// spatial and temporal noun compound (non-final component)
	static Symbol POS_NULL;	// tag not defined in documentation, possibly a trace
	static Symbol POS_PRP;	// pronoun, personal & possessive
	static Symbol POS_PRPC;	// pronoun compound, non-final component
	static Symbol POS_PSP;	// postposition, only when a separate token
	static Symbol POS_PSPC;	// postposition compound, non-final component, found in auto-tagged hindi train data
	static Symbol POS_QC;	// cardinals
	static Symbol POS_QCC;	// cardinal compound, non-final component
	static Symbol POS_QF;	// quantifiers, only when used to modify a noun
	static Symbol POS_QFC;	// quantifier compound, non-final component
	static Symbol POS_QO;	// ordinals
	static Symbol POS_RB;	// adverb, comparative & superlative; manner only, not time and place
	static Symbol POS_RBC;	// adverb compound, non-final component
	static Symbol POS_RDP;	// reduplication (second component only)
	static Symbol POS_RP;	// particle, including honorifics
	static Symbol POS_RPC;	// particle compound, non-final component (found in auto-tagged hindi training data)
	static Symbol POS_SYM;	// special symbol, including $, %
	static Symbol POS_SYMC;	// special symbol compound, non-final component
	static Symbol POS_UNK;	// unknown - use sparingly, can include foreign words if syntactic function is unclear
	static Symbol POS_UNKC;	// unknown compound, non-final component
	static Symbol POS_UT;	// quotative
	static Symbol POS_VAUX;	// verb auxiliary
	static Symbol POS_VGF;	// should be reserved for chunk level, but found in hindi training data pos level
	static Symbol POS_VM;	// main verb, finite or non-finite, includes gerunds
	static Symbol POS_VMC;	// main verb compound, non-final component
	static Symbol POS_WQ;	// question words, used as pronouns and determiners
	static Symbol POS_XC;	// x-compound, should not be used but found in 'auto'-labeled hindi data

	//============================================================
	// Chunk Tags
	// HTB-v2.5, occurs in corpus as chunkId='X', can be followed by a number
	// defined in HPST-ver0.51-2012/HTB-ver0.51/docs/Appendix-10.3-Chunk-POS-Annotaion-Guidelines.doc
	//============================================================
	static Symbol BLK;			// miscellaneous entities including interjections
	static Symbol CCP;			// conjuncts 
	static Symbol FRAGP;			// chunk fragments
	static Symbol JJP;			// adjectiveal chunk (predicative)
	static Symbol NEGP;			// negative, when separated from its verb or noun
	static Symbol NN;			// error(?) found in gold-tagged hindi chunk train data
	static Symbol NNP;			// error(?) found in gold-tagged hindi chunk train data
	static Symbol NP;			// noun chunk (and needed for STags Generic)
	static Symbol PRP;			// error found in gold-tagged hindi chunk train data (once)
	static Symbol RBP;			// adverb chunk
	static Symbol VGF;			// finite verb chunk
	static Symbol VGINF;			// infinitival verb chunk (may be more/less common depending on language)
	static Symbol VGNF;			// non-finite verb chunk
	static Symbol VGNN;			// gerunds
	// null_X tags are not defined in HTB-guiedelines-ver2.5.doc, seem to refer to traces
	static Symbol NULL__CCP;
	static Symbol NULL__JJP;
	static Symbol NULL__NP;
	static Symbol NULL__RBP;
	static Symbol NULL__VGF;
	static Symbol NULL__VGINF;
	static Symbol NULL__VGNF;
	static Symbol NULL__VGNN;


	//==================================================================
	// Syntactico-semantic (Paninian) Dependency tags
	// HTB-v2.5, occurs in corpus as drel="X:tok"
	// as defined in HPST-ver0.51-2012/HTB-ver0.51/docs/HTB-guidelines-ver2.5.doc
	// those without descriptions are found in training data, not documentation
	//==================================================================
	static Symbol ADV;		// kryAvisheSaNa, adverbs of manner (not time/place)
	static Symbol CCOF;		// coordination and subordination
	static Symbol ENM;		// enumerator
	static Symbol FRAGOF;	// fragment of
	static Symbol JJMOD;		// adjective modifier
	static Symbol JJMOD_INTF;
	static Symbol JJMOD_RELC;	// adjective relative clause
	static Symbol JK1;		// causee
	static Symbol K1;		// kartaa, one who carries out action denoted by verb 
	static Symbol K1S;		// noun complement of kartaa 
	static Symbol K1U;		// comparison with kartaa
	static Symbol K2;		// karma, object/patient, also complement clauses
	static Symbol K2G;		// secondary karma
	static Symbol K2P;		// goal, destination
	static Symbol K2S;		// object complement
	static Symbol K2U;		// comparison with karma
	static Symbol K3;		// karana, instrument
	static Symbol K3U;		// comparison with karana
	static Symbol K4;		// sampradana, recipient
	static Symbol K4A;		// anubhava karta, experiencer
	static Symbol K4U;		// comparison with sampradana
	static Symbol K5;		// apadana, source i.e. point of departure
	static Symbol K5PRK;		// source material (w/ change of state)
	static Symbol K5U;		// comparison with apadana
	static Symbol K7;		// vishayadhikarana, location elsewhere, can be metaphorical
	static Symbol K7A;		// according to
	static Symbol K7P;		// deshadhikarana, location in space, non-metaphorical
	static Symbol K7PU;		// comparison with K7P
	static Symbol K7T;		// kAlAdhikarana, location in time
	static Symbol K7TU;		// comparison with K7T
	static Symbol K7U;		// comparison with K7
	static Symbol LWG;		// logical word grouping
	static Symbol LWG_CONT;
	static Symbol LWG_NEG;
	static Symbol LWG_PSP;
	static Symbol LWG_QF;
	static Symbol LWG_RP;
	static Symbol LWG_UNK;
	static Symbol LWG_VAUX;
	static Symbol LWG_VM;
	static Symbol MK1;		// mediator-causer
	static Symbol MOD;
	static Symbol MOD_CC;
	static Symbol MOD_NULL;
	static Symbol MOD_WQ;
	static Symbol NMOD;		// participles etc modifying nouns
	static Symbol NMOD_ADJ;
	static Symbol NMOD_EMPH;
	static Symbol NMOD_K1INV;	// shared argument noun modifier and k1 inverse
	static Symbol NMOD_K2INV;	// shared argument noun modifier and k2 inverse
	static Symbol NMOD_K3INV;	// shared argument noun modifier and k3 inverse
	static Symbol NMOD_K4INV;	// shared argument noun modifier and k4 inverse
	static Symbol NMOD_K5INV;	// shared argument noun modifier and k5 inverse
	static Symbol NMOD_K7INV;	// shared argument noun modifier and k7 inverse
	static Symbol NMOD_NEG;
	static Symbol NMOD_POFINV;
	static Symbol NMOD_RELC;	// noun relative clause
	static Symbol PK1;		// causer
	static Symbol POF;		// part-of units such as conjunct verbs and other MWEs
	static Symbol POF_CN;
	static Symbol POF_INV;
	static Symbol POF_JJ;
	static Symbol POF_QC;
	static Symbol POF_REDUP;
	static Symbol POF_IDIOM;
	static Symbol PSP_CL;	// relation between a clause and its postposition
	static Symbol R6;		// shashthi, genitive
	static Symbol R6_K1;		// karta of a conjunct verb (complex predicate)
	static Symbol R6_K2;		// karma of a conjunct verb (complex predicate)
	static Symbol R6_K2S;
	static Symbol R6_K2U;
	static Symbol R6V;		// 'kA' relation between a noun and a verb
	static Symbol RAD;		// address (title)
	static Symbol RAS_K1;	// associative with k1
	static Symbol RAS_K1U;	// associative with k1u
	static Symbol RAS_K2;	// associative with k2
	static Symbol RAS_K3;	// associative with k3
	static Symbol RAS_K4;	// associative with k4
	static Symbol RAS_K5;	// associative with k5
	static Symbol RAS_K7;	// associative with k7
	static Symbol RAS_K7P;	// associative with k7p
	static Symbol RAS_NEG;	// negation in associatives
	static Symbol RAS_POF;	
	static Symbol RAS_R6;
	static Symbol RAS_R6_K2;
	static Symbol RAS_RT;
	static Symbol RBMOD;
	static Symbol RBMOD_RELC;	// adverb relative clause
	static Symbol RD;		// prati, relation direction
	static Symbol RH;		// hetu, reason
	static Symbol RS;		// samanadhikaran, noun elaboration
	static Symbol RSP;		// relation for duratives
	static Symbol RSYM;		// tag for a symbol
	static Symbol RT;		// tadarthya, purpose
	static Symbol RTU;
	static Symbol SENT_ADV;	// sentential adverbs
	static Symbol VMOD;		// verb modifier1
	static Symbol VMOD_ADV;
	
	

	// =================================================================
	// Phrase Structure tags, as found in hindi/Data/PSGuidelines/PSguidelines.xml
	// =================================================================
	static Symbol A;
	static Symbol AP_MOD;
	static Symbol AP_PRED;
	static Symbol C;
	static Symbol CC;
	static Symbol CP;
	static Symbol CP_2;
	static Symbol DEG;
	static Symbol DEGP;
	static Symbol DEM;
	static Symbol FOC;
	static Symbol N;
	static Symbol NPA;
	static Symbol NP_2;
	static Symbol NP_P;
	static Symbol NP_PRED;
	static Symbol NP_P_PRED;
	static Symbol NP_V;
	static Symbol NP_V_P;
	static Symbol NST;
	static Symbol NSTP;
	static Symbol NUM;
	static Symbol N_P;
	static Symbol N_PROP;
	static Symbol P;
	static Symbol PP;
	static Symbol QAP;
	static Symbol QP;
	static Symbol SC_A;
	static Symbol SC_DEG;
	static Symbol SC_N;
	static Symbol SC_P;
	static Symbol V_;
	static Symbol V;
	static Symbol VP;
	static Symbol VP_PART;
	static Symbol VP_PRED;
	static Symbol VP_PRED_PART;
	static Symbol V_AUX;
	static Symbol V_PART;

	static Symbol DATE;  // required for Generic/parser/STags
	static Symbol COMMA; // required for Generic/parse/STags


/*
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
*/
	static void initializeTagList(std::vector<Symbol> tags);
};
#endif
