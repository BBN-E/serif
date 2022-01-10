// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCLINK_FEATURE_FUNCTIONS_H
#define DESCLINK_FEATURE_FUNCTIONS_H

/** This module defines DescLinkFeatureFunctions, which provides
 * various static helper methods that are used to define descriptor
 * linking features.  Examples include getSentenceDistance() and
 * findEntLastPronoun().  A few of these static methods can be given
 * language-specific definitions, by delegating to a singleton
 * instance of a langauge-specific subclass of
 * DescLinkFeatureFunctionsInstance.
 */

#include <stdio.h>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
class Mention;
class MentionSet;
class RelMentionSet;
class Entity;
class EntitySet;
class SynNode;
class EntityType;
class DocumentRelMentionSet;
class SymbolHash;

/** A collection of static helper methods that are used to define
 * descriptor linking features.  A few of these methods delgate to a
 * private language-specific singleton, which in turn delegates to
 * static methods in a language-specific DescLinkFeatureFunctions
 * class, such as EnglishDescLinkFeatureFunctions or
 * ArabicDescLinkFeatureFunctions.
 *
 * These language-specific classes (such as
 * EnglishDescLinkFeatureFunctions and ArabicDescLinkFeatureFunctions)
 * should *not* be subclasses of DescLinkFeatureFunctions; in
 * particular, if a language-specific DescLinkFeatureFunctions class
 * was subclassed from DescLinkFeatureFunctions, and neglected to
 * implement some static method, then any call to that method would
 * result in an infinite delegation loop.
 *
 * The 'DescLinkFeatureFunctions::setImplementation()' method can be
 * used to set the language-specific DescLinkFeatureFunctions class
 * that is used to implement the static methods.
 *
 * Related language-specific classes, such as
 * EnglishDescLinkFeatureFunctions, may define additional static
 * methods.  These should be accessed directly from the class (rather
 * than via the DescLinkFeatureFunctions superclass).
 */
class DescLinkFeatureFunctions {
public:
	/** Set the language-specific DescLinkFeatureFunctions instance
	 * that should be used to implement the static methods in this
	 * class.  E.g.:
	 * DescLinkFeatureFunctions::setImplementation<EnglishDescLinkFeatureFunctions>(); */
    template<typename LanguageDescLinkFeatureFunctions>
    static void setImplementation() {
        getInstance() = boost::shared_ptr<DescLinkFeatureFunctionsInstance>
            (_new(DescLinkFeatureFunctionsInstanceFor<LanguageDescLinkFeatureFunctions>));
	}

	/* ================================================================ */
	/* These functions can be overridden w/ language-specific values: */
	/* ================================================================ */
	static const SynNode* getNumericMod(const Mention *ment) {
		return getInstance()->getNumericMod(ment); }
	static std::vector<const Mention*> getModNames(const Mention* ment) {
		return getInstance()->getModNames(ment); }
	static std::vector<Symbol> getMods(const Mention* ment) {
		return getInstance()->getMods(ment); }
	static bool hasMod(const Mention* ment) {
		return getInstance()->hasMod(ment); }
	static Symbol getLinkSymbol(){
		return getInstance()->getLinkSymbol(); }

