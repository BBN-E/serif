// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_LANGUAGESPECIFICFUNCTIONS_H
#define xx_LANGUAGESPECIFICFUNCTIONS_H

class CorrectMention;
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/NPChunkTheory.h"

class GenericLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// LangaugeSpecificFunctions.  See LangaugeSpecificFunctions.h for
	// an explanation.
private:
	static bool defaultRet(){
		std::cerr<<"<<<<<<<<<WARNING: Using unimplemented Language Specific Functions!>>>>>\n";
		return false;
	};
	static Symbol defaultSymRet(){
		std::cerr<<"<<<<<<<<<WARNING: Using unimplemented Language Specific Functions!>>>>>\n";
		return Symbol();
	}
public:
	static bool isHyphen(Symbol sym){return defaultRet();}
	static bool isBasicPunctuationOrConjunction(Symbol sym){return defaultRet();}
	static bool isNoCrossPunctuation(Symbol sym){return defaultRet();}
	static bool isSentenceEndingPunctuation(Symbol sym){return defaultRet();}
	static bool isNPtypeLabel(Symbol sym){return defaultRet();}
	static bool isPPLabel(Symbol sym){return defaultRet();}
	static Symbol getNPlabel(){return defaultSymRet();}
	static Symbol getNameLabel(){return defaultSymRet();}	
	static Symbol getCoreNPlabel(){return defaultSymRet();}	
	static Symbol getDateLabel(){return defaultSymRet();}
	static Symbol getProperNounLabel(){return defaultSymRet();}
	static bool isCoreNPLabel(Symbol sym){return defaultRet();}
	static bool isPrimaryNamePOStag(Symbol sym, EntityType type){return defaultRet();}
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type){return defaultRet();}
	static Symbol getDefaultNamePOStag(EntityType type){return defaultSymRet();}
	static bool isNPtypePOStag(Symbol sym){return defaultRet();}
	static Symbol getSymbolForParseNodeLeaf(Symbol sym){return defaultSymRet();}
	static bool matchesHeadToken(Mention *, CorrectMention *) {return defaultRet();}
	static bool matchesHeadToken(const SynNode *, CorrectMention *) {return defaultRet();}
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
		std::cerr<<"<<<<<<<<<WARNING: Using unimplemented Language Specific Functions!>>>>>\n";
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
