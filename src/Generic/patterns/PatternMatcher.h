// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PATTERN_MATCHER_H
#define PATTERN_MATCHER_H

#include "Generic/common/Symbol.h"
#include <map>
#include <set>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/hash_map.h"
#include "Generic/patterns/EntityLabelPattern.h"
#include "Generic/patterns/AbstractSlot.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignmentTable.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Generic/PropTree/DocPropForest.h"
#include <boost/shared_ptr.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/optional.hpp"

// Forward declarations
class DocTheory;
class LocatedString;
class ValueMention;
class Mention;
class Quotation;
class Proposition;
class PatternSet;
class QueryDate;
class SentenceTheory;
class SimpleSlot;
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;
typedef std::map<std::wstring,double> NameSynonyms;
typedef std::map<std::wstring,NameSynonyms> NameDictionary;

/** PatternMatcher is the class that is used to match a PatternSet against
  * a document.  It serves two basic roles:
  *
  *     1. It acts as the driver for the matching process, handling the
  *        different stages of matching (such as assigning entity labels,
  *        matching document-level patterns, etc).
  *
  *     2. It serves as a repository for information about the match as it
  *        is performed.  In particular, the PatternMatcher maintains:
  *
  *          - A pointer to the PatternSet that is being used to match
  *            against the document, and a pointer to the document's
  *            DocTheory.
  *
  *          - (optionally) A pointer to an "activity date", which is used
  *            by ValueMentionPattern, and is used to fake some 
  *            document-level patterns.
  *
  *          - The entity labels that have been assigned by any 
  *            entity-labeling patterns that have been run so far.
  * 
  *          - The results of applying document-level patterns.
  *
  * At some point, we could potentially split this into two classes: 
  * PatternMatcher (the driver) and PatternMatchInfo (the repository).
  */
class PatternMatcher: private boost::noncopyable, public boost::enable_shared_from_this<PatternMatcher>  {
public:
	typedef enum {DO_NOT_COMBINE_SNIPPETS, 
	              COMBINE_SNIPPETS_BY_COVERAGE, 
	              COMBINE_SNIPPETS_BY_ANSWER_FEATURES} SnippetCombinationMethod;

	/** Construct a new PatternMatcher that will be used to match
	  * a given PatternSet against a given document.  An activity
	  * date may optionally be specified, which will be used by 
	  * patterns that restrict the date of the document or a value
	  * mention to have some relation to the activity date.
	  *
	  * This constructor automatically runs the entity labeling 
	  * patterns and the document-level patterns. */
	static PatternMatcher_ptr makePatternMatcher(const DocTheory *docTheory, PatternSet_ptr patternSet, 
		const QueryDate *activityDate=0,
		SnippetCombinationMethod snippet_combination_method=DO_NOT_COMBINE_SNIPPETS,
		Symbol::HashMap<const AbstractSlot*> slots=Symbol::HashMap<const AbstractSlot*>(), 
		const NameDictionary *nameDictionary=0,
		bool reset_mention_to_entity_cache = true);

	static PatternMatcher_ptr makePatternMatcher(const AlignedDocSet_ptr docSet, PatternSet_ptr patternSet, 
		const QueryDate *activityDate=0,
		SnippetCombinationMethod snippet_combination_method=DO_NOT_COMBINE_SNIPPETS,
		Symbol::HashMap<const AbstractSlot*> slots=Symbol::HashMap<const AbstractSlot*>(), 
		const NameDictionary *nameDictionary=0,
		bool reset_mention_to_entity_cache = true);

	/** Convenience method to construct a PatternMatcher which simply matches
		a single pattern, which happens frequently in e.g. LearnIt **/
	static PatternMatcher_ptr makePatternMatcher(const DocTheory* docTheory, Pattern_ptr pattern, bool reset_mention_to_entity_cache = true);
	static PatternMatcher_ptr makePatternMatcher(const AlignedDocSet_ptr docSet, Pattern_ptr pattern, bool reset_mention_to_entity_cache = true);

	virtual ~PatternMatcher() {}

	/** Return the document that this PatternMatcher is matching
	  * its PatternSet against. */
	const DocTheory* getDocTheory() const { return _docSet->getDocTheory(_activeLanguageVariant); }

