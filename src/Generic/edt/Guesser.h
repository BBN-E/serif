// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/** This module defines Guesser, which provides static methods that
 * can attempt to determine the gender/number/type of a mention or an
 * entity.  These static methods are given language-specific
 * definitions, by delegating to a singleton instance of a
 * langauge-specific subclass of GuesserInstance.
 */
#ifndef GUESSER_H
#define GUESSER_H

#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>

class SynNode;
class Mention;
class EntitySet;
class Entity;

/** A collection of static methods that can attempt to determine the
 * gender/number/type of a mention or an entity.  These methods
 * delgate to a private language-specific singleton, which in turn
 * delegates to static methods in a language-specific Guesser
 * class, such as EnglishGuesser or ArabicGuesser.
 *
 * These language-specific classes (such as EnglishGuesser and
 * ArabicGuesser) should *not* be subclasses of
 * Guesser; in particular, if a language-specific
 * Guesser class was subclassed from Guesser, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'Guesser::setImplementation()' method can be used to
 * set the language-specific Guesser class that is used to
 * implement the static methods.
 *
 * Related language-specific classes, such as EnglishGuesser,
 * may define additional static methods.  These should be accessed
 * directly from the class (rather than via the Guesser
 * superclass).
 */
class Guesser {
public: 
	/** Set the language-specific Guesser instance that should
	 * be used to implement the static methods in this class.  E.g.:
	 * Guesser::setImplementation<EnglishGuesser>(); */
    template<typename LanguageGuesser>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<GuesserInstance>
            (_new(GuesserInstanceFor<LanguageGuesser>));
	}

	static void initialize() {
		return getInstance()->initialize(); }
	static void destroy() {
		return getInstance()->destroy(); }

	static Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()) {
		return getInstance()->guessGender(node, mention, suspectedSurnames); }
	static Symbol guessType(const SynNode *node, const Mention *mention) {
		return getInstance()->guessType(node, mention); }
	static Symbol guessNumber(const SynNode *node, const Mention *mention) {
		return getInstance()->guessNumber(node, mention); }

	static Symbol guessGender(const EntitySet *entitySet, const Entity* entity) {
		return getInstance()->guessGender(entitySet, entity); }
	static Symbol guessNumber(const EntitySet *entitySet, const Entity* entity) {
		return getInstance()->guessNumber(entitySet, entity); }

	// Result values:
	static Symbol FEMININE;
	static Symbol MASCULINE;
	static Symbol NEUTER;
	static Symbol NEUTRAL;
	static Symbol SINGULAR;
	static Symbol PLURAL;
	static Symbol UNKNOWN;
private:
	Guesser(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific Guesser methods.  The templated class
     * GuesserInstanceFor<T>, defined below, is used to generate
     * language-specific implementations of this class. */
    class GuesserInstance: private boost::noncopyable {
    public:
		virtual void initialize() const = 0;
		virtual void destroy() const = 0;
		virtual Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()) const = 0; 
		virtual Symbol guessType(const SynNode *node, const Mention *mention) const = 0;
		virtual Symbol guessNumber(const SynNode *node, const Mention *mention) const = 0;
		virtual Symbol guessGender(const EntitySet *entitySet, const Entity* entity) const = 0;
		virtual Symbol guessNumber(const EntitySet *entitySet, const Entity* entity) const = 0;
	};
	
    /** Return a pointer to the singleton GuesserInstance used
     * by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<GuesserInstance> &getInstance();

    /** Templated implementation of GuesserInstance, based on a
     * given class.  That class must define a static method
     * corresponding to each method defined by Guesser. */
    template<typename T>
    class GuesserInstanceFor: public GuesserInstance {
    public:
        // Define a method corresponding to each method in the
        // Guesser class, that delgates to T.
		void initialize() const { return T::initialize(); }
		void destroy() const { return T::destroy(); }
		Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()) const {
			return T::guessGender(node, mention, suspectedSurnames); }
		Symbol guessType(const SynNode *node, const Mention *mention) const {
			return T::guessType(node, mention); }
		Symbol guessNumber(const SynNode *node, const Mention *mention) const {
			return T::guessNumber(node, mention); }
		Symbol guessGender(const EntitySet *entitySet, const Entity* entity) const {
			return T::guessGender(entitySet, entity); }
		Symbol guessNumber(const EntitySet *entitySet, const Entity* entity) const {
			return T::guessNumber(entitySet, entity); }
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/edt/en_Guesser.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/edt/ch_Guesser.h"
// //#elif defined(ARABIC_LANGUAGE)
// //	#include "Arabic/edt/ar_Guesser.h"
// #else
// 	#include "Generic/edt/xx_Guesser.h"
// #endif


#endif
