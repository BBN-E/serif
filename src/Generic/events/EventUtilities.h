// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_UTILITIES_H
#define EVENT_UTILITIES_H

#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
class Proposition;
class PropositionSet;
class EventMention;
class MentionSet;
class SurfaceLevelSentence;
class EntitySet;
class TokenSequence;
class ValueMentionSet;


/** A collection of static helper methods that are used for event
 * detection.  These methods delgate to a private language-specific
 * singleton, which in turn delegates to static methods in a
 * language-specific EventUtilities class, such as
 * EnglishEventUtilities or ArabicEventUtilities.
 *
 * These language-specific classes (such as EnglishEventUtilities
 * and ArabicEventUtilities) should *not* be subclasses of
 * EventUtilities; in particular, if a language-specific
 * EventUtilities class was subclassed from EventUtilities, and
 * neglected to implement some static method, then any call to that
 * method would result in an infinite delegation loop.
 *
 * The 'EventUtilities::setImplementation()' method can be used to
 * set the language-specific EventUtilities class that is used to
 * implement the static methods.
 */
class EventUtilities {
public:
	/** Set the language-specific EventUtilities instance
	 * that should be used to implement the static methods in this
	 * class.  E.g.:
	 * EventUtilities::setImplementation<EnglishEventUtilities>(); */
    template<typename LanguageEventUtilities>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<EventUtilitiesInstance>
            (_new(EventUtilitiesInstanceFor<LanguageEventUtilities>));
	}

	/* ================================================================ */
	/* These functions can be overridden w/ language-specific values: */
	/* ================================================================ */
	static Symbol getStemmedNounPredicate(const Proposition *prop) {
		return getInstance()->getStemmedNounPredicate(prop); }
	static Symbol getStemmedVerbPredicate(const Proposition *prop) {
		return getInstance()->getStemmedVerbPredicate(prop); }

	static int compareEventMentions(EventMention *mention1, EventMention *mention2) {
		return getInstance()->compareEventMentions(mention1, mention2); }
	static void postProcessEventMention(EventMention *mention,
										MentionSet *mentionSet) {
		getInstance()->postProcessEventMention(mention, mentionSet); }
	static bool isInvalidEvent(EventMention *mention) {
		return getInstance()->isInvalidEvent(mention); }
	static void fixEventType(EventMention *mention, Symbol correctType) {
		getInstance()->fixEventType(mention, correctType); }
	static void populateForcedEvent(EventMention *mention, const Proposition *prop, 
									Symbol correctType, const MentionSet *mentionSet) {
		getInstance()->populateForcedEvent(mention, prop, correctType, mentionSet); }
	static void addNearbyEntities(EventMention *mention,
								  SurfaceLevelSentence *sentence,
								  EntitySet *entitySet) {
		getInstance()->addNearbyEntities(mention, sentence, entitySet); }
	static void identifyNonAssertedProps(const PropositionSet *propSet, 
										 const MentionSet *mentionSet, bool *isNonAsserted) {
		getInstance()->identifyNonAssertedProps(propSet, mentionSet, isNonAsserted); }

	static bool includeInConnectingString(Symbol tag, Symbol next_tag) {
		return getInstance()->includeInConnectingString(tag, next_tag); }
	static bool includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag) {
		return getInstance()->includeInAbbreviatedConnectingString(tag, next_tag); }

	static void runLastDitchDateFinder(EventMention *evMention,
											const TokenSequence *tokens, 
											ValueMentionSet *valueMentionSet,
									   PropositionSet *props) {
		getInstance()->runLastDitchDateFinder(evMention, tokens, valueMentionSet, props); }

	static Symbol getReduced2005EventType(Symbol sym) {
		return getInstance()->getReduced2005EventType(sym); }
	static bool isMoneyWord(Symbol sym) {
		return getInstance()->isMoneyWord(sym); }

