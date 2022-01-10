// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define ar_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/common/Symbol.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Entity.h"
#include "Generic/parse/xx_LanguageSpecificFunctions.h"
#include "Generic/theories/PartOfSpeech.h"
#include <string>

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include <boost/algorithm/string.hpp> 

class MentionSet;

class ArabicLanguageSpecificFunctions : public GenericLanguageSpecificFunctions {
public:

	static bool isHyphen(Symbol sym){return false;}

	///// PUNCTUATION FUNCTIONS
	/**
	 *  if true, possiblePunctuationOrConjunction[index] = true
	 *  in English = , : . plus all conjunctions
	 */
	static bool isBasicPunctuationOrConjunction(Symbol sym){
		if (sym == ArabicSTags::CC ||
			sym == ArabicSTags::COLON ||
			sym == ArabicSTags::COMMA ||
			sym == ArabicSTags::DOT)
				return true;
		else return false;
	};

	/**
	 *  Leave these the same as English for now
	 *  in English = , ; - . --
	 */
	static bool isNoCrossPunctuation(Symbol sym){
		if (sym == ArabicSTags::COMMA ||
			sym == ArabicSTags::HYPHEN ||
			sym == ArabicSTags::SEMICOLON ||
			sym == ArabicSTags::DOT)
			return true;
		else return false;
	};

	/**
	 *  returns true if this symbol is punctuation that can end a sentence.
	 *  if, given the last token in sentence, this returns true,
	 *  lastIsPunct = true in ChartDecoder::decode
	 *
	 *  No ! in arabic so far, so just . ,?
	 */
	static bool isSentenceEndingPunctuation(Symbol sym){
		if (sym == ArabicSTags::DOT || sym == ArabicSTags::QUESTION)
			return true;
		else return false;
	};
	

	///// SYNTACTIC CATEGORY FUNCTIONS
	//Should NX be added?
	static bool isNPtypeLabel(Symbol sym){
		if( sym == ArabicSTags::NP ||
			sym == ArabicSTags::NPA ||
			sym == ArabicSTags::DEF_NP ||
			sym == ArabicSTags::DEF_NPA)
			return true;
		else return false;
	};

	static bool isPPLabel(Symbol sym){
		if (sym == ArabicSTags::PP)
			return true;
		else return false;
	};
	static Symbol getNPlabel(){
		return ArabicSTags::NP;
	};
	static Symbol getProperNounLabel() {
		return ArabicSTags::NNP;
	}
	//In English this is NPP, NPP's are added in the HeadlessParseNode.
	//part of training whereever there is a name label
	//Since Arabic training doesn't have names marked in it (yet),
	//label names NOUN_PROP
	//TODO Ask Liz about this, since it is not what she said happens
	static Symbol getNameLabel(){
		//return ArabicSTags::NOUN_PROP;
		return ArabicSTags::NPP;
	}

	/**
	 *TODO: does parser find Arabic NPA's?
	 *NPA's and NPP's are added in HeadlessParseNode part of training
	 * in English = NPA or getNameLabel()
	 */
	static bool isCoreNPLabel(Symbol sym){
		if (sym == ArabicSTags::NPA ||sym == getNameLabel())
			return true;
		else return false;
	};	
	static Symbol getCoreNPlabel() {
		return ArabicSTags::NPA;
	}





