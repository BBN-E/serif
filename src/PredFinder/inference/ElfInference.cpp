#include "Generic/common/leak_detection.h"

#pragma warning(disable: 4996)

#include <boost/algorithm/string.hpp>
#include <vector>
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/inference/EIDocData.h"
#include "PredFinder/inference/EIFilterManager.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "PredFinder/inference/EIUtils.h"
#include "PredFinder/inference/ElfInference.h"
#include "PredFinder/inference/PlaceInfo.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Value.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/elf/ElfIndividualFactory.h"
#include "PredFinder/elf/ElfRelationArgFactory.h"
#include "PredFinder/inference/EIBlitz.h"
#include "LearnIt/MainUtilities.h"
#include "boost/algorithm/string/trim.hpp"
#include "Generic/edt/Guesser.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternSet.h"

using boost::dynamic_pointer_cast;

// PRIVATE STATIC DATA MEMBER
bool ElfInference::_initialized = false;
EIFilterManager ElfInference::_filter_manager;

////////////////////
//                //
// PUBLIC METHODS //
//   -- for class //
//   -- for doc   //
//                //
////////////////////

/**
 * This constructor must be called before any other ElfInference method is used. A run-time check
 * (throw_if_ei_not_initialized()) called by all public methods other than the constructor enforces this. 
 * @param failed_xdoc_path Path to a failed xdoc file; in practice, the caller determines this path by 
 * reading the "failed_xdoc" parameter from a param file (sample value: "problematic_cluster_output.txt").
 **/
ElfInference::ElfInference(std::string filter_ordering_file, std::vector<std::string> superfilters, const std::string& failed_xdoc_path) {
	Guesser::initialize();
	EITbdAdapter::init(); // if it has already been called during pattern validation, no problem
	EIUtils::init(failed_xdoc_path);
	//EIFilter::init();
	if (filter_ordering_file.size() > 0)
		initializeFilters(filter_ordering_file,superfilters);
	_initialized = true;
}

void ElfInference::initializeFilters(std::string filter_ordering_file,std::vector<std::string> superfilters) {
	//register filters
	_filter_manager.registerFilter(new DoubleEntityFilter());
	_filter_manager.registerFilter(new UnmappedNFLTeamFilter());
	_filter_manager.registerFilter(new LeadershipFilter());
	_filter_manager.registerFilter(new EmploymentFilter());
	_filter_manager.registerFilter(new LocationFilter());
	_filter_manager.registerFilter(new MarriageFilter());
	_filter_manager.registerFilter(new PersonnelHiredFiredFilter());
	_filter_manager.registerFilter(new PersonnelHeldPositionFilter());
	_filter_manager.registerFilter(new GenericViolenceFilter());
	_filter_manager.registerFilter(new GenderFilter());
	_filter_manager.registerFilter(new MilitaryAttackFilter());
	_filter_manager.registerFilter(new BoundIDMemberishFilter());
	_filter_manager.registerFilter(new NatBoundIDFilter());
	_filter_manager.registerFilter(new InformalMemberFilter());
	_filter_manager.registerFilter(new OrgLocationFilter());
	_filter_manager.registerFilter(new PerLocationFromDescriptorFilter());
	_filter_manager.registerFilter(new PerLocationFromAttackFilter());
	_filter_manager.registerFilter(new LeadershipPerOrgFilter(ElfMultiDoc::get_ontology_domain()));
	_filter_manager.registerFilter(new EmploymentPerOrgFilter());
	_filter_manager.registerFilter(new LocationFromSubSuperFilter());
	_filter_manager.registerFilter(new PersonGroupEntityTypeFilter());
	_filter_manager.registerFilter(new RemoveUnusedIndividualsFilter());
	_filter_manager.registerFilter(new RemovePersonGroupRelations());
	_filter_manager.registerFilter(new AddTitleSubclassFilter());
	_filter_manager.registerFilter(new RenameMembershipEmployment());
	_filter_manager.registerFilter(new KBPTemporalFilter());
	_filter_manager.registerFilter(new KBPConflictingDateTemporalFilter());
	_filter_manager.registerFilter(new KBPBirthDeathTemporalFilter());
	_filter_manager.registerFilter(new KBPConflictingBirthDeathTemporalFilter());
	_filter_manager.registerFilter(new InferHoldsThroughout());
	_filter_manager.registerFilter(new KBPMatchLearnedAndManualDatesFilter());
	_filter_manager.registerFilter(new CopyPTIOEmploymentMembershipFilter());
	_filter_manager.registerFilter(new BoundTitleFilter());
	_filter_manager.registerFilter(new DuplicateRelationFilter());
	_filter_manager.registerFilter(new KBPStartEndFilter());
	_filter_manager.registerFilter(new KBPCompletePTIOFilter());
	_filter_manager.registerFilter(new MakePTIOsFilter());
	_filter_manager.registerFilter(new MarriageWithSetOrOfficialFilter());
	_filter_manager.registerFilter(new KBPMinisterGPEFilter());
	_filter_manager.registerFilter(new FocusGPEResidenceFilter());
	_filter_manager.registerFilter(new CrazyBoundIDFilter());
	_filter_manager.registerFilter(new AddTitleXDocIDsFilter());
	
	//register superfilters
	_filter_manager.registerSuperFilters(superfilters);

	//load the ordering
	_filter_manager.loadFilterOrder(filter_ordering_file);
}

ElfInference::~ElfInference() {
	_initialized = false;
	EIUtils::destroyCountryNameEquivalenceTable();
}

// All ElfInference methods other than the constructor and destructor are static.
/* main function */
// Called by PredicationFinder::run().
// Assumes that docData->setDocInfo() has already been called on the same document (by prepareDocumentForInference()),
// and passes in the return value from that call as the parameter docData.
void ElfInference::do_inference(EIDocData_ptr docData, ElfDocument_ptr doc) {
	throw_if_ei_not_initialized();
	docData->setElfDoc(doc);
	//dump();
	if (ParamReader::getRequiredTrueFalseParam("coerce_bound_types") && ElfMultiDoc::get_ontology_domain() != L"blitz")
		EIUtils::addBoundTypesToIndividuals(docData);
	// add rule-based types to individuals where SERIF doesn't help (e.g. terrorists)
	if (ElfMultiDoc::get_ontology_domain() == L"ic") {
		EIUtils::addIndividualTypes(docData);
	}

	if (ElfMultiDoc::get_ontology_domain() == L"blitz") {
		int id_counter = 0;
		EIBlitz::manageBlitzCoreference(docData, L"ic:PhysiologicalCondition", id_counter);
		EIBlitz::manageBlitzCoreference(docData, L"ic:PharmaceuticalSubstance", id_counter);
		EIBlitz::cleanupBlitzIndividualTypes(docData, L"ic:PhysiologicalCondition");
		EIBlitz::cleanupBlitzIndividualTypes(docData, L"ic:PharmaceuticalSubstance");
		EIBlitz::filterBlitzRelations(docData);
	}

	//run filters according to the loaded ordering
	_filter_manager.processFiltersSequence(docData);

	// shut things down
	EIUtils::cleanupDocument(docData);
}
	

// All ElfInference methods other than the constructor and destructor are static.
// Called by PredicationFinder::run() after S-ELF.
// Assumes that docData->setDocInfo() has already been called on the same document (by prepareDocumentForInference()),
// and passes in the return value from that call as the parameter docData.
void ElfInference::do_partner_inference(EIDocData_ptr docData, ElfDocument_ptr doc) {
	throw_if_ei_not_initialized();
	docData->setElfDoc(doc);

	if(ElfMultiDoc::get_ontology_domain() == L"kbp"){
		applyKBPTemporalFilter(docData);
		applyKBPConflictingDateTemporalFilter(docData);
		applyKBPConflictingBirthDeathTemporalFilter(docData);
		applyDuplicateRelationFilter(docData);
	}

	// shut things down
	EIUtils::cleanupDocument(docData);
}

/** 
 * This function must be called before calling do_inference() and the ElfDocument constructor.
 * Pass the return value from this method as the docData parameter to those methods. 
 * @param docTheory A Serif theory for a given document.
 * @return An EIDocData_ptr to be passed into other ElfInference methods.
 **/
