#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/Offset.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/bsp_declare.h"
#include "boost/tuple/tuple.hpp"
#include "boost/date_time/gregorian/gregorian_types.hpp"
#include <set>
#include <vector>
#include <map>

class NameEquivalenceTable;
class ValueMention;
class DocTheory;
class Value;
BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ElfRelationArg);
BSP_DECLARE(EIDocData);
#pragma once


class EIUtils {
public:
	EIUtils() {}
	~EIUtils() {}

	//
	static bool containsKBPStart(PatternFeatureSet_ptr featureSet);
	static bool containsKBPEnd(PatternFeatureSet_ptr featureSet);
	static const ValueMention* getBestDateMention(std::set<const ValueMention*> dateMentions, std::set<const ValueMention*> nonSpecificDateMentions, SentenceTheory* sent_theory, PatternFeatureSet_ptr featureSet);
	static bool isStartingRangeDate(const SentenceTheory* sent_theory, const ValueMention* vm);
	static bool isEndingRangeDate(const SentenceTheory* sent_theory, const ValueMention* vm);
	static std::wstring getKBPSpecString(const DocTheory* docTheory, const ValueMention* vm1,  const ValueMention* vm2);
	static std::wstring getKBPSpecString(const DocTheory* docTheory, const ValueMention* vm);
	static std::wstring getKBPSpecStringHoldsWithin(const DocTheory* docTheory, const ValueMention* vm);
	static boost::gregorian::date_period DatePeriodFromKBPSpecString(const std::wstring kbpSpec);
	static boost::gregorian::date_period DatePeriodFromValue(const std::wstring& val);
	typedef std::pair<boost::gregorian::date_period,
			boost::gregorian::date_period> DatePeriodPair;
	static DatePeriodPair DatePeriodPairFromKBPSpecString(const std::wstring& s);
	static std::wstring DatePeriodPairToKBPSpecString(const DatePeriodPair& dpp);
	static std::wstring DatePeriodToKBPSpecString(const boost::gregorian::date_period& dp);

	static ElfIndividual_ptr dateIndividualFromSpecString(const std::wstring& spec,
			const std::wstring& text, EDTOffset new_start, EDTOffset new_end);

	static bool is_govt_type_symbol(const Symbol& sym);
	static bool is_person_victim_headword(const Symbol & sym);
	static bool is_opined_headword(const Symbol & sym);
	static bool is_location_prep_headword(const Symbol & sym);
	static bool is_continental_headword(const Symbol & sym);
	// Helper functions for creating relations and relation args
	static ElfRelation_ptr createBinaryRelation(EIDocData_ptr docData, const std::wstring & relation_name, 
		EDTOffset start_offset, EDTOffset end_offset, 
		const std::wstring & role1, const std::wstring & type1, const Mention *arg1, const std::wstring & role2, 
		const std::wstring & type2, const Mention *arg2, double confidence);
	static ElfRelation_ptr createBinaryRelationWithSentences(EIDocData_ptr docData,
		const std::wstring& relation_name, const std::wstring& role1, ElfIndividual_ptr indiv1,
		const std::wstring& role2, ElfIndividual_ptr indiv2, double confidence);
	static std::vector<ElfRelationArg_ptr> createRelationArgFromMention(EIDocData_ptr docData, const std::wstring & role, 
		const std::wstring & type, const Mention *arg);
	static std::vector<ElfRelationArg_ptr> createRelationArgFromTokenSpan(EIDocData_ptr docData, const std::wstring & role, 
		const std::wstring & type, int sent_no, int start, int end);

	static void addPersonOrgMentions(EIDocData_ptr docData, const ElfRelation_ptr elf_relation, const Mention *orgMention,
										std::set<ElfRelation_ptr> & relationsToAdd);
	static const Mention *findMentionForRelationArg(EIDocData_ptr docData, ElfRelationArg_ptr rel_arg);
	static ElfIndividual_ptr findElfIndividualForEntity(EIDocData_ptr docData, int entity_id, const std::wstring & ic_type);
	static const Mention *findMentionByNameStrings(EIDocData_ptr docData, EntityType et, std::set<std::wstring>& validNames);
	static const Entity *findEntityForRelationArg(EIDocData_ptr docData, const ElfRelationArg_ptr& arg);