	/** Return the activity date, or NULL if no activity date is 
	  * being used. */
	const QueryDate* getActivityDate() { return _activityDate; }

	/** Return the PatternSet that is being used to match against this
	  * PatternMatcher's document. */
	const PatternSet_ptr getPatternSet() const { return _patternSet; }

	/** Return the OriginalText string owned by the Document that this
	  * PatternMatcher is matching against. */
	const LocatedString *getString() const;

	const SnippetCombinationMethod getSnippetCombinationMethod() const 
	{ return _snippet_combination_method; }

	// Helper method: return getDocTheory()->getDocument()->getName().to_string()
	std::wstring getDocID() const;

	// Helper method: return getPatternSet()->isBlockedPropositionStatus(prop_status)
	bool isBlockedPropositionStatus(PropositionStatusAttribute prop_status) const;

	/** ... */
	void disableSlotNameFilter(const AbstractSlot* slot) {
		_slotsWithDisabledNameFilter.insert(slot);
	}

	/*********************************************************************
	 * Running the Pattern Matcher
	 *********************************************************************/

	/** Match any of the top-level patterns that match at the document level
	  * against the given document, and return a feature set for each 
	  * successful match found.  Currently, the only pattern that matches at
	  * the document level is QuotationPattern. 
	  */
	std::vector<PatternFeatureSet_ptr> getDocumentSnippets();

	/** Match any of the top-level patterns that match at the sentence level
	  * against the given sentence, and return a feature set for each 
	  * successful match found.  (Most patterns match at the sentence level). 
	  *
	  * NOTE: the Brandy implementation had special handling of PropPatterns
	  * here, which did some kind of duplicate result elimination.  This 
	  * special behavior is not currently preserved in the ported implementation.
	  */
	std::vector<PatternFeatureSet_ptr> getSentenceSnippets(SentenceTheory* sTheory, UTF8OutputStream *out = 0, bool force_multimatches = false);

	/*********************************************************************
	 * Pattern Match Information
	 *********************************************************************/
	
	/** Return a PatternFeatureSet that was generated when this entity was labeled. 
	  * If the entity was not labeled, it will be PatternFeatureSet_ptr(). 
	  */
	PatternFeatureSet_ptr getEntityLabelMatch(const Symbol& label, int entity_id) const;

	/** Manually add an entity label. This is used for "automatic" entity
	  * labels such as AGENT1, or for things like FactFinder. 
	  * The confidence is NOT currently used.
	  */
	void setEntityLabelConfidence(const Symbol& label, int entity_id, float confidence);

	/** Manually clear entity labeling so that old labeling won't affect a new match. */
	void clearEntityLabels();
	void printEntityLabels();
	void printSlotScores();

	/** Relabel entities without clearing.
	  * Necessary if you have externally set some entity labels and then want to use 
	  * them in your constructed entity labels */
	void relabelEntitiesWithoutClearing() { labelEntities(); }

	/** Label pattern-driven entities only (as opposed to, e.g., AGENT1) */
	void labelPatternDrivenEntities();

	/** Return true if the document-level pattern with the given ID 
	  * succesfully matched against the document. */
	bool matchesDocPattern(const Symbol& docPatternId) const;

	PatternFeatureSet_ptr getFullFeatures(const AbstractSlot* slot, int sentno);
	PatternFeatureSet_ptr getEdgeFeatures(const AbstractSlot* slot, int sentno);
	PatternFeatureSet_ptr getNodeFeatures(const AbstractSlot* slot, int sentno);
	
	float getFullScore(const AbstractSlot* slot, int sentno, int filter_size=1);
	float getEdgeScore(const AbstractSlot* slot, int sentno, int filter_size=1);
	float getNodeScore(const AbstractSlot* slot, int sentno, int filter_size=1);
	float getFullScore(const AbstractSlot* slot, int sentno, const Proposition* prop);
	float getEdgeScore(const AbstractSlot* slot, int sentno, const Proposition* prop);
	float getFullScore(const AbstractSlot* slot, int sentno, const Mention* mention);
	float getEdgeScore(const AbstractSlot* slot, int sentno, const Mention* mention);
	float getDecisionTreeScore(const AbstractSlot* slot, int sentNumber);
	const AbstractSlot* getSlot(Symbol name);
	void regenerateProptreeScores(const AbstractSlot* slot);

