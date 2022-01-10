// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/** This module defines WordConstants, which provides static methods
 * that can be used to check whether a word belongs to a given 
 * category.  Examples include isPronoun() and isTimeExpression().
 * These static methods are given language-specific definitions,
 * by delegating to a singleton instance of a langauge-specific
 * subclass of WordConstantsInstance. */

#ifndef WORDCONSTANTS_H
#define WORDCONSTANTS_H

#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

/** A collection of static methods that can be used to check whether a
 * given word belongs to some category.  These methods delgate to a
 * private language-specific singleton, which in turn delegates to
 * static methods in a language-specific word-constants class, such as
 * EnglishWordConstants or ArabicWordConstants.
 *
 * These language-specific classes (such as EnglishWordConstants and
 * ArabicWordConstants) should *not* be subclasses of WordConstants;
 * in particular, if a language-specific word constants class was
 * subclassed from WordConstants, and neglected to implement some
 * static method, then any call to that method would result in an
 * infinite delegation loop.
 *
 * The 'WordConstants::setImplementation()' method can be used to set the
 * language-specific word-constants class that is used to implement
 * the static methods.
 *
 * Related language-specific classes, such as EnglishWordConstants,
 * may define additional static methods, as well as static constant
 * Symbols.  These should be accessed directly from the class (rather
 * than via the WordConstants superclass).
 *
 * The WordConstants class itself should *not* include any static
 * constant Symbols, since these are by definition language-specific.
 */

class WordConstants {
public:
	/** Set the language-specific WordConstants instance that should be
	 * used to implement the static methods in this class.  E.g.:
	 * WordConstants::setImplementation<EnglishWordConstants>(); */
	template<typename LanguageWordConstants>
	static void setImplementation() {
		getInstance() = boost::shared_ptr<WordConstantsInstance>
			(_new(WordConstantsInstanceFor<LanguageWordConstants>));
	}

	/* ============= Word Shape ============= */
	static bool isNumeric(Symbol word) {
		return getInstance()->isNumeric(word); }
	static bool isAlphabetic(Symbol word) {
		return getInstance()->isAlphabetic(word); }
	static bool isSingleCharacter(Symbol word) {
		return getInstance()->isSingleCharacter(word); }
	static bool startsWithDash(Symbol word) {
		return getInstance()->startsWithDash(word); }
	static bool isOrdinal(Symbol word) {
		return getInstance()->isOrdinal(word); }
	static bool isURLCharacter(Symbol word) {
		return getInstance()->isURLCharacter(word); }
	static bool isPhoneCharacter(Symbol word) {
		return getInstance()->isPhoneCharacter(word); }
	static bool isASCIINumericCharacter(Symbol word) {
		return getInstance()->isASCIINumericCharacter(word); }
	static Symbol getNumericPortion(Symbol word) {
		return getInstance()->getNumericPortion(word); }

	/* ============= Temporal ============= */
	static bool isTimeExpression(Symbol word) {
		return getInstance()->isTimeExpression(word); }
	static bool isTimeModifier(Symbol word) {
		return getInstance()->isTimeModifier(word); }
	static bool isFourDigitYear(Symbol word) {
		return getInstance()->isFourDigitYear(word); }
	static bool isDashedDuration(Symbol word) {
		return getInstance()->isDashedDuration(word); }
	static bool isDecade(Symbol word) {
		return getInstance()->isDecade(word); }
	static bool isDateExpression(Symbol word) {
		return getInstance()->isDateExpression(word); }
	static bool isLowNumberWord(Symbol word) {
		return getInstance()->isLowNumberWord(word); }
	static bool isDailyTemporalExpression(Symbol word) {
		return getInstance()->isDailyTemporalExpression(word); }

	/* ============= Names ============= */
	static bool isNameSuffix(Symbol word) {
		return getInstance()->isNameSuffix(word); }
	static bool isHonorificWord(Symbol word) {
		return getInstance()->isHonorificWord(word); }

	/* ============= Word Classes ============= */
	static bool isMilitaryWord(Symbol word) {
		return getInstance()->isMilitaryWord(word); }

	/* ============= Verbs ============= */

	static bool isCopula(Symbol word) {
		return getInstance()->isCopula(word); }
	static bool isTensedCopulaTypeVerb(Symbol word) {
		return getInstance()->isTensedCopulaTypeVerb(word); }