EIDocData_ptr ElfInference::prepareDocumentForInference(const DocTheory* docTheory){
	throw_if_ei_not_initialized();
	EIDocData_ptr docData = boost::make_shared<EIDocData>();
	docData->setDocTheory(docTheory);
	EIUtils::createPlaceNameIndex(docData);
	const EntitySet* entity_set = docTheory->getEntitySet();
	/*
	std::map<Symbol, std::set<int> > place_name_to_entity;
	//get the set of all place names in the document
	for(int i = 0; i < entity_set->getNEntities(); i++){	
		const Entity* ent = docTheory->getEntitySet()->getEntity(i);
		if(ent->getType().matchesGPE() || ent->getType().matchesLOC()){
			for(int j =0; j < entity_set->getEntity(i)->getNMentions(); j++){
				const Mention* ment = entity_set->getMention(ent->getMention(j));
				if(ment->getMentionType() == Mention::NAME){
					Symbol name_symbol =Symbol(UnicodeUtil::normalizeTextString(ment->getHead()->toTextString()));
					place_name_to_entity[name_symbol].insert(i);
				}
			}
		}
	}
	*/

	// Make sure this is well-defined no matter what
	docData->getDocumentPlaceInfo().clear();
	docData->getDocumentPlaceInfo().resize(entity_set->getNEntities());
		
	if (EIUtils::getLocationExpansions().size() != 0) {
		//record place sub-locations
		for(int ent_no = 0; ent_no < entity_set->getNEntities(); ent_no++){
			const Entity* ent = docTheory->getEntitySet()->getEntity(ent_no);
			if(ent->getType().matchesGPE() || ent->getType().matchesLOC()){
				docData->getDocumentPlaceInfo(ent_no) = PlaceInfo(ent, docTheory->getEntitySet(), ent_no);
				//std::set<int> sub_and_equiv = getWorldKnowledgeSubLocations(ent);
				std::set<std::wstring> name_strings;
				for(int ment_no = 0; ment_no < ent->getNMentions(); ment_no++){
					const Mention* mention = entity_set->getMention(ent->getMention(ment_no));
					if(mention->getMentionType() == Mention::NAME){			
						std::wstring name_string = UnicodeUtil::normalizeTextString(mention->getHead()->toTextString());
						if(name_strings.find(name_string) == name_strings.end()){
							name_strings.insert(name_string);
							if (EIUtils::getLocationStringsInTable().find(name_string) != EIUtils::getLocationStringsInTable().end())
								docData->getDocumentPlaceInfo(ent_no).setIsInLookupTable(true);
							std::map<std::wstring, std::vector<std::wstring> >::const_iterator iter = EIUtils::getLocationExpansions().find(name_string);
							if (iter != EIUtils::getLocationExpansions().end()) {			
								BOOST_FOREACH(std::wstring sub, (*iter).second) {
									if(docData->getPlaceNameToEntity().find(sub) != docData->getPlaceNameToEntity().end()){
										BOOST_FOREACH(int entity_id, docData->getPlaceNameToEntity()[sub]){
											if(entity_id != ent_no){
												docData->getDocumentPlaceInfo(ent_no).getWKSubLocations().insert(entity_id);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//update super-locations and collect counts of by subtypes for convenience
		BOOST_FOREACH(PlaceInfo pi, docData->getDocumentPlaceInfo()){
			BOOST_FOREACH(int subloc, pi.getWKSubLocations()){
				docData->getDocumentPlaceInfo(subloc).getWKSuperLocations().insert(pi.getIndex());
				if(pi.isCityLike()){
					docData->getCities().push_back(pi.getIndex());
				}
				else if(pi.isStateLike()){
					docData->getStates().push_back(pi.getIndex());
				}
				else if(pi.isNationLike()){
					docData->getCountries().push_back(pi.getIndex());
				}
			}
		}
	}
	return docData;
}



///////////////////////////////
//                           //
// ORG AFFILIATION FUNCTIONS //
//                           //
///////////////////////////////

////////////////////////////////
//                            //
// FUNCTIONS TO ADD ARGUMENTS //
//                            //
////////////////////////////////

// Called by ElfDocument constructor.
void ElfInference::addInferredLocationForEvent(EIDocData_ptr docData, int sent_no, const PatternFeatureSet_ptr featureSet) {
	throw_if_ei_not_initialized();
	if (!EIUtils::isViolentTBDEvent(featureSet))
		return;

	PropositionSet *pSet = docData->getSentenceTheory(sent_no)->getPropositionSet();
	const MentionSet *mSet = docData->getSentenceTheory(sent_no)->getMentionSet();
			
	// We take the first one we come across, for better or for worse
	std::set<const Mention*> locationMentions;
	std::set<const Mention*> mentionsWithExistingReturnValues;
	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);

		// we just look at propositions, not plain mentions, because a mention could
		// be something entirely non-location-like, e.g. "the /Israeli/ soldiers killed him"
		//if (feature->getFeatureType() == SnippetFeature::PROP) {
		if (PropPFeature_ptr pf = dynamic_pointer_cast<PropPFeature>(feature)) {
			EIUtils::getLocations(docData->getDocTheory(), sent_no, pf->getProp(), locationMentions);

			// Deal with "Hebron, where they attacked...", e.g. where<modifier>(<ref>:e0, where:p7)
			for (int p = 0; p < pSet->getNPropositions(); p++) {
				const Proposition *prop = pSet->getProposition(p);
				for (int a = 0; a < prop->getNArgs(); a++) {
					if (prop->getArg(a)->getRoleSym() == Symbol(L"where") &&
						prop->getArg(a)->getType() == Argument::PROPOSITION_ARG &&
						prop->getArg(a)->getProposition() == pf->getProp() &&
						prop->getArg(0)->getRoleSym() == Argument::REF_ROLE &&
						prop->getArg(0)->getType() == Argument::MENTION_ARG)
					{
						locationMentions.insert(prop->getArg(0)->getMention(mSet));
					}
				}
			}

		} else if (MentionPFeature_ptr mf = dynamic_pointer_cast<MentionPFeature>(feature)) {
			// We don't call on the Mention itself, or "Israel attacking" would get a location of Israel
			// However, "people in Israel" attacking is OK
			const Proposition *defProp = pSet->getDefinition(mf->getMention()->getIndex());
			if (defProp != 0)
				EIUtils::getLocations(docData->getDocTheory(), sent_no, defProp, locationMentions);
		} else if (MentionReturnPFeature_ptr rf = dynamic_pointer_cast<MentionReturnPFeature>(feature)) {
			if ((rf->hasReturnValue(L"role") || rf->hasReturnValue(L"tbd")))
			{
				mentionsWithExistingReturnValues.insert(rf->getMention());	
			}
		}
	}

	BOOST_FOREACH(const Mention* locationMention, locationMentions) {
		// Don't add new roles for things that already exist with some return value
		if (mentionsWithExistingReturnValues.find(locationMention) != mentionsWithExistingReturnValues.end())
			continue;
		MentionReturnPFeature_ptr locationFeature = boost::make_shared<MentionReturnPFeature>(Symbol(L"inferredLocation"), locationMention);
		locationFeature->setCoverage(docData->getDocTheory()); // essential!
		SessionLogger::dbg("LEARNIT") << "Adding location (" << 
			locationMention->getNode()->toDebugTextString() << ") to event in sentence: " 
			<< docData->getSentenceTheory(sent_no)->getPrimaryParse()->getRoot()->toDebugTextString() << "\n";
		if (locationMention->getEntityType().matchesGPE()) {
			locationFeature->setReturnValue(L"role", L"eru:eventLocationGPE");
			locationFeature->setReturnValue(L"type", L"ic:GeopoliticalEntity");
			featureSet->addFeature(locationFeature);
		} else if (locationMention->getEntityType().matchesLOC() || locationMention->getEntityType().matchesFAC()) {		
			locationFeature->setReturnValue(L"role", L"eru:eventLocation");
			locationFeature->setReturnValue(L"type", L"ic:Location");
			featureSet->addFeature(locationFeature);
		}
	}
}

// Get inferred dates, but exclude dates on propositions that explicitly do NOT refer to the event,
//   e.g. "he was accused Tuesday of bombing the embassy".
// Called by ElfDocument constructor.
void ElfInference::addInferredDateForEvent(EIDocData_ptr docData, int sent_no, const PatternFeatureSet_ptr featureSet) {
	throw_if_ei_not_initialized();
	if (!EIUtils::isViolentTBDEvent(featureSet))
		return;	

	std::set<const ValueMention*> dateMentions;
	std::set<const ValueMention*> badDateMentions;
		
	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (PropPFeature_ptr pf = dynamic_pointer_cast<PropPFeature>(feature)) {
			EIUtils::getDates(docData->getDocTheory(), sent_no, pf->getProp(), dateMentions);
		} else if (MentionPFeature_ptr mf = dynamic_pointer_cast<MentionPFeature>(feature)) {
			PropositionSet *pSet = docData->getSentenceTheory(sent_no)->getPropositionSet();
			const Proposition *defProp = pSet->getDefinition(mf->getMention()->getIndex());
			if (defProp != 0)
				EIUtils::getDates(docData->getDocTheory(), sent_no, defProp, dateMentions);
		} else if (ValueMentionReturnPFeature_ptr rf = dynamic_pointer_cast<ValueMentionReturnPFeature>(feature)) {
			// These are dates on propositions that explicitly do NOT refer to the event,
			//   e.g. "he was accused Tuesday of bombing the embassy"
			if (rf->getReturnValue(L"tbd").compare(L"bad_date"))
				badDateMentions.insert(rf->getValueMention());
		} 
	}

	/*
	// MUCH more aggressive 
	if (dateMentions.size() == 0) {
		PropositionSet *pSet = docData->getSentenceTheory(sent_no)->getPropositionSet();
		for (int p = 0; p < pSet->getNPropositions(); p++) {
			EIUtils::getDates(docInfo->getDocTheory(), sent_no, pSet->getProposition(p), dateMentions);			
		}
	}
	*/

	BOOST_FOREACH(const ValueMention* dateMention, dateMentions) {	
		if (badDateMentions.find(dateMention) != badDateMentions.end())
			continue;
		ValueMentionReturnPFeature_ptr dateFeature = boost::make_shared<ValueMentionReturnPFeature>(Symbol(L"inferredDate"), dateMention);
		dateFeature->setCoverage(docData->getDocTheory()); // essential!
		dateFeature->setReturnValue(L"role", L"eru:eventDate");
		dateFeature->setReturnValue(L"type", L"xsd:date");
		dateFeature->setReturnValue(L"value", L"true");
		featureSet->addFeature(dateFeature);
	}
}

void ElfInference::addInferredDateForKBP(EIDocData_ptr docData, int sent_no, const PatternSet_ptr patternSet, const PatternFeatureSet_ptr featureSet){
	throw_if_ei_not_initialized();
	Symbol pattern_name = patternSet->getPatternSetName();
	if(! ((pattern_name == Symbol(L"eru:AttendsSchool")) ||
		(pattern_name == Symbol(L"eru:BelongsToHumanOrganization")) ||
		(pattern_name == Symbol(L"eru:IsAffiliateOf")) ||
		(pattern_name == Symbol(L"eru:HasEmployer")) ||
		(pattern_name == Symbol(L"eru:HasSpouse")) ||
		(pattern_name == Symbol(L"eru:HasSubordinateHumanOrganization")) ||
		(pattern_name == Symbol(L"eru:HasTopMemberOrEmployee")) ||
		(pattern_name == Symbol(L"eru:PersonTitleInOrganization")) ||
		(pattern_name == Symbol(L"eru:ResidesInGPE-spec")) ))
	{
		return;
	}
	//need at least one prop match for this feature to be meaningful
	bool hasPropFeature = false;
	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (PropPFeature_ptr pf = dynamic_pointer_cast<PropPFeature>(feature)) {
			hasPropFeature = true;
		}
	}
	if(!hasPropFeature)
		return;
	bool is_start = false;
	bool is_end = false;
	if (EIUtils::containsKBPStart(featureSet))
		is_start = true;
	if (EIUtils::containsKBPEnd(featureSet))
		is_end = true;
	if(is_start && is_end){
		std::ostringstream ostr;
		std::wcout<<"ELFInference::addInferredDateForKBP() Warning: featureset contains both a start and end trigger: "<<std::endl;
		for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
			PatternFeature_ptr feature = featureSet->getFeature(i);
			ostr<<"\t";
			feature->getPattern()->dump(ostr);
			ostr<<"\n";
		}
		std::wcout<<ostr<<std::endl;
	}
	SentenceTheory* sent_theory = docData->getSentenceTheory(sent_no);
	std::set<const ValueMention*> dateMentions;
	std::set<const ValueMention*> startDateMentions;
	std::set<const ValueMention*> endDateMentions;
	std::set<const ValueMention*> badDateMentions;
	const TokenSequence* toks = sent_theory->getTokenSequence();

	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (PropPFeature_ptr pf = dynamic_pointer_cast<PropPFeature>(feature)) {
			EIUtils::getDates(docData->getDocTheory(), sent_no, pf->getProp(), dateMentions);
		} 
		else if (MentionPFeature_ptr mf = dynamic_pointer_cast<MentionPFeature>(feature)) {
			PropositionSet *pSet = docData->getSentenceTheory(sent_no)->getPropositionSet();
			const Proposition *defProp = pSet->getDefinition(mf->getMention()->getIndex());
			if (defProp != 0)
				EIUtils::getDates(docData->getDocTheory(), sent_no, defProp, dateMentions);
		}
		else if (ValueMentionReturnPFeature_ptr rf = dynamic_pointer_cast<ValueMentionReturnPFeature>(feature)) {
			// These are dates on propositions that explicitly do NOT refer to the event,
			//   e.g. "he was accused Tuesday of bombing the embassy"
			if (rf->getReturnValue(L"tbd").compare(L"bad_date"))
				badDateMentions.insert(rf->getValueMention());
		} 
		else if (PropositionReturnPFeature_ptr prf = dynamic_pointer_cast<PropositionReturnPFeature>(feature)) {
			//so for start and end we look in the correct place of the proposition structure
			std::wstring role = prf->getReturnValue(L"role");
			if (role.compare(L"KBP:START_SUB") == 0){
				EIUtils::getDates(docData->getDocTheory(), sent_no, prf->getProp(), startDateMentions, true);
				BOOST_FOREACH(const ValueMention* dateMention, startDateMentions) {
					dateMentions.insert(dateMention);
				}
			} else if (role.compare(L"KBP:END_SUB") == 0) {
				EIUtils::getDates(docData->getDocTheory(), sent_no, prf->getProp(), endDateMentions, true);
				BOOST_FOREACH(const ValueMention* dateMention, endDateMentions) {
					dateMentions.insert(dateMention);
				}
			}
		}
	}
	std::set<const ValueMention*> goodDateMentions;
	std::set<const ValueMention*> accountForDateMentions;
	std::set<const ValueMention*> fromDates;
	std::set<const ValueMention*> toDates;
	std::set<const ValueMention*> durationDatesMentions;
	std::set<const ValueMention*> nonSpecificDateMentions;
	//get groups of date mentions
	BOOST_FOREACH(const ValueMention* dateMention, dateMentions) {	
		const Value *val = docData->getDocTheory()->getValueSet()->getValueByValueMention(dateMention->getUID());

		if(badDateMentions.find(dateMention) != badDateMentions.end())
			continue;
		std::wstring prevWord = L"";
		if(dateMention->getStartToken() > 0){
			prevWord = toks->getToken(dateMention->getStartToken()-1)->getSymbol().to_string();
			std::transform(prevWord.begin(), prevWord.end(), prevWord.begin(), ::tolower);
		}

		if (val->getTimexVal() == EIUtils::PRESENT_REF 
				|| val->getTimexVal() == EIUtils::PAST_REF 
				|| val->getTimexVal() == EIUtils::FUTURE_REF)
		{
			nonSpecificDateMentions.insert(dateMention);
		} else if(prevWord == L"for" || prevWord == L"of"){
			durationDatesMentions.insert(dateMention);
		}
		else if(!val->isSpecificDate()){
			nonSpecificDateMentions.insert(dateMention);
		}
		else{
			goodDateMentions.insert(dateMention);
			if(EIUtils::isEndingRangeDate(sent_theory, dateMention)){
				toDates.insert(dateMention); 
			}
			if(EIUtils::isStartingRangeDate(sent_theory, dateMention)){
				fromDates.insert(dateMention);
			}
		}
	}

	std::set<const ValueMention*> goodStartDateMentions;
	std::set_intersection(startDateMentions.begin(), startDateMentions.end(), goodDateMentions.begin(), goodDateMentions.end(), 
		std::inserter(goodStartDateMentions, goodStartDateMentions.end()));
	std::set<const ValueMention*> goodEndDateMentions;
	std::set_intersection(endDateMentions.begin(), endDateMentions.end(), goodDateMentions.begin(), goodDateMentions.end(), 
		std::inserter(goodEndDateMentions, goodEndDateMentions.end()));
	//remove the start and end specific mentions from the general 'good' ones
	BOOST_FOREACH(const ValueMention* m, goodStartDateMentions) {
		goodDateMentions.erase(m);
	}
	BOOST_FOREACH(const ValueMention* m, goodEndDateMentions) {
		goodDateMentions.erase(m);
	}
	bool start_set = false;
	bool end_set = false;

	if(!is_start && !is_end && goodStartDateMentions.size() == 0 && goodEndDateMentions.size() == 0){	//this is a holds relation, accept multiple dates
		if(fromDates.size() == 1 && toDates.size() == 1){
			std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), (*fromDates.begin()), (*toDates.begin()));
			DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, 
				(*fromDates.begin())->getSentenceNumber(), (*fromDates.begin()) , (*toDates.begin())); 
			dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
			dateSpecFeature->setReturnValue(L"role", L"t:HoldsThroughout");
			dateSpecFeature->setReturnValue(L"type", L"xsd:string");
			dateSpecFeature->setReturnValue(L"value", L"true");
			featureSet->addFeature(dateSpecFeature);
			accountForDateMentions.insert((*toDates.begin()));
			accountForDateMentions.insert((*fromDates.begin()));
		}
		BOOST_FOREACH(const ValueMention* dateMention, goodDateMentions) {
			if(accountForDateMentions.find(dateMention) != accountForDateMentions.end())
				continue;
			const ValueMention* emptyDate = 0;
			std::wstring date_role;
			DateSpecReturnPFeature_ptr dateSpecFeature;
			std::wstring specString;
			if(EIUtils::isStartingRangeDate(sent_theory, dateMention)){
				date_role = L"t:ClippedBackward";
				specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), dateMention, emptyDate);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), dateMention, emptyDate); 
			}
			else if(EIUtils::isEndingRangeDate(sent_theory, dateMention)){
				date_role = L"t:ClippedForward";
				specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), emptyDate, dateMention);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), emptyDate, dateMention); 
			} 
			else{
				date_role = L"t:HoldsWithin";
				specString =  EIUtils::getKBPSpecStringHoldsWithin(docData->getDocTheory(), dateMention);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), dateMention, emptyDate); 
			}
			dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
			dateSpecFeature->setReturnValue(L"role", date_role);
			dateSpecFeature->setReturnValue(L"type", L"xsd:string");
			dateSpecFeature->setReturnValue(L"value", L"true");
			featureSet->addFeature(dateSpecFeature);
			accountForDateMentions.insert(dateMention);
		}
		BOOST_FOREACH(const ValueMention* dateMention, durationDatesMentions) {	
			ValueMentionReturnPFeature_ptr dateFeature = boost::make_shared<ValueMentionReturnPFeature>(Symbol(L"inferredDate"), dateMention);
			dateFeature->setCoverage(docData->getDocTheory()); // essential!
			dateFeature->setReturnValue(L"role", L"t:Duration");
			dateFeature->setReturnValue(L"type", L"xsd:string");
			dateFeature->setReturnValue(L"value", L"true");
			featureSet->addFeature(dateFeature);
		}
		BOOST_FOREACH(const ValueMention* dateMention, nonSpecificDateMentions) {
			std::wstring s = dateMention->toString(toks);
			const ValueMention* emptyDate = 0;
			if(dateMention->getDocValue()->getTimexVal() == EIUtils::PAST_REF){
			/*else if(EIUtils::isEndingRangeDate(sent_theory, dateMention)){
				date_role = L"t:ClippedForward";
				specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), emptyDate, dateMention);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), emptyDate, dateMention); 
			*/
				std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), emptyDate, dateMention);
				DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), 
					emptyDate, dateMention); 
				dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
				dateSpecFeature->setReturnValue(L"role", L"t:clippedForward");
				dateSpecFeature->setReturnValue(L"type", L"xsd:string");
				dateSpecFeature->setReturnValue(L"value", L"true");
				featureSet->addFeature(dateSpecFeature);
			} else if(dateMention->getDocValue()->getTimexVal() == EIUtils::PRESENT_REF){
				/*
				//since this is the default document assumption, don't add the date 
				std::wstring specString =  EIUtils::getKBPSpecStringHoldsWithin(docData->getDocTheory(), dateMention);
				const ValueMention* empty = 0;
				DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), empty , dateMention); 
				dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
				dateSpecFeature->setReturnValue(L"role", L"t:HoldsWithin");
				dateSpecFeature->setReturnValue(L"type", L"xsd:string");
				dateSpecFeature->setReturnValue(L"value", L"true");
				featureSet->addFeature(dateSpecFeature);
				*/

			} else if(dateMention->getDocValue()->getTimexVal() == EIUtils::FUTURE_REF){
			/*if(EIUtils::isStartingRangeDate(sent_theory, dateMention)){
				date_role = L"t:ClippedBackward";
				specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), dateMention, emptyDate);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), dateMention, emptyDate);	
			*/
				std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), dateMention, emptyDate);

				DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), 
					dateMention, emptyDate); 
				dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
				dateSpecFeature->setReturnValue(L"role", L"t:clippedBackwards");
				dateSpecFeature->setReturnValue(L"type", L"xsd:string");
				dateSpecFeature->setReturnValue(L"value", L"true");
				featureSet->addFeature(dateSpecFeature);

			}
			else{
				const ValueMention* emptyDate = 0;
				std::wstring date_role = L"t:HoldsWithin";
				std::wstring specString =  EIUtils::getKBPSpecStringHoldsWithin(docData->getDocTheory(), dateMention);
				if(specString != L"[[,],[,]]"){
					DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), dateMention, emptyDate); 
					dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
					dateSpecFeature->setReturnValue(L"role", date_role);
					dateSpecFeature->setReturnValue(L"type", L"xsd:string");
					dateSpecFeature->setReturnValue(L"value", L"true");
					featureSet->addFeature(dateSpecFeature);
					accountForDateMentions.insert(dateMention);
				}
			}
		}
	}

	else if((is_start || is_end) &&  goodDateMentions.size() > 0) {
		const ValueMention* bestDate = EIUtils::getBestDateMention(goodDateMentions,nonSpecificDateMentions,sent_theory,featureSet);
		const ValueMention* emptyDate = 0;
		if(bestDate != 0){
			if(is_start){	
			/*if(EIUtils::isStartingRangeDate(sent_theory, dateMention)){
				date_role = L"t:ClippedBackward";
				specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), dateMention, emptyDate);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), dateMention, emptyDate);	
			*/
				std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), bestDate, emptyDate);
				DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, bestDate->getSentenceNumber(),
					bestDate, emptyDate); 
				dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
				dateSpecFeature->setReturnValue(L"role", L"t:ClippedBackward");
				dateSpecFeature->setReturnValue(L"type", L"xsd:string");
				dateSpecFeature->setReturnValue(L"value", L"true");
				featureSet->addFeature(dateSpecFeature);
				start_set = true;
			}
			if(is_end){
			/*else if(EIUtils::isEndingRangeDate(sent_theory, dateMention)){
				date_role = L"t:ClippedForward";
				specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), emptyDate, dateMention);
				dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), emptyDate, dateMention); 
			*/
				std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), emptyDate, bestDate);
				DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, bestDate->getSentenceNumber(), 
					emptyDate, bestDate); 
				dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
				dateSpecFeature->setReturnValue(L"role", L"t:ClippedForward");
				dateSpecFeature->setReturnValue(L"type", L"xsd:string");
				dateSpecFeature->setReturnValue(L"value", L"true");
				featureSet->addFeature(dateSpecFeature);
				end_set = true;
			}
		}
	}
	
	// set date mentions that are specifically in the start or end subtrees
	if (!start_set && goodStartDateMentions.size() > 0) {
		const ValueMention* bestDate = EIUtils::getBestDateMention(goodStartDateMentions,nonSpecificDateMentions,sent_theory,featureSet);
		const ValueMention* emptyDate = 0;
		if (bestDate != 0) {
			std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), bestDate, emptyDate);
			DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, bestDate->getSentenceNumber(), bestDate, emptyDate); 
			dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
			dateSpecFeature->setReturnValue(L"role", L"t:ClippedBackward");
			dateSpecFeature->setReturnValue(L"type", L"xsd:string");
			dateSpecFeature->setReturnValue(L"value", L"true");
			featureSet->addFeature(dateSpecFeature);
		}
	}
	if (!end_set && goodEndDateMentions.size() > 0) {
		const ValueMention* bestDate = EIUtils::getBestDateMention(goodEndDateMentions,nonSpecificDateMentions,sent_theory,featureSet);
		const ValueMention* emptyDate = 0;
		if (bestDate != 0) {
			std::wstring specString =  EIUtils::getKBPSpecString(docData->getDocTheory(), emptyDate, bestDate);
			DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, bestDate->getSentenceNumber(), bestDate, emptyDate); 
			dateSpecFeature->setCoverage(docData->getDocTheory()); // essential!
			dateSpecFeature->setReturnValue(L"role", L"t:ClippedForward");
			dateSpecFeature->setReturnValue(L"type", L"xsd:string");
			dateSpecFeature->setReturnValue(L"value", L"true");
			featureSet->addFeature(dateSpecFeature);
		}
	}
}


