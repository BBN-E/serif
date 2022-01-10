// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRONOUNLINKERUTILS_H
#define PRONOUNLINKERUTILS_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "Generic/common/Symbol.h"
class SynNode;

/** A collection of static helper methods that are used for pronoun
 * linking.  These methods delgate to a private language-specific
 * singleton, which in turn delegates to static methods in a
 * language-specific PronounLinkerUtils class, such as
 * EnglishPronounLinkerUtils or ArabicPronounLinkerUtils.
 *
 * These language-specific classes (such as EnglishPronounLinkerUtils
 * and ArabicPronounLinkerUtils) should *not* be subclasses of
 * PronounLinkerUtils; in particular, if a language-specific
 * PronounLinkerUtils class was subclassed from PronounLinkerUtils, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'PronounLinkerUtils::setImplementation()' method can be used to
 * set the language-specific PronounLinkerUtils class that is used to
 * implement the static methods.
 */
class PronounLinkerUtils {
public: 
	/** Set the language-specific PronounLinkerUtils instance
	 * that should be used to implement the static methods in this
	 * class.  E.g.:
	 * PronounLinkerUtils::setImplementation<EnglishPronounLinkerUtils>(); */
    template<typename LanguagePronounLinkerUtils>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<PronounLinkerUtilsInstance>
            (_new(PronounLinkerUtilsInstanceFor<LanguagePronounLinkerUtils>));
	}

	/* ================================================================ */
	/* These functions can be overridden w/ language-specific values: */
	/* ================================================================ */
	static Symbol combineSymbols(Symbol symArray[], int nSymArray, bool use_delimiter = true) {
		return getInstance()->combineSymbols(symArray, nSymArray, use_delimiter); }
	static Symbol getNormDistSymbol(int distance) {
		return getInstance()->getNormDistSymbol(distance); }
	static Symbol getAugmentedParentHeadWord(const SynNode *node) {
		return getInstance()->getAugmentedParentHeadWord(node); }
	static Symbol getAugmentedParentHeadTag(const SynNode *node) {
		return getInstance()->getAugmentedParentHeadTag(node); }
	static Symbol augmentIfPOS(Symbol type, const SynNode *node) {
		return getInstance()->augmentIfPOS(type, node); }


private:
	PronounLinkerUtils(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific PronounLinkerUtils methods.  The templated
     * class PronounLinkerUtilsInstanceFor<T>, defined below, is used
     * to generate language-specific implementations of this class. */
    struct PronounLinkerUtilsInstance: private boost::noncopyable {
        // Define a single abstract method corresponding to each
        // overridable method in the PronounLinkerUtils class.
		virtual Symbol combineSymbols(Symbol symArray[], int nSymArray, bool use_delimiter = true) const = 0;
		virtual Symbol getNormDistSymbol(int distance) const = 0;
		virtual Symbol getAugmentedParentHeadWord(const SynNode *node) const = 0;
		virtual Symbol getAugmentedParentHeadTag(const SynNode *node) const = 0;
		virtual Symbol augmentIfPOS(Symbol type, const SynNode *node) const = 0;

	};

    /** Return a pointer to the singleton PronounLinkerUtilsInstance
     * used by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<PronounLinkerUtilsInstance> &getInstance();

    /** Templated implementation of PronounLinkerUtilsInstance, based
     * on a given class.  That class must define a method
     * corresponding to each overridable static method defined by
     * PronounLinkerUtils. */
    template<typename T>
	struct PronounLinkerUtilsInstanceFor: public PronounLinkerUtilsInstance {
        // Define a method corresponding to each method in the
        // PronounLinkerUtils class, that delgates to T.
		Symbol combineSymbols(Symbol symArray[], int nSymArray, bool use_delimiter = true) const {
			return T::combineSymbols(symArray, nSymArray, use_delimiter); }
		Symbol getNormDistSymbol(int distance) const {
			return T::getNormDistSymbol(distance); }
		Symbol getAugmentedParentHeadWord(const SynNode *node) const {
			return T::getAugmentedParentHeadWord(node); }
		Symbol getAugmentedParentHeadTag(const SynNode *node) const {
			return T::getAugmentedParentHeadTag(node); }
		Symbol augmentIfPOS(Symbol type, const SynNode *node) const {
			return T::augmentIfPOS(type, node); }
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/edt/en_PronounLinkerUtils.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/edt/ch_PronounLinkerUtils.h"
// //#elif defined(ARABIC_LANGUAGE)
// //	#include "Arabic/edt/ar_PronounLinkerUtils.h"
// #else
// 	#include "Generic/edt/xx_PronounLinkerUtils.h"
// #endif


#endif
