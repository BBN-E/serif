// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef FactFinder_H
#define FactFinder_H

#include <memory>
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/theories/Fact.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PseudoPattern.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/factfinder/EntityLinker.h"

class DistillationDoc;
class FactPatternManager;
class FactGoldStandardStorage;
class ValueMention;
class SectorFactFinder;

class FactFinder {
public:
	FactFinder();
	~FactFinder();

	void findFacts(DocTheory *docTheory);
	void findFacts(std::wstring filename, std::vector<DocTheory *>& documentTheories);
	
private:
	bool _run_fact_finder;
	
	// Static strings/symbols
	static const std::wstring FF_FACT_TYPE;
	static const std::wstring FF_AGENT1;
	static const std::wstring FF_ROLE;
	static const std::wstring FF_DATE;
	static const Symbol FF_START_DATE;
	static const Symbol FF_END_DATE;
	static const Symbol FF_HOLD_DATE;
	static const Symbol FF_NON_HOLD_DATE;
	static const Symbol FF_ACTIVITY_DATE;

	static const Symbol FF_EMPLOYMENT;
	static const Symbol FF_VISITED_PLACE;
	static const Symbol FF_QUOTATION;
	static const Symbol FF_QUOTATION_ABOUT;
	static const Symbol FF_ACTION;
	static bool isActivityDate(Symbol sym);

	const FactPatternManager* _pattern_manager;

	// We handle English and MT in separate runs
	bool _is_english_source;

	// Debug options
	UTF8OutputStream _debugStream;
	bool DEBUG;
	bool _is_verbose;

	// Entity restrictions
	typedef std::map<std::wstring,std::set<int> > DocidEntityTable;
	DocidEntityTable _restricted_entities;
	bool isRestrictedEntity(std::wstring docid, int entity_id);

	// Document-entity pair fact-finding
	std::map<int, std::vector<Fact_ptr> > _documentFacts;
	void processDocument(DocTheory *docTheory, UTF8OutputStream& kbStream, UTF8OutputStream& matchStream, EntityLinker& entityLinker);
	
	bool _find_custom_facts;
	void findCustomFacts(PatternMatcher_ptr pm, int entity_id);
	void addDescriptionFacts(PatternMatcher_ptr pm, int entity_id, SentenceTheory *sTheory);

	void processPatternSet(PatternMatcher_ptr pm, Symbol patSetEntityTypeSym, Symbol extraFlagSymbol);
	void processEntity(PatternMatcher_ptr pm, int entity_id);
	void addFactsFromFeatureSet(PatternMatcher_ptr pm, int entity_id, PatternFeatureSet_ptr featureSet);
	
	// Split facts into actual triples
	std::vector< std::vector<FactArgument_ptr> > recursiveAddOneArgPerRole(std::vector< std::vector<FactArgument_ptr> > existingLists, 
		std::vector<Symbol> roles, std::map<Symbol, std::vector<FactArgument_ptr> >& argMap, int start_role); 

	// Helpers
	bool isLikelyBadFact(std::wstring fact_text);
	bool entityShouldBeProcessed(PatternMatcher_ptr pm, int entity_id);
	static void addToSetFromString(std::set<std::wstring> &set, std::wstring joined_tokens, const std::wstring & possible_tok_delims = L" ");
	static std::wstring getMultiSentenceString(const DocTheory *docTheory, int start_sentence, int end_sentence, int start_token, int end_token);
	std::wstring getFactsFileName(const DocTheory *docTheory);
	int getPassageID(DocTheory *docTheory, Fact_ptr fact);
	bool hasArgumentRole(Fact_ptr fact, Symbol role);
		
	// Employment augmentation helpers
	bool _augment_employment_facts;
	void augmentEmploymentFact(PatternMatcher_ptr pm, Fact_ptr fact, bool found_date_through_pattern);
	void extractTitle(PatternMatcher_ptr pm, const Mention *employee, const Mention *employer,
							  int employee_entity_id, int& start_token, int& end_token, float& score);
	void findEmploymentStatus(PatternMatcher_ptr pm, const Mention *employee, const Mention *employer,
		int title_start, int title_end, int& start, int& end, std::wstring& statusWord);
	bool isEmploymentStatusWord(std::wstring word);
	std::set<std::wstring> _validTitleAdjectives;
	bool canInferHoldDate(const Mention *employer, const Mention *employee);
	std::set<std::wstring> _futureEmploymentStatusWords;
	std::set<std::wstring> _currentEmploymentStatusWords;
	std::set<std::wstring> _pastEmploymentStatusWords;

	std::set<std::wstring> _validGPETitles;
	std::set<std::wstring> _validGPETitleStarts;
	std::set<std::wstring> _validGPETitleEnds;
	std::set<std::wstring> _validTitleModifiers;
	
	// Prune and store facts in docTheory and kbStream
	void pruneAndStoreFacts(DocTheory *docTheory,  UTF8OutputStream& kbStream, UTF8OutputStream& matchStream, int entity_id);
	std::map<Symbol, Symbol> _reciprocalRoleLabelTransformations;

	// ColdStart functions
	bool _print_factfinder_coldstart_info;
	bool _print_factfinder_match_info;
	bool _use_actor_id_for_entity_linker;
	double _min_actor_match_conf;

	// ##
	// split coref with mention-level actor matching
	std::multimap<int,std::multimap<Symbol, MentionUID> > entityId2actorId2mentionId;
	std::map<MentionUID, Symbol> mentionId2actorId;
	void updateMentionActorIdMap(int entityId, Symbol actorId, MentionUID mentionUID);
	double _minActorPatternConf;
	double _minEditDistance;
	double _minPatternMatchScorePlusAssociationScore;
	double _min_actor_entity_match_conf_accept_at_mention_level;
	// ##

	std::map<int, Symbol> _entityLinkerCache;
	void writeAllMentionsForColdStart(const DocTheory *docTheory, UTF8OutputStream& kbStream, EntityLinker& entityLinker);
	void printFactForColdStart(const DocTheory *docTheory, Fact_ptr fact, UTF8OutputStream& kbStream, UTF8OutputStream& matchStream);
	std::wstring printColdStartArgument(const DocTheory *docTheory, FactArgument_ptr arg, bool force_write_mention_text, std::wstring& strTextArg, std::wstring& strTextArgCanonical, Mention::Type& argMentionType);
	void writeNestedNamesFactsForColdStart(const DocTheory *docTheory, UTF8OutputStream& kbStream);
	std::wstring normalizeString(std::wstring str);

	SectorFactFinder *_sectorFactFinder;
};

#endif // #ifndef FactFinder_H