/////////////////////
//                 //
// MERGE FUNCTIONS //
//                 //
/////////////////////
// First, gather all location, mediatingAgent, and date arguments from existing relations.
// Second, assign those arguments to other relations, as deemed safe.
// Third, remove relations which are entirely subsumed by another.
// Fourth, find relations which are contradictory and split up their event individuals.
// For now, we will do this when they have contradictory dates or when the patient of one is the agent of another.
// Called by ElfDocument constructor.
void ElfInference::mergeRelationArguments(EIDocData_ptr docData, int sent_no,
										  std::set<ElfRelation_ptr>& sentence_relations, 
										  const std::set<ElfIndividual_ptr>& sentence_individuals) 
{
	throw_if_ei_not_initialized();
	// NOTE: THIS CURRENTLY ONLY FIRES FOR MANUAL PATTERN RESULTS

	/** Approach:

	    * Dates are shared between all violent events in a sentence
		* Locations are shared between all violent events in a sentence
		* If X is the agent of a bombing or attack, share X with all violent events in sentence

	 **/

	// Does this sentence have a justice event? These often lead to bad dates.
	bool has_justice_event = false;
	SentenceTheory *st = docData->getSentenceTheory(sent_no);
	EventMentionSet *ems = st->getEventMentionSet();
	for (int i = 0; i < ems->getNEventMentions(); i++) {
		std::wstring event_type = ems->getEventMention(i)->getEventType().to_string();
		if (event_type.find(L"Justice") == 0) {
			has_justice_event = true;
			break;
		}
	}

	std::set<ElfRelationArg_ptr> attackDates;
	std::set<ElfRelationArg_ptr> attackLocations;
	std::set<ElfRelationArg_ptr> mediatingAgents;
	std::set<ElfRelation_ptr> bombingEvents;
	std::set<ElfRelation_ptr> killingOrInjuryEvents;

	const Mention *locationMent = 0;
	bool has_multiple_locations = false;

	// We only want to run some of these steps if we are dealing with violent events.
	// We figure this out simply by making sure we see at least one violent event, which
	//   tells us that we are dealing with the violent event pattern file...
	bool has_violent_event = false;

	//
	// First, gather all location, mediatingAgent, and date arguments from existing relations
	//
	bool is_too_dangerous = false;
	BOOST_FOREACH(ElfRelation_ptr relation, sentence_relations) {
		if (EIUtils::isBombing(relation))
			bombingEvents.insert(relation);
		else if (EIUtils::isKilling(relation) || EIUtils::isInjury(relation))
			killingOrInjuryEvents.insert(relation);
		else if (EIUtils::isAttack(relation) || EIUtils::isGenericViolence(relation)) {
			// OK			
		} else continue;

		has_violent_event = true;

		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			if (arg->get_role().compare(L"eru:eventLocation") == 0 ||
				arg->get_role().compare(L"eru:eventLocationGPE") == 0 ||
				arg->get_role().compare(L"eru:aceEventLocationGPE") == 0)
			{
				const Mention *ment = EIUtils::findMentionForRelationArg(docData, arg);
				if (ment == 0) continue;
				
				Symbol headword = ment->getNode()->getHeadWord();
				if (arg->get_role().compare(L"eru:aceEventLocationGPE") == 0) {
					// if this location came from an ACE event and it's a nationality, beware!
					if (NationalityRecognizer::isLowercaseNationalityWord(headword))
						continue;
					arg->set_role(L"eru:eventLocationGPE");
				}

				// don't allow "to LOCATION" or "from LOCATION"
				if (ment->getNode()->getParent() != 0 &&
					ment->getNode()->getParent()->getTag() == Symbol(L"PP") &&
					(ment->getNode()->getParent()->getHeadWord() == Symbol(L"to") ||
					 ment->getNode()->getParent()->getHeadWord() == Symbol(L"from")))
					continue;

				attackLocations.insert(arg);

				// figure out whether there is more than one distinct location mention in this sentence
				if (locationMent != 0 && locationMent != ment)
					has_multiple_locations = true;
				else locationMent = ment;
			}
			else if (arg->get_role().compare(L"eru:mediatingAgent") == 0)
				mediatingAgents.insert(arg);
			else if (arg->get_role().compare(L"eru:eventDate") == 0 ||
					arg->get_role().compare(L"eru:aceEventDate") == 0) 
			{
				// If there is a justice event mentioned, probably the crime is in the past 
				//   and the trial (or whatever) is recent
				if (has_justice_event && EIUtils::isRecentDate(arg))
					continue;
				if (arg->get_role().compare(L"eru:aceEventDate") == 0)
					arg->set_role(L"eru:eventDate");
				attackDates.insert(arg);
				BOOST_FOREACH(ElfRelationArg_ptr otherDate, attackDates) {
					if (EIUtils::isContradictoryDate(arg, otherDate)) {
						// Too dangerous to do any merging in a sentence with contradictory dates
						is_too_dangerous = true;
					}
				}
			}
		}
	}
	
	if (!has_violent_event)
		return;
	
	//
	// Second, assign those arguments to other relations, as deemed safe.
	//
	if (!is_too_dangerous) {
		std::map<ElfRelation_ptr, std::set<ElfRelationArg_ptr> > patientMap;
		BOOST_FOREACH(ElfRelation_ptr relation, sentence_relations) {
			std::wstring event_type = EIUtils::getTBDEventType(relation);
			if (event_type == L"")
				continue;

			// Don't add dates if one already exists
			if (relation->get_arg(L"eru:eventDate") == ElfRelationArg_ptr()) {
				BOOST_FOREACH(ElfRelationArg_ptr date, attackDates) { 
					relation->insert_argument(date);
				}
			}

			// Don't add locations if one already exists
			if (!has_multiple_locations &&
				relation->get_arg(L"eru:eventLocation") == ElfRelationArg_ptr() && 
				relation->get_arg(L"eru:eventLocationGPE") == ElfRelationArg_ptr()) 
			{
				BOOST_FOREACH(ElfRelationArg_ptr location, attackLocations) { 	
					relation->insert_argument(location);
				}
			}

			// Agents are a bit more complicated because they might change type/role
			std::wstring agent_role = EITbdAdapter::getAgentRole(event_type);
			if (relation->get_arg(agent_role) == ElfRelationArg_ptr()) {
				bool has_agent = false;
				BOOST_FOREACH(ElfRelationArg_ptr agent, mediatingAgents) { 
					ElfRelationArg_ptr new_agent = boost::make_shared<ElfRelationArg>(agent);
					new_agent->set_role(agent_role);
					relation->insert_argument(new_agent);
					has_agent = true;
				}
				if (has_agent && (event_type == L"killing" || event_type == L"injury")) {
					relation->set_name(EITbdAdapter::getEventName(event_type, true));
					ElfIndividual_ptr event_individual = relation->get_arg(EITbdAdapter::getEventRole(event_type))->get_individual();
					ElfType_ptr old_event_type = event_individual->get_type();
					EDTOffset start, end;
					old_event_type->get_offsets(start, end);
					event_individual->set_type(boost::make_shared<ElfType>(EITbdAdapter::getEventType(event_type, true), old_event_type->get_string(), start, end));
				}
			}
		}
	}

	//
	// Third, remove relations which are entirely subsumed by another
	//
	std::set<ElfRelation_ptr> relationsToRemove;
	BOOST_FOREACH(ElfRelation_ptr relation, sentence_relations) {
		if (EIUtils::isGenericViolence(relation))
			continue;
		BOOST_FOREACH(ElfRelation_ptr other_relation, sentence_relations) {

			if (relation == other_relation)
				continue;

			// don't allow a removed relation to subsume you...
			if (relationsToRemove.find(other_relation) != relationsToRemove.end())
				continue;

			// A relation is considered "subsumed" if all of its arguments are 'covered'
			//  by some other relation. An argument is 'covered' if the individual IDs are the
			//  same and the roles are the same. 
			// Note that this includes the event individual, so there can only be subsumption between
			//  events of the same "type". (Bombing cannot match to HumanKillingEvent. However,
			//  this does allow a match between the withAgent and withoutAgent varieties of killings/injuries,
			//  since those event individuals have the same role.)
			bool is_subsumed = true;
			BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {				
				if (arg->get_individual() == ElfIndividual_ptr() || arg->get_individual()->has_value()) {
					is_subsumed = false;
					break;
				}
				bool matched = false;
				BOOST_FOREACH(ElfRelationArg_ptr other_arg, other_relation->get_args_with_role(arg->get_role())) {
					if (other_arg->get_individual() == ElfIndividual_ptr() || other_arg->get_individual()->has_value())
						continue;
					if (arg->get_individual()->get_best_uri() == other_arg->get_individual()->get_best_uri() &&
						arg->get_role() == other_arg->get_role()) 
					{
						matched = true;
						break;
					}
				}
				if (!matched) {
					is_subsumed = false;
					break;
				} 
			}

			if (is_subsumed) {
				relationsToRemove.insert(relation);		
				break;
			}
		}
	}
	BOOST_FOREACH(ElfRelation_ptr relation, relationsToRemove) {
		std::set<ElfRelation_ptr>::iterator relation_i = sentence_relations.find(relation);
		if (relation_i != sentence_relations.end()) {
			sentence_relations.erase(relation_i);
		}
	}

	//
	// Fourth, find relations which are contradictory and split up their event individuals.
	// For now, we will do this when they have contradictory dates or when the patient of one is the agent of another.
	//		
	std::vector<ElfRelationSortedSet>sortedRelations;	
	BOOST_FOREACH(ElfRelation_ptr relation, sentence_relations) {
		if (EIUtils::isGenericViolence(relation))
			continue;
		bool found_match = false;
		for (size_t i = 0; i < sortedRelations.size(); i++) {
			bool is_contradictory = false;
			BOOST_FOREACH(ElfRelation_ptr previous_relation, sortedRelations.at(i)) {
				if (EIUtils::isContradictoryRelation(docData, relation, previous_relation)) {
					is_contradictory = true;
					break;
				}				
			}
			if (!is_contradictory) {
				found_match = true;
				sortedRelations.at(i).insert(relation);
				break;
			}
		}		
		if (!found_match) {				
			ElfRelationSortedSet new_relations;
			new_relations.insert(relation);
			sortedRelations.push_back(new_relations);
		}
	}
	if (sortedRelations.size() > 1) {
		//SessionLogger::info("LEARNIT") << "\nSplitting events in sentence: " << docData->getSentenceTheory(sent_no)->getPrimaryParse()->getRoot()->toDebugTextString() << "\n";
		// Create new individual IDs for the events in these bins
		for (size_t i = 0; i < sortedRelations.size(); i++) {			
			BOOST_FOREACH(ElfRelation_ptr relation, sortedRelations.at(i)) {
				std::wstring event_type = EIUtils::getTBDEventType(relation);
				BOOST_FOREACH(ElfRelationArg_ptr arg, 
					relation->get_args_with_role(EITbdAdapter::getEventRole(event_type))) {
					if (arg->get_individual() != ElfIndividual_ptr()) {
						std::wstringstream new_id;
						new_id << arg->get_individual()->get_generated_uri();
						new_id << L".";
						new_id << i;
						arg->get_individual()->set_generated_uri(new_id.str());
					}
				}
			}
		}
	}
	//addPatientBasedEventIndividualCoref(sentence_relations, previous_relations);
}

