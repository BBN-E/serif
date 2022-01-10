// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LANGUAGE_SPECIFIC_FUNCTIONS_H
#define LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/EntityType.h"
#include "Generic/parse/ChartDecoder.h"
//#include "Generic/parse/ParseNode.h"
//#include "Generic/theories/NPChunkTheory.h"

#include <boost/shared_ptr.hpp>
class SynNode;
class CorefItem;
class TokenSequence;
class NameTheory;
class PartOfSpeech;
class MentionSet;
class Mention;
class Constraint;
class NPChunkTheory;
class ChartDecoder;
class ParseNode;
class CorrectMention;

/** A collection of static helper methods that are used for parsing.
 * These methods delgate to a private language-specific singleton,
 * which in turn delegates to static methods in a language-specific
 * LanguageSpecificFunctions class, such as EnglishLanguageSpecificFunctions or
 * ArabicLanguageSpecificFunctions.
 *
 * These language-specific classes (such as EnglishLanguageSpecificFunctions and
 * ArabicLanguageSpecificFunctions) should *not* be subclasses of
 * LanguageSpecificFunctions; in particular, if a language-specific
 * LanguageSpecificFunctions class was subclassed from
 * LanguageSpecificFunctions, and neglected to implement some static
 * method, then any call to that method would result in an infinite
 * delegation loop.
 *
 * The 'LanguageSpecificFunctions::setImplementation()' method can be
 * used to set the language-specific LanguageSpecificFunctions class
 * that is used to implement the static methods.
 */
