// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRELINKER_H
#define PRELINKER_H

class Mention;
class MentionSet;
class PropositionSet;
class EntitySet;
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <map>

/** A collection of static helper methods that are used for pre-
 * linking.  These methods delgate to a private language-specific
 * singleton, which in turn delegates to static methods in a
 * language-specific PreLinker class, such as
 * EnglishPreLinker or ArabicPreLinker.
 *
 * These language-specific classes (such as EnglishPreLinker
 * and ArabicPreLinker) should *not* be subclasses of
 * PreLinker; in particular, if a language-specific
 * PreLinker class was subclassed from PreLinker, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'PreLinker::setImplementation()' method can be used to
 * set the language-specific PreLinker class that is used to
 * implement the static methods.
 */
class PreLinker {
public:
	/** Set the language-specific PreLinker instance
	 * that should be used to implement the static methods in this
	 * class.  E.g.:
	 * PreLinker::setImplementation<EnglishPreLinker>(); */
    template<typename LanguagePreLinker>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<PreLinkerInstance>
            (_new(PreLinkerInstanceFor<LanguagePreLinker>));
	}

	/** A mapping from mention index (as returned by Mention::getIndex()
	 * to Mention pointer. */
	typedef std::map<int, const Mention*> MentionMap;

	/* ================================================================ */
	/* These functions can be overridden w/ language-specific values: */
	/* ================================================================ */
	static void preLinkAppositives(MentionMap& preLinks,
								   const MentionSet *mentionSet) {
		getInstance()->preLinkAppositives(preLinks, mentionSet); }
	static void preLinkCopulas(MentionMap& preLinks,
							   const MentionSet *mentionSet,
							   const PropositionSet *propSet) {
		getInstance()->preLinkCopulas(preLinks, mentionSet, propSet); }
	static void preLinkSpecialCases(MentionMap& preLinks,
									const MentionSet *mentionSet,
									const PropositionSet *propSet) {
		getInstance()->preLinkSpecialCases(preLinks, mentionSet, propSet); }
	static void preLinkTitleAppositives(MentionMap& preLinks,
										const MentionSet *mentionSet) {
		getInstance()->preLinkTitleAppositives(preLinks, mentionSet); }
	// Handles special metonymy like linking for ACE05 (arabic only):
	static const int postLinkSpecialCaseNames(const EntitySet* entitySet, 
											  int* entity_link_pairs, int maxpairs) {
		return getInstance()->postLinkSpecialCaseNames(entitySet, entity_link_pairs, maxpairs); }
	static void setSpecialCaseLinkingSwitch(bool linking_switch) {
		getInstance()->setSpecialCaseLinkingSwitch(linking_switch); }
	static void setEntitySubtypeFilteringSwitch(bool filtering_switch) {
		getInstance()->setEntitySubtypeFilteringSwitch(filtering_switch); }


private:
	PreLinker(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific PreLinker methods.  The templated
     * class PreLinkerInstanceFor<T>, defined below, is used
     * to generate language-specific implementations of this class. */
    struct PreLinkerInstance: private boost::noncopyable {
        // Define a single abstract method corresponding to each
        // overridable method in the PreLinker class.
		virtual void preLinkAppositives(MentionMap& preLinks,
								const MentionSet *mentionSet) const = 0;
		virtual void preLinkCopulas(MentionMap& preLinks,
							const MentionSet *mentionSet,
							const PropositionSet *propSet) const = 0;
		virtual void preLinkSpecialCases(MentionMap& preLinks,
								 const MentionSet *mentionSet,
								 const PropositionSet *propSet) const = 0;
		virtual void preLinkTitleAppositives(MentionMap& preLinks,
									 const MentionSet *mentionSet) const = 0;
		virtual void setSpecialCaseLinkingSwitch(bool linking_switch) const = 0;
		virtual void setEntitySubtypeFilteringSwitch(bool filtering_switch) const = 0;
		virtual const int postLinkSpecialCaseNames(const EntitySet* entitySet, 
										   int* entity_link_pairs, int maxpairs) const = 0;
	};

    /** Return a pointer to the singleton PreLinkerInstance
     * used by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<PreLinkerInstance> &getInstance();

    /** Templated implementation of PreLinkerInstance, based
     * on a given class.  That class must define a method
     * corresponding to each overridable static method defined by
     * PreLinker. */
    template<typename T>
	struct PreLinkerInstanceFor: public PreLinkerInstance {
        // Define a method corresponding to each method in the
        // PreLinker class, that delgates to T.
		void preLinkAppositives(MentionMap& preLinks,
								const MentionSet *mentionSet) const {
			T::preLinkAppositives(preLinks, mentionSet); }
		void preLinkCopulas(MentionMap& preLinks,
							const MentionSet *mentionSet,
							const PropositionSet *propSet) const {
			T::preLinkCopulas(preLinks, mentionSet, propSet); }
		void preLinkSpecialCases(MentionMap& preLinks,
								 const MentionSet *mentionSet,
								 const PropositionSet *propSet) const {
			T::preLinkSpecialCases(preLinks, mentionSet, propSet); }
		void preLinkTitleAppositives(MentionMap& preLinks,
									 const MentionSet *mentionSet) const {
			T::preLinkTitleAppositives(preLinks, mentionSet); }
		void setSpecialCaseLinkingSwitch(bool linking_switch) const {
			T::setSpecialCaseLinkingSwitch(linking_switch); }
		void setEntitySubtypeFilteringSwitch(bool filtering_switch) const {
			T::setEntitySubtypeFilteringSwitch(filtering_switch); }
		// Handles special metonymy like linking for ACE05 (arabic only):
		const int postLinkSpecialCaseNames(const EntitySet* entitySet, 
										   int* entity_link_pairs, int maxpairs) const {
			return T::postLinkSpecialCaseNames(entitySet, entity_link_pairs, maxpairs); }
	};
};

// #if defined(ENGLISH_LANGUAGE)
// 	#include "English/edt/en_PreLinker.h"
// #elif defined(ARABIC_LANGUAGE)
// 	#include "Arabic/edt/ar_PreLinker.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Generic/edt/xx_PreLinker.h"
// #else
// 	#include "Generic/edt/xx_PreLinker.h"
// #endif



#endif