//void ElfInference::addPatientBasedEventIndividualCoref(std::set<ElfRelation_ptr>& sentence_relations,
//													   std::set<ElfRelation_ptr>& previous_relations) {
//	// uber-inefficient!
//	// Code to add patient-based event individual coreference
//	BOOST_FOREACH(ElfRelation_ptr relation, sentence_relations) {
//		std::set<ElfRelationArg_ptr> patientArgs = getPatientArguments(relation);
//		if (patientArgs.size() == 0)
//			continue;
//		std::wstring event_type = EIUtils::getTBDEventType(relation);				
//		BOOST_FOREACH(ElfRelation_ptr previous_relation, previous_relations) {
//			std::wstring prev_event_type = EIUtils::getTBDEventType(previous_relation);	
//			if (event_type != prev_event_type)
//				continue;
//			std::set<ElfRelationArg_ptr> prevPatientArgs = getPatientArguments(previous_relation);
//			bool found_match = false;
//			BOOST_FOREACH(ElfRelationArg_ptr arg1, patientArgs) {
//				BOOST_FOREACH(ElfRelationArg_ptr arg2, prevPatientArgs) {
//					if (arg1->get_individual() != ElfIndividual_ptr() &&
//						arg2->get_individual() != ElfIndividual_ptr() &&
//						arg1->get_individual()->get_id() == arg2->get_individual()->get_id())
//					{
//						found_match = true;
//					}
//				}
//			}
//			if (found_match && !isContradictoryRelation(relation, previous_relation)) {
//				std::wstring prev_id = L"";
//				BOOST_FOREACH(ElfRelationArg_ptr arg, previous_relation->get_args_with_role(EITbdAdapter::getEventRole(event_type))) {
//					if (arg->get_individual() != ElfIndividual_ptr()) {
//						prev_id = arg->get_individual()->get_id();
//						break;
//					}
//				}
//				if (prev_id.size() != 0) {
//					BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args_with_role(EITbdAdapter::getEventRole(event_type))) {
//						if (arg->get_individual() != ElfIndividual_ptr()) {
//							arg->get_individual()->set_id(prev_id);
//						}
//					}
//				}
//			}
//		}
//	}
//}