	boost::optional<boost::gregorian::date> getDocumentDate();
	/*
	float getBestFullScore(const AbstractSlot* slot) {
		return _slotBestMatchScores[slot][AbstractSlot::FULL];
	}
	float getBestEdgeScore(const AbstractSlot* slot) {
		return _slotBestMatchScores[slot][AbstractSlot::EDGE];
	}
	float getBestNodeScore(const AbstractSlot* slot) {
		return _slotBestMatchScores[slot][AbstractSlot::NODE];
	}
	*/

	/** All Information Stored about a single doc theory, we index this by language-variant */
	struct PatternMatchDocumentInfo {
		/** A 2-level map recording whether a given label should be applied to a 
		  * given entity.  This is initialized by the private helper method labelEntities(). 
		  */
		EntityLabelPattern::EntityLabeling entityLabelFeatureMap;

		/** A set containing the pattern IDs of all document-level patterns that
		  * matched somewhere in the document.  This is initialized by the private
		  * helper method runDocumentPatterns()*/
		std::set<Symbol> matchedDocPatterns;
	};
	typedef boost::shared_ptr<PatternMatchDocumentInfo> PatternMatchDocumentInfo_ptr; 

	/** Class for switching languages during matching, 
	  * automatically switches back after goes out of scope */
	class LanguageSwitcher {
	private:
		LanguageVariant_ptr _priorVariant;
		PatternMatcher_ptr _patternMatcher;
	public:
		LanguageSwitcher(PatternMatcher_ptr pm, LanguageVariant_ptr var)
			: _priorVariant(pm->getActiveLanguageVariant()), _patternMatcher(pm)
		{
			//SessionLogger::info("Matcher") << L"Switching to " << var->toString() << L"\n";
			pm->setActiveLanguageVariant(var);
		}
		~LanguageSwitcher() { _patternMatcher->setActiveLanguageVariant(_priorVariant); }
	};

	/** Getters and Setters for language variant */
	LanguageVariant_ptr getActiveLanguageVariant() {return _activeLanguageVariant; }
	void setActiveLanguageVariant(LanguageVariant_ptr languageVariant) { _activeLanguageVariant = languageVariant; }

	std::vector<LanguageVariant_ptr> getAlignedLanguageVariants(const LanguageVariant_ptr& restriction) const;

	AlignedDocSet_ptr getDocumentSet() {return _docSet; }
protected:
	/*********************************************************************
	 * Private Member Variables
	 *********************************************************************/
	typedef serif::hash_map<LanguageVariant_ptr,PatternMatchDocumentInfo_ptr,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> PatternMatchDocInfoMap;
	PatternMatchDocInfoMap _docInfos; 

	/** The aligned documents for which this PatternMatcher records
	  * auxiliary information. */
	const AlignedDocSet_ptr _docSet;

	/** The "activity date", which is examined by some patterns to check
	  * whether the document and its contents bear an appropriate relation
	  * to this date.  If no activity date is specified, this will be NULL. */
	const QueryDate *_activityDate;

	/** Date of the document, used in ValueMentionPattern */
	boost::optional<boost::gregorian::date> _documentDate;

	/** The collection of patterns which are being used to search the
	  * document. */
	const PatternSet_ptr _patternSet;

	/** Should we combine snippets?  And if so, how? */
	const SnippetCombinationMethod _snippet_combination_method;

	/** Current Language Variant, gets switched by patterns during matching */
	LanguageVariant_ptr _activeLanguageVariant; 

	/*********************************************************************
	 * Private Helper Methods
	 *********************************************************************/

	PatternMatchDocumentInfo_ptr getDocumentInfo() const { return (*_docInfos.find(_activeLanguageVariant)).second; }

	/** Run the entity-label patterns in the PatternSet, and label the 
	  * document's entities based on the matching patterns.  The entity
	  * labels are recorded in _entityLabelFeatureMap. */
	virtual void labelEntities();
	void labelEntities(const Symbol& label, const SimpleSlot* slot, float min_topicality, bool match_names_loosely, bool ultimate_backoff);

