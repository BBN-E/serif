#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/Offset.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/bsp_declare.h"
#include "boost/tuple/tuple.hpp"

#include <set>
#include <vector>
#include <map>

#include "PredFinder/inference/EIFilterManager.h"
class DocTheory;
BSP_DECLARE(SnippetFeatureSet);
BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ElfRelationArg);
BSP_DECLARE(EIDocData);
BSP_DECLARE(ElfInference);
BSP_DECLARE(PatternSet);
#pragma once


class ElfInference {
public:
	// This constructor must be called before any other ElfInference method is used.
	ElfInference(std::string filter_ordering_file, std::vector<std::string> superfilters, const std::string& failed_xdoc_path = "");
	~ElfInference();

	static EIDocData_ptr prepareDocumentForInference(const DocTheory* docTheory);

	/** 
	 * Main function, called by PredicationFinder::run(). Modifies the ElfDocument in place.
	 * @param docData An EIDocData_ptr returned from a previous call to prepareDocumentForInference() invoked on the same document.
	 * @param elfDocument An ElfDocument_ptr with the ELF content for the same document.
	 **/
	static void do_inference(EIDocData_ptr docData, ElfDocument_ptr elfDocument);
	static void do_partner_inference(EIDocData_ptr docData, ElfDocument_ptr elfDocument);

	/**
	 * Adds a sub-organization relationship for government organizations and their parent GPEs.
	 * Adds arguments to SnippetFeatureSets.
	 * Called by ElfDocument constructor.
	 **/
	static void addInferredLocationForEvent(EIDocData_ptr docData, int sent_no, const PatternFeatureSet_ptr featureSet);
	/**
	 * Get inferred dates, but exclude dates on propositions that explicitly do NOT refer to the event,
	 *   e.g. "he was accused Tuesday of bombing the embassy".
	 * Adds arguments to SnippetFeatureSets.
	 * Called by ElfDocument constructor.
	 **/
	static void addInferredDateForEvent(EIDocData_ptr docData, int sent_no, const PatternFeatureSet_ptr featureSet) ;

	/**
	 * Get dates for KBP-- simpler than adding date specific patterns
	 **/
	static void addInferredDateForKBP(EIDocData_ptr docData, int sent_no, const PatternSet_ptr patternSet, const PatternFeatureSet_ptr featureSet) ;

	// Remove doubled arguments from events.
	// Where the agent and the patient are the same, and there is no explicit sign that this is okay
	// (such as the presence of the word "suicide" or "himself" or "herself"), remove the agent args.
	// If we remove all the agents, we may need to re-set the event type and the relation name.
	// Called by the ElfDocument constructor.
	static void pruneDoubleEntitiesFromEvents(EIDocData_ptr docData, int sent_no, 
		const std::set<ElfRelation_ptr> & sentence_relations);
	
	// Pass ElfRelationArgs around between similar events in the same sentence.
	// Not currently called.
	static void mergeRelationArguments(EIDocData_ptr docData, int sent_no, std::set<ElfRelation_ptr>& sentence_relations, 
		const std::set<ElfIndividual_ptr>& sentence_individuals);

	// This is a static method called by PredicationFinder::run().
	// It looks for the case where a single person, identified as a victim, is mentioned in a sentence,
	// and merges entities for that mention within the sentence. It performs this action for the first
	// sentence in the document and sentences that are likely datelines.
	static void fixNewswireLeads(const DocTheory *docTheory);
	
	// Dumps information pertaining to a given document into two files. Each has the same path and basename as 
	// the original, but one has an extension of ".dump" and the other has an extension of ".txt".
	// Takes an EIDocData_ptr returned from a previous call to prepareDocumentForInference() invoked 
	// on the same document.
	static void dump(EIDocData_ptr docData);

	// not currently called
	//void removeDangerousPronounsForNFLGames();
	// not currently called
	//std::set<ElfRelationArg_ptr> removeConflictingMentions(const Mention* m1, const ElfRelationArg_ptr arg1, 
		//const Mention* m2, const ElfRelationArg_ptr arg2);
	// not currently called
	//void addPatientBasedEventIndividualCoref(std::set<ElfRelation_ptr>& sentence_relations,
		//std::set<ElfRelation_ptr>& previous_relations);

private:
	// DATA MEMBERS
	static bool _initialized;

