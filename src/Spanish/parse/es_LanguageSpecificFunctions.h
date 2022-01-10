// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_LANGUAGESPECIFICFUNCTIONS_H
#define es_LANGUAGESPECIFICFUNCTIONS_H

class CorrectMention;
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/NPChunkTheory.h"

class SpanishLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// LangaugeSpecificFunctions.  See LangaugeSpecificFunctions.h for
	// an explanation.
 public:
	static bool isHyphen(Symbol sym);
	static bool isBasicPunctuationOrConjunction(Symbol sym);
	static bool isNoCrossPunctuation(Symbol sym);
	static bool isSentenceEndingPunctuation(Symbol sym);
	static bool isNPtypeLabel(Symbol sym);
	static bool isPPLabel(Symbol sym);
	static Symbol getNPlabel();
	static Symbol getNameLabel();
	static Symbol getCoreNPlabel();
	static Symbol getDateLabel();
	static Symbol getProperNounLabel();
	static bool isCoreNPLabel(Symbol sym);
	static bool isPrimaryNamePOStag(Symbol sym, EntityType type);
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type);
	static Symbol getDefaultNamePOStag(EntityType type);
	static bool isNPtypePOStag(Symbol sym);
	static Symbol getSymbolForParseNodeLeaf(Symbol sym);
	static bool matchesHeadToken(Mention *, CorrectMention *);
	static bool matchesHeadToken(const SynNode *, CorrectMention *);
	static bool isAdverbPOSLabel(Symbol sym) { return false; }
	static bool isVerbPOSLabel(Symbol sym) { return false; }
	static bool isPrepPOSLabel(Symbol sym) { return false; }
	static bool isPreplikePOSLabel(Symbol sym) { return false; }
	static bool isPronounPOSLabel(Symbol sym) { return false; }
	static bool isParticlePOSLabel(Symbol sym) { return false; }
	static bool isNoun(Symbol pos_tag) { return false; }
	static bool isNounPOS(Symbol pos_tag) { return false; }
	static bool isVerbPOS(Symbol pos_tag) { return false; }
	static bool isAdjective(Symbol pos_tag) { return false; }
	static Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) {
		return Symbol();
	}
	static bool isPastTenseVerbPOStag(Symbol sym) { return false; }
	static bool isPresentTenseVerbPOStag(Symbol sym) { return false; }
	static bool isFutureTenseVerbPOStag(Symbol sym) { return false; }
	static bool isPastTenseVerbChain(wstring tags, wstring words) { return false; }
	static bool isFutureTenseVerbChain(wstring tags, wstring words) { return false; }
	static bool isPresentTenseVerbChain(wstring tags, wstring words) { return false; }
	static int findNameListEnd(const TokenSequence *tokenSequence,
							   const NameTheory *nameTheory, int start) { return -1; }
	static Symbol getParseTagForWord(Symbol word) { return Symbol(); }
	static bool isTrulyUnknownWord(Symbol word) { return false; }
	static bool isKnownNoun(Symbol word) { return false; }
	static bool isKnownVerb(Symbol word) { return false; }
	static bool isPotentialGerund(Symbol word) { return false; }
	static void setAsStandAloneParser(){}
	static bool isStandAloneParser(){return false;}
	static Symbol convertPOSTheoryToParserPOS(Symbol tag){ return tag;}
	static bool POSTheoryContainsNounPL(const PartOfSpeech* tags){ return false;}
	static bool POSTheoryContainsNounSG(const PartOfSpeech* tags){ return false;}
	static bool POSTheoryContainsNounAmbiguousNumber(const PartOfSpeech* tags){ return false;}
	static bool POSTheoryContainsNounMasc(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsNounFem(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsNounAmbiguousGender(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsPronFem(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsPronMasc(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsPronSg(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsPronPl(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsPron1p(const PartOfSpeech* pos) { return false; }
	static bool POSTheoryContainsPron2p(const PartOfSpeech* pos) { return false; }
	static void fix2005Nationalities(MentionSet* mention_set){}
	static int findNPHead(ParseNode* arr[], int numNodes) { return 0; }
	static int findNPHead(const SynNode* const arr[], int numNodes) { return 0; }

	static void modifyParse(ParseNode* node){
		return;
	}
	static void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index) { return; }

	static Constraint* getConstraints(const NPChunkTheory* theory, const TokenSequence* ts, int& num_constraints) {
		Constraint* constraints = _new Constraint[theory->n_npchunks];
		for (int i=0; i < theory->n_npchunks; i++){
            constraints[i].left = theory->npchunks[i][0];
            constraints[i].right = theory->npchunks[i][1];
            constraints[i].type = Symbol();
            constraints[i].entityType = EntityType::getUndetType();    
		}
		num_constraints = theory->n_npchunks;
		return constraints;
	}

};
#endif