	static ElfRelationMap findRelationsForEntity(EIDocData_ptr docData, int entity_id);
	static ElfRelation_ptr copyRelationAndSubstituteArgs(EIDocData_ptr docData, const ElfRelation_ptr old_relation, 
		const std::wstring & source, ElfRelationArg_ptr arg_to_replace, const Mention* ment, 
		const std::wstring & type, EDTOffset start, EDTOffset end);
	static ElfRelation_ptr copyRelationAndSubstituteIndividuals(EIDocData_ptr docData, ElfRelation_ptr old_relation,  
		const std::wstring & source, ElfIndividual_ptr individual_to_replace, const Mention* ment, 
		const std::wstring & type, EDTOffset start, EDTOffset end);

	static std::vector<ElfRelationArg_ptr> getArgsWithEntity(ElfRelation_ptr relation, const Entity* entity);
	static std::vector<std::vector<ElfRelation_ptr> > groupEquivalentRelations(const std::vector<ElfRelation_ptr> & relations);
	static std::pair<EDTOffset, EDTOffset> getEDTOffsetsForMention(EIDocData_ptr docData, 
		const Mention* mention, bool head_offsets = false);
	static std::pair<EDTOffset, EDTOffset> getSentenceEDTOffsetsForMention(EIDocData_ptr docData, const Mention* mention);

	static bool isBombing(const ElfRelation_ptr relation);
	static bool isAttack(const ElfRelation_ptr relation);
	static bool isKilling(const ElfRelation_ptr relation);
	static bool isInjury(const ElfRelation_ptr relation);
	static bool isGenericViolence(const ElfRelation_ptr relation);
	static std::wstring getTBDEventType(const ElfRelation_ptr relation);
	static void initCountryNameEquivalenceTable();
	static void destroyCountryNameEquivalenceTable();
	static std::set<std::wstring> getEquivalentNames(std::wstring originalName, double min_score = 0.0);
	// Reads same file read by ElfMultiDoc::load_mr_xdoc_output_file(), but looks at the lines
	// (i.e., the ones containing the strings "NONE" or "MILITARY") that are not read by that method.
	static void load_xdoc_failures(const std::string& failed_xdoc_path);
	static bool extractNationalityPrefix(const std::wstring& s, std::wstring& nat,	std::wstring& rest);
	static bool looksLikeMilitary(const std::wstring& boundString, std::wstring& nat);
	static void xdocWarnings();
	static std::wstring normalizeCountryName(const std::wstring& s);

	typedef std::pair<std::wstring, std::wstring> MilPair;
	typedef boost::tuple<std::wstring, std::wstring, std::wstring> NatTuple;