	/* ============= Stop Words ============= */
	static bool isNameLinkStopWord(Symbol word) {
		return getInstance()->isNameLinkStopWord(word); }
	static bool isAcronymStopWord(Symbol word) {
		return getInstance()->isAcronymStopWord(word); }

	/* ============= Pronouns ============= */
	static bool isPronoun(Symbol word) {
		return getInstance()->isPronoun(word); }
	static bool isReflexivePronoun(Symbol word) {
		return getInstance()->isReflexivePronoun(word); }
	static bool is1pPronoun(Symbol word) {
		return getInstance()->is1pPronoun(word); }
	static bool isSingular1pPronoun(Symbol word) {
		return getInstance()->isSingular1pPronoun(word); }
	static bool is2pPronoun(Symbol word) {
		return getInstance()->is2pPronoun(word); }
	static bool is3pPronoun(Symbol word) {
		return getInstance()->is3pPronoun(word); }
	static bool isOtherPronoun(Symbol word) {
		return getInstance()->isOtherPronoun(word); }
	static bool isWHQPronoun(Symbol word) {
		return getInstance()->isWHQPronoun(word); }
	static bool isPERTypePronoun(Symbol word) {
		return getInstance()->isPERTypePronoun(word); }
	static bool isLOCTypePronoun(Symbol word) {
		return getInstance()->isLOCTypePronoun(word); }
	static bool isSingularPronoun(Symbol word) {
		return getInstance()->isSingularPronoun(word); }
	static bool isPluralPronoun(Symbol word) {
		return getInstance()->isPluralPronoun(word); }
	static bool isPossessivePronoun(Symbol word) {
		return getInstance()->isPossessivePronoun(word); }
	static bool isLinkingPronoun(Symbol word) {
		return getInstance()->isLinkingPronoun(word); }
	/** Is this a pronoun that cannot modify a person?  eg "it" in English */
	static bool isNonPersonPronoun(Symbol word) {
		return getInstance()->isNonPersonPronoun(word); }
	static bool isRelativePronoun(Symbol word) {
		return getInstance()->isRelativePronoun(word); }

	/* ============= Prepositions ============= */
	static bool isLocativePreposition(Symbol word) {
		return getInstance()->isLocativePreposition(word); }
	static bool isUnknownRelationReportedPreposition(Symbol word) {
		return getInstance()->isUnknownRelationReportedPreposition(word); }
	static bool isForReasonPreposition(Symbol word) {
		return getInstance()->isForReasonPreposition(word); }
	static bool isOfActionPreposition(Symbol word) {
		return getInstance()->isOfActionPreposition(word); }

	/* ============= Puctuation ============= */
	static bool isPunctuation(Symbol word) {
		return getInstance()->isPunctuation(word); }
	static bool isOpenDoubleBracket(Symbol word) {
		return getInstance()->isOpenDoubleBracket(word); }
	static bool isClosedDoubleBracket(Symbol word) {
		return getInstance()->isClosedDoubleBracket(word); }

	/* ============= Partitive ============= */
	static bool isPartitiveWord(Symbol word) {
		return 	getInstance()->isPartitiveWord(word); }

	/* ============= Determiners ============= */
	static bool isDeterminer(Symbol word) {
		return getInstance()->isDeterminer(word); }
	static bool isDefiniteArticle(Symbol word) {
		return getInstance()->isDefiniteArticle(word); }
	static bool isIndefiniteArticle(Symbol word) {
		return getInstance()->isIndefiniteArticle(word); }

private:
	WordConstants(); // private constructor: this class may not be instantiated.