	///// PART OF SPEECH TAG FUNCTIONS
	/**
	 * returns true if sym is a primary choice for the POS tag to stand in for a name 
	 * in English = NNP, NNPS
	 */
	static bool isPrimaryNamePOStag(Symbol sym, EntityType type) {
		if ( (sym == ArabicSTags::NNP) || (sym == ArabicSTags::NNPS) 
			|| (sym == ArabicSTags::DET_NNP) || (sym == ArabicSTags::DET_NNPS))
			return true;
		else return false;
	}
	/**
	 * returns true if sym is a secondary choice for the POS tag to stand in for a name 
	 * in English = JJ
	 * TODO: Should NOUN go here?  NOUN occurs frequently in names
	 */
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type) {
		if (sym == ArabicSTags::JJ)
			return true;
		if (sym == ArabicSTags::DET_JJ)
			return true;
		if (sym == ArabicSTags::NN)
			return true;
		if (sym == ArabicSTags::NNS)
			return true;
		if (sym == ArabicSTags::DET_NN)
			return true;
		if (sym == ArabicSTags::DET_NNS)
			return true;
		if (sym == ArabicSTags::UNKNOWN)	//allow unknown to get annotated parse !
			return true;
		else return false;
	}	
	/**
	 * return default POS tag to stand in for an unseen-before name
	 * in English = NNP
	 */
	static Symbol getDefaultNamePOStag(EntityType type) {
		return ArabicSTags::NNP;
	}
	/**
	 * return true if this tag can typically be the headtag of an NP
	 */
	//TODO: IS NOUN_PROPER, still in newest TB release
	static bool isNPtypePOStag(Symbol sym) {
		if (isNoun(sym)) return true; 
		else return false;
	}	

	static Symbol getDateLabel() {
		return ArabicSTags::NPA;
	}
	
	// SPECIAL
	/**
	 * DON'T lowercase b/c Romanization of Arabic characters treats upper and 
	 * lower case letters as different!
	 * TODO : Readd when parsing Arabic
	 * given a symbol, return the version of the symbol to be
	 * stored in the ParseNode. In English, this function lowercases it.
	 */
	static Symbol getSymbolForParseNodeLeaf(Symbol sym){
		std::wstring str = sym.to_string();
		// boost::to_lower(str); NOT FOR ARABIC.
		boost::replace_all(str, L"(", L"-LRB-");
		boost::replace_all(str, L")", L"-RRB-");
		return Symbol(str.c_str());
	};
	//The english string will return P0S0N0, this is the most generic feature vector
	static Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) {
		return Symbol(L"Default");
	};
	/**
	* In Arabic its convenient to have a test for nouns, since there are 8 Noun POS tags
	* I've tried to create correct versions of this for English and Chinese, but check the tag set
	* 
	*/
	static bool isNoun(Symbol tag){
		return ArabicSTags::isNoun(tag);

	}
	/**
	* In Arabic definite nouns get a different POS tag, this is used in the Stats Collector
	* 
	*/
	static bool isDefiniteNounTag(Symbol tag){
		return ( 
			(tag == ArabicSTags::DET_NN) ||
			(tag == ArabicSTags::DET_NNS) ||
			(tag == ArabicSTags::DET_NNP) ||
			(tag == ArabicSTags::DET_NNPS)
			);
	}

	static bool isAdjective(Symbol tag){
		return (
			(tag == ArabicSTags::DET_JJ) ||
			(tag == ArabicSTags::JJ));
	}
	static bool isPrepPOSLabel(Symbol sym) {
		return sym == ArabicSTags::IN;
	}
	static Symbol convertPOSTheoryToParserPOS(Symbol dictpos){
		return ArabicSTags::convertDictPOSToParserPOS(dictpos);
	}
	/**
	 * NOT IMPLEMENTED!!!!!!!
	 * Given a ParseNode, perform operations on it that alter the structure
	 * in a specified manner. In English, this ensures a single NP premod under
	 * NPPOS, and changes an NP with list-ish structure into a set of NPs
	 */
	static void modifyParse(ParseNode* node){return;};
	static void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index){return;};

	//check dictionary POS
	static bool containsPOSTheoryNoun(const PartOfSpeech* pos);
	static bool POSTheoryContainsNounPL(const PartOfSpeech* pos);
	static bool POSTheoryContainsNounSG(const PartOfSpeech* pos);
	static bool POSTheoryContainsNounAmbiguousNumber(const PartOfSpeech* pos);

	static bool POSTheoryContainsNounMasc(const PartOfSpeech* pos);
	static bool POSTheoryContainsNounFem(const PartOfSpeech* pos);
	static bool POSTheoryContainsNounAmbiguousGender(const PartOfSpeech* pos);
	
	static bool POSTheoryContainsPronFem(const PartOfSpeech* pos);
	static bool POSTheoryContainsPronMasc(const PartOfSpeech* pos);
	static bool POSTheoryContainsPronSg(const PartOfSpeech* pos);
	static bool POSTheoryContainsPronPl(const PartOfSpeech* pos);
	static bool POSTheoryContainsPron1p(const PartOfSpeech* pos);
	static bool POSTheoryContainsPron2p(const PartOfSpeech* pos);


	static bool posTheoryFindMatching2Substrings(const PartOfSpeech* pos, const wchar_t* str1,
		 const wchar_t* str2);
	static bool posTheoryFindMatchingSubstring(const PartOfSpeech* pos, const wchar_t* str1);
	static void fix2004MentionSet(MentionSet* mention_set, const TokenSequence* tokenSeq);
    static void fix2005Nationalities(MentionSet* mention_set);

	static int findNPHead(ParseNode* arr[], int numNodes);
	static int findNPHead(const SynNode* const arr[], int numNodes);

	static bool matchesHeadToken(Mention *mention, CorrectMention *correctMention) {
		const SynNode *mentionNode = mention->getNode();
		return (mentionNode->getHeadPreterm()->getStartToken() == correctMention->getHeadStartToken());
	}
	
	static bool matchesHeadToken(const SynNode *node, CorrectMention *correctMention) {
		return (node->getHeadPreterm()->getStartToken() == correctMention->getHeadStartToken());
	}

};
#endif