class LanguageSpecificFunctions {
public:
	/** Set the language-specific LanguageSpecificFunctions instance
	 * that should be used to implement the static methods in this
	 * class.  E.g.:
	 * LanguageSpecificFunctions::setImplementation<EnglishLanguageSpecificFunctions>(); */
    template<typename LanguageLanguageSpecificFunctions>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<LanguageSpecificFunctionsInstance>
            (_new(LanguageSpecificFunctionsInstanceFor<LanguageLanguageSpecificFunctions>));
	}

	// for parsing hyphenated names right (e.g. "the Verizon - BBN relationship")
	static bool isHyphen(Symbol sym) {
		return getInstance()->isHyphen(sym); }

	///// PUNCTUATION FUNCTIONS
	/**
	 *  if true, possiblePunctuationOrConjunction[index] = true
	 *  in English = , : . plus all conjunctions
	 */
	static bool isBasicPunctuationOrConjunction(Symbol sym) {
		return getInstance()->isBasicPunctuationOrConjunction(sym); }

	/**
	 *  not totally clear on the purpose of this one, but
	 *  has something to do with crossing punct boundaries?
	 *  in English = , ; - . --
	 */
	static bool isNoCrossPunctuation(Symbol sym) {
		return getInstance()->isNoCrossPunctuation(sym); }

	/**
	 *  returns true if this symbol is punctuation that can end a sentence.
	 *  if, given the last token in sentence, this returns true,
	 *  lastIsPunct = true in ChartDecoder::decode
	 *
	 *  in English = ?!.
	 */
	static bool isSentenceEndingPunctuation(Symbol sym) {
		return getInstance()->isSentenceEndingPunctuation(sym); }
	
	///// SYNTACTIC CATEGORY FUNCTIONS
	static bool isNPtypeLabel(Symbol sym) {
		return getInstance()->isNPtypeLabel(sym); }
	static bool isPPLabel(Symbol sym) {
		return getInstance()->isPPLabel(sym); }
	static bool isAdverbPOSLabel(Symbol sym) {
		return getInstance()->isAdverbPOSLabel(sym); }
	static bool isVerbPOSLabel(Symbol sym) {
		return getInstance()->isVerbPOSLabel(sym); }
	static bool isPrepPOSLabel(Symbol sym) {
		return getInstance()->isPrepPOSLabel(sym); }
	static bool isPreplikePOSLabel(Symbol sym) {
		return getInstance()->isPreplikePOSLabel(sym); }
	static bool isPronounPOSLabel(Symbol sym) {
		return getInstance()->isPronounPOSLabel(sym); }
	static bool isParticlePOSLabel(Symbol sym) {
		return getInstance()->isParticlePOSLabel(sym); }
	static Symbol getNPlabel() {
		return getInstance()->getNPlabel(); }
	static Symbol getCoreNPlabel() {
		return getInstance()->getCoreNPlabel(); }
	static Symbol getProperNounLabel() {
		return getInstance()->getProperNounLabel(); }
	static Symbol getNameLabel() {
		return getInstance()->getNameLabel(); }
	static bool isNoun(Symbol pos_tag) {
		return getInstance()->isNoun(pos_tag); }
	static bool isNounPOS(Symbol pos_tag) { // only implemented in English right now
		return getInstance()->isNounPOS(pos_tag); }
	static bool isVerbPOS(Symbol pos_tag) { // only implemented in English right now
		return getInstance()->isVerbPOS(pos_tag); }
	static bool isAdjective(Symbol pos_tag) {
		return getInstance()->isAdjective(pos_tag); }
	
	/**
	 * in English = NPA or getNameLabel()
	 */
	static bool isCoreNPLabel(Symbol sym) {
		return getInstance()->isCoreNPLabel(sym); }


	///// PART OF SPEECH TAG FUNCTIONS
	/**
	 * returns true if sym is a primary choice for the POS tag to stand in for a name 
	 * in English = NNP, NNPS
	 */
	static bool isPrimaryNamePOStag(Symbol sym, EntityType type) {
		return getInstance()->isPrimaryNamePOStag(sym, type); }

	/**
	 * returns true if sym is a secondary choice for the POS tag to stand in for a name 
	 * in English = JJ
	 */
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type) {
		return getInstance()->isSecondaryNamePOStag(sym, type); }
	
	/**
	 * return default POS tag to stand in for an unseen-before name
	 * in English = NNP
	 */
	static Symbol getDefaultNamePOStag(EntityType type) {
		return getInstance()->getDefaultNamePOStag(type); }

	/**
	 * return word that will translate to a default name feature vector
	 * in English = Name
	 */
	static Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) {
		return getInstance()->getDefaultNameWord(decoder_type, type); }

	/**
	 * returns true if sym represents a past tense verb part of speech tag
	 */
	static bool isPastTenseVerbPOStag(Symbol sym) {
		return getInstance()->isPastTenseVerbPOStag(sym); }

    /**
	 * returns true if sym represents a present tense verb part of speech tag
	 */
	static bool isPresentTenseVerbPOStag(Symbol sym) {
		return getInstance()->isPresentTenseVerbPOStag(sym); }

	/**
	 * returns true if sym represents a future tense verb part of speech tag
	 */
	static bool isFutureTenseVerbPOStag(Symbol sym) {
		return getInstance()->isFutureTenseVerbPOStag(sym); }

	static bool isPastTenseVerbChain(wstring tags, wstring words) {
		return getInstance()->isPastTenseVerbChain(tags, words); }
	static bool isFutureTenseVerbChain(wstring tags, wstring words) {
		return getInstance()->isFutureTenseVerbChain(tags, words); }
	static bool isPresentTenseVerbChain(wstring tags, wstring words) {
		return getInstance()->isPresentTenseVerbChain(tags, words); }



	/**
	 * return true if this tag can typically be the headtag of an NP
	 */
	static bool isNPtypePOStag(Symbol sym) {
		return getInstance()->isNPtypePOStag(sym); }
	
	/**
	 * returns DATE constituent tag
	 */
	static Symbol getDateLabel() {
		return getInstance()->getDateLabel(); }

	// SPECIAL
	/**
	 * given a symbol, return the version of the symbol to be
	 * stored in the ParseNode. In English, this function lowercases it.
	 */
	static Symbol getSymbolForParseNodeLeaf(Symbol sym) {
		return getInstance()->getSymbolForParseNodeLeaf(sym); }

	/**
	 * Given a ParseNode, perform operations on it that alter the structure
	 * in a specified manner. In English, this ensures a single NP premod under
	 * NPPOS, and changes an NP with list-ish structure into a set of NPs
	 */
	static void modifyParse(ParseNode* node) {
		return getInstance()->modifyParse(node); }

	/**
	 * Given a SynNode and its children, perform operations on it that alter the 
	 * structure in a specified manner. In English, this ensures a single NP premod 
	 * under NPPOS, and changes an NP with list-ish structure into a set of NPs
	 *
	 * Note: This version is used in DescriptorClassifierTrainer
	 */
	static void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index) {
		return getInstance()->modifyParse(node, coref, children, n_children, head_index); }

	/**
	* In Arabic definite nouns get a different POS tag, this is used in the Stats Collector
	* 
	*/
	//static bool isDefiniteNounTag(Symbol pos_tag) {
	//    return getInstance()->isDefiniteNounTag(pos_tag); }
	static int findNameListEnd(const TokenSequence *tokenSequence,
							   const NameTheory *nameTheory, int start) {
		return getInstance()->findNameListEnd(tokenSequence, nameTheory, start); }

	static Symbol getParseTagForWord(Symbol word) {
		return getInstance()->getParseTagForWord(word); }

	/* In English, this will return true if we try to look it up 
	 * in WordNet and it's nowhere to be found.
	 */
	static bool isTrulyUnknownWord(Symbol word) {
		return getInstance()->isTrulyUnknownWord(word); }

	/* In English, this will return true if we try to look it up 
	 * in WordNet and it's found as a noun/verb.
	 */
	static bool isKnownNoun(Symbol word) {
		return getInstance()->isKnownNoun(word); }
	static bool isKnownVerb(Symbol word) {
		return getInstance()->isKnownVerb(word); }

	/* In English, this will return true if the word ends in "ing".
	   This is used for WordNet POS guessing: WN does not include all gerunds.
	 */
	static bool isPotentialGerund(Symbol word) {
		return getInstance()->isPotentialGerund(word); }

	//MRF 2/12/2004 this switch is only implemented for English
	//It is used to prevent certain parse post processing 
	//(eg. NPPOS transformations, unknown NPA -> NPPs) from happening
	//when the standalone version of the parser is called
	//These transformatsions help downstream processing for SERIF, but
	//do not stictly conform with Treebank Keys.
	static void setAsStandAloneParser() {
		return getInstance()->setAsStandAloneParser(); }
	static bool isStandAloneParser() {
		return getInstance()->isStandAloneParser(); }
	static Symbol convertPOSTheoryToParserPOS(Symbol tag) {
		return getInstance()->convertPOSTheoryToParserPOS(tag); }
	//The following are used for subtype selection based on the POS in the part of speech theory
	//since only Arabic actually uses the POS theory they always return false for English/Chinese

	static bool POSTheoryContainsNounPL(const PartOfSpeech* tags) {
		return getInstance()->POSTheoryContainsNounPL(tags); }
	static bool POSTheoryContainsNounSG(const PartOfSpeech* tags) {
		return getInstance()->POSTheoryContainsNounSG(tags); }
	static bool POSTheoryContainsNounAmbiguousNumber(const PartOfSpeech* tags) {
		return getInstance()->POSTheoryContainsNounAmbiguousNumber(tags); }
	static bool POSTheoryContainsNounMasc(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsNounMasc(pos); }
	static bool POSTheoryContainsNounFem(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsNounFem(pos); }
	static bool POSTheoryContainsNounAmbiguousGender(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsNounAmbiguousGender(pos); }
	static bool POSTheoryContainsPronFem(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsPronFem(pos); }
	static bool POSTheoryContainsPronMasc(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsPronMasc(pos); }
	static bool POSTheoryContainsPronSg(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsPronSg(pos); }
	static bool POSTheoryContainsPronPl(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsPronPl(pos); }
	static bool POSTheoryContainsPron1p(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsPron1p(pos); }
	static bool POSTheoryContainsPron2p(const PartOfSpeech* pos) {
		return getInstance()->POSTheoryContainsPron2p(pos); }
	static void fix2005Nationalities(MentionSet* mention_set) {
		return getInstance()->fix2005Nationalities(mention_set); }

    // NP-CHUNKER OUTPUT CONVERSION
	/** Caller is responsible for deleting the returned array. */
	static Constraint* getConstraints(const NPChunkTheory* theory, const TokenSequence* ts, int& num_constraints) {
		return getInstance()->getConstraints(theory, ts, num_constraints); }

	static int findNPHead(ParseNode* arr[], int numNodes) {
		return getInstance()->findNPHead(arr, numNodes); }
	static int findNPHead(const SynNode* const arr[], int numNodes) {
		return getInstance()->findNPHead(arr, numNodes); }

	static bool matchesHeadToken(Mention *mention, CorrectMention *correctMention) {
		return getInstance()->matchesHeadToken(mention, correctMention); }
	static bool matchesHeadToken(const SynNode *node, CorrectMention *correctMention) {
		return getInstance()->matchesHeadToken(node, correctMention); }

