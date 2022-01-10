// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_UTILITIES_H
#define SYMBOL_UTILITIES_H

/** This module defines SymbolUtilities, which provides static methods
 * that can be used to transform symbols (typically symbols that
 * contain a single word).  Examples include stemWord() and
 * lowercaseSymbol().  These static methods are given
 * language-specific definitions, by delegating to a singleton
 * instance of a langauge-specific subclass of SymbolUtilitiesInstance.
 *
 * Note: some of the methods defined here (eg isPluralMention) don't
 * really belong here; but they're currently here for historical
 * reasons.
 */

//#include "Generic/theories/MentionSet.h"
//#include "Generic/theories/Mention.h"
#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>
class SynNode;
class Mention;
class MentionSet;

/** A collection of static methods that can be used to transform
 * symbols (typically symbols that contain a single word).  These
 * methods delgate to a private language-specific singleton, which in
 * turn delegates to static methods in a language-specific
 * SymbolUtilities class, such as EnglishSymbolUtilities or
 * ArabicSymbolUtilities.
 *
 * These language-specific classes (such as EnglishSymbolUtilities and
 * ArabicSymbolUtilities) should *not* be subclasses of
 * SymbolUtilities; in particular, if a language-specific
 * SymbolUtilities class was subclassed from SymbolUtilities, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'SymbolUtilities::setImplementation()' method can be used to
 * set the language-specific SymbolUtilities class that is used to
 * implement the static methods.
 *
 * Related language-specific classes, such as EnglishSymbolUtilities,
 * may define additional static methods.  These should be accessed
 * directly from the class (rather than via the SymbolUtilities
 * superclass).
 */
class SymbolUtilities {
public:
	/** Set the language-specific SymbolUtilities instance that should
	 * be used to implement the static methods in this class.  E.g.:
	 * SymbolUtilities::setImplementation<EnglishSymbolUtilities>(); */
    template<typename LanguageSymbolUtilities>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<SymbolUtilitiesInstance>
            (_new(SymbolUtilitiesInstanceFor<LanguageSymbolUtilities>));
	}

	/** Return the word stem for the given descriptor word. */
	static Symbol stemDescriptor(Symbol word) {
		return getInstance()->stemDescriptor(word); }

	/** Find the hypernyms of the given word, and put the wordnet
	 * offsets of those hypernyms in the given array ('offsets').
	 * Return the number of offsets that were added to the array.  Do
	 * not add more than MAX_OFFSETS reults (excess results are
	 * discarded). */
	static int fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS) {
		return getInstance()->fillWordNetOffsets(word, offsets, MAX_OFFSETS); }
	   
	/** Find the hypernyms of the given word (given its part of speech
	 * tag 'pos'), and put the wordnet offsets of those hypernyms in
	 * the given array ('offsets').  Return the number of offsets that
	 * were added to the array.  Do not add more than MAX_OFFSETS
	 * reults (excess results are discarded). */
	static int fillWordNetOffsets(Symbol word, Symbol pos, 
								  int *offsets, int MAX_OFFSETS) {
		return getInstance()->fillWordNetOffsets(word, pos, offsets, 
												 MAX_OFFSETS); }
	   
	/** Find the hypernyms of the given word, and put them in the
	 * given array ('results').  Return the number of hypernyms that
	 * were added to the array.  Do not add more than MAX_RESULTS
	 * reults (excess results are discarded). */
	static int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) {
		return getInstance()->getHypernyms(word, results, MAX_RESULTS); }

	/** Return a single Symbol containing the full name for the 
	 * given syntactic node, which should contain a name. */
	static Symbol getFullNameSymbol(const SynNode *nameNode) {
		return getInstance()->getFullNameSymbol(nameNode); }

	/** [XX] Please document this method! */
	static float getDescAdjScore(MentionSet* mentionSet) {
		return getInstance()->getDescAdjScore(mentionSet); }

	/** Return the symbol formed by lower-casing all characters in the
	 * given symbol. */
	static Symbol lowercaseSymbol(Symbol sym) {
		return getInstance()->lowercaseSymbol(sym); }

	/** Return the word stem for the given word symbol.  'pos' specifies
	 * the part of speech of the word, which can be used to determine
	 * how to stem the word. */
	static Symbol stemWord(Symbol word, Symbol pos) {
		return getInstance()->stemWord(word, pos); }

	/** Find a list of stem variants for the given word, and put them in
	 * the given result array ('variants').  Return the number of stem
	 * variants that were added to the array.  Do not add more than
	 * MAX_RESULTS results (excess results are discarded). */
	static int getStemVariants(Symbol word, Symbol *variants, int MAX_RESULTS){
		return getInstance()->getStemVariants(word, variants, MAX_RESULTS); }

	/** Return a single Symbol containing a the full name for the 
	 * given syntactic node (which should contain a name), with any
	 * suffixes/prefixes stripped. */
	static Symbol getStemmedFullName(const SynNode *nameNode) {
		return getInstance()->getStemmedFullName(nameNode); }

	/** Return the given symbol with the Arabic "al-" prefix stripped
	 * off.  Note that this is defined for all langauges (not just
	 * Arabic), since Arabic names can still occur in non-Arabic
	 * text. */
	static Symbol getWordWithoutAl(Symbol word) {
		return getInstance()->getWordWithoutAl(word); }

	/** Return true if the given mention is plural. */
	static bool isPluralMention(const Mention* ment) {
		return getInstance()->isPluralMention(ment); }

	/** Return true if the given word is a parenthesis, bracket, or
		brace.  */
	static bool isBracket(Symbol word) {
		return getInstance()->isBracket(word); }

	/** Return true if the given word is "clusterable."  By default, a
	 * word is considered clusterable if it contains at least one
	 * letter. */
	static bool isClusterableWord(Symbol word) {
		return getInstance()->isClusterableWord(word); }