	static const std::vector< MilPair > & getMilitarySpecialCases() {
		return _militarySpecialCases;}
	static void appendToMilitarySpecialCases(const std::wstring & nationality, const std::wstring & boundID) {
		_militarySpecialCases.push_back(make_pair(nationality, boundID));}
	static const std::vector< NatTuple > & getNationalityPrefixedXDocFailures() {
		return _nationalityPrefixedXDocFailures;}
	static bool hasAttendedSchoolRelation(EIDocData_ptr docData, ElfRelationArg_ptr per, ElfRelationArg_ptr org);
	static bool hasCitizenship(EIDocData_ptr docData, const ElfIndividual_ptr& indiv, const std::wstring& country);
	static bool isContinentConflict(EIDocData_ptr docData, ElfRelationArg_ptr super, ElfRelationArg_ptr sub);
	static const Mention* getContinent(EIDocData_ptr docData, const Entity* e1);
	static std::set<std::wstring> getCountrySynonymsForIndividual(EIDocData_ptr docData, 
		const ElfIndividual_ptr& indiv);
	static bool isInContinent(EIDocData_ptr docData, const std::wstring & continent, const Entity* entity);
	static std::set<int> getSuperLocations(EIDocData_ptr docData, int entity_id);
	static std::set<int> getSubLocations(EIDocData_ptr docData, int entity_id);
	static void initLocationTables();
	static void readSTOPSeparatedSubLocationFile(const std::string & file_name, 
		std::set<std::wstring>& strings_in_table, std::map<std::wstring, std::vector<std::wstring> >& expansion_table);
	static void createPlaceNameIndex(EIDocData_ptr docData);
	static const std::map<std::wstring, std::vector<std::wstring> > & getLocationExpansions() {
		return _locationExpansions;}
	static const std::set<std::wstring> & getLocationStringsInTable() {return _locationStringsInTable;}
	static const std::map<std::wstring, std::vector<std::wstring> > & getContinentExpansions() {
		return _continentExpansions;}
	static const std::set<std::wstring> & getContinentStringsInTable() {return _continentStringsInTable;}
	static const std::map<std::wstring, std::wstring> & getIndividualTypesByName() {
		return _individualTypesByName;}
	// Rule-based individual type addition
	static void addIndividualTypes(EIDocData_ptr docData);
	static void initIndividualTypeNameList();

	static bool isTerroristOrganizationName(const std::wstring& name);
	static void loadTerroristOrganizations(std::set<std::wstring>& terroristOrg);
	static void normalizeForTerroristOrgs(std::wstring& name);

	static const std::set<std::wstring> & getDemocraticWords() {return _democraticWords;}
	static const std::set<std::wstring> & getGovernmentWords() {return _governmentWords;}
	static const void insertDemocraticWord(const std::wstring & word) {_democraticWords.insert(word);}
	static const void insertGovernmentWord(const std::wstring & word) {_governmentWords.insert(word);}
	static void getDates(const DocTheory* docTheory, int sent_no, const Proposition *prop, 
		std::set<const ValueMention*>& dateMentions, bool ignoreOrphanCheck = false, std::set<const Proposition*> seenProps = std::set<const Proposition*>());
	static void getDates(const DocTheory* docTheory, int sent_no, const Mention *mention,
		bool is_from_temp_arg, std::set<const ValueMention*>& dateMentions);
	static void getDatesWithinTokenSpan(const DocTheory* docTheory, ValueMentionSet *vmSet, 
		int stoken, int etoken, bool is_from_temp_arg, std::set<const ValueMention*>& dateMentions);
	static std::vector<const ValueMention*> getDatesFromSentence(EIDocData_ptr docData, int sent_no);
	static bool isViolentTBDEvent(PatternFeatureSet_ptr featureSet);
	static bool isRecentDate(ElfRelationArg_ptr arg);

	static void spanOfIndividuals(ElfIndividual_ptr a, ElfIndividual_ptr b,
			EDTOffset& start, EDTOffset& end);
	static void sentenceSpanOfIndividuals(EIDocData_ptr docData,
			ElfIndividual_ptr a, ElfIndividual_ptr b,
			EDTOffset& start, EDTOffset& end);
	static std::wstring textOfSpan(EIDocData_ptr docData, EDTOffset start,
			EDTOffset end);
	static void spanOfSentences(EIDocData_ptr docData, int sn1, int sn2,
			EDTOffset& start, EDTOffset& end);
	static std::wstring textOfSentence(EIDocData_ptr docData, int sn);
	static std::wstring textFromIndividualSentences(EIDocData_ptr docData,
		ElfIndividual_ptr a, ElfIndividual_ptr b);