	/** Abstract base class for the singletons that implement
	 * language-specific word-category methods.  The templated class
	 * WordConstantsInstanceFor<T>, defined below, is used to generate
	 * language-specific implementations of this class. */
	class WordConstantsInstance: private boost::noncopyable {
	public:
		// Define a single abstract method corresponding to each method in
		// the WordConstants class.
		virtual bool isNumeric(Symbol word) const = 0;
		virtual bool isAlphabetic(Symbol word) const = 0;
		virtual bool isSingleCharacter(Symbol word) const = 0;
		virtual bool startsWithDash(Symbol word) const = 0;
		virtual bool isOrdinal(Symbol word) const = 0;
		virtual bool isURLCharacter(Symbol word) const = 0;
		virtual bool isPhoneCharacter(Symbol word) const = 0;
		virtual bool isASCIINumericCharacter(Symbol word) const = 0;
		virtual Symbol getNumericPortion(Symbol word) const = 0;
		virtual bool isDailyTemporalExpression(Symbol word) const = 0;
		virtual bool isTimeExpression(Symbol word) const = 0;
		virtual bool isTimeModifier(Symbol word) const = 0;
		virtual bool isFourDigitYear(Symbol word) const = 0;
		virtual bool isDashedDuration(Symbol word) const = 0;
		virtual bool isDecade(Symbol word) const = 0;
		virtual bool isDateExpression(Symbol word) const = 0;
		virtual bool isLowNumberWord(Symbol word) const = 0;
		virtual bool isNameSuffix(Symbol word) const = 0;
		virtual bool isHonorificWord(Symbol word) const = 0;
		virtual bool isMilitaryWord(Symbol word) const = 0;
		virtual bool isCopula(Symbol word) const = 0;
		virtual bool isTensedCopulaTypeVerb(Symbol word) const = 0;
		virtual bool isNameLinkStopWord(Symbol word) const = 0;
		virtual bool isAcronymStopWord(Symbol word) const = 0;
		virtual bool isPronoun(Symbol word) const = 0;
		virtual bool isReflexivePronoun(Symbol word) const = 0;
		virtual bool is1pPronoun(Symbol word) const = 0;
		virtual bool isSingular1pPronoun(Symbol word) const = 0;
		virtual bool is2pPronoun(Symbol word) const = 0;
		virtual bool is3pPronoun(Symbol word) const = 0;
		virtual bool isOtherPronoun(Symbol word) const = 0;
		virtual bool isWHQPronoun(Symbol word) const = 0;
		virtual bool isPERTypePronoun(Symbol word) const = 0;
		virtual bool isLOCTypePronoun(Symbol word) const = 0;
		virtual bool isSingularPronoun(Symbol word) const = 0;
		virtual bool isPluralPronoun(Symbol word) const = 0;
		virtual bool isPossessivePronoun(Symbol word) const = 0;
		virtual bool isLinkingPronoun(Symbol word) const = 0;
		virtual bool isNonPersonPronoun(Symbol word) const = 0;
		virtual bool isRelativePronoun(Symbol word) const = 0;
		virtual bool isLocativePreposition(Symbol word) const = 0;
		virtual bool isForReasonPreposition(Symbol word) const = 0;
		virtual bool isOfActionPreposition(Symbol word) const =0;
		virtual bool isUnknownRelationReportedPreposition(Symbol word) const = 0;
		virtual bool isPunctuation(Symbol word) const = 0;
		virtual bool isOpenDoubleBracket(Symbol word) const = 0;
		virtual bool isClosedDoubleBracket(Symbol word) const = 0;
		virtual bool isPartitiveWord(Symbol word) const = 0;
		virtual bool isDeterminer(Symbol word) const = 0;
		virtual bool isDefiniteArticle(Symbol word) const = 0;
		virtual bool isIndefiniteArticle(Symbol word) const = 0;
		virtual ~WordConstantsInstance() {}
	};
	
	/** Return a pointer to the singleton WordConstantsInstance used
	 * by the static methods for delegating to language-specific
	 * implementations. */
	static boost::shared_ptr<WordConstantsInstance> &getInstance();

	/** Templated implementation of WordConstantsInstance, based on a
	 * given class.  That class must define a static method
	 * corresponding to each method defined by WordConstants. */
	template<typename T>
	class WordConstantsInstanceFor: public WordConstantsInstance {
	public:
		// Define a method corresponding to each method in the
		// WordConstants class, that delgates to T.
		bool isNumeric(Symbol word) const { return T::isNumeric(word); }
		bool isAlphabetic(Symbol word) const { return T::isAlphabetic(word); }
		bool isSingleCharacter(Symbol word) const { return T::isSingleCharacter(word); }
		bool startsWithDash(Symbol word) const { return T::startsWithDash(word); }
		bool isOrdinal(Symbol word) const { return T::isOrdinal(word); }
		bool isURLCharacter(Symbol word) const { return T::isURLCharacter(word); }
		bool isPhoneCharacter(Symbol word) const { return T::isPhoneCharacter(word); }
		bool isASCIINumericCharacter(Symbol word) const { return T::isASCIINumericCharacter(word); }
		Symbol getNumericPortion(Symbol word) const { return T::getNumericPortion(word); }