private:
	SymbolUtilities(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific SymbolUtilities methods.  The templated class
     * SymbolUtilitiesInstanceFor<T>, defined below, is used to generate
     * language-specific implementations of this class. */
    class SymbolUtilitiesInstance: private boost::noncopyable {
    public:
        // Define a single abstract method corresponding to each method in
        // the SymbolUtilities class.
		virtual Symbol stemDescriptor(Symbol word) const = 0;
		virtual int fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS) const = 0;
		virtual int fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS) const = 0;
		virtual int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) const = 0;
		virtual Symbol getFullNameSymbol(const SynNode *nameNode) const = 0;
		virtual float getDescAdjScore(MentionSet* mentionSet) const = 0;
		virtual Symbol lowercaseSymbol(Symbol sym) const = 0;
		virtual Symbol stemWord(Symbol word, Symbol pos) const = 0;
		virtual int getStemVariants(Symbol word, Symbol *variants, int MAX_RESULTS) const = 0;
		virtual Symbol getStemmedFullName(const SynNode *nameNode) const = 0;
		virtual Symbol getWordWithoutAl(Symbol word) const = 0;
		virtual bool isPluralMention(const Mention* ment) const = 0;
		virtual bool isBracket(Symbol word) const = 0;
		virtual bool isClusterableWord(Symbol word) const = 0;
	};
	
    /** Return a pointer to the singleton SymbolUtilitiesInstance used
     * by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<SymbolUtilitiesInstance> &getInstance();

    /** Templated implementation of SymbolUtilitiesInstance, based on a
     * given class.  That class must define a static method
     * corresponding to each method defined by SymbolUtilities. */
    template<typename T>
    class SymbolUtilitiesInstanceFor: public SymbolUtilitiesInstance {
    public:
        // Define a method corresponding to each method in the
        // SymbolUtilities class, that delgates to T.
		Symbol stemDescriptor(Symbol word) const { 
			return T::stemDescriptor(word); }
		int fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS) const {
			return T::fillWordNetOffsets(word, offsets, MAX_OFFSETS); }
		int fillWordNetOffsets(Symbol word, Symbol pos, 
							   int *offsets, int MAX_OFFSETS) const {
			return T::fillWordNetOffsets(word, pos, offsets, MAX_OFFSETS); }
		int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) const {
			return T::getHypernyms(word, results, MAX_RESULTS); }
		Symbol getFullNameSymbol(const SynNode *nameNode) const {
			return T::getFullNameSymbol(nameNode); }
		float getDescAdjScore(MentionSet* mentionSet) const {
			return T::getDescAdjScore(mentionSet); }
		Symbol lowercaseSymbol(Symbol sym) const {
			return T::lowercaseSymbol(sym); }
		Symbol stemWord(Symbol word, Symbol pos) const {
			return T::stemWord(word, pos); }
		int getStemVariants(Symbol word, Symbol *variants, int MAX_RESULTS) const {
			return T::getStemVariants(word, variants, MAX_RESULTS); }
		Symbol getStemmedFullName(const SynNode *nameNode) const {
			return T::getStemmedFullName(nameNode); }
		Symbol getWordWithoutAl(Symbol word) const {
			return T::getWordWithoutAl(word); }
		bool isPluralMention(const Mention* ment) const {
			return T::isPluralMention(ment); }
		bool isBracket(Symbol word) const {
			return T::isBracket(word); }
		bool isClusterableWord(Symbol word) const {
			return T::isClusterableWord(word); }
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/common/en_SymbolUtilities.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/common/ch_SymbolUtilities.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/common/ar_SymbolUtilities.h"
// #elif defined(HINDI_LANGUAGE)
// 	#include "Hindi/common/hi_SymbolUtilities.h"
// #elif defined(BENGALI_LANGUAGE)
// 	#include "Bengali/common/be_SymbolUtilities.h"
// #else
// 	#include "Generic/common/xx_SymbolUtilities.h"
// #endif




#endif