	/* ================================================================ */
	/* The remaining functions can *not* be overridden w/ 
	 * language-specific values: */
	/* ================================================================ */
	static int testHeadWordsMatch(MentionUID mentUID, Entity *entity, const MentionSymArrayMap *HWMap);
	static bool testHeadWordMatch(Symbol headWord, Entity *entity, const EntitySet *entitySet);
	static bool testHeadWordMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet);
	static bool testHeadWordNodeMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet);
	static bool testLastCharMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet);
	static Symbol getSentenceDistance(int sent_num, Entity *entity, const EntitySet *entitySet);
	static Symbol getSentenceDistanceWide(int sent_num, Entity *entity, const EntitySet *entitySet);
	static Symbol getMentionDistance4(const Mention *mention, Entity *entity, const EntitySet *entitySet);
	static Symbol getMentionDistance8(const Mention *mention, Entity *entity, const EntitySet *entitySet);
	static bool exactStringMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet);
	static int getHeadNPWords(const SynNode* node, Symbol results[], int max_results);
	static void getMentionInternalNames(const SynNode* node, const MentionSet* ms, std::vector<const Mention*>& results);

	// how many entities of this type are in the set?
	static Symbol getNumEntsByType(EntityType type, const EntitySet *set);
	// how many entities are in the set?
	static Symbol getNumEnts(const EntitySet *set);

	static Symbol findMentHeadWord(const Mention *ment);
	static Symbol findEntLastPronoun(const Entity* ent, const EntitySet* ents);
	static std::set<Symbol> getEntHeadWords(Entity* ent, const EntitySet* ents);

	// head of the parent node (hopefully predicate)
	// for mention and all entity mentions
	static Symbol getParentHead(const Mention* ment);
	static Symbol getCommonParent(const Mention* ment, Entity* ent, EntitySet* ents);

	static bool entityContainsOnlyNames(Entity *entity, const EntitySet *entitySet);
	static bool mentOverlapsEntity(const Mention *mention, Entity *entity, const EntitySet *entSet);
	static bool mentIncludedInEntity(const Mention *mention, Entity *entity, const EntitySet *entSet);

	static Symbol getClusterScore(const Mention *ment, Entity *ent, const EntitySet *entSet); 
	static bool hasClusterMatch(const Mention *ment, Entity *ent, const EntitySet *entSet);
	static std::set<Symbol> findMentClusters(const Mention *ment); 
	static std::set<Symbol> findEntClusters(Entity* ent, const EntitySet* ents);
	static std::vector<Symbol> getClusters(Symbol word);
	
	//this looks at all modifiers ignoring head, does't just look at pre/post head things.
	static Symbol getUniqueModifierRatio(const Mention *ment, Entity *entity, const EntitySet *entitySet);
	static Symbol getUniqueCharRatio(const Mention *ment, Entity *entity, const EntitySet *entitySet);

	static bool mentHasNumeric(const Mention *ment);
	static bool entHasNumeric(const Entity *ent, const EntitySet *entitySet);
	static bool hasNumericClash(const Mention *ment, Entity *ent, const EntitySet *entitySet); 
	static bool hasNumericMatch(const Mention *ment, Entity *ent, const EntitySet *entitySet);
	static bool hasHeadwordClash(const Mention* ment, const Entity* ent, const EntitySet* ents, const DocTheory *docTheory);

	static Symbol checkCommonRelationsTypes(const Mention *ment,
										const Entity *entity,
										const EntitySet* ents,
										DocumentRelMentionSet *docRelMentionSet);
	static bool checkCommonRelationsTypesPosAnd2ndHW(const Mention *mention,
		const Entity *entity,  const EntitySet *eset, DocumentRelMentionSet *docRMSet,
		Symbol &type, Symbol &hw2, Symbol &pos);
	static bool checkEntityHasHW(const Entity *entity, const EntitySet *entSet, const Symbol hw);
	static bool mentOverlapInAnotherMention(const Mention *mention, const MentionSet *mentSet);


	//anything that handles modifiers is language specific 
	//(Arabic- postmodifiers, English Chinese premodifiers)

	static std::set<Symbol> getMentMods(const Mention* ment);
	static std::set<Symbol> getEntMods(Entity* ent, const EntitySet* ents);

	static void loadNationalities();
	static void loadNameGPEAffiliations();
	static bool globalGPEAffiliationClash(const DocTheory *docTheory, EntitySet * currSolution, Entity *entity, Mention *mention);
	static bool localGPEAffiliationClash(const Mention* ment, const Entity* ent, const EntitySet* ents, const DocTheory *docTheory);
	static std::vector<std::wstring> getGPEAffiliations(const Mention *mention, const DocTheory *docTheory); 
	static void getGPEModNames(const Mention* ment, const DocTheory *docTheory, std::set<std::wstring>& results);

	static std::vector<const Mention*> getGPEModNames(const Mention* ment);
	static std::vector<const Mention*> getNationalityModNames(const Mention* ment);

	static bool hasModClash(const Mention* ment, Entity* ent, const EntitySet* ents);
	static bool hasModMatch(const Mention* ment, Entity* ent, const EntitySet* ents);

	static bool hasModName(const Mention* ment);
	static bool hasModNameEntityClash(const Mention* ment, Entity* ent, const EntitySet* ents);
	static void initializeWordClusterTable();
	static void initializeStopWords();
	static int getMentionStartToken(const Mention *m1);
	static int getMentionEndToken(const Mention *m1);
	static Symbol getPatternBetween(const MentionSet* mentset, const Mention* ment1, const Mention* ment2);
	static std::wstring addTextToPatternString(const SynNode* node, const MentionSet* mentset, 
		int& count, std::wstring str, int start_tok, int end_tok);
	static Symbol getPatternSymbol(const MentionSet* mentset, const Mention* ment1, const Mention* ment2);
	static bool subtypeMatch(const EntitySet *currSolution, const Entity *entity, const Mention *mention);

	/* ================================================================ */
	/* Public symbols: */
	/* ================================================================ */
	const static Symbol NONE_SYMBOL;
	const static Symbol LEFT;
	const static Symbol RIGHT;