	static EIFilterManager _filter_manager;
	static void initializeFilters(std::string filter_ordering_file,std::vector<std::string> superfilters);
	//This isn't really a filter, it assigns bound ids to individuals
	static void addTitleXDocIDs(EIDocData_ptr docData);
	// FILTERS
	// A filter can be used to perform one or more of the following actions on the elements in a set of ELF documents:
	// - add relations
	// - remove selected relations
	// - remove selected individuals
	// See EIFilter.h/cpp for more information about the individual filters.
	// Each method is called from do_inference().

	// Each of these methods calls multiple applyXXXFilter() methods sequentially. (Note the final "s" in the method names.) 
	static void applyRelationSpecificFilters(EIDocData_ptr docData);
	static void applySpecialBoundIDFilters(EIDocData_ptr docData);
	static void applyPerLocationFilters(EIDocData_ptr docData);	

	// Each of these methods instantiates a single filter and applies it.
	static void applyDoubleEntityFilter(EIDocData_ptr docData);
	static void applyLocationFilter(EIDocData_ptr docData);
	static void applyMarriageFilter(EIDocData_ptr docData);
	static void applyAddTitleSubclassFilter(EIDocData_ptr docData);
	static void applyGPEMinisterFilter(EIDocData_ptr docData);
	static void applyFocusGPEResidence(EIDocData_ptr docData);
	static void applyPersonnelHiredFiredFilter(EIDocData_ptr docData);
	static void applyPersonnelHeldPositionFilter(EIDocData_ptr docData);
	static void applyLeadershipFilter(EIDocData_ptr docData);
	static void applyEmploymentFilter(EIDocData_ptr docData);
	static void applyGenericViolenceFilter(EIDocData_ptr docData);
	static void applyUnmappedNFLTeamFilter(EIDocData_ptr docData);
	static void applyMilitaryAttackFilter(EIDocData_ptr docData);
	static void applyNatBoundIDFilter(EIDocData_ptr docData);
	static void applyBoundIDMemberishFilter(EIDocData_ptr docData);
	static void applyInformalMemberFilter(EIDocData_ptr docData);
	static void applyOrgLocationFilter(EIDocData_ptr docData);
	static void applyPerLocationFromDescriptorFilter(EIDocData_ptr docData);
	static void applyPerLocationFromAttackFilter(EIDocData_ptr docData);
	static void applyLocationFromSubSuperFilter(EIDocData_ptr docData);
	static void applyLeadershipPerOrgFilter(EIDocData_ptr docData,
			const std::wstring& ontology);
	static void applyEmploymentPerOrgFilter(EIDocData_ptr docData);
	static void applyGenderFilter(EIDocData_ptr docData);
	static void applyPersonGroupEntityTypeFilter(EIDocData_ptr docData);
	//static void inferAttackInformationFromDocument(EIDocData_ptr docData); // NOT IMPLEMENTED FULLY, NOT USED FOR EVAL
	//KBP-specific filters
	static void applyRemovePersonGroupFilter(EIDocData_ptr docData);
	static void applyRenameMembershipEmployment(EIDocData_ptr docData);
	static void applyKBPTemporalFilter(EIDocData_ptr docData);
	static void applyKBPCompletePTIOFilter(EIDocData_ptr docData);
	static void applyMakePTIOsFilter(EIDocData_ptr docData);
	static void applyInferHoldsThroughout(EIDocData_ptr docData);
	static void applyDuplicateRelationFilter(EIDocData_ptr docData);
	static void applyKBPConflictingBirthDeathTemporalFilter(EIDocData_ptr docData);
	static void applyKBPBirthDeathTemporalFilter(EIDocData_ptr docData);
	static void applyKBPConflictingDateTemporalFilter(EIDocData_ptr docData);
	static void applyKBPStartEndFilter(EIDocData_ptr docData);
	static void applyKBPMatchLearnedAndManualDatesFilter(EIDocData_ptr docData);
	static void applyBoundTitleFilter(EIDocData_ptr docData);
	static void applyRemoveUnusedIndividualsFilter(EIDocData_ptr docData);
	static void applyMarriageWithSetOrOfficialFilter(EIDocData_ptr docData);
	static void applyBrokenArgFilter(EIDocData_ptr docData);
	static void applyCopyPTIOEmploymentMembershipFilter(EIDocData_ptr docData);
	static void applyCrazyBoundIDFilter(EIDocData_ptr docDatA);
	/*** Not fully tested, do not use
	//static void applyDuplicateArgFilter(EIDocData_ptr docData);
	***/

	// END FILTERS

	// METHODS
	static void throw_if_ei_not_initialized(); // called by public members to ensure that an ElfInference has been instantiated
	// Not currently called.
	//bool argumentHasType(const ElfRelationArg_ptr & arg, const std::wstring& checkType);
};