// Where the agent and the patient are the same, and there is no explicit sign that this is okay
// (such as the presence of the word "suicide" or "himself" or "herself"), remove the agent args.
// If we remove all the agents, we may need to re-set the event type and the relation name.
// Called by the ElfDocument constructor.
void ElfInference::pruneDoubleEntitiesFromEvents(EIDocData_ptr docData, int sent_no, 
												 const std::set<ElfRelation_ptr> & sentence_relations) {
	throw_if_ei_not_initialized();
	std::wstring sentenceString = docData->getSentenceTheory(sent_no)->getPrimaryParse()->getRoot()->toTextString();
	BOOST_FOREACH(ElfRelation_ptr relation, sentence_relations) {
		std::wstring event_type = L"";
		if (EIUtils::isKilling(relation))
			event_type = L"killing";
		else if (EIUtils::isInjury(relation))
			event_type = L"injury";
		else if (EIUtils::isBombing(relation))
			event_type = L"bombing";
		else if (EIUtils::isAttack(relation))
			event_type = L"attack";
		else continue;

		std::wstring agent_role = EITbdAdapter::getAgentRole(event_type);
		std::set<std::wstring> patient_roles;
		try {
			patient_roles.insert(EITbdAdapter::getPatientRole(event_type, 
				EntityType::getPERType(), EntitySubtype(L"PER.Individual")));
		} catch ( ... ) {}
		try {
			patient_roles.insert(EITbdAdapter::getPatientRole(event_type, 
				EntityType::getPERType(), EntitySubtype(L"PER.Group")));
		} catch ( ... ) {}
		patient_roles.insert(EITbdAdapter::getPatientRole(event_type, 
			EntityType::getORGType(), EntitySubtype::getUndetType()));

		std::set<ElfRelationArg_ptr> agents;
		std::set<ElfRelationArg_ptr> patients;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			if (arg->get_role().compare(agent_role) == 0)
				agents.insert(arg);
			else if (patient_roles.find(arg->get_role()) != patient_roles.end())
				patients.insert(arg);
		}

		if (agents.size() == 0 || patients.size() == 0)
			continue;

		unsigned int n_agents_removed = 0;
		BOOST_FOREACH(ElfRelationArg_ptr agent, agents) {
			if (agent->get_individual().get() == NULL)
				continue;
			std::wstring agent_id = agent->get_individual()->get_best_uri();				
			BOOST_FOREACH(ElfRelationArg_ptr patient, patients) {
				if (patient->get_individual().get() == NULL)
					continue;
				std::wstring patient_id = patient->get_individual()->get_best_uri();	
				if (patient_id == agent_id) {
					// block cases where it's likely ok that the agent/patient are the same
					const Mention *patientMent = EIUtils::findMentionForRelationArg(docData, patient);
					// actually this seems quite unlikely since we don't get these as PERs :(
					if (patientMent && 
						(patientMent->getNode()->getHeadWord() == Symbol(L"himself") ||
						 patientMent->getNode()->getHeadWord() == Symbol(L"herself")))
					{
						continue;
					}
					if (sentenceString.find(L"suicide") != std::wstring::npos)
					{
						continue;
					}
					relation->remove_argument(agent);
					n_agents_removed++;
					break;
				}
			}
		}

		// if we removed all the agents, we may need to re-set the event type and the relation name
		if (n_agents_removed == agents.size()) {
			// e.g., if event_type = "injury", event_role will be "humanInjuryEvent".
			std::wstring event_role = EITbdAdapter::getEventRole(event_type);
			// e.g., if event_type = "injury", new_event_type will be "ic:HumanAgentInjuringAPerson" or
			// "ic:HumanInjuryEvent"
			std::wstring new_event_type = EITbdAdapter::getEventType(event_type, false);
			BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args_with_role(event_role)) {
				ElfIndividual_ptr event_individual = arg->get_individual();
				ElfType_ptr old_event_type = event_individual->get_type();
				EDTOffset start, end;
				old_event_type->get_offsets(start, end);
				event_individual->set_type(boost::make_shared<ElfType>(new_event_type, old_event_type->get_string(), start, end));
			}
			std::wstring new_event_name = EITbdAdapter::getEventName(event_type, false);
			relation->set_name(new_event_name);
		}
	}
}