private:
	DescLinkFeatureFunctions(); // private: this class may not be instantiated.

    /** Abstract base class for the singletons that implement
     * language-specific DescLinkFeatureFunctions methods.  The templated class
     * DescLinkFeatureFunctionsInstanceFor<T>, defined below, is used to generate
     * language-specific implementations of this class. */
    struct DescLinkFeatureFunctionsInstance: private boost::noncopyable {
        // Define a single abstract method corresponding to each
        // overridable method in the DescLinkFeatureFunctions class.
		virtual const SynNode* getNumericMod(const Mention *ment) const = 0;
		virtual std::vector<const Mention*> getModNames(const Mention* ment) const = 0;
		virtual std::vector<Symbol> getMods(const Mention* ment) const = 0;
		virtual bool hasMod(const Mention* ment) const = 0;
		virtual Symbol getLinkSymbol() const = 0;
	};

    /** Return a pointer to the singleton DescLinkFeatureFunctionsInstance used
     * by the static methods for delegating to language-specific
     * implementations. */
    static boost::shared_ptr<DescLinkFeatureFunctionsInstance> &getInstance();

    /** Templated implementation of DescLinkFeatureFunctionsInstance,
     * based on a given class.  That class must define a method
     * corresponding to each overridable static method defined by
     * DescLinkFeatureFunctions. */
    template<typename T>
	struct DescLinkFeatureFunctionsInstanceFor: public DescLinkFeatureFunctionsInstance {
        // Define a method corresponding to each method in the
        // DescLinkFeatureFunctions class, that delgates to T.
		const SynNode* getNumericMod(const Mention *ment) const {
			return T::getNumericMod(ment); }
		std::vector<const Mention*> getModNames(const Mention* ment) const {
			return T::getModNames(ment); }
		std::vector<Symbol> getMods(const Mention* ment) const {
			return T::getMods(ment); }
		bool hasMod(const Mention* ment) const {
			return T::hasMod(ment); }
		Symbol getLinkSymbol() const {
			return T::getLinkSymbol(); }
	};

private:
	/* ================================================================ */
	/* Private symbols: */
	/* ================================================================ */
	static SymbolHash *_stopWords;

	static std::map<std::wstring, std::wstring> _nationalities;
	static std::map<std::wstring, std::vector<std::wstring> > _name_gpe_affiliations;
};


// #if defined(ARABIC_LANGUAGE)
// 	#include "Arabic/edt/ar_DescLinkFeatureFunctions.h"
// #elif defined(ENGLISH_LANGUAGE)
// 	#include "English/edt/en_DescLinkFeatureFunctions.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/edt/ch_DescLinkFeatureFunctions.h"
// #else
// 	#include "Generic/edt/xx_DescLinkFeatureFunctions.h"
// #endif

#endif
