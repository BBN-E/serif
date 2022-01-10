// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAMELINKFUNCTIONS_H
#define NAMELINKFUNCTIONS_H

/** This module defines NameLinkFunctions, which provides various
 * static helper methods that are used for name linking.  These static
 * methods can be given language-specific definitions, by delegating
 * to a singleton instance of a langauge-specific subclass of
 * NameLinkFunctionsInstance.
 */

#include "Generic/common/Symbol.h"
#include "Generic/edt/CountsTable.h"
#include "Generic/theories/EntityType.h"
#include <boost/shared_ptr.hpp>
class Mention;

/** A collection of static helper methods that are used for name
 * linking.  These methods delgate to a private language-specific
 * singleton, which in turn delegates to static methods in a
 * language-specific NameLinkFunctions class, such as
 * EnglishNameLinkFunctions or ArabicNameLinkFunctions.
 *
 * These language-specific classes (such as EnglishNameLinkFunctions
 * and ArabicNameLinkFunctions) should *not* be subclasses of
 * NameLinkFunctions; in particular, if a language-specific
 * NameLinkFunctions class was subclassed from NameLinkFunctions, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'NameLinkFunctions::setImplementation()' method can be used to
 * set the language-specific NameLinkFunctions class that is used to
 * implement the static methods.
 */
class NameLinkFunctions {
public:
	/** Set the language-specific NameLinkFunctions instance
	 * that should be used to implement the static methods in this
	 * class.  E.g.:
	 * NameLinkFunctions::setImplementation<EnglishNameLinkFunctions>(); */
    template<typename LanguageNameLinkFunctions>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<NameLinkFunctionsInstance>
            (_new(NameLinkFunctionsInstanceFor<LanguageNameLinkFunctions>));
	}

	/* ================================================================ */
	/* These functions can be overridden w/ language-specific values: */
	/* ================================================================ */
	static void destroyDataStructures() {
		getInstance()->destroyDataStructures(); }

	/** dynamically add to Abbrev table */
	static bool populateAcronyms(const Mention *mention, EntityType type) {
		return getInstance()->populateAcronyms(mention, type); }

	/** Allows back resolving (linking a name to a *previously* seen
	 * potential short form). */
	static void recomputeCounts(CountsTable &inTable, CountsTable &outTable, 
								int &outTotalCount) {
		getInstance()->recomputeCounts(inTable, outTable, outTotalCount); }

	/** Get list of terms to be considered in any lexical counts or
	 * probabilites.  Default: return array of words as is */
	static int getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results) {
		return getInstance()->getLexicalItems(words, nWords, results, max_results); }

private:
	NameLinkFunctions(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific NameLinkFunctions methods.  The templated
     * class NameLinkFunctionsInstanceFor<T>, defined below, is used
     * to generate language-specific implementations of this class. */
    struct NameLinkFunctionsInstance: private boost::noncopyable {
        // Define a single abstract method corresponding to each
        // overridable method in the NameLinkFunctions class.
		virtual void destroyDataStructures() const = 0;
		virtual bool populateAcronyms(const Mention *mention, EntityType type) const = 0;
		virtual void recomputeCounts(CountsTable &inTable, CountsTable &outTable, int &outTotalCount) const = 0;
		virtual int getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results) const = 0;
	};

    /** Return a pointer to the singleton NameLinkFunctionsInstance
     * used by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<NameLinkFunctionsInstance> &getInstance();

    /** Templated implementation of NameLinkFunctionsInstance, based
     * on a given class.  That class must define a method
     * corresponding to each overridable static method defined by
     * NameLinkFunctions. */
    template<typename T>
	struct NameLinkFunctionsInstanceFor: public NameLinkFunctionsInstance {
        // Define a method corresponding to each method in the
        // NameLinkFunctions class, that delgates to T.
		void destroyDataStructures() const {
			T::destroyDataStructures(); }
		bool populateAcronyms(const Mention *mention, EntityType type) const {
			return T::populateAcronyms(mention, type); }
		void recomputeCounts(CountsTable &inTable, CountsTable &outTable, int &outTotalCount) const {
			T::recomputeCounts(inTable, outTable, outTotalCount); }
		int getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results) const {
			return T::getLexicalItems(words, nWords, results, max_results); }
	};

};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/edt/en_NameLinkFunctions.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/edt/ch_NameLinkFunctions.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/edt/ar_NameLinkFunctions.h"
// #else
// 	#include "Generic/edt/xx_NameLinkFunctions.h"
// #endif


#endif
