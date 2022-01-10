// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>

/** A collection of static helper methods that are used for parser
 * training.  These methods delgate to a private language-specific
 * singleton, which in turn delegates to static methods in a
 * language-specific ParserTrainerLanguageSpecificFunctions class,
 * such as EnglishParserTrainerLanguageSpecificFunctions or
 * ArabicParserTrainerLanguageSpecificFunctions.
 *
 * These language-specific classes (such as
 * EnglishParserTrainerLanguageSpecificFunctions and
 * ArabicParserTrainerLanguageSpecificFunctions) should *not* be
 * subclasses of ParserTrainerLanguageSpecificFunctions; in
 * particular, if a language-specific
 * ParserTrainerLanguageSpecificFunctions class was subclassed from
 * ParserTrainerLanguageSpecificFunctions, and neglected to implement
 * some static method, then any call to that method would result in an
 * infinite delegation loop.
 *
 * The 'ParserTrainerLanguageSpecificFunctions::setImplementation()'
 * method can be used to set the language-specific
 * ParserTrainerLanguageSpecificFunctions class that is used to
 * implement the static methods.
 */
class ParserTrainerLanguageSpecificFunctions {
	public:
	/** Set the language-specific
	 * ParserTrainerLanguageSpecificFunctions instance that should be
	 * used to implement the static methods in this class.  E.g.:
	 * ParserTrainerLanguageSpecificFunctions::setImplementation<EnglishParserTrainerLanguageSpecificFunctions>(); */
    template<typename LanguageParserTrainerLanguageSpecificFunctions>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<ParserTrainerLanguageSpecificFunctionsInstance>
            (_new(ParserTrainerLanguageSpecificFunctionsInstanceFor<LanguageParserTrainerLanguageSpecificFunctions>));
	}

	/**
	* Given the label of a HeadlessParseNode, the label of its child, if the child is 
	* PreTerminal, return what the new label should be.  Used by the ParserTrainer
	*
	*/
	static Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal) { 
		return getInstance()->adjustNPLabelsForTraining(label, childLabel, childIsPreTerminal); }
	/**
	* Given the label of a HeadlessParseNode  return NPP for name lables,
	* otherwise return the label.  Used by the ParserTrainer
	*
	*/
	static Symbol adjustNameLabelForTraining(Symbol label) { 
		return getInstance()->adjustNameLabelForTraining(label); }
	/**
	* Given the label of a HeadlessParseNode  return NPP-NNP for NPP and NPP-NPPS for NPPS,
	* otherwise return the label.  Used by the ParserTrainer
	*
	*/
	static Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label) { 
		return getInstance()->adjustNameChildrenLabelForTraining(parent_label, child_label); }
	static void initialize() { 
		getInstance()->initialize(); }
	static const std::vector<Symbol> &openClassTags() {
		return getInstance()->openClassTags(); }

private:
	ParserTrainerLanguageSpecificFunctions(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific ParserTrainerLanguageSpecificFunctions
     * methods.  The templated class
     * ParserTrainerLanguageSpecificFunctionsInstanceFor<T>, defined
     * below, is used to generate language-specific implementations of
     * this class. */
    struct ParserTrainerLanguageSpecificFunctionsInstance: private boost::noncopyable {
        // Define a single abstract method corresponding to each
        // overridable method in the ParserTrainerLanguageSpecificFunctions class.
		virtual Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal) const = 0;
		virtual Symbol adjustNameLabelForTraining(Symbol label) const = 0;
		virtual Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label) const = 0;
		virtual void initialize() const = 0;
		virtual const std::vector<Symbol> &openClassTags() const = 0;
	};

    /** Return a pointer to the singleton
     * ParserTrainerLanguageSpecificFunctionsInstance used by the
     * static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<ParserTrainerLanguageSpecificFunctionsInstance> &getInstance();

    /** Templated implementation of
     * ParserTrainerLanguageSpecificFunctionsInstance, based on a
     * given class.  That class must define a method corresponding to
     * each overridable static method defined by
     * ParserTrainerLanguageSpecificFunctions. */
    template<typename T>
	struct ParserTrainerLanguageSpecificFunctionsInstanceFor: public ParserTrainerLanguageSpecificFunctionsInstance {
        // Define a method corresponding to each method in the
        // ParserTrainerLanguageSpecificFunctions class, that delgates to T.
		Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal) const { 
			return T::adjustNPLabelsForTraining(label, childLabel, childIsPreTerminal); }
		Symbol adjustNameLabelForTraining(Symbol label) const { 
			return T::adjustNameLabelForTraining(label); }
		Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label) const { 
			return T::adjustNameChildrenLabelForTraining(parent_label, child_label); }
		void initialize() const { 
			T::initialize(); }
		const std::vector<Symbol> &openClassTags() const{
			return T::openClassTags(); }
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/parse/ParserTrainer/en_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/parse/ParserTrainer/ch_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/parse/ParserTrainer/ar_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(HINDI_LANGUAGE)
// 	#include "English/parse/ParserTrainer/en_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(BENGALI_LANGUAGE)
// 	#include "English/parse/ParserTrainer/en_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(THAI_LANGUAGE)
// 	#include "English/parse/ParserTrainer/en_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined (KOREAN_LANGUAGE)
// 	#include "Generic/parse/ParserTrainer/xx_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined (URDU_LANGUAGE)
// 	#include "Generic/parse/ParserTrainer/xx_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(UNSPEC_LANGUAGE)
// 	#include "Generic/parse/ParserTrainer/xx_ParserTrainerLanguageSpecificFunctions.h"
// #elif defined(FARSI_LANGUAGE)
// 	#include "Generic/parse/ParserTrainer/xx_ParserTrainerLanguageSpecificFunctions.h"
// #endif

#endif