private:
	LanguageSpecificFunctions(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific LanguageSpecificFunctions methods.  The templated
     * class LanguageSpecificFunctionsInstanceFor<T>, defined below, is used
     * to generate language-specific implementations of this class. */
    struct LanguageSpecificFunctionsInstance: private boost::noncopyable {
		virtual ~LanguageSpecificFunctionsInstance() {}
		// Define a single abstract method corresponding to each
        // overridable method in the LanguageSpecificFunctions class.
		virtual bool isHyphen(Symbol sym) const = 0;
		virtual bool isBasicPunctuationOrConjunction(Symbol sym) const = 0;
		virtual bool isNoCrossPunctuation(Symbol sym) const = 0;
		virtual bool isSentenceEndingPunctuation(Symbol sym) const = 0;
		virtual bool isNPtypeLabel(Symbol sym) const = 0;
		virtual bool isPPLabel(Symbol sym) const = 0;
		virtual bool isAdverbPOSLabel(Symbol sym) const = 0;
		virtual bool isVerbPOSLabel(Symbol sym) const = 0;
		virtual bool isPrepPOSLabel(Symbol sym) const = 0;
		virtual bool isPreplikePOSLabel(Symbol sym) const = 0;
		virtual bool isPronounPOSLabel(Symbol sym) const = 0;
		virtual bool isParticlePOSLabel(Symbol sym) const = 0;
		virtual Symbol getNPlabel() const = 0;
		virtual Symbol getCoreNPlabel() const = 0;
		virtual Symbol getProperNounLabel() const = 0;
		virtual Symbol getNameLabel() const = 0;
		virtual bool isNoun(Symbol pos_tag) const = 0;
		virtual bool isNounPOS(Symbol pos_tag) const = 0;
		virtual bool isVerbPOS(Symbol pos_tag) const = 0;
		virtual bool isAdjective(Symbol pos_tag) const = 0;
		virtual bool isCoreNPLabel(Symbol sym) const = 0;
		virtual bool isPrimaryNamePOStag(Symbol sym, EntityType type) const = 0;
		virtual bool isSecondaryNamePOStag(Symbol sym, EntityType type) const = 0;
		virtual Symbol getDefaultNamePOStag(EntityType type) const = 0;
		virtual Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) const = 0;
		virtual bool isPastTenseVerbPOStag(Symbol sym) const = 0;
		virtual bool isPresentTenseVerbPOStag(Symbol sym) const = 0;
		virtual bool isFutureTenseVerbPOStag(Symbol sym) const = 0;
		virtual bool isPastTenseVerbChain(wstring tags, wstring words) const = 0;
		virtual bool isFutureTenseVerbChain(wstring tags, wstring words) const = 0;
		virtual bool isPresentTenseVerbChain(wstring tags, wstring words) const = 0;
		virtual bool isNPtypePOStag(Symbol sym) const = 0;
		virtual Symbol getDateLabel() const = 0;
		virtual Symbol getSymbolForParseNodeLeaf(Symbol sym) const = 0;
		virtual void modifyParse(ParseNode* node) const = 0;
		virtual void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index) const = 0;
		virtual int findNameListEnd(const TokenSequence *tokenSequence, const NameTheory *nameTheory, int start) const = 0;
		virtual Symbol getParseTagForWord(Symbol word) const = 0;
		virtual bool isTrulyUnknownWord(Symbol word) const = 0;
		virtual bool isKnownNoun(Symbol word) const = 0;
		virtual bool isKnownVerb(Symbol word) const = 0;
		virtual bool isPotentialGerund(Symbol word) const = 0;
		virtual void setAsStandAloneParser() const = 0;
		virtual bool isStandAloneParser() const = 0;
		virtual Symbol convertPOSTheoryToParserPOS(Symbol tag) const = 0;
		virtual bool POSTheoryContainsNounPL(const PartOfSpeech* tags) const = 0;
		virtual bool POSTheoryContainsNounSG(const PartOfSpeech* tags) const = 0;
		virtual bool POSTheoryContainsNounAmbiguousNumber(const PartOfSpeech* tags) const = 0;
		virtual bool POSTheoryContainsNounMasc(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsNounFem(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsNounAmbiguousGender(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsPronFem(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsPronMasc(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsPronSg(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsPronPl(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsPron1p(const PartOfSpeech* pos) const = 0;
		virtual bool POSTheoryContainsPron2p(const PartOfSpeech* pos) const = 0;
		virtual void fix2005Nationalities(MentionSet* mention_set) const = 0;
		virtual Constraint* getConstraints(const NPChunkTheory* theory, const TokenSequence* ts, int& num_constraints) const = 0;
		virtual int findNPHead(ParseNode* arr[], int numNodes) const = 0;
		virtual int findNPHead(const SynNode* const arr[], int numNodes) const = 0;
		virtual bool matchesHeadToken(Mention *mention, CorrectMention *correctMention) const = 0;
		virtual bool matchesHeadToken(const SynNode *node, CorrectMention *correctMention) const = 0;  
	};

    /** Return a pointer to the singleton LanguageSpecificFunctionsInstance
     * used by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<LanguageSpecificFunctionsInstance> &getInstance();

    /** Templated implementation of LanguageSpecificFunctionsInstance, based
     * on a given class.  That class must define a method
     * corresponding to each overridable static method defined by
     * LanguageSpecificFunctions. */
    template<typename T>
	struct LanguageSpecificFunctionsInstanceFor: public LanguageSpecificFunctionsInstance {
        // Define a method corresponding to each method in the
        // LanguageSpecificFunctions class, that delgates to T.
		bool isHyphen(Symbol sym) const {
			return T::isHyphen(sym); }
		bool isBasicPunctuationOrConjunction(Symbol sym) const {
			return T::isBasicPunctuationOrConjunction(sym); }
		bool isNoCrossPunctuation(Symbol sym) const {
			return T::isNoCrossPunctuation(sym); }
		bool isSentenceEndingPunctuation(Symbol sym) const {
			return T::isSentenceEndingPunctuation(sym); }
		bool isNPtypeLabel(Symbol sym) const {
			return T::isNPtypeLabel(sym); }
		bool isPPLabel(Symbol sym) const {
			return T::isPPLabel(sym); }
		bool isAdverbPOSLabel(Symbol sym) const {
			return T::isAdverbPOSLabel(sym); }
		bool isVerbPOSLabel(Symbol sym) const {
			return T::isVerbPOSLabel(sym); }
		bool isPrepPOSLabel(Symbol sym) const {
			return T::isPrepPOSLabel(sym); }
		bool isPreplikePOSLabel(Symbol sym) const {
			return T::isPreplikePOSLabel(sym); }
		bool isPronounPOSLabel(Symbol sym) const {
			return T::isPronounPOSLabel(sym); }
		bool isParticlePOSLabel(Symbol sym) const {
			return T::isParticlePOSLabel(sym); }
		Symbol getNPlabel() const {
			return T::getNPlabel(); }
		Symbol getCoreNPlabel() const {
			return T::getCoreNPlabel(); }
		Symbol getProperNounLabel() const {
			return T::getProperNounLabel(); }
		Symbol getNameLabel() const {
			return T::getNameLabel(); }
		bool isNoun(Symbol pos_tag) const {
			return T::isNoun(pos_tag); }
		bool isNounPOS(Symbol pos_tag) const {
			return T::isNounPOS(pos_tag); }
		bool isVerbPOS(Symbol pos_tag) const {
			return T::isVerbPOS(pos_tag); }
		bool isAdjective(Symbol pos_tag) const {
			return T::isAdjective(pos_tag); }
		bool isCoreNPLabel(Symbol sym) const {
			return T::isCoreNPLabel(sym); }
		bool isPrimaryNamePOStag(Symbol sym, EntityType type) const {
			return T::isPrimaryNamePOStag(sym, type); }
		bool isSecondaryNamePOStag(Symbol sym, EntityType type) const {
			return T::isSecondaryNamePOStag(sym, type); }
		Symbol getDefaultNamePOStag(EntityType type) const {
			return T::getDefaultNamePOStag(type); }
		Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) const {
			return T::getDefaultNameWord(decoder_type, type); }
		bool isPastTenseVerbPOStag(Symbol sym) const {
			return T::isPastTenseVerbPOStag(sym); }
		bool isPresentTenseVerbPOStag(Symbol sym) const {
			return T::isPresentTenseVerbPOStag(sym); }
		bool isFutureTenseVerbPOStag(Symbol sym) const {
			return T::isFutureTenseVerbPOStag(sym); }
		bool isPastTenseVerbChain(wstring tags, wstring words) const {
			return T::isPastTenseVerbChain(tags, words); }
		bool isFutureTenseVerbChain(wstring tags, wstring words) const {
			return T::isFutureTenseVerbChain(tags, words); }
		bool isPresentTenseVerbChain(wstring tags, wstring words) const {
			return T::isPresentTenseVerbChain(tags, words); }
		bool isNPtypePOStag(Symbol sym) const {
			return T::isNPtypePOStag(sym); }
		Symbol getDateLabel() const {
			return T::getDateLabel(); }
		Symbol getSymbolForParseNodeLeaf(Symbol sym) const {
			return T::getSymbolForParseNodeLeaf(sym); }
		void modifyParse(ParseNode* node) const {
			return T::modifyParse(node); }
		void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index) const {
			return T::modifyParse(node, coref, children, n_children, head_index); }
		int findNameListEnd(const TokenSequence *tokenSequence, const NameTheory *nameTheory, int start) const {
			return T::findNameListEnd(tokenSequence, nameTheory, start); }
		Symbol getParseTagForWord(Symbol word) const {
			return T::getParseTagForWord(word); }
		bool isTrulyUnknownWord(Symbol word) const {
			return T::isTrulyUnknownWord(word); }
		bool isKnownNoun(Symbol word) const {
			return T::isKnownNoun(word); }
		bool isKnownVerb(Symbol word) const {
			return T::isKnownVerb(word); }
		bool isPotentialGerund(Symbol word) const {
			return T::isPotentialGerund(word); }
		void setAsStandAloneParser() const {
			return T::setAsStandAloneParser(); }
		bool isStandAloneParser() const {
			return T::isStandAloneParser(); }
		Symbol convertPOSTheoryToParserPOS(Symbol tag) const {
			return T::convertPOSTheoryToParserPOS(tag); }
		bool POSTheoryContainsNounPL(const PartOfSpeech* tags) const {
			return T::POSTheoryContainsNounPL(tags); }
		bool POSTheoryContainsNounSG(const PartOfSpeech* tags) const {
			return T::POSTheoryContainsNounSG(tags); }
		bool POSTheoryContainsNounAmbiguousNumber(const PartOfSpeech* tags) const {
			return T::POSTheoryContainsNounAmbiguousNumber(tags); }
		bool POSTheoryContainsNounMasc(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsNounMasc(pos); }
		bool POSTheoryContainsNounFem(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsNounFem(pos); }
		bool POSTheoryContainsNounAmbiguousGender(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsNounAmbiguousGender(pos); }
		bool POSTheoryContainsPronFem(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsPronFem(pos); }
		bool POSTheoryContainsPronMasc(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsPronMasc(pos); }
		bool POSTheoryContainsPronSg(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsPronSg(pos); }
		bool POSTheoryContainsPronPl(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsPronPl(pos); }
		bool POSTheoryContainsPron1p(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsPron1p(pos); }
		bool POSTheoryContainsPron2p(const PartOfSpeech* pos) const {
			return T::POSTheoryContainsPron2p(pos); }
		void fix2005Nationalities(MentionSet* mention_set) const {
			return T::fix2005Nationalities(mention_set); }
		Constraint* getConstraints(const NPChunkTheory* theory, const TokenSequence* ts, int& num_constraints) const {
			return T::getConstraints(theory, ts, num_constraints); }
		int findNPHead(ParseNode* arr[], int numNodes) const {
			return T::findNPHead(arr, numNodes); }
		int findNPHead(const SynNode* const arr[], int numNodes) const {
			return T::findNPHead(arr, numNodes); }
		bool matchesHeadToken(Mention *mention, CorrectMention *correctMention) const {
			return T::matchesHeadToken(mention, correctMention); }
		bool matchesHeadToken(const SynNode *node, CorrectMention *correctMention) const {
			return T::matchesHeadToken(node, correctMention); }
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/parse/en_LanguageSpecificFunctions.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/parse/ch_LanguageSpecificFunctions.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/parse/ar_LanguageSpecificFunctions.h"
// #elif defined(KOREAN_LANGUAGE)
// 	#include "Korean/parse/kr_LanguageSpecificFunctions.h"
// #else
// 	#include "Generic/parse/xx_LanguageSpecificFunctions.h"
// #endif




#endif