private:
	EventUtilities(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific EventUtilities methods.  The templated
     * class EventUtilitiesInstanceFor<T>, defined below, is used
     * to generate language-specific implementations of this class. */
    struct EventUtilitiesInstance: private boost::noncopyable {
        // Define a single abstract method corresponding to each
        // overridable method in the EventUtilities class.
		virtual Symbol getStemmedNounPredicate(const Proposition *prop) const = 0;
		virtual Symbol getStemmedVerbPredicate(const Proposition *prop) const = 0;
	
		virtual int compareEventMentions(EventMention *mention1, EventMention *mention2) const = 0;
		virtual void postProcessEventMention(EventMention *mention,
											 MentionSet *mentionSet) const = 0;
		virtual bool isInvalidEvent(EventMention *mention) const = 0;
		virtual void fixEventType(EventMention *mention, Symbol correctType) const = 0;
		virtual void populateForcedEvent(EventMention *mention, const Proposition *prop, 
										 Symbol correctType, const MentionSet *mentionSet) const = 0;
		virtual void addNearbyEntities(EventMention *mention,
									   SurfaceLevelSentence *sentence,
									   EntitySet *entitySet) const = 0;
		virtual void identifyNonAssertedProps(const PropositionSet *propSet, 
											  const MentionSet *mentionSet, bool *isNonAsserted) const = 0;
	
		virtual bool includeInConnectingString(Symbol tag, Symbol next_tag) const = 0;
		virtual bool includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag) const = 0;
	
		virtual void runLastDitchDateFinder(EventMention *evMention,
											const TokenSequence *tokens, 
											ValueMentionSet *valueMentionSet,
											PropositionSet *props) const = 0;
	
		virtual Symbol getReduced2005EventType(Symbol sym) const = 0;
		virtual bool isMoneyWord(Symbol sym) const = 0;
		virtual ~EventUtilitiesInstance() {}
	};

    /** Return a pointer to the singleton EventUtilitiesInstance
     * used by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<EventUtilitiesInstance> &getInstance();

    /** Templated implementation of EventUtilitiesInstance, based
     * on a given class.  That class must define a method
     * corresponding to each overridable static method defined by
     * EventUtilities. */
    template<typename T>
	struct EventUtilitiesInstanceFor: public EventUtilitiesInstance {
        // Define a method corresponding to each method in the
        // EventUtilities class, that delgates to T.
		Symbol getStemmedNounPredicate(const Proposition *prop) const {
			return T::getStemmedNounPredicate(prop); }
		Symbol getStemmedVerbPredicate(const Proposition *prop) const {
			return T::getStemmedVerbPredicate(prop); }
	
		int compareEventMentions(EventMention *mention1, EventMention *mention2) const {
			return T::compareEventMentions(mention1, mention2); }
		void postProcessEventMention(EventMention *mention,
									 MentionSet *mentionSet) const {
			T::postProcessEventMention(mention, mentionSet); }
		bool isInvalidEvent(EventMention *mention) const {
			return T::isInvalidEvent(mention); }
		void fixEventType(EventMention *mention, Symbol correctType) const {
			T::fixEventType(mention, correctType); }
		void populateForcedEvent(EventMention *mention, const Proposition *prop, 
								 Symbol correctType, const MentionSet *mentionSet) const {
			T::populateForcedEvent(mention, prop, correctType, mentionSet); }
		void addNearbyEntities(EventMention *mention,
							   SurfaceLevelSentence *sentence,
							   EntitySet *entitySet) const {
			T::addNearbyEntities(mention, sentence, entitySet); }
		void identifyNonAssertedProps(const PropositionSet *propSet, 
									  const MentionSet *mentionSet, bool *isNonAsserted) const {
			T::identifyNonAssertedProps(propSet, mentionSet, isNonAsserted); }
	
		bool includeInConnectingString(Symbol tag, Symbol next_tag) const {
			return T::includeInConnectingString(tag, next_tag); }
		bool includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag) const {
			return T::includeInAbbreviatedConnectingString(tag, next_tag); }
	
		void runLastDitchDateFinder(EventMention *evMention,
									const TokenSequence *tokens, 
									ValueMentionSet *valueMentionSet,
									PropositionSet *props) const {
			T::runLastDitchDateFinder(evMention, tokens, valueMentionSet, props); }
	
		Symbol getReduced2005EventType(Symbol sym) const {
			return T::getReduced2005EventType(sym); }
		bool isMoneyWord(Symbol sym) const {
			return T::isMoneyWord(sym); }
	};
};

// #ifdef ENGLISH_LANGUAGE
// 	#include "English/events/en_EventUtilities.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/events/ch_EventUtilities.h"
// #else
// 	#include "Generic/events/xx_EventUtilities.h"
// #endif



#endif