	// Functions to help merge ElfRelationArgs
	static bool isContradictoryDate(ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2);
	static bool isContradictoryLocation(EIDocData_ptr docData, ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2);
	static bool isContradictoryRelation(EIDocData_ptr docData, ElfRelation_ptr rel1, ElfRelation_ptr rel2);
	static std::set<ElfRelationArg_ptr> getPatientArguments(ElfRelation_ptr rel);
	static const Mention *findEmployerForPersonEntity(EIDocData_ptr docData, const Entity *person);
	static ElfRelation_ptr copyReplacingWithBoundIndividual( 
		const ElfRelation_ptr& old_relation, const ElfIndividual_ptr& indiv_to_replace,
		const std::wstring& boundID);
	static bool extractPersonMemberishRelation(const ElfRelation_ptr& relation, 
		std::wstring& relName, ElfIndividual_ptr& org, ElfIndividual_ptr& person); 
	// This method is not currently called.
	//bool isMemberishRelation(const ElfRelation_ptr& relation);
	static bool isMilitaryORG(EIDocData_ptr docData, const ElfIndividual_ptr& org);
	static bool isSubOrgOfCountry(EIDocData_ptr docData, const ElfIndividual_ptr indiv, const std::wstring& countryName);
	static bool isCountryWord(const std::wstring& word);
	// Use this function to add relations to the document; do NOT call add_relations directly.
	// If src_to_add is nonempty, it will be appended to the value of the "source" attribute of each relation.
	static void addNewRelations(EIDocData_ptr docData, const std::set<ElfRelation_ptr> & relations,
		const std::wstring & src_to_add = L"");
	// Function to help add inferred relations based on a document's predominant location
	static const Mention * identifyFocusNationState(EIDocData_ptr docData, 
			bool ic = true);
	// Function to help add arguments to SnippetFeatureSets
	static void getLocations(const DocTheory* docTheory, int sent_no, const Proposition *prop, 
		std::set<const Mention*>& locationMentions);
	static std::vector<ElfRelation_ptr> inferLocationFromSubSuperLocation(EIDocData_ptr docData,
																			const ElfRelation_ptr elf_relation);
	static void init(const std::string& failed_xdoc_path);
	static void initRelationSpecificFilterWords();
	static void initLeadershipFilterWords();
	static void initEmploymentFilterWords();
	static void initNonLocalWords();
	static bool containsWord(const SynNode* s, const std::set<Symbol>& strSet);
	static bool containsWord(const Symbol & sym, const std::set<Symbol>& strSet) {
		return (strSet.find(sym) != strSet.end());}
	static bool isDeputyWord(const SynNode* s) {return containsWord(s, _deputyWords);}
	static bool isDeputyWord(const Symbol & s) {return containsWord(s, _deputyWords);}
	static bool isPartyEmployeeWord(const SynNode* s) {return containsWord(s, _partyEmployees);}
	static bool isPartyEmployeeWord(const Symbol & s) {return containsWord(s, _partyEmployees);}
	static bool isPartyNameWord(const SynNode* s) {return containsWord(s, _partyNames);}
	static bool isPartyNameWord(const Symbol & s) {return containsWord(s, _partyNames);}
	static const std::vector<std::wstring> & getNonLocalWords() {return _nonLocalWords;}
	static bool isAllowedDoubleEntityRelation(ElfRelation_ptr relation, 
		ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2);
	// The public method init() calls this.
	static void initAllowedDoubleEntityRelations();
	static bool passesGPELeadershipFilter(EIDocData_ptr docData, const Mention* leaderMention, 
		const Mention* ledMention, const ElfIndividual_ptr& ledIndividual);
	static bool isPossessorOfORGOrGPE(EIDocData_ptr docData, const Mention* m);
	static bool isPoliticalParty(EIDocData_ptr docData, const Entity* e);
	static bool isPoliticalParty(EIDocData_ptr docData, const ElfRelationArg_ptr& arg);
	static bool isNatBoundArg(EIDocData_ptr docData, const ElfRelationArg_ptr& arg, 
								 std::set<std::wstring>& natBoundIDs);
	static bool strContainsSubstrFromVector(const std::wstring & str, const std::vector<std::wstring> & substrs);
	static void cleanupDocument(EIDocData_ptr docData);
	static void sortIndividualMapByOffsets(const ElfIndividualSet & individuals, std::list<ElfIndividual_ptr> & sortedList);
	//Person Groups
	static std::set<ElfRelation_ptr> getRelationsFromPersonCollections(EIDocData_ptr docData);