		bool isDailyTemporalExpression(Symbol word) const {return T::isDailyTemporalExpression(word); }
		bool isTimeExpression(Symbol word) const { return T::isTimeExpression(word); }
		bool isTimeModifier(Symbol word) const { return T::isTimeModifier(word); }
		bool isFourDigitYear(Symbol word) const { return T::isFourDigitYear(word); }
		bool isDashedDuration(Symbol word) const { return T::isDashedDuration(word); }
		bool isDecade(Symbol word) const { return T::isDecade(word); }
		bool isDateExpression(Symbol word) const { return T::isDateExpression(word); }
		bool isLowNumberWord(Symbol word) const { return T::isLowNumberWord(word); }
		bool isNameSuffix(Symbol word) const { return T::isNameSuffix(word); }
		bool isHonorificWord(Symbol word) const { return T::isHonorificWord(word); }
		bool isMilitaryWord(Symbol word) const { return T::isMilitaryWord(word); }
		bool isCopula(Symbol word) const { return T::isCopula(word); }
		bool isTensedCopulaTypeVerb(Symbol word) const { return T::isTensedCopulaTypeVerb(word); }
		bool isNameLinkStopWord(Symbol word) const { return T::isNameLinkStopWord(word); }
		bool isAcronymStopWord(Symbol word) const { return T::isAcronymStopWord(word); }
		bool isPronoun(Symbol word) const { return T::isPronoun(word); }
		bool isReflexivePronoun(Symbol word) const { return T::isReflexivePronoun(word); }
		bool is1pPronoun(Symbol word) const { return T::is1pPronoun(word); }
		bool isSingular1pPronoun(Symbol word) const { return T::isSingular1pPronoun(word); }
		bool is2pPronoun(Symbol word) const { return T::is2pPronoun(word); }
		bool is3pPronoun(Symbol word) const { return T::is3pPronoun(word); }
		bool isOtherPronoun(Symbol word) const { return T::isOtherPronoun(word); }
		bool isWHQPronoun(Symbol word) const { return T::isWHQPronoun(word); }
		bool isPERTypePronoun(Symbol word) const { return T::isPERTypePronoun(word); }
		bool isLOCTypePronoun(Symbol word) const { return T::isLOCTypePronoun(word); }
		bool isSingularPronoun(Symbol word) const { return T::isSingularPronoun(word); }
		bool isPluralPronoun(Symbol word) const { return T::isPluralPronoun(word); }
		bool isPossessivePronoun(Symbol word) const { return T::isPossessivePronoun(word); }
		bool isLinkingPronoun(Symbol word) const { return T::isLinkingPronoun(word); }
		bool isNonPersonPronoun(Symbol word) const { return T::isNonPersonPronoun(word); }
		bool isRelativePronoun(Symbol word) const { return T::isRelativePronoun(word); }
		bool isLocativePreposition(Symbol word) const { return T::isLocativePreposition(word); }
		bool isForReasonPreposition(Symbol word) const { return T::isForReasonPreposition(word); }
		bool isOfActionPreposition(Symbol word) const { return T::isOfActionPreposition(word); }
		bool isUnknownRelationReportedPreposition(Symbol word) const { return T::isUnknownRelationReportedPreposition(word); }
		bool isPunctuation(Symbol word) const { return T::isPunctuation(word); }
		bool isOpenDoubleBracket(Symbol word) const { return T::isOpenDoubleBracket(word); }
		bool isClosedDoubleBracket(Symbol word) const { return T::isClosedDoubleBracket(word); }
		bool isPartitiveWord(Symbol word) const { return T::isPartitiveWord(word); }
		bool isDeterminer(Symbol word) const { return T::isDeterminer(word); }
		bool isDefiniteArticle(Symbol word) const { return T::isDefiniteArticle(word); }
		bool isIndefiniteArticle(Symbol word) const { return T::isIndefiniteArticle(word); }
	};
};

#endif