	/** Run the document-level patterns on the document, and record which
	  * of the patterns match in _matchedDocPatterns. */
	void runDocumentPatterns(UTF8OutputStream *debug=0);

	/** Run document-level "pseudo-patterns".  These are document-level 
	  * patterns that are always run, whether declared or not.  Currently,
	  * the are: IN_AD_RANGE, OUT_OF_AD_RANGE, AD_UNDEFINED, and 
	  * AD_NOT_IN_CORPUS.  This helper method is called by 
	  * runDocumentPatterns(). */
	void runDocumentPseudoPatterns();

	// Helpers for runDocumentPseudoPatterns.
	bool docDateWithinActivityRange(int extra_months=0) const;

	void combineSnippets(std::vector<PatternFeatureSet_ptr> &featureSets, bool keep_all_features=false);

	/*********************************************************************
	 * proptree stuff
	 *********************************************************************/

	Symbol::HashMap<const AbstractSlot*> _slots;

	// Cached PropTree match results, comparing each slot to each sentence
	// _slotSentenceNodeScores[slot][match_type][sentno] = score.
	typedef std::vector<float> SentenceMatchScores;
	typedef std::vector<PatternFeatureSet_ptr> SentenceMatchFeatures;
	typedef std::map<const AbstractSlot*, std::map<AbstractSlot::MatchType, SentenceMatchScores> > SlotSentenceMatchScores;
	typedef std::map<const AbstractSlot*, std::map<AbstractSlot::MatchType, SentenceMatchFeatures> > SlotSentenceMatchFeatures;
	SlotSentenceMatchScores _slotSentenceMatchScores;
	SlotSentenceMatchFeatures _slotSentenceMatchFeatures;

	// Cached maximums of the slot-vs-sentence PropTree match results:
	typedef std::map<const AbstractSlot*, std::map<AbstractSlot::MatchType, float> > SlotBestMatchScores;
	SlotBestMatchScores _slotBestMatchScores;

	// Helper methods to get match scores.
	float getFilteredScore(const std::vector<float>& array, int index, int filter_size);
	float getMatchScore(const AbstractSlot* slot, int sentno, const Proposition* prop, AbstractSlot::MatchType matchType);
	float getMatchScore(const AbstractSlot* slot, int sentno, const Mention *mention, AbstractSlot::MatchType matchType);


	// Helpers to initialize PropTree results.
	virtual void fillSlotScores(const AbstractSlot* slot);
	void fillSentencesNearSlotNames(DocPropForest::ptr docPropForest, const AbstractSlot* slot, int context_size=5);

	const NameDictionary *_nameDictionary;

	// This replaces the "slot-name-aware" stuff:
	std::set<const AbstractSlot*> _slotsWithDisabledNameFilter;
	std::map<const AbstractSlot*, std::set<int> > _sentencesNearSlotNames;

	// Because of an issue with make_shared_from_this(), we have to use a factory method
	PatternMatcher(const DocTheory *docTheory, PatternSet_ptr patternSet, const QueryDate *activityDate,
		SnippetCombinationMethod snippet_combination_method, Symbol::HashMap<const AbstractSlot*> slots, 
		const NameDictionary *nameDictionary); // Call makePatternMatcher() instead of using this.
	PatternMatcher(const AlignedDocSet_ptr docSet, PatternSet_ptr patternSet, const QueryDate *activityDate,
		SnippetCombinationMethod snippet_combination_method, Symbol::HashMap<const AbstractSlot*> slots, 
		const NameDictionary *nameDictionary); // Call makePatternMatcher() instead of using this.
	// Required in order to call make_shared<PatternMatcher>(arg0, arg1, ..., arg5)
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(PatternMatcher, const DocTheory *, PatternSet_ptr, const QueryDate *, 
		SnippetCombinationMethod, Symbol::HashMap<const AbstractSlot*>, const NameDictionary *);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(PatternMatcher, const AlignedDocSet_ptr, PatternSet_ptr, const QueryDate *, 
		SnippetCombinationMethod, Symbol::HashMap<const AbstractSlot*>, const NameDictionary *);
	void initializePatternMatcher(bool reset_mention_to_entity_cache=false);

};

typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

#endif
