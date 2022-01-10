// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENERIC_STAGS_H
#define GENERIC_STAGS_H

/** This module defines STags, which provides static methods that can
 * be used to get specific syntactic constituent tags.  Examples
 * include getNP() and getNPA().  These static methods are given
 * language-specific definitions, by delegating to a singleton
 * instance of a langauge-specific subclass of STagsInstance. */

#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

/** A collection of static methods that can be used to look up symbols
 * used for specific syntactic constituent tags.  These methods
 * delgate to a private language-specific singleton, which in turn
 * delegates to static methods in a language-specific STags
 * class, such as EnglishSTags or ArabicSTags.
 *
 * These language-specific classes (such as EnglishSTags and
 * ArabicSTags) should *not* be subclasses of STags; in particular, if
 * a language-specific STags class was subclassed from STags, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'STags::setImplementation()' method can be used to set the
 * language-specific STags class that is used to implement the static
 * methods.
 *
 * Related language-specific classes, such as EnglishSTags,
 * may define additional static methods, as well as static constant
 * Symbols.  These should be accessed directly from the class (rather
 * than via the STags superclass).
 *
 * The STags class itself should *not* include any static
 * constant Symbols, since these are by definition language-specific.
 */
class STags: private boost::noncopyable {
public:
	/** Set the language-specific STags instance that should be
	 * used to implement the static methods in this class.  E.g.:
	 * STags::setImplementation<EnglishSTags>(); */
	template<typename LanguageSTags>
	static void setImplementation() {
		getInstance() = boost::shared_ptr<STagsInstance>
			(_new(STagsInstanceFor<LanguageSTags>));
	}

	/** Return a list of all tags defined by this instance. */
	static const std::vector<Symbol> &getTagList() { 
		return getInstance()->getTagList(); }
	static const Symbol getCOMMA() {
		return getInstance()->getCOMMA(); }
	static const Symbol getDATE() {
		return getInstance()->getDATE(); }
	static const Symbol getNP() {
		return getInstance()->getNP(); }
	static const Symbol getNPA() {
		return getInstance()->getNPA(); }
	static const Symbol getPP() {
		return getInstance()->getPP(); }

private:
	STags(); // private constructor: this class may not be instantiated.

	/** Abstract base class for the singletons that implement
	 * language-specific STags methods.  The templated class
	 * STagsInstanceFor<T>, defined below, is used to generate
	 * language-specific implementations of this class. */
	class STagsInstance: private boost::noncopyable {
	public:
		// Define a single abstract method corresponding to each method in
		// the STags class.
		virtual const std::vector<Symbol> &getTagList() const = 0;
		virtual const Symbol getCOMMA() const = 0;
		virtual const Symbol getDATE() const = 0;
		virtual const Symbol getNP() const = 0;
		virtual const Symbol getNPA() const = 0;
		virtual const Symbol getPP() const = 0;
	};


	/** Return a pointer to the singleton STagsInstance used
	 * by the static methods for delegating to language-specific
	 * implementations. */
	static boost::shared_ptr<STagsInstance> &getInstance();

	/** Templated implementation of STagsInstance, based on a
	 * given class.  That class must define a static method
	 * corresponding to each method defined by STags. */
	template<typename T>
	class STagsInstanceFor: public STagsInstance {
	public:
		// Define a method corresponding to each method in the
		// STags class, that delgates to T.
		const Symbol getCOMMA() const { return T::COMMA; }
		const Symbol getDATE() const { return T::DATE; }
		const Symbol getNP() const { return T::NP; }
		const Symbol getNPA() const { return T::NPA; }
		const Symbol getPP() const { return T::PP; }
		const std::vector<Symbol> &getTagList() const {
			if (_tagList.empty())
				T::initializeTagList(_tagList);
			return _tagList;
		}
	private:
		std::vector<Symbol> _tagList;
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/parse/en_STags.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/parse/ch_STags.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/parse/ar_STags.h"
// #else
// 	#include "Generic/parse/xx_STags.h"
// #endif


#endif