	// Static methods
	static bool isLikelyDateline(const SentenceTheory *st);	
	static bool isLikelyNewswireLeadDescriptor(const Mention *ment);
	static void attemptNewswireLeadFix(const DocTheory *docTheory, int sent_no, const Mention *potentialDescMention);
	static int getSentenceNumberForArg(const DocTheory* docTheory, ElfRelationArg_ptr arg);
	static int getSentenceNumberForIndividualStart(const DocTheory* docTheory, 
			ElfIndividual_ptr indiv);

	// Functions to help add arguments to SnippetFeatureSets
	static std::vector<ElfRelationArg_ptr> getUnconfidentArguments(EIDocData_ptr docData, 
		const std::vector<ElfRelationArg_ptr> & args, 
		const std::wstring & uriPrefix = L"");
	static void addBoundTypesToIndividuals(EIDocData_ptr docData);
	static void fixNewswireLeads(const DocTheory *docTheory);
	//get a premod that is co-referent with mention
	static const Mention* getImmediateTitle(const DocTheory* docTheory, const Mention* mention);
	//return true if the words in the atomic head (or entity if use_entity = true) match a symbol in sGroup
	static const bool mentionMatchesSymbolGroup(const DocTheory* docTheory, const Mention* mention, Symbol::SymbolGroup sGroup, bool use_entity = false);
	static const bool entityMatchesSymbolGroup(const DocTheory* docTheory, const Entity* entity, Symbol::SymbolGroup sGroup);
	static const bool isFutureDate(const DocTheory* docTheory, const Value* v);
	static const std::set<ElfRelationArg_ptr> alignArgSets(const DocTheory* docTheory, const std::set<ElfRelationArg_ptr> args1, const std::set<ElfRelationArg_ptr> args2);
	static std::vector<ElfRelation_ptr> makeBestCoveringRelation(const DocTheory* docTheory, const ElfRelation_ptr r1, const ElfRelation_ptr r2);
	static bool titleFromMention(EIDocData_ptr docData,
			const Mention* m, int & start_token, int & end_token);
	static bool isBadTitleWord(const std::wstring& word);


	static Symbol PAST_REF;
	static Symbol FUTURE_REF;	
	static Symbol PRESENT_REF;

	static bool argSentenceContainsWord(ElfRelationArg_ptr arg,
		EIDocData_ptr docData, const Symbol& word);
protected:

	static NameEquivalenceTable * _countryNameEquivalenceTable;
	static std::vector<MilPair > _militarySpecialCases;
	static std::vector<NatTuple> _nationalityPrefixedXDocFailures;
	// Data structures for locations in a document
	static std::map<std::wstring, std::vector<std::wstring> > _locationExpansions;
	static std::set<std::wstring> _locationStringsInTable;
	static std::map<std::wstring, std::vector<std::wstring> > _continentExpansions;
	static std::set<std::wstring> _continentStringsInTable;
	static std::map<std::wstring, std::wstring> _individualTypesByName;
	static std::set<std::wstring> _democraticWords;
	static std::set<std::wstring> _governmentWords;
	static std::vector<std::wstring> _nonLocalWords;
	static std::set<Symbol> _deputyWords;
	static std::set<Symbol> _partyEmployees;
	static std::set<Symbol> _partyNames;
	typedef boost::tuple<std::wstring, std::wstring, std::wstring> RelationRoleRole;
	typedef std::set<RelationRoleRole> RelationRoleRoleSet;

	static RelationRoleRoleSet _allowedDoubleEntityRelations;

};