// This is a static method called by PredicationFinder::run().
void ElfInference::fixNewswireLeads(const DocTheory *docTheory) {
	throw_if_ei_not_initialized();
	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {
		SentenceTheory *st = docTheory->getSentenceTheory(sent);
		if (sent == 0 || EIUtils::isLikelyDateline(docTheory->getSentenceTheory(sent-1))) {
			const MentionSet *ms = st->getMentionSet();
			const Mention *onlyPersonMention = 0;
			bool more_than_one_person = false;
			bool person_name = false;
			for (int m = 0; m < ms->getNMentions(); m++) {
				if (!ms->getMention(m)->getEntityType().matchesPER())
					continue;
				if (ms->getMention(m)->getMentionType() == Mention::NAME)
					person_name = true;
				else if (EIUtils::isLikelyNewswireLeadDescriptor(ms->getMention(m)))
				{
					if (onlyPersonMention) {
						more_than_one_person = true;
					} else onlyPersonMention = ms->getMention(m);
				}
			}
			if (!more_than_one_person && !person_name && onlyPersonMention != 0) {
				EIUtils::attemptNewswireLeadFix(docTheory, sent, onlyPersonMention);
			}
		}
	}	
}

// Currently called only by removeDangerousPronounsForNFLGames() (whose only invocation
// is currently commented out).
//std::set<ElfRelationArg_ptr> ElfInference::removeConflictingMentions(const Mention* m1, const ElfRelationArg_ptr arg1, 
//																	 const Mention* m2, const ElfRelationArg_ptr arg2){
//	const EntitySet* es  =docData->getEntitySet();
//	std::set<ElfRelationArg_ptr> toRemove;
//	if(arg1 == NULL){
//		return toRemove;
//	}
//	if(arg2 == NULL){
//		return toRemove;
//	}
//	if(m1 != 0 && m1->getMentionType() == Mention::PRON){
//		if(m2 != 0){
//			const Entity* e1 = es->getEntityByMentionWithoutType(m1->getUID());
//			const Entity* e2 =  es->getEntityByMentionWithoutType(m2->getUID());
//			if(e1 != 0 && e1 == e2)
//				toRemove.insert(arg1);
//		}
//		else if(arg2->get_value() == arg1->get_value()){
//			toRemove.insert(arg1);
//		}
//		else
//			SessionLogger::info("LEARNIT")<<"Allow: "<<arg2->get_value()<<", "<<arg1->get_value()<<std::endl;
//	}
//	if(m2 != 0 && m2->getMentionType() == Mention::PRON){
//		if(m1 != 0){
//			const Entity* e1 = es->getEntityByMentionWithoutType(m1->getUID());
//			const Entity* e2 =  es->getEntityByMentionWithoutType(m2->getUID());
//			if(e1 != 0 && e1 == e2)
//				toRemove.insert(arg2);
//		}
//		else if(arg2->get_value() == arg1->get_value()){
//			toRemove.insert(arg2);
//		}
//		else
//			SessionLogger::info("LEARNIT")<<"Allow: "<<arg2->get_value()<<", "<<arg1->get_value()<<std::endl;
//	}
//	if(m1 && m2 && m2->getMentionType() != Mention::PRON && m1->getMentionType() != Mention::PRON){
//		SessionLogger::info("LEARNIT")<<"Not pronouns"<<std::endl;
//		SessionLogger::info("LEARNIT")<<"Arg1: "<<m1->getHead()->toTextString()<<std::endl;
//		SessionLogger::info("LEARNIT")<<"Arg2: "<<m2->getHead()->toTextString()<<std::endl;
//	}
//	return toRemove;
//}

// The only call to this function is currently commented out.
//void ElfInference::removeDangerousPronounsForNFLGames(){
//	/*BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()){
//		std::vector<ElfRelationArg_ptr> args =  relation->get_args();
//		std::vector<ElfRelationArg_ptr> unconfidentArgs = getUnconfidentArguments(args);
//		BOOST_FOREACH(ElfRelationArg_ptr arg, unconfidentArgs){
//          std::ostringstream ostr;
//			ostr<<"Delete Pronoun (all): "<<std::endl;
//			relation->dump(ostr, 5);
//			ostr<<"Arg to delete"<<std::endl;
//			arg->dump(ostr, 5);
//			const Mention* arg_mention = findMentionForRelationArg(arg);
//			ostr<<arg_mention->getHead()->toTextString()<<std::endl;
//          SessionLogger::info("LEARNIT")<<ostr.str();
//			relation->remove_argument(arg);
//		}
//	}*/
//	bool printed = false;
//	const DocTheory *  docTheory = docData->getDocTheory();
//	BOOST_FOREACH(ElfRelation_ptr relation1, docData->get_relations()){
//		std::vector<ElfRelationArg_ptr> winnerArgs1 = relation1->get_args_with_role(L"eru:gameWinner");
//		std::vector<ElfRelationArg_ptr> loserArgs1 = relation1->get_args_with_role(L"eru:gameLoser");
//		std::vector<ElfRelationArg_ptr> gameArgs1 = relation1->get_args_with_role(L"eru:NFLGame");
//		if((winnerArgs1.size() != 1 || loserArgs1.size() !=  1) && gameArgs1.size() != 1)
//			continue;
//		BOOST_FOREACH(ElfRelation_ptr relation2, docData->get_relations()){
//			std::vector<ElfRelationArg_ptr> winnerArgs2 = relation2->get_args_with_role(L"eru:gameWinner");
//			std::vector<ElfRelationArg_ptr> loserArgs2 = relation2->get_args_with_role(L"eru:gameLoser");			
//			std::vector<ElfRelationArg_ptr> gameArgs2 = relation2->get_args_with_role(L"eru:NFLGame");
//			if((winnerArgs2.size() != 1 || loserArgs2.size() !=  1) && gameArgs2.size() != 1)
//				continue;
//			ElfRelationArg_ptr g1Arg = gameArgs1[0];
//			ElfRelationArg_ptr g2Arg = gameArgs2[0];
//			int g1Sent = getSentenceNumberForArg(docTheory, gameArgs1[0]);
//			int g2Sent = getSentenceNumberForArg(docTheory, gameArgs2[0]);
//			if(g1Sent != g2Sent)
//				continue;
//			if(g1Sent == -1)
//				continue;
//			const Mention* w1Ment = 0; 
//			ElfRelationArg_ptr w1Arg;
//			if(winnerArgs1.size() > 0){
//				w1Arg = winnerArgs1[0];
//				w1Ment = findMentionForRelationArg(w1Arg);
//			}
//			const Mention* l1Ment = 0; 
//			ElfRelationArg_ptr l1Arg;
//			if(loserArgs1.size() > 0){
//				l1Arg = loserArgs1[0];
//				l1Ment = findMentionForRelationArg(l1Arg);
//			}
//			const Mention* w2Ment = 0; 
//			ElfRelationArg_ptr w2Arg;
//			if(winnerArgs2.size() > 0){
//				w2Arg = winnerArgs2[0];
//				w2Ment = findMentionForRelationArg(w2Arg);
//			}
//			const Mention* l2Ment = 0; 
//			ElfRelationArg_ptr l2Arg;
//			if(loserArgs2.size() > 0){
//				l2Arg = loserArgs2[0];
//				l2Ment = findMentionForRelationArg(l2Arg);
//			}
//			std::set<ElfRelationArg_ptr> argsToRemove = removeConflictingMentions(w1Ment, w1Arg, l2Ment, l2Arg);
//			BOOST_FOREACH(ElfRelationArg_ptr arg, argsToRemove){
//              std::ostringstream ostr;
//				ostr << "Delete loser (w1--l2): "<<std::endl;
//				relation2->dump(ostr, 5);
//				ostr<<"Arg to delete"<<std::endl;
//				l2Arg->dump(SessionLogger::info("LEARNIT"), 5);
//              SessionLogger::info("LEARNIT")<<ostr.str();
//				relation1->remove_argument(l2Arg);
//			}
//			argsToRemove = removeConflictingMentions(w2Ment, w2Arg, l1Ment, l1Arg);
//			BOOST_FOREACH(ElfRelationArg_ptr arg, argsToRemove){
//              std::ostringstream ostr;
//				ostr <<"Delete loser (w2--l1): "<<std::endl;
//				relation2->dump(ostr, 5);
//				ostr<<"Arg to delete"<<std::endl;
//				l2Arg->dump(ostr, 5);
//              SessionLogger::info("LEARNIT")<<ostr.str();
//				relation1->remove_argument(l2Arg);
//			}
//		}
//	}
//}
//

/**
 * Dumps information pertaining to a given document into two files. Each has the same path and basename as 
 * the original, but one has an extension of ".dump" and the other has an extension of ".txt".
 * @param docData An EIDocData_ptr returned from a previous call to prepareDocumentForInference() invoked 
 * on the same document.
 **/
void ElfInference::dump(EIDocData_ptr docData) {
	throw_if_ei_not_initialized();
	std::string dump_dir = ParamReader::getParam("dump_dir", "elf-dumps");
	//_mkdir(dump_dir);
	const DocTheory *docTheory = docData->getDocTheory();
	std::ostringstream dumpfile;
	std::string docName = docTheory->getDocument()->getName().to_debug_string();
	dumpfile << docName << ".dump";

	std::ostringstream srcTxtFile;
	srcTxtFile << docName << ".txt";

	ofstream dumpStream(dumpfile.str().c_str());
	UTF8OutputStream txtStream(srcTxtFile.str().c_str());
	SessionLogger::info("ei_dump_0") << "Dumping text to: " << dumpfile.str() <<std::endl;
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		docTheory->getSentenceTheory(i)->dump(dumpStream);
		txtStream << docTheory->getSentenceTheory(i)->getTokenSequence()->toString() << "\n";
	}
	docTheory->getEntitySet()->dump(dumpStream);
	dumpStream.close();
	txtStream.close();
	BOOST_FOREACH(ElfRelation_ptr r, docData->get_relations()){
		std::ostringstream ostr;
		ostr<<"\nrelation: " <<std::endl;
		r->dump(ostr, 5);
		SessionLogger::info("ei_dump_1")<<ostr.str();
	}
}

// PRIVATE STATIC METHOD
void ElfInference::throw_if_ei_not_initialized() {
	static const std::string err_msg("ERROR: Attempt to call an ElfInference method before constructing an ElfInference object.");
	if (!_initialized) {
		SessionLogger::info("LEARNIT") << err_msg << std::endl;
		std::runtime_error e(err_msg.c_str());
		throw e;
	}
}

void ElfInference::addTitleXDocIDs(EIDocData_ptr docData){
	//first assign the ids to all individuals that have a title
	std::set<ElfIndividual_ptr> titleIndividuals;
	BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()){
		if(relation->get_name() != L"eru:PersonTitleInOrganization"){
			continue;
		}
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args_with_role(L"eru:titleOfPTIO")){
			std::wstring arg_str = arg->get_text();
			if(arg->get_individual() == NULL)
				continue;
			ElfIndividual_ptr individual = arg->get_individual();
			if(boost::ends_with(individual->get_type()->get_value(), L"Title")){
				titleIndividuals.insert(individual);
			}
		}
	}
	BOOST_FOREACH(ElfIndividual_ptr individual, docData->getElfDoc()->get_individuals_by_type()){
		if(boost::ends_with(individual->get_type()->get_value(), L"Title"))
			titleIndividuals.insert(individual);
	}
	const DocTheory* docTheory = docData->getDocTheory();
	const EntitySet* entitySet = docTheory->getEntitySet();
	BOOST_FOREACH(ElfIndividual_ptr individual, titleIndividuals){
		std::set<ElfIndividual_ptr> corefInds;
		if(boost::starts_with(individual->get_best_uri(), L"bbn:")){//hasn't been changed yet, and it isn't bound
			BOOST_FOREACH(ElfIndividual_ptr ind2, titleIndividuals){
				if(individual->get_best_uri() == ind2->get_best_uri())
					corefInds.insert(ind2);
			}
		}
		std::set<std::wstring> moreSpecificTitles;
		BOOST_FOREACH(ElfIndividual_ptr ind2, corefInds){
			std::wstring individual_type = individual->get_type()->get_value();
			std::wstring prefixless_type = individual_type.substr(individual_type.find_first_of(L":") + 1);
			if(prefixless_type != L"Title")
				moreSpecificTitles.insert(prefixless_type);
		}
		if(moreSpecificTitles.size() > 1){ //shouldn't have assigned multiple specific types, if we did this could be a problem
			std::wcout<<"ElfInference::addTitleXDocIDs(): Warning multiple title subtypes: ";
			BOOST_FOREACH(std::wstring t, moreSpecificTitles){
				std::wcout<<t<<",  ";
			}
			std::wcout<<std::endl;
		}
		std::wstring prefix = L"bbn:XDoc-Title-";
		if(moreSpecificTitles.size() > 0){
			std::wstringstream ss;
			ss <<"bbn:XDoc";
			BOOST_FOREACH(std::wstring t, moreSpecificTitles){
				ss<<L"-"<<t;
			}
			ss<<L"-";
			prefix = ss.str();
		}
		BOOST_FOREACH(ElfIndividual_ptr ind2, corefInds){
			std::wstring txt;
			if(ind2->has_mention_uid()){
				const Mention* m = entitySet->getMention(ind2->get_mention_uid());
				txt = m->getNode()->toTextString();
			} 
			else{
				txt = ind2->get_name_or_desc()->get_value();
			}
			txt = UnicodeUtil::normalizeTextString(txt);
			boost::replace_all(txt, L" ", L"_");
			if(txt.length() > 5){
				std::wstring oldtitle = txt;
				if(boost::starts_with(txt, L"the_")){
					boost::replace_first(txt, L"the_", L"");
				}
				if(boost::starts_with(txt, L"a_")){
					boost::replace_first(txt, L"a_", L"");
				}
			}
			ind2->set_coref_uri(prefix+txt);
		}		
	}
	
}

