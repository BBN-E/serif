#include "Generic/common/leak_detection.h"

#pragma warning(disable: 4996)

#include <boost/algorithm/string.hpp>
#include <vector>
#include <limits>
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/inference/EIDocData.h"
#include "PredFinder/inference/EIFilter.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "PredFinder/inference/EIUtils.h"
#include "PredFinder/inference/ElfInference.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/foreach_pair.hpp"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/common/TimexUtils.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "LearnIt/MainUtilities.h"
#include "boost/algorithm/string/trim.hpp"
#include "Generic/edt/Guesser.h"


// stores number of relations to add/remove and individuals to remove at a point in time
EIFilterSnapshot::EIFilterSnapshot(EIFilter * filt) {
	_orig_count_of_relations_to_add = filt->getCountOfRelationsToAdd();
	_orig_count_of_relations_to_remove = filt->getCountOfRelationsToRemove();
	_orig_count_of_individuals_to_remove = filt->getCountOfIndividualsToRemove();
	_orig_count_of_individuals_to_add = filt->getCountOfIndividualsToAdd();
}

// convenience method used by processAll(), processAllRelations(), and processAllIndividuals() methods
EIFilter::action_type EIFilter::handleSnapshot(EIDocData_ptr docData, const EIFilterSnapshot & snapshot) {
	action_type ret(NOP);
	if (_relationsToAdd.size() > snapshot.getOrigCountOfRelationsToAdd()) {
		ret |= ADD_RELATION;
	}
	if (_relationsToRemove.size() > snapshot.getOrigCountOfRelationsToRemove()) {
		ret |= REMOVE_RELATION;
	}
	if (_individualsToRemove.size() > snapshot.getOrigCountOfIndividualsToRemove()) {
		ret |= REMOVE_INDIVIDUAL;
	}
	if (_individualsToAdd.size() > snapshot.getOrigCountOfIndividualsToAdd()) {
		ret |= ADD_INDIVIDUAL;
	}
	return ret;
}

void EIFilter::apply(EIDocData_ptr docData) {
	if (this->isRelationFilter()) {
		BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()) {
			if (this->matchesRelation(docData, relation)) {
				this->handleRelation(docData, relation);
			}
		}
		//handle the relation changes
		docData->remove_relations(_relationsToRemove);
		EIUtils::addNewRelations(docData, _relationsToAdd, getDecoratedFilterName());
		//handle the individual changes
		docData->remove_individuals(_individualsToRemove);
		BOOST_FOREACH(ElfIndividual_ptr individual, _individualsToAdd) {
			docData->getElfDoc()->insert_individual(individual);
		}
		this->clearRelationsToAdd();
		this->clearRelationsToRemove();
		this->clearIndividualsToAdd();
		this->clearIndividualsToRemove();
	}
	if (this->isIndividualFilter()) {
		BOOST_FOREACH(ElfIndividual_ptr individual, docData->get_individuals_by_type()) {
			if (this->matchesIndividual(docData, individual)) {
				this->handleIndividual(docData, individual);
			}
		}
		//handle the individual changes
		docData->remove_individuals(_individualsToRemove);
		BOOST_FOREACH(ElfIndividual_ptr individual, _individualsToAdd) {
			docData->getElfDoc()->insert_individual(individual);
		}
		this->clearIndividualsToAdd();
		this->clearIndividualsToRemove();
	}
	if (this->isProcessAllFilter()) {
		this->handleAll(docData);
		//handle the relation changes
		docData->remove_relations(_relationsToRemove);
		EIUtils::addNewRelations(docData, _relationsToAdd, getDecoratedFilterName());
		//handle the individual changes
		docData->remove_individuals(_individualsToRemove);
		BOOST_FOREACH(ElfIndividual_ptr individual, _individualsToAdd) {
			docData->getElfDoc()->insert_individual(individual);
		}
		this->clearRelationsToAdd();
		this->clearRelationsToRemove();
		this->clearIndividualsToAdd();
		this->clearIndividualsToRemove();
	}
}

bool DoubleEntityFilter::doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	const std::vector<ElfRelationArg_ptr> args = relation->get_args();
	for (size_t i=0; i<args.size(); ++i) {
		ElfRelationArg_ptr arg1=args[i];
		ElfIndividual_ptr indiv1=arg1->get_individual();
		if (indiv1) {
			wstring id1=indiv1->get_best_uri();
			for (size_t j=i+1; j<args.size(); ++j) {
				ElfRelationArg_ptr arg2=args[j];
				ElfIndividual_ptr indiv2=arg2->get_individual();
				// the same argument can appear more than once in the
				// argument list, so only fire if the roles differ
				if (indiv2 && (arg1->get_role() != arg2->get_role())) {
					wstring id2=indiv2->get_best_uri();
					if (id1==id2)  {
						// check we're not a known exception
						if (!EIUtils::isAllowedDoubleEntityRelation(relation,
							arg1, arg2)) 
						{
							return true;
						}
					}
				}
			}
		} 
	}
	return false;
}

bool UnmappedNFLTeamFilter::doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	BOOST_FOREACH(const ElfRelationArg_ptr& arg, relation->get_args()) {	
		if (arg->get_type()->get_value() == L"nfl:NFLTeam") {
			if (arg->get_individual().get() == NULL ||
				!boost::starts_with(arg->get_individual()->get_bound_uri(), L"nfl:")) {
				//SessionLogger::warn("LEARNIT") << "Removing relation involving <" << arg->get_text() << ">; "
				//	<< "can't be mapped to a known NFL team." << endl;
				return true;
			}
		}
	}
	return false;
}
bool LeadershipFilter::doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	ElfRelationArg_ptr leaderArg; 
	ElfRelationArg_ptr ledArg;
	if(relation->get_name()== L"eru:isLedBy" ){
		leaderArg = relation->get_arg(L"eru:leadingPerson");
		ledArg = relation->get_arg(L"eru:humanOrganization");
	}
	else if(relation->get_name() == L"eru:HasTopMemberOrEmployee" ){
		leaderArg = relation->get_arg(L"eru:objectOfHasTopMemberOrEmployee");
		ledArg = relation->get_arg(L"eru:subjectOfHasTopMemberOrEmployee");
	}
	if (leaderArg && ledArg) {
		const Mention* leaderMention = EIUtils::findMentionForRelationArg(docData, leaderArg);
		const SynNode* leaderNode = leaderMention ? leaderMention->getNode() : 0;
		//const wstring leaderText = leaderNode  ? leaderNode->toTextString() : L"";
		const Mention* ledMention = EIUtils::findMentionForRelationArg(docData, ledArg);
		const ElfIndividual_ptr ledIndividual = ledArg->get_individual();
		if((leaderMention == NULL || ledMention == NULL)){
			return true;
		}
		else{
			std::wstring leaderHead = L" "+leaderMention->getAtomicHead()->toTextString()+L" ";
			std::wstring leaderExtent = L" "+leaderMention->getNode()->toTextString()+L" ";
			if((leaderHead.find(L" director ") != std::wstring::npos) && (ledMention->getEntityType().matchesGPE())){
				return true;
			} 
			else if((leaderHead.find(L" officer ") != std::wstring::npos)&&
					(leaderExtent.find(L" chief ") == std::wstring::npos) && 
					(leaderExtent.find(L" head ") == std::wstring::npos) && 
					(leaderExtent.find(L" exectuive ") == std::wstring::npos) &&
					(leaderExtent.find(L" top ") == std::wstring::npos))
			{
				return true;
			}
			else if (EIUtils::isDeputyWord(leaderNode) || 
				!EIUtils::passesGPELeadershipFilter(docData, leaderMention, ledMention,
					ledIndividual)) 
			{
				return true;
			} else if (ledMention && EIUtils::isPossessorOfORGOrGPE(docData, ledMention)) {
				// would transferring the relation to the possessed
				// entity be better than simple deletion?
				return true;
			} else if (EIUtils::hasAttendedSchoolRelation(docData, leaderArg, ledArg)) {
				return true;
			}
		}
	}
	return false;
}

bool EmploymentFilter::doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	std::vector<ElfRelationArg_ptr> employerArgs; 
	std::vector<ElfRelationArg_ptr> employeeArgs; 
	if(relation->get_name()== L"eru:employs" ){
		employerArgs = relation->get_args_with_role(L"eru:humanAgentEmployer");
		employeeArgs = relation->get_args_with_role(L"eru:personEmployed");
	}
	else if(relation->get_name() == L"eru:HasEmployer" ){
		employerArgs = relation->get_args_with_role(L"eru:subjectOfHasEmployer");
		employeeArgs = relation->get_args_with_role(L"eru:objectOfHasEmployer");
	}
	if (!employerArgs.empty() && !employeeArgs.empty()) {
		BOOST_FOREACH(ElfRelationArg_ptr employerArg, employerArgs) {
			BOOST_FOREACH( ElfRelationArg_ptr employeeArg, employeeArgs) {
				if (EIUtils::isPoliticalParty(docData, employerArg)) {
					const Mention* employeeMention = EIUtils::findMentionForRelationArg(docData, employeeArg);
					const SynNode* employeeNode = employeeMention ? employeeMention->getNode() : 0;
					//const wstring employeeText = employeeNode ? employeeNode->toTextString() : L"";

					if (!EIUtils::isPartyEmployeeWord(employeeNode)) {
						return true;
					}
				}
				
				if (EIUtils::hasAttendedSchoolRelation(docData, employeeArg, employerArg)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool LocationFilter::doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	const ElfRelationArg_ptr subArg = relation->get_arg(L"eru:sub");
	const ElfRelationArg_ptr superArg = relation->get_arg(L"eru:super");
	if (EIUtils::isContradictoryLocation(docData, subArg, superArg))
		return true;
	else if (EIUtils::isContinentConflict(docData, superArg, subArg)){
		return true;
	}
	return false;
}

void MarriageFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	bool bRemoveRelation(false);
	const ElfRelationArg_ptr dateArg = relation->get_arg(L"eru:date");
	if (!dateArg)
		return;

	// "this" can go either way, but it's usually the future for weddings
	std::wstring date_text = dateArg->get_text();
	if (date_text.find(L"next") != std::wstring::npos ||
		date_text.find(L"this") != std::wstring::npos) 
	{
		bRemoveRelation = true;
	}

	std::wstring date_value = boost::static_pointer_cast<ElfIndividual>(dateArg->get_individual())->get_value();
	if (date_value == L"") {
		return;
	}

	// Assumes XXX_XXX_YYYYMMDD. Can be changed later if necessary!
	std::wstring origDocID = docData->getDocument()->getName().to_string();
	if (date_value.length() == 10 && origDocID.length() > 16) {
		std::wstring date_value_int_ready = 
			date_value.substr(0,4) + date_value.substr(5,2) + date_value.substr(8,2);
		int date_value_int = _wtoi(date_value_int_ready.c_str());
		int doc_date_int = _wtoi(origDocID.substr(8,8).c_str());
		if (doc_date_int != 0 && doc_date_int < date_value_int) {
			//SessionLogger::info("LEARNIT") << "Marriage date later than document date: " 
			//	<< date_value_int << " ~ " << doc_date_int << "\n";
			bRemoveRelation = true;
		} 
	}
	if (date_value.length() == 7 && origDocID.length() > 16) {
		std::wstring date_value_int_ready = date_value.substr(0,4) + date_value.substr(5,2);
		int date_value_int = _wtoi(date_value_int_ready.c_str());
		int doc_date_int = _wtoi(origDocID.substr(8,6).c_str());
		if (doc_date_int != 0 && doc_date_int < date_value_int) {
			//SessionLogger::info("LEARNIT") << "Marriage date later than document date: " 
			//	<< date_value_int << " ~ " << doc_date_int << "\n";
			bRemoveRelation = true;
		} 
	}

	// this relation is defined to only want the year for a date, so..
	if (TimexUtils::hasYear(date_value)) {
		// if we have more we need to keep only the year
		dateArg->get_individual()->set_value(TimexUtils::toYearOnly(date_value));
	} else {
		// if we don't have even that, throw out the relation
		bRemoveRelation = true;
	}
	if (bRemoveRelation)
		_relationsToRemove.insert(relation);
}

void PersonnelHiredFiredFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	const ElfRelationArg_ptr repArg = relation->get_arg(L"eru:humanOrganizationRepresentative");
	const ElfRelationArg_ptr orgArg = relation->get_arg(L"eru:humanOrganization");			
	if (repArg) {
		// if a human-organization representative is the one doing the hiring/firing, find
		//   the organization they are associated with and replace them with that
		const Entity* repEntity = EIUtils::findEntityForRelationArg(docData, repArg);
		if (repEntity == 0) {
			// if you can't replace, just remove the relation
			_relationsToRemove.insert(relation);
		}
		const Mention *orgMention = EIUtils::findEmployerForPersonEntity(docData, repEntity);
		EIUtils::addPersonOrgMentions(docData, relation, orgMention, _relationsToAdd);
	} else if (!orgArg) {
		// This person was fired, but we don't know from where!
		const ElfRelationArg_ptr perArg = relation->get_arg(L"eru:person");
		const Entity* perEntity = EIUtils::findEntityForRelationArg(docData, perArg);
		const Mention *orgMention = EIUtils::findEmployerForPersonEntity(docData, perEntity);
		EIUtils::addPersonOrgMentions(docData, relation, orgMention, _relationsToAdd);
	} else {
		return;
	}
	_relationsToRemove.insert(relation);
}

void PersonnelHeldPositionFilter::processRelation(EIDocData_ptr docData, 
												  const ElfRelation_ptr relation) {
	_relationsToRemove.insert(relation);
	const ElfRelationArg_ptr perArg = relation->get_arg(L"eru:person");
	if (!perArg)
		return;
	const Mention *perMention = EIUtils::findMentionForRelationArg(docData, perArg);

	int start_token = -1, end_token = -1;
	if (EIUtils::titleFromMention(docData, perMention, start_token, end_token)) {
		SentenceTheory *sTheory = 
			docData->getSentenceTheory(perMention->getSentenceNumber());	
		const TokenSequence *ts = sTheory->getTokenSequence();

		std::vector<ElfRelationArg_ptr> arguments = 
			EIUtils::createRelationArgFromMention(docData, L"eru:person", L"ic:Person", perMention);
		std::vector<ElfRelationArg_ptr> args2 = 
			EIUtils::createRelationArgFromTokenSpan(docData, L"eru:position", L"ic:Position", 
					perMention->getSentenceNumber(), start_token, end_token);
		arguments.insert(arguments.end(), args2.begin(), args2.end());
		EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
		EDTOffset end_offset = ts->getToken(ts->getNTokens()-1)->getEndEDTOffset();
		LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(
				docData->getDocument()->getOriginalText(), start_offset, end_offset);
		std::wstring relation_text = relationTextLS->toString();
		delete relationTextLS;
		ElfRelation_ptr relation_to_add = boost::make_shared<ElfRelation>(L"eru:heldPosition", 
				arguments, relation_text, start_offset, end_offset, relation->get_confidence(), relation->get_score_group());
		relation_to_add->set_source(relation->get_source());
		_relationsToAdd.insert(relation_to_add);
	}
}

void GenderFilter::processAll(EIDocData_ptr docData) {
	ElfIndividualSet individuals = docData->get_merged_individuals_by_type(L"ic:Person");

	//Guesser is too unreliable here: Sunni = Female, Christian = Male...
	/*BOOST_FOREACH(ind_pair_t pair, docData->get_merged_individuals_by_type(L"ic:PersonGroup")) {
		ElfIndividual_ptr individual = pair.second;
		individuals.push_back(individual);
	}*/

	//std::set<ElfRelation_ptr> relationsToAdd;
	bool print_me = false;
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
		if (!individual->has_entity_id())
			continue;
		ElfRelationMap relMap = docData->get_relations_by_individual(individual);
		if (relMap.find(L"ic:hasGender") != relMap.end())
			continue;
		int serif_entity_id = individual->get_entity_id();
		const Entity *entity = docData->getEntity(serif_entity_id);
		std::vector<const Mention*> male_mentions;
		std::vector<const Mention*> female_mentions;
		for(int i = 0; i< entity->getNMentions(); i++){
			const Mention* mention = docData->getEntitySet()->getMention(entity->getMention(i));
			Symbol gender = Guesser::guessGender(mention->getNode(), mention);
			if(gender == Guesser::MASCULINE)
				male_mentions.push_back(mention);
			if(gender == Guesser::FEMININE){
				female_mentions.push_back(mention);
			}
		}

		// Determine which gender URI to use
		std::wstring gender_uri = L"";
		std::vector<const Mention*> gender_mentions;
		if (male_mentions.size() > 0 && female_mentions.size() == 0) {
			gender_uri = L"ic:Male";
			gender_mentions = male_mentions;
		}
		if (female_mentions.size() > 0 && male_mentions.size() == 0) {
			gender_uri = L"ic:Female";
			gender_mentions = female_mentions;
		}
		
		// Create a binary hasGender relation
		BOOST_FOREACH(const Mention* mention, gender_mentions){
			int sent = mention->getSentenceNumber();
			const TokenSequence *ts = docData->getSentenceTheory(sent)->getTokenSequence();
			EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
			EDTOffset end_offset = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();
			EDTOffset m_start = ts->getToken(mention->getNode()->getStartToken())->getStartEDTOffset();
			EDTOffset m_end = ts->getToken(mention->getNode()->getEndToken())->getEndEDTOffset();
			LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(
				docData->getDocument()->getOriginalText(), start_offset, end_offset);
			std::wstring relation_text = relationTextLS->toString();
			delete relationTextLS;
			std::vector<ElfRelationArg_ptr> arguments = EIUtils::createRelationArgFromMention(docData, L"eru:person", L"ic:Person" , mention);
			ElfRelationArg_ptr gender_arg = boost::make_shared<ElfRelationArg>(L"eru:gender", L"ic:Gender", gender_uri, arguments[0]->get_text(), m_start, m_end);
			arguments.push_back(gender_arg);
			ElfRelation_ptr gender_relation = boost::make_shared<ElfRelation>(L"eru:hasGender", arguments, relation_text, start_offset, end_offset, 
				Pattern::UNSPECIFIED_SCORE, Pattern::UNSPECIFIED_SCORE_GROUP);
			_relationsToAdd.insert(gender_relation);
		}
	}
}

void MilitaryAttackFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	wstring eventType = EIUtils::getTBDEventType(relation);
	wstring agentRole = EITbdAdapter::getAgentRole(eventType);
	if (!agentRole.empty()) {
		ElfRelationArg_ptr agent = relation->get_arg(agentRole);
		if (agent) {
			ElfIndividual_ptr indiv = agent->get_individual();
			if (indiv) {
				std::set<std::wstring> synonyms = EIUtils::getCountrySynonymsForIndividual(docData, indiv);
                if (synonyms.empty()) {
                    return;
                }
				//SessionLogger::info("LEARNIT") << "mil checking (";
				//BOOST_FOREACH(const std::wstring& s, synonyms) {
				//	//SessionLogger::info("LEARNIT") << s << ", ";
				//}
				//SessionLogger::info("LEARNIT") << ")" << endl;
				BOOST_FOREACH(const EIUtils::MilPair& mil_pair, EIUtils::getMilitarySpecialCases()) {
                    // Example: "u s" -> "ic:U.S._military"
					//SessionLogger::info("LEARNIT") << "\tLooking for " << mil_pair.first << endl;
					if (synonyms.find(mil_pair.first) != synonyms.end()) {
						/*
						std::ostringstream ostr;
						ostr << "\nMil replace in relation " << endl;
						relation->dump(ostr, 1);
						ostr << "\n\tReplacing arg " << endl;
						agent->dump(ostr,2);
						ostr << "\n\twith\n\t\t" << mil_pair.second << endl;
						SessionLogger::info("LEARNIT") << ostr.str();
						*/

						_relationsToAdd.insert(EIUtils::copyReplacingWithBoundIndividual(
							relation, indiv, mil_pair.second));
					}
				}
			}
		}
	}
}

void BoundIDMemberishFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	std::wstring relName;
	ElfIndividual_ptr org, person;
	if (EIUtils::extractPersonMemberishRelation(relation, relName, org, person)) {
		if (EIUtils::isMilitaryORG(docData, org)) {
			//SessionLogger::info("LEARNIT") << "It's a military ORG!\t" <<  org->get_id() << endl;
			BOOST_FOREACH(const EIUtils::MilPair& mil_pair, EIUtils::getMilitarySpecialCases()) {
				if (EIUtils::isSubOrgOfCountry(docData, org, mil_pair.first)) {
					_relationsToAdd.insert(EIUtils::copyReplacingWithBoundIndividual(
						relation, org, mil_pair.second));
				}
			}
		}
	}
}

void NatBoundIDFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	// TODO: if the same individual needs to be replaced and appears more
	// than once in a relation, this will only catch the first one
	// because of how copies are made.
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
		std::set<std::wstring> natBoundIDs;
		if (EIUtils::isNatBoundArg(docData, arg, natBoundIDs)) {
			ElfIndividual_ptr indiv = arg->get_individual();
			if (indiv) {
				BOOST_FOREACH(const std::wstring& boundID, natBoundIDs) {
					_relationsToAdd.insert(EIUtils::copyReplacingWithBoundIndividual(
						relation, indiv, boundID));
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
//                                                                   // 
// Use hasInformalMember relations (from pattern file) to propagate  // 
// relations from a group to its members. hasInformalMember only     // 
// finds high precision groups within a single sentence              // 
// Example:                                                          // 
// *Former President George H.W. Bush is a graduate,                 // 
// as are his sons, George W. and Jeb Bush.* -->                     // 
//   eru:hasInformalMember (manual)                                  // 
//       eru:sub: Jeb Bush                                           //
//       eru:sub: George W.                                          //    
//       eru:super: his sons                                         // 
//                                                                   //
// Note: This function deletes hasInformalMember relations.          // 
//       Possibly deletion should be handled by macros.              // 
//                                                                   // 
//                                                                   // 
///////////////////////////////////////////////////////////////////////
void InformalMemberFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	_relationsToRemove.insert(relation);
	std::vector<ElfRelationArg_ptr> super_args = relation->get_args_with_role(L"eru:super");
	std::vector<ElfRelationArg_ptr> sub_args = relation->get_args_with_role(L"eru:sub");
	BOOST_FOREACH(ElfRelationArg_ptr super, super_args){
		ElfRelationMap super_rel_map = 
			docData->get_relations_by_individual(super->get_individual());
		BOOST_FOREACH(ElfRelationMap::value_type super_rel_map_value, super_rel_map){
			if(super_rel_map_value.first == L"eru:hasInformalMember" ||
				super_rel_map_value.first == L"eru:numberOfMembersInt"){
				continue;
			}
			BOOST_FOREACH(ElfRelation_ptr super_relation, super_rel_map_value.second){
				BOOST_FOREACH(ElfRelationArg_ptr sub, sub_args){
					ElfRelationMap sub_rel_map = docData->get_relations_by_individual(sub->get_individual());
					if(sub_rel_map.find(super_relation->get_name()) != sub_rel_map.end()){
						continue; // sub already has a relation of this type, so don't add a second one
					}
					const Mention* sub_mention = EIUtils::findMentionForRelationArg(docData, sub);
					if(sub_mention == 0){
						continue; //no mention, so skip argument
					}
					std::pair<EDTOffset, EDTOffset> mention_sent_offsets = 
						EIUtils::getSentenceEDTOffsetsForMention(docData, sub_mention);
					ElfRelation_ptr new_relation = EIUtils::copyRelationAndSubstituteIndividuals(docData, 
						super_relation, 
						L"applyInformalMemberFilters", 
						super->get_individual(), sub_mention, sub->get_type()->get_value(), 
						min(mention_sent_offsets.first, relation->get_start()), 
						max(mention_sent_offsets.second, relation->get_end()));
					_relationsToAdd.insert(new_relation);
				}
			}
		}
	}
}

///////////////////////////////
//                           //
// ORG AFFILIATION FUNCTIONS //
//                           //
///////////////////////////////

/** 
 * This function attempts to add a sub-organization relationship for government organizations and their parent GPEs.
 * 
 * Approach #1: Organization names that explicitly mention the parent GPE, e.g "U.S. Army" or "Israeli Foreign Ministry".
 *   We require that the organization name string begin with the name of some Nation-State (we do not allow non-nations). 
 *   Matching DOES enforce token breaks, so "US" will not match "USAir".
 *   That Nation-State must exist separately elsewhere in the document (with the actual name used for the ORG, or some
 *   equivalent).
 *
 * Approach #2: Organization descriptions that do not explicitly mention the parent GPE, e.g. "the military".
 *   We require that there be a single "in focus" Nation-State for this document (see identifyFocusNationState()).
 *   We restrict this to descriptions with a certain set of headwords (e.g. "military", "police") and with 
 *   no modifiers (e.g. "our military").
 *
 **/
void OrgLocationFilter::processAll(EIDocData_ptr docData) {
	Symbol::SymbolGroup ok_media = Symbol::makeSymbolGroup(	L"district committee commission council reserve assembly"); 
	std::wstring TARGET_RELATION_NAME = L"eru:gpeHasSubOrg";
	std::wstring CONFLICTING_RELATION_NAME = L"eru:hasMemberHumanAgent";
	std::wstring ORG_ROLE = L"eru:sub";
	std::wstring GPE_ROLE = L"eru:super";
	std::wstring ORG_TYPE = L"ic:GovernmentOrganization";
	std::wstring GPE_TYPE = L"ic:NationState";
	// P-ELF!
	if(ElfMultiDoc::get_ontology_domain() == L"kbp"){
		TARGET_RELATION_NAME = L"eru:HasSubordinateHumanOrganization";
		CONFLICTING_RELATION_NAME = L"NONE";
		ORG_ROLE = L"eru:objectOfHasSubordinateHumanOrganization";
		GPE_ROLE = L"eru:subjectOfHasSubordinateHumanOrganization";
		ORG_TYPE = L"kbp:HumanOrganization";
		GPE_TYPE = L"kbp:GeopoliticalEntity";
	}
	const DocTheory *docTheory = docData->getDocTheory();

	typedef std::pair<std::wstring, ElfIndividual_ptr> ind_pair_t; // for BOOST_FOREACH
	typedef std::pair<std::wstring, const Mention *> ment_pair_t; // for BOOST_FOREACH

	// Index all nation-states in this document by their names
	// Also, find the most common nation-state and store the first mention of its name
	std::map<std::wstring, const Mention*> nationStatesByName;
	const Mention* mostCommonNationState = 0;
	int n_mentions_of_most_common_nation_state = 0;
	BOOST_FOREACH(ElfIndividual_ptr individual, docData->get_merged_individuals_by_type(GPE_TYPE)) {
		if (!individual->has_entity_id())
			continue;
		int serif_entity_id = individual->get_entity_id();
		const Entity *ent = docData->getEntity(serif_entity_id);
		if (ent == 0)
			continue;

		// If this is a candidate for most common nation-state, try to find the
		//   first mention of its name. (We assume the entity's mentions are sorted in rough
		//   document order.) If you can find it, make this the new mostCommonNationState.
		if (ent->getNMentions() > n_mentions_of_most_common_nation_state) {
			const Mention* firstNameMention = 0;
			for (int mentno = 0; mentno < ent->getNMentions(); mentno++) {
				const Mention *ment = docTheory->getEntitySet()->getMention(ent->getMention(mentno));
				if (ment->getMentionType() == Mention::NAME) {
					firstNameMention = ment;
					break;
				}
			} 
			if (firstNameMention != 0) {
				mostCommonNationState = firstNameMention;
				n_mentions_of_most_common_nation_state = ent->getNMentions();
			}				
		}

		// For all nation-states, find name mentions and use them to create name->mention map
		for (int mentno = 0; mentno < ent->getNMentions(); mentno++) {
			const Mention *ment = docTheory->getEntitySet()->getMention(ent->getMention(mentno));
			if (ment->getMentionType() == Mention::NAME) {
				const SynNode *node = ment->getAtomicHead();
				std::wstring name = node->toTextString();
				// Take the first instance of each name we see in the document
				if (nationStatesByName.find(name) == nationStatesByName.end())
					nationStatesByName[name] = ment;
				boost::trim(name);
				// Add in alternative names, e.g. "Israeli" for "Israel"
				BOOST_FOREACH(std::wstring altName, EIUtils::getEquivalentNames(name)) {
					altName += L" ";
					if (nationStatesByName.find(altName) == nationStatesByName.end())
						nationStatesByName[altName] = ment;
				}
			}
		}
	}
	// Look at all organizations of our target type (here, government organizations)
	ElfIndividualSet org_individuals = docData->get_merged_individuals_by_type(ORG_TYPE);
	BOOST_FOREACH(ElfIndividual_ptr individual, org_individuals) {
		if (!individual->has_entity_id())
			continue;
		
		// If this thing already has a relation of roughly this type, don't add a new one
		ElfRelationMap relMap = docData->get_relations_by_individual(individual);
		if (relMap.find(TARGET_RELATION_NAME) != relMap.end())
			continue;
		if (relMap.find(CONFLICTING_RELATION_NAME) != relMap.end())
			continue;
		
		int serif_entity_id = individual->get_entity_id();
		const Entity *ent = docData->getEntity(serif_entity_id);
		if (ent == 0)
			continue;
		EntitySubtype subType = docData->getDocTheory()->getEntitySet()->guessEntitySubtype(ent);
		if(subType.getName() == Symbol(L"Media") && 
			!EIUtils::entityMatchesSymbolGroup(docData->getDocTheory(), ent, ok_media)){
				continue;
		}	
		if(subType.getName() == Symbol(L"Educational") || 
			subType.getName() == Symbol(L"Commercial") ||
			subType.getName() == Symbol(L"Sports") ||
			subType.getName() == Symbol(L"Individual"))
			continue;
		if(EIUtils::isPoliticalParty(docData, ent))
			continue;
		bool found_gpe = false;
		bool has_name = false;
		const Mention *localOrganization = 0;
		for (int mentno = 0; mentno < ent->getNMentions(); mentno++) {
			const Mention *ment = docTheory->getEntitySet()->getMention(ent->getMention(mentno));
			if (ment->getMentionType() == Mention::NAME) {
				has_name = true;
				const SynNode *node = ment->getAtomicHead();
				std::wstring org_string = node->toTextString();

				// Names with dashes are dangerous, e.g. France-Germany Discussion Forum.
				if (org_string.find(L"-") != std::wstring::npos)
					continue;

				// Try to find a nation-state name that matches the beginning of this organization name
				BOOST_FOREACH(ment_pair_t ns_pair, nationStatesByName) {
					// Don't allow it to match itself somehow, it must be a substring
					if (org_string.size() > ns_pair.first.size() && org_string.find(ns_pair.first) == 0) {
						// Add a relation; make whole sentence the provenance for this fact
						TokenSequence *ts = docData->getSentenceTheory(
							ment->getSentenceNumber())->getTokenSequence();
						EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
						EDTOffset end_offset = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();
						_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, TARGET_RELATION_NAME, 
							start_offset, end_offset, ORG_ROLE, ORG_TYPE, ment, GPE_ROLE, GPE_TYPE, 
							ns_pair.second, Pattern::UNSPECIFIED_SCORE));
						found_gpe = true;
						break;
					}
				}
			} else if (ment->getMentionType() == Mention::DESC) {	

				// We are looking for organizations that should probably be affiliated with the local government
				const SynNode *node = ment->getAtomicHead();
				std::wstring fullText = ment->getNode()->toTextString();
				bool bFound = false;
				if (EIUtils::strContainsSubstrFromVector(fullText, EIUtils::getNonLocalWords()))
					continue;

				// We only infer local-ness for certain types of government organizations
				Symbol headword = node->getHeadWord();
				if (EIUtils::is_govt_type_symbol(headword))
				{
					PropositionSet *pSet = docTheory->getSentenceTheory(ment->getSentenceNumber())->getPropositionSet();
					Proposition *defProp = pSet->getDefinition(ment->getIndex());

					// We don't allow local-ness for organizations that are modified in any way, e.g.
					//   "our military" or "his administration" or "the navy in Manila"
					if (defProp != 0 && defProp->getNArgs() == 1) {
						localOrganization = ment;
					} 
				}
			}
			if (found_gpe)
				break;
		}
		// We only want to infer local-ness for unnamed organizations (found_gpe is redundant here, but we add it for clarity)
		if (found_gpe || has_name)
			continue;		
		if (localOrganization != 0) {
			const Mention *nationState = EIUtils::identifyFocusNationState(docData);
			if (nationState != 0) {
				// Provenance is the document context, from the first mention of the nation-state until this sentence
				int start_sent = std::min(nationState->getSentenceNumber(), localOrganization->getSentenceNumber());
				int end_sent = std::max(nationState->getSentenceNumber(), localOrganization->getSentenceNumber());
				TokenSequence *start_ts = docData->getSentenceTheory(start_sent)->getTokenSequence();
				EDTOffset start_offset = start_ts->getToken(0)->getStartEDTOffset();
				TokenSequence *end_ts = docData->getSentenceTheory(end_sent)->getTokenSequence();
				EDTOffset end_offset = end_ts->getToken(end_ts->getNTokens() - 1)->getEndEDTOffset();
				_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, TARGET_RELATION_NAME, start_offset, 
					end_offset, ORG_ROLE, ORG_TYPE, localOrganization, GPE_ROLE, GPE_TYPE, nationState, Pattern::UNSPECIFIED_SCORE));
			}
		} 
	}
}

/** 
 * This function attempts to add a hasCitizenship relationship for people described as, e.g., "a Canadian".
 * That Nation-State must exist separately elsewhere in the document. 
 */
void PerLocationFromDescriptorFilter::processAll(EIDocData_ptr docData) {
	// P-ELF!
	static const std::wstring TARGET_RELATION_NAME = L"eru:hasCitizenship";
	static const std::wstring PER_ROLE = L"eru:person";
	static const std::wstring NATION_ROLE = L"eru:nationState";
	static const std::wstring NATION_TYPE = L"ic:NationState";
	
	typedef std::pair<std::wstring, ElfIndividual_ptr> ind_pair_t; // for BOOST_FOREACH

	const DocTheory *docTheory = docData->getDocTheory();
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		TokenSequence *ts = docTheory->getSentenceTheory(sentno)->getTokenSequence();
		MentionSet *ms = docTheory->getSentenceTheory(sentno)->getMentionSet();
		PropositionSet *ps = docTheory->getSentenceTheory(sentno)->getPropositionSet();
		for (int m = 0; m < ms->getNMentions(); m++) {
			const Mention *ment = ms->getMention(m);
			if (!ment->getEntityType().matchesPER() || ment->getMentionType() != Mention::DESC)
				continue;
			const Proposition *defProp = ps->getDefinition(ment->getIndex());
			if (defProp == 0 || defProp->getPredType() != Proposition::NOUN_PRED)
				continue;
			Symbol headword = ment->getNode()->getHeadWord();
			if (NationalityRecognizer::isLowercaseNationalityWord(headword) &&
				!EIUtils::is_continental_headword(headword))
			{
				std::wstring fullMentionText = ment->getNode()->toTextString();

				// We don't want to be too aggressive here; most of the stuff
				//   not covered by this check is due to a bad parse

				// plurals, e.g. "four israelis" should be covered by the isLowercaseCertainNationalityWord
				if (NationalityRecognizer::isLowercaseCertainNationalityWord(headword) ||
					fullMentionText.find(L"a ") == 0 ||
					fullMentionText.find(L"an ") == 0 ||
					fullMentionText.find(L"one ") == 0)
				{
					// these are OK
				} else if (fullMentionText.find(L"the ") == 0) {
					// Eliminate frequent bad construction: "(the GPE) and (GPE noun)"
					if (ment->getNode()->getEndToken() + 1 < ts->getNTokens()) {
						std::wstring next_word = ts->getToken(ment->getNode()->getEndToken() + 1)->getSymbol().to_string();
						boost::to_lower(next_word);
						if (next_word.compare(L"and") == 0)
							continue; 
					}
				} else continue;

				std::wstring nationality_word = headword.to_string();

				// HACK to retrieve the actual nationality from the word; assumes English
				if (nationality_word.size() > 2 &&
					nationality_word.compare(L"swiss") != 0 &&
					nationality_word.at(nationality_word.size() - 1) == L's')
				{
					nationality_word = nationality_word.substr(0, nationality_word.size() - 1);
				}

				std::set<std::wstring> validNames = EIUtils::getEquivalentNames(nationality_word);

				// Find the nation mention, if it exists
				const Mention *nationMention = 
					EIUtils::findMentionByNameStrings(docData, EntityType::getGPEType(), validNames);
				if (nationMention == 0)
					continue;

				// Find the person individual so we can be sure we have the right type
				const Entity *personEntity = docTheory->getEntitySet()->getEntityByMention(ment->getUID());
				if (personEntity == 0)
					continue;
				std::wstring personType = L"ic:Person";
				ElfIndividual_ptr personIndividual = 
					EIUtils::findElfIndividualForEntity(docData, personEntity->getID(), personType);
				if (personIndividual == ElfIndividual_ptr()) {
					personType = L"ic:PersonGroup";
					personIndividual = EIUtils::findElfIndividualForEntity(docData, personEntity->getID(), personType);
				}
				if (personIndividual == ElfIndividual_ptr())
					continue;

				EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
				EDTOffset end_offset = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();
				_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, TARGET_RELATION_NAME, start_offset, end_offset, 
					PER_ROLE, personType, ment, NATION_ROLE, NATION_TYPE, nationMention, Pattern::UNSPECIFIED_SCORE));
			}			
		}
	}
}

PerLocationFromAttackFilter::PerLocationFromAttackFilter() : EIFilter(L"PerLocationFromAttackFilter",true) {
	try {
	// Wrap these in case we have different subtypes someday.
	// These could throw, but we'd rather know about it than add a catch block that throws away exceptions.
		_victimRoles.insert(EITbdAdapter::getPatientRole(L"killing", EntityType::getPERType(), 
			EntitySubtype(Symbol(L"PER.Group"))));
		_victimRoles.insert(EITbdAdapter::getPatientRole(L"injury", EntityType::getPERType(), 
			EntitySubtype(Symbol(L"PER.Group"))));
		_victimRoles.insert(EITbdAdapter::getPatientRole(L"killing", EntityType::getPERType(), 
			EntitySubtype(Symbol(L"PER.Individual"))));
		_victimRoles.insert(EITbdAdapter::getPatientRole(L"injury", EntityType::getPERType(), 
			EntitySubtype(Symbol(L"PER.Individual"))));
	} catch (...) {
		throw UnexpectedInputException("PerLocationFromAttackFilter::PerLocationFromAttackFilter", 
			"could not insert into _victimRoles");
	}

	// The constructor will throw if Nation is not a valid subtype
	try {
		_nationSubtype = EntitySubtype(Symbol(L"GPE.Nation"));
	} catch ( ... ) {
		throw UnexpectedInputException("PerLocationFromAttackFilter::PerLocationFromAttackFilter", 
			"could not set EntitySubtype(Symbol(L\"GPE.Nation\"))");
	}
}

/** 
 * This function attempts to add a hasCitizenship relationship for person victims of killings/injuries and the
 * place where that event occurred. We only allow described actors with a limited set of headwords.
 * 
 */
void PerLocationFromAttackFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	// P-ELF!
	static const std::wstring TARGET_RELATION_NAME = L"eru:hasCitizenship";
	static const std::wstring CONFLICTING_RELATION_NAME = L"eru:employs";
	static const std::wstring PER_ROLE = L"eru:person";
	static const std::wstring NATION_ROLE = L"eru:nationState";
	static const std::wstring NATION_TYPE = L"ic:NationState";

	// Not used
	//const Mention *focusNationMent = EIUtils::identifyFocusNationState();	
	//Entity *focusNationEntity = 0;
	//if (focusNationMent != 0)
	//	focusNationEntity = docData->getDocTheory()->getEntityByMention(focusNationMent->getUID());
	

	// Find killings/injuries located somewhere
	std::set<ElfRelationArg_ptr> victimArgs;
	std::set<ElfRelationArg_ptr> locationArgs;
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
		if (arg->get_role().compare(L"eru:eventLocationGPE") == 0)
			locationArgs.insert(arg);
		else if (_victimRoles.find(arg->get_role()) != _victimRoles.end())
			victimArgs.insert(arg);
	}
	if (victimArgs.size() == 0 || locationArgs.size() == 0)
		return;
	BOOST_FOREACH(ElfRelationArg_ptr location, locationArgs) {
		const Mention *locationMent = EIUtils::findMentionForRelationArg(docData, location);
		if (locationMent == 0)
			continue;
		Entity *locationEntity = docData->getEntityByMention(locationMent->getUID());
		if (locationEntity == 0)
			continue;
		if (locationMent->guessEntitySubtype(docData->getDocTheory()) != _nationSubtype) {
			std::set<int> superLocations = EIUtils::getSuperLocations(docData, locationEntity->getID());
			bool found_nation = false;
			BOOST_FOREACH(int id, superLocations) {
				locationEntity = docData->getEntity(id);
				if (locationEntity->getNMentions() == 0)
					continue; // highly unlikely
				if (locationEntity->guessEntitySubtype(docData->getDocTheory()) == _nationSubtype) {
					locationMent = docData->getEntitySet()->getMention(locationEntity->getMention(0));	
					found_nation = true;
					break;
				}					
			}
			if (!found_nation)
				continue;
		}
		BOOST_FOREACH(ElfRelationArg_ptr victim, victimArgs) {
			if (victim->get_individual() == ElfIndividual_ptr() ||
				!victim->get_individual()->has_entity_id())
				continue;
			int serif_entity_id = victim->get_individual()->get_entity_id();
			Entity *victimEntity = docData->getEntity(serif_entity_id);
			// Don't allow named entities; too risky
			if (victimEntity == 0 || victimEntity->hasNameMention(docData->getEntitySet()))	
				continue;
			// If this thing already has a relation of this type, don't add a new one
			ElfRelationMap relMap = docData->get_relations_by_individual(victim->get_individual());
			if (relMap.find(TARGET_RELATION_NAME) != relMap.end())
				continue;	
			if (relMap.find(CONFLICTING_RELATION_NAME) != relMap.end())
				continue;

			const Mention *victimMent = EIUtils::findMentionForRelationArg(docData, victim);
			if (victimMent == 0)
				continue;
			Symbol victim_headword = victimMent->getNode()->getHeadWord();
			std::wstring fullVictimText = victimMent->getNode()->toTextString();
			if (!EIUtils::is_person_victim_headword(victim_headword) &&
				fullVictimText.find(L"local") == std::wstring::npos)
			{
				continue;
			}
			
			// definitely not things like "cambodians"! (these might not be caught by the other function if
			//   "cambodia" doesn't exist as a GPE in the document
			if (NationalityRecognizer::isLowercaseNationalityWord(victim_headword))
				continue;
			bool found_other_nation = false;
			MentionSet *ms = docData->getSentenceTheory(victimMent->getSentenceNumber())->getMentionSet();
			for (int m = 0; m < ms->getNMentions(); m++) {
				if (ms->getMention(m)->getEntitySubtype() == _nationSubtype) {
					const Entity *otherEntity = docData->getEntityByMention(
						ms->getMention(m)->getUID());
					if (otherEntity != locationEntity) {
						found_other_nation = true;
						break;
					}
				}
			}
			// If there's another nation mentioned in this sentence, this is too risky
			if (found_other_nation)
				continue;				
			
			TokenSequence *ts = docData->getSentenceTheory(victimMent->getSentenceNumber())->getTokenSequence();
			EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
			EDTOffset end_offset = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();
			_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, TARGET_RELATION_NAME, start_offset, end_offset, PER_ROLE, 
				victim->get_type()->get_value(), victimMent, NATION_ROLE, NATION_TYPE, locationMent, Pattern::UNSPECIFIED_SCORE));
		}
	}
} 

LeadershipPerOrgFilter::LeadershipPerOrgFilter(const std::wstring& ontology)
: EIFilter(L"LeadershipPerOrgFilter",true)
{
	if (ontology != L"ic" && ontology !=L"kbp") {
		throw UnexpectedInputException("LeadershipPerOrgFilter::LeadershipPerOrgFilter",
				"Filter is only valid for ic and kbp domains.");
	}

	// if true, domain is ic; otherwise it's kbp
	bool ic = ontology == L"ic";

	LEADERSHIP_RELATION_NAME = ic?L"eru:isLedBy":L"eru:HasTopMemberOrEmployee";
	LEADERSHIP_PERSON_ROLE = ic?L"eru:leadingPerson":L"eru:objectOfHasTopMemberOrEmployee";
	LEADERSHIP_ORG_ROLE = ic?L"eru:humanOrganization":L"eru:subjectOfHasTopMemberOrEmployee";

	EMPLOYS_RELATION_NAME = ic?L"eru:employs":L"eru:HasEmployer";
	EMPLOYS_PERSON_ROLE = ic?L"eru:personEmployed":L"eru:subjectOfHasEmployer";
	EMPLOYS_ORG_ROLE = ic?L"eru:humanAgentEmployer":L"eru:objectOfHasEmployer";
		
	MEMBER_PERSON_RELATION_NAME = ic?L"eru:hasMemberHumanAgent":L"";
	MEMBER_PERSON_PERSON_ROLE = ic?L"eru:humanAgentMember":L"";
	MEMBER_PERSON_ORG_ROLE = ic?L"eru:humanAgentGroup":L"";
		
	MEMBER_RELATION_NAME = ic?L"eru:hasMemberPerson":L"eru:BelongsToHumanOrganization";
	MEMBER_PERSON_ROLE = ic?L"eru:personMember":L"eru:subjectOfBelongsToHumanOrganization";
	MEMBER_ORG_ROLE = ic?L"eru:personGroup":L"eru:objectOfBelongsToHumanOrganization";
	
	addRelationMatch(LEADERSHIP_RELATION_NAME);
}

// treated separately from LeadershipFilter
void LeadershipPerOrgFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	// P-ELF!
	BOOST_FOREACH (ElfRelationArg_ptr leader, relation->get_args_with_role(LEADERSHIP_PERSON_ROLE)) {
		const Mention *perMent = EIUtils::findMentionForRelationArg(docData, leader);
		if (perMent == 0 || !perMent->getEntityType().matchesPER())
			continue;
		BOOST_FOREACH (ElfRelationArg_ptr org, relation->get_args_with_role(LEADERSHIP_ORG_ROLE)) {
			const Mention *orgMent = EIUtils::findMentionForRelationArg(docData, org);
			if (orgMent == 0)
				continue;
			
			TokenSequence *ts = docData->getSentenceTheory(perMent->getSentenceNumber())->getTokenSequence();
			EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
			EDTOffset end_offset = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();
			
			// Infer employs/member from leadership
			bool use_employs = false;
			if (orgMent->getEntityType().matchesGPE()) {
				use_employs = true;					
			} else if (orgMent->getEntityType().matchesPER()) {
				use_employs = false;
			} else if (orgMent->getEntityType().matchesORG()) {
				use_employs = true;					
				if (org->get_individual() != ElfIndividual_ptr()) {
					// Check if this ORG could be merged with a TerroristOrganization
					ElfIndividual_ptr merged_ind = docData->get_merged_individual_by_uri(org->get_individual()->get_best_uri());
					if (merged_ind->has_type(L"ic:TerroristOrganization"))
						use_employs = false;
				}
				Symbol headword = orgMent->getNode()->getHeadWord();
				if (orgMent->getMentionType() == Mention::DESC && headword == Symbol(L"group"))
					use_employs = false;
				if (headword == Symbol(L"party") ||
					headword == Symbol(L"republican") ||
					headword == Symbol(L"democratic") ||
					headword == Symbol(L"democrat"))
					use_employs = false;
			}

			if (use_employs) {
				_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, EMPLOYS_RELATION_NAME, 
					start_offset, end_offset, EMPLOYS_PERSON_ROLE, leader->get_type()->get_value(), 
					perMent, EMPLOYS_ORG_ROLE, org->get_type()->get_value(), orgMent, relation->get_confidence()));
			} else {
				_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, MEMBER_RELATION_NAME, 
					start_offset, end_offset, MEMBER_PERSON_ROLE, leader->get_type()->get_value(), 
					perMent, MEMBER_ORG_ROLE, org->get_type()->get_value(), orgMent, relation->get_confidence()));
			} 
		}
	}
}

void EmploymentPerOrgFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	// P-ELF!
	static const std::wstring EMPLOYS_RELATION_NAME = L"eru:employs";
	static const std::wstring EMPLOYS_PERSON_ROLE = L"eru:personEmployed";
	static const std::wstring EMPLOYS_ORG_ROLE = L"eru:humanAgentEmployer";

	static const std::wstring CITIZENSHIP_RELATION_NAME = L"eru:hasCitizenship";
	static const std::wstring CITIZENSHIP_PERSON_ROLE = L"eru:person";
	static const std::wstring CITIZENSHIP_GPE_ROLE = L"eru:nationState";
	
	// Add Citizenship relation if a person is employed by a Nation
	BOOST_FOREACH (ElfRelationArg_ptr employee, relation->get_args_with_role(EMPLOYS_PERSON_ROLE)) {
		const Mention *perMent = EIUtils::findMentionForRelationArg(docData, employee);
		if (perMent == 0 || !perMent->getEntityType().matchesPER())
			continue;
		BOOST_FOREACH (ElfRelationArg_ptr org, relation->get_args_with_role(EMPLOYS_ORG_ROLE)) {
			const Mention *orgMent = EIUtils::findMentionForRelationArg(docData, org);
			if (orgMent == 0)
				continue;
			
			TokenSequence *ts = docData->getSentenceTheory(
				perMent->getSentenceNumber())->getTokenSequence();
			EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();
			EDTOffset end_offset = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();

			if (orgMent->getEntityType().matchesGPE()) {
				EntitySubtype gpeEST = orgMent->guessEntitySubtype(docData->getDocTheory());					
				if (gpeEST.getName() == Symbol(L"Nation")) {
					_relationsToAdd.insert(EIUtils::createBinaryRelation(docData, CITIZENSHIP_RELATION_NAME, 
						start_offset, end_offset, CITIZENSHIP_PERSON_ROLE, 
						employee->get_type()->get_value(), perMent, CITIZENSHIP_GPE_ROLE, 
						org->get_type()->get_value(), orgMent, relation->get_confidence()));
				}
			}
		}
	}
}

void LocationFromSubSuperFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	std::vector<ElfRelation_ptr> loc_ext = EIUtils::inferLocationFromSubSuperLocation(docData, relation);
	if(loc_ext.begin() != loc_ext.end()){
		_relationsToAdd.insert(loc_ext.begin(), loc_ext.end());
	}
}

void PersonGroupEntityTypeFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	BOOST_FOREACH(const ElfRelationArg_ptr& arg, relation->get_args()) {
		ElfIndividual_ptr individual = arg->get_individual();
		if (individual.get() != NULL && individual->has_entity_id()) {
			const Entity* entity = docData->getEntity(individual->get_entity_id());
			if (entity->getType() == EntityType::getPERType()) {
				ElfType_ptr old_type = individual->get_type();
				if (old_type->get_value() == L"ic:Person" || old_type->get_value() == L"ic:PersonGroup" ||
					old_type->get_value() == L"kbp:Person" || old_type->get_value() == L"kbp:PersonGroup" ) 
				{
					// Correct the groupness based on the EntitySubtype
					ElfType_ptr new_type;
					EDTOffset start, end;
					old_type->get_offsets(start, end);
					if (entity->guessEntitySubtype(docData->getDocTheory()) == EntitySubtype(Symbol(L"PER.Group"))) {
						if (old_type->get_value() == L"ic:Person")
							new_type = boost::make_shared<ElfType>(L"ic:PersonGroup", old_type->get_string(), start, end);
						if (old_type->get_value() == L"kbp:Person")
							new_type = boost::make_shared<ElfType>(L"kbp:PersonGroup", old_type->get_string(), start, end);
					} else {
						if (old_type->get_value() == L"ic:PersonGroup")
							new_type = boost::make_shared<ElfType>(L"ic:Person", old_type->get_string(), start, end);
						if (old_type->get_value() == L"kpb:PersonGroup")
							new_type = boost::make_shared<ElfType>(L"kbp:Person", old_type->get_string(), start, end);
					}
					if (new_type.get() != NULL)
						individual->set_type(new_type);
				}
			}
		}
	}
}

//This filter use SERIF entity-subtypes to choose whether a PERSON is a group (e.g the lawyers) or individual (e.g. the lawyer)
void PersonGroupEntityTypeFilter::processIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) {
	if (individual.get() != NULL && individual->has_entity_id()) {
		const Entity* entity = docData->getEntity(individual->get_entity_id());
		if (entity->getType() == EntityType::getPERType()) {
			ElfType_ptr old_type = individual->get_type();
			if (old_type->get_value() == L"ic:Person" || old_type->get_value() == L"ic:PersonGroup" ||
				old_type->get_value() == L"kbp:Person" || old_type->get_value() == L"kbp:PersonGroup" ) 
			{
				// Correct the groupness based on the EntitySubtype
				ElfType_ptr new_type;
				EDTOffset start, end;
				old_type->get_offsets(start, end);
				if (entity->guessEntitySubtype(docData->getDocTheory()) == EntitySubtype(Symbol(L"PER.Group"))) {
					if (old_type->get_value() == L"ic:Person")
						new_type = boost::make_shared<ElfType>(L"ic:PersonGroup", old_type->get_string(), start, end);
					if (old_type->get_value() == L"kbp:Person"){
						new_type = boost::make_shared<ElfType>(L"kbp:PersonGroup", old_type->get_string(), start, end);
					}
				} else {
					if (old_type->get_value() == L"ic:PersonGroup")
						new_type = boost::make_shared<ElfType>(L"ic:Person", old_type->get_string(), start, end);
					if (old_type->get_value() == L"kbp:PersonGroup")
						new_type = boost::make_shared<ElfType>(L"kbp:Person", old_type->get_string(), start, end);
				}
				if (new_type.get() != NULL) {
					ElfIndividual_ptr new_individual = boost::make_shared<ElfIndividual>(individual);
					new_individual->set_type(new_type);
					_individualsToRemove.insert(individual);
					_individualsToAdd.insert(new_individual);
				}
			}
		}
	}
}

void RemoveUnusedIndividualsFilter::processAll(EIDocData_ptr docData){
	std::set<std::wstring> relationArgIndividualURIs;
	BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()){
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
			if(arg->get_individual() != NULL){
				std::wstring uri = arg->get_individual()->get_best_uri();
				relationArgIndividualURIs.insert(uri);
			}
		}
	}
	int nToRemove = 0;
	int nDocIndividuals = 0;
	BOOST_FOREACH(ElfIndividual_ptr ind, docData->getElfDoc()->get_individuals_by_type()){
		nDocIndividuals++;
		std::wstring uri = ind->get_best_uri();
		if(relationArgIndividualURIs.find(uri) == relationArgIndividualURIs.end()){
			EIFilter::_individualsToRemove.insert(ind);
			nToRemove++;
		}
	}
	//std::wcout<<"Removed Unused Individuals: "<<nToRemove<<" out of "<<nDocIndividuals<<std::endl;
}

//Begin filters for KBP Task: 
bool RemovePersonGroupRelations::doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation){
	std::vector<ElfRelationArg_ptr> args;
	if(relation->get_name() == L"eru:IsAffiliateOf" || relation->get_name() == L"eru:BelongsToHumanOrganization"){
		args = relation->get_args_with_role(L"eru:subjectOfIsAffiliateOf");
	}
	else{
		args = relation->get_args();
	}
	BOOST_FOREACH(const ElfRelationArg_ptr& arg, args) {
		ElfIndividual_ptr individual = arg->get_individual();
		if (individual.get() != NULL && individual->has_entity_id()) {
			const Entity* entity = docData->getEntity(individual->get_entity_id());
			if (entity->guessEntitySubtype(docData->getDocTheory()) == EntitySubtype(Symbol(L"PER.Group"))) {
				return true;
			}
		}
	}
	return false;
}

void AddTitleSubclassFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	bool added_new_title = false;	
	ElfRelation_ptr relation_to_add = boost::make_shared<ElfRelation>(relation);

	BOOST_FOREACH(ElfRelationArg_ptr newTitleArg, relation_to_add->get_args_with_role(L"eru:titleOfPTIO")) {
		std::wstring title_text = newTitleArg->get_text();
		boost::to_lower(title_text);		
		std::wstring new_title = L"";

		if (title_text == L"ceo" ||
			title_text == L"chairman" ||
			title_text == L"chairwoman" ||
			title_text == L"chief executive officer" ||
			title_text == L"chief executive" ||
			title_text == L"executive director" ||
			title_text == L"national director" ||
			title_text == L"executive chairman" ||
			title_text == L"executive editor" ||
			title_text == L"executive chairwoman" ||
			title_text == L"chairman and ceo" ||
			title_text == L"chairman and chief executive officer" ||
			title_text == L"top executive")
			new_title = L"kbp:HeadOfCompanyTitle";
		else if (title_text == L"king" ||
			title_text == L"queen" ||
			title_text.find(L"prime minister") != std::wstring::npos ||
			title_text == L"emperor" ||
			title_text == L"emir")
			new_title = L"kbp:HeadOfNationStateTitle";
		else if (title_text == L"mayor")
			// just assume this is a city, town, or village (?)
			new_title = L"kbp:HeadOfCityTownOrVillageTitle";
		else if (title_text.find(L"minister") != std::wstring::npos)
			new_title = L"kbp:MinisterTitle";
		else {
			const std::vector<ElfRelationArg_ptr> orgArgs = relation->get_args_with_role(L"eru:organizationOfPTIO");
			bool not_company = false;
			BOOST_FOREACH(ElfRelationArg_ptr orgArg, orgArgs) {
				ElfIndividual_ptr orgInd = orgArg->get_individual();
				if (!orgInd || !orgInd->has_mention_uid())
					continue;
				const Entity *orgEnt = docData->getEntityByMention(orgInd->get_mention_uid());
				if (orgEnt == 0)
					continue;
				EntitySubtype my_subtype = orgEnt->guessEntitySubtype(docData->getDocTheory());
				if (!not_company && orgEnt->getType().matchesORG()) {
					if ( my_subtype.getName() == Symbol(L"Commercial") || !my_subtype.isDetermined()) {
						if (title_text == L"president" || title_text == L"leader" || title_text == L"chief" || 
							title_text == L"head" || title_text == L"director") {
							new_title = L"kbp:HeadOfCompanyTitle";	//last case
						}
					} 
				} else if (orgEnt->getType().matchesGPE()) {
					if (title_text == L"president" || title_text == L"leader" || title_text == L"chief" || 
						title_text == L"premier" || title_text == L"dictator") 
					{
						if (my_subtype.getName() == Symbol(L"Nation")) {
							new_title = L"kbp:HeadOfNationStateTitle"; //first trigger
							break;
						} else if (my_subtype.getName() == Symbol(L"Population-Center")) { // known cities only
							new_title = L"kbp:HeadOfCityTownOrVillageTitle";  //second trigger
							not_company = true;
						}
					}
				}
			}
		}			

		if (new_title == L"")
			continue;

		ElfIndividual_ptr newTitleInd = newTitleArg->get_individual();
		if (newTitleInd == ElfIndividual_ptr())
			continue;

		ElfType_ptr type_to_copy = newTitleInd->get_type();
		EDTOffset start;
		EDTOffset end;
		type_to_copy->get_offsets(start, end);
		ElfType_ptr newTitleType = boost::make_shared<ElfType>(new_title, type_to_copy->get_string(), start, end);
		newTitleInd->set_type(newTitleType);
		added_new_title = true;
	}

	if (added_new_title)
		_relationsToAdd.insert(relation_to_add);

	/*
	bool added_new_title = false;
	BOOST_FOREACH(ElfRelationArg_ptr titleArg, relation->get_args_with_role(L"eru:titleOfPTIO")) {
		// We're going to try to add a subclass for title if it matches certain rules. 
		std::wstring new_title = L"";

		std::wstring title_text = titleArg->get_text();
		boost::to_lower(title_text);

		if (title_text == L"ceo" ||
			title_text == L"chairman" ||
			title_text == L"chairwoman" ||
			title_text == L"chief executive officer" ||
			title_text == L"chief executive" ||
			title_text == L"executive director" ||
			title_text == L"national director" ||
			title_text == L"executive chairman" ||
			title_text == L"executive editor" ||
			title_text == L"executive chairwoman" ||
			title_text == L"chairman and ceo" ||
			title_text == L"chairman and chief executive officer" ||
			title_text == L"top executive")
			new_title = L"kbp:HeadOfCompanyTitle";
		else if (title_text == L"king" ||
			title_text == L"queen" ||
			title_text.find(L"prime minister") != std::wstring::npos ||
			title_text == L"emperor" ||
			title_text == L"emir")
			new_title = L"kbp:HeadOfNationStateTitle";
		else if (title_text == L"mayor")
			// just assume this is a city, town, or village (?)
			new_title = L"kbp:HeadOfCityTownOrVillageTitle";
		else if (title_text.find(L"minister") != std::wstring::npos)
			new_title = L"kbp:MinisterTitle";
		else {
			const ElfRelationArg_ptr orgArg = relation->get_arg(L"eru:organizationOfPTIO");
			if (!orgArg) 
				continue;
			ElfIndividual_ptr orgInd = orgArg->get_individual();
			if (!orgInd || !orgInd->has_mention_uid())
				continue;
			const Entity *orgEnt = docData->getEntityByMention(orgInd->get_mention_uid());
			if (orgEnt == 0)
				continue;
			EntitySubtype my_subtype = DistillUtilities::getEntitySubtype(docData->getDocTheory(), orgEnt);
			if (orgEnt->getType().matchesORG()) {
				if (my_subtype.getName() == Symbol(L"Commercial") || !my_subtype.isDetermined()) {
					if (title_text == L"president" || title_text == L"leader" || title_text == L"chief" || 
						title_text == L"head" || title_text == L"director")
						new_title = L"kbp:HeadOfCompanyTitle";					
				} 
			} else if (orgEnt->getType().matchesGPE()) {
				if (title_text == L"president" || title_text == L"leader" || title_text == L"chief" || 
					title_text == L"premier" || title_text == L"dictator") 
				{
					if (my_subtype.getName() == Symbol(L"Nation"))
						new_title = L"kbp:HeadOfNationStateTitle";
					else if (my_subtype.getName() == Symbol(L"Population-Center")) // known cities only
						new_title = L"kbp:HeadOfCityTownOrVillageTitle";
				}
			}
		}			

		if (new_title == L"")
			continue;

		BOOST_FOREACH(ElfRelationArg_ptr newTitleArg, relation_to_add->get_args_with_role(L"eru:titleOfPTIO")) {
			std::wstring new_title_text = newTitleArg->get_text();
			boost::to_lower(new_title_text);
			if (title_text != new_title_text)
				continue;

			ElfIndividual_ptr newTitleInd = newTitleArg->get_individual();
			if (newTitleInd == ElfIndividual_ptr())
				continue;
			ElfType_ptr type_to_copy = newTitleInd->get_type();
			EDTOffset start;
			EDTOffset end;
			type_to_copy->get_offsets(start, end);
			ElfType_ptr newTitleType = boost::make_shared<ElfType>(new_title, type_to_copy->get_string(), start, end);
			newTitleInd->set_type(newTitleType);
			added_new_title = true;
			break;
		}
	}*/
}

void RenameMembershipEmployment::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation){
	_relationsToRemove.insert(relation);

	std::wstring sub_str = L"eru:subjectOf";
	std::wstring obj_str = L"eru:objectOf";

	//we need different symbol groups for different ACE types
	std::wstring govt_employing_depts = L"bureau agency agencies ministry ministries department departments administration"; 
	std::wstring govt_member_orgs = L"court courts army navy forces parliament legislature " 
									L"fed board party congress democrat democratic republican labor communist marxist conservative";
	std::wstring generic_member_orgs = L"cabinet administration commission committee club";
	std::wstring govt_person_titles  = 		L"chancellor congressman congresswoman emir emperor empress judge justice president president "
		 L"representative rep. rep secretary-general sen. senator solicitor vice-president queen king lord duke prince minister governor gov. princess "
		 L"official mayor councilman councilwoman duchess general gen. gen colonel col. admiral adm. sergeant sgt sgt. major dictator vice-premier premier monarch soldier lawmaker "
		 L"attorney-general speaker official";
	std::wstring govt_member_titles = L"secretary sec sec. chairman";
	std::wstring govt_empl_titles = L"magistrate sheriff attorney clerk registrar procesecutor";
	std::wstring generic_members = L"member fellow player striker goalkeeper quarterback baseman pitcher back end";
	std::wstring generic_employees = L"spokesman spokeswoman executive coach manager";
	std::wstring ignore_all = L"student freshman sophmore junior senior player alum fan commentator viewer";
	std::wstring sports_employee = L"coach manager president owner executive";
	std::wstring space =L" ";

	Symbol::SymbolGroup possibleMemberGroup = Symbol::makeSymbolGroup(govt_member_orgs + space + generic_member_orgs);
	Symbol::SymbolGroup possibleMemberAndEmployingGroup = Symbol::makeSymbolGroup(govt_employing_depts);
	Symbol::SymbolGroup possibleGPEEmployee = Symbol::makeSymbolGroup(govt_person_titles + space + govt_empl_titles + space + govt_member_titles);
	Symbol::SymbolGroup alwaysOrgMember = Symbol::makeSymbolGroup(govt_person_titles);
	Symbol::SymbolGroup likelyOrgMember = Symbol::makeSymbolGroup(govt_person_titles + space + govt_member_titles + space + generic_members);
	Symbol::SymbolGroup likelyOrgEmployee = Symbol::makeSymbolGroup(generic_employees);
	Symbol::SymbolGroup sportsEmployee = Symbol::makeSymbolGroup(sports_employee);
	Symbol::SymbolGroup students = Symbol::makeSymbolGroup(L"student freshman sophmore junior senior player alum");
	Symbol::SymbolGroup ignore = Symbol::makeSymbolGroup(ignore_all);
	Symbol::SymbolGroup genericMembers = Symbol::makeSymbolGroup(govt_person_titles + space + generic_members);


	if(relation->get_name() == L"eru:IsAffiliateOf"){
		//subjectOfIsAffiliateWith
		std::vector<ElfRelationArg_ptr> subjectArgs = relation->get_args_with_role(sub_str +L"IsAffiliateOf"); 
		std::vector<ElfRelationArg_ptr> objectArgs = relation->get_args_with_role(obj_str +L"IsAffiliateOf"); 
		typedef std::pair<ElfRelationArg_ptr, ElfRelationArg_ptr> subj_obj_pair;
		std::set<subj_obj_pair> empArgPairs;
		std::set<subj_obj_pair> memberArgPairs;
		const DocTheory* docTheory = docData->getDocTheory();
		const EntitySet* entitySet = docTheory->getEntitySet();
		EntitySubtype COMMERCIAL = EntitySubtype(Symbol(L"ORG.Commercial"));
		EntitySubtype MEDIA = EntitySubtype(Symbol(L"ORG.Media"));
		EntitySubtype GOVERNMENT = EntitySubtype(Symbol(L"ORG.Government"));
		EntitySubtype SPORTS = EntitySubtype(Symbol(L"ORG.Sports"));
		EntitySubtype NON_GOVERNMENT = EntitySubtype(Symbol(L"ORG.Non-Governmental"));
		EntitySubtype EDUCATIONAL = EntitySubtype(Symbol(L"ORG.Educational"));
		BOOST_FOREACH(const ElfRelationArg_ptr& obj, objectArgs){
			const Mention* objMention = EIUtils::findMentionForRelationArg(docData, obj);
			if(!objMention) continue;
			const Entity* objEntity = docTheory->getEntityByMention(objMention);
			if(!objEntity) continue;
			EntitySubtype objSubtype = entitySet->guessEntitySubtype(objEntity);
			Symbol objHW = objMention->getAtomicHead()->getHeadWord();
			//bool is_likely_member_org = EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), objMention, likely_member_orgs), true);
			//bool is_always_member_org = EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), objMention, always_member_orgs, true);


			BOOST_FOREACH(const ElfRelationArg_ptr& subj, subjectArgs){
				//we care about the lexical nature of the person (e.g. coaches are employees, players are members
				const Mention* subMention = EIUtils::findMentionForRelationArg(docData, subj);
				if(!subMention)
					continue;
				Symbol subMentionTitle = Symbol(L"NONE");
				if(subMention->getMentionType() == Mention::DESC){
					subMentionTitle = subMention->getAtomicHead()->getHeadWord();
				} 
				else if(subMention->getMentionType() == Mention::NAME){
					const Mention* title = EIUtils::getImmediateTitle(docData->getDocTheory(), subMention);
					if(title)
						subMentionTitle = title->getAtomicHead()->getHeadWord();
				}	
				if(objMention->getEntityType().matchesGPE()){
					//only employment relations are valid here, and only if they talk about relevant government people
					if(subMentionTitle.isInSymbolGroup(possibleGPEEmployee)){ 
						empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
					} 
				}
				else if(objMention->getEntityType().matchesORG()){
					if (EIUtils::isTerroristOrganizationName(obj->get_text())) {
						memberArgPairs.insert(std::make_pair(subj, obj));
					} else if((objSubtype == COMMERCIAL) || (objSubtype == MEDIA)){	
						empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
					} else if ((objSubtype == GOVERNMENT) || (objSubtype == NON_GOVERNMENT)) {
						//start by checking for mention/mention matching
						if(subMentionTitle.isInSymbolGroup(alwaysOrgMember)){
							memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else if(EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), objMention, possibleMemberGroup) && subMentionTitle.isInSymbolGroup(likelyOrgMember)){
							memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else if(subMentionTitle.isInSymbolGroup(likelyOrgEmployee)){
							empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						//move on to entity level matching
						else if(EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), subMention, alwaysOrgMember, true)){
							memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else if(EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), objMention, possibleMemberGroup, true) &&
							EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), subMention, likelyOrgMember, true)){
							memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else if(EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), subMention, likelyOrgEmployee, true)){
							empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else{//default-- avoid doing stupid things
							if(subMentionTitle.isInSymbolGroup(students)||
								subMentionTitle.isInSymbolGroup(ignore)){
								;
							}
							else if(subMentionTitle.isInSymbolGroup(sportsEmployee)){
								empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
							}
							else if(EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), objMention, possibleMemberGroup)){
								memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
							}
							else if(EIUtils::mentionMatchesSymbolGroup(docData->getDocTheory(), objMention, possibleMemberAndEmployingGroup)){
								memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
								empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
							}
							else{
								empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
							}
						}
					}
					else if (objSubtype == SPORTS){ //treat these l
						if(subMentionTitle.isInSymbolGroup(sportsEmployee)){ 
							empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else if(!subMentionTitle.isInSymbolGroup(ignore)){ 
							memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						} 
					}
					else if (objSubtype == EDUCATIONAL){ //treat these l
						if(!subMentionTitle.isInSymbolGroup(students)){ 
							empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
					}
					else{ //assume undetermined subtype{
						if(subMentionTitle.isInSymbolGroup(ignore)){
							continue;
						}
						else if(subMentionTitle.isInSymbolGroup(genericMembers)){
							memberArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
							empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
						else{
							empArgPairs.insert(std::pair<ElfRelationArg_ptr,ElfRelationArg_ptr>(subj, obj));
						}
					}
				}
			}
		}
		//get the set of temporal arguments that get added to other relations
		std::vector<ElfRelationArg_ptr> argsToKeep;
		std::vector<ElfRelationArg_ptr>  args = relation->get_args();
		BOOST_FOREACH(ElfRelationArg_ptr arg, args){
			if(arg->get_role() == sub_str +L"IsAffiliateOf")
				continue;
			else if(arg->get_role() == obj_str+L"IsAffiliateOf")
				continue;
			argsToKeep.push_back(arg);
		}
		//get the offsets and text for the new relation
		LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(docData->getDocument()->getOriginalText(), relation->get_start(), relation->get_end());
		std::wstring r_text = relationTextLS->toString();
		delete relationTextLS;

		//now make new relations
		BOOST_FOREACH(subj_obj_pair so, empArgPairs){
			std::vector<ElfRelationArg_ptr> new_args;
			BOOST_FOREACH(ElfRelationArg_ptr arg, argsToKeep){
				new_args.push_back(boost::make_shared<ElfRelationArg>(ElfRelationArg(arg)));
			}
			const Mention* subMention = EIUtils::findMentionForRelationArg(docData, so.first);
			std::vector<ElfRelationArg_ptr> t1 = EIUtils::createRelationArgFromMention(docData, sub_str+L"HasEmployer", so.first->get_type()->get_value(), subMention);
			new_args.insert(new_args.begin(), t1.begin(), t1.end());

			const Mention* objMention = EIUtils::findMentionForRelationArg(docData, so.second);
			std::vector<ElfRelationArg_ptr> t2 = EIUtils::createRelationArgFromMention(docData, obj_str+L"HasEmployer", so.second->get_type()->get_value(), objMention);
			new_args.insert(new_args.begin(), t2.begin(), t2.end());

			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(L"eru:HasEmployer", 
				new_args, r_text, relation->get_start(), relation->get_end(), relation->get_confidence(), relation->get_score_group());
			new_relation->set_source(relation->get_source());
			new_relation->add_source(L"eru:copyRelationAndSubstituteArgs");
			new_relation->add_source(L"eru:transformAffiliates");
			_relationsToAdd.insert(new_relation);
		}
	
		BOOST_FOREACH(subj_obj_pair so, memberArgPairs){
			std::vector<ElfRelationArg_ptr> new_args;
			BOOST_FOREACH(ElfRelationArg_ptr arg, argsToKeep){
				new_args.push_back(boost::make_shared<ElfRelationArg>(ElfRelationArg(arg)));
			}
			const Mention* subMention = EIUtils::findMentionForRelationArg(docData, so.first);
			std::vector<ElfRelationArg_ptr> t1 = EIUtils::createRelationArgFromMention(docData, sub_str+L"BelongsToHumanOrganization", so.first->get_type()->get_value(), subMention);
			new_args.insert(new_args.begin(), t1.begin(), t1.end());

			const Mention* objMention = EIUtils::findMentionForRelationArg(docData, so.second);
			std::vector<ElfRelationArg_ptr> t2 = EIUtils::createRelationArgFromMention(docData, obj_str+L"BelongsToHumanOrganization", so.second->get_type()->get_value(), objMention);
			new_args.insert(new_args.begin(), t2.begin(), t2.end());

			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(L"eru:BelongsToHumanOrganization", 
				new_args, r_text, relation->get_start(), relation->get_end(), relation->get_confidence(), relation->get_score_group());
			new_relation->set_source(relation->get_source());
			new_relation->add_source(L"eru:copyRelationAndSubstituteArgs");
			new_relation->add_source(L"eru:transformAffiliates");
			_relationsToAdd.insert(new_relation);
		}
	}
}

void KBPTemporalFilter::processAll(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> allRelations = docData->get_relations();
	const ValueSet* vSet = docData->getDocTheory()->getValueSet();
	std::vector<ElfRelation_ptr> deathRelations;
	std::vector<ElfRelation_ptr> birthRelations;
	BOOST_FOREACH(ElfRelation_ptr relation, allRelations){
		std::set<ElfRelationArg_ptr> temporalArgs;
		std::set<ElfRelationArg_ptr> nonTemporalArgs;
		std::set<ElfRelationArg_ptr> subjArgs; //include hasDecedent and hasPersonBorn for convenience
		std::set<ElfRelationArg_ptr> objArgs;
		std::set<ElfRelationArg_ptr> goodTemporalArgs;
		std::set<ElfRelationArg_ptr> underSpecifiedTemporalArgs;
		std::set<ElfRelationArg_ptr> futureTemporalArgs;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){

			if(arg->get_role().find(L"t:") == 0){
				temporalArgs.insert(arg);
			}
			else{
				nonTemporalArgs.insert(arg);
				/*
				if(arg->get_role().find(L"eru:object") == 0){
					objArgs.insert(arg);
				}
				else if((arg->get_role().find(L"eru:subject") == 0) ||
					(arg->get_role().find(L"eru:hasDecedent") == 0) ||
					(arg->get_role().find(L"eru:hasPersonBorn") == 0))
				{
					subjArgs.insert(arg);
				}
				else{
					subjArgs.insert(arg);
				}*/
			}
		}
		bool deletedRelation = false;
		/*
		//MRF: This seems to hurt as much as it helps
		std::set< std::wstring > futureConditional = makeStrSet(L"Future Negative Modal If Unreliable");

		if((relation->get_source().find(L"bbn:learnit") != std::wstring::npos) && (relation->get_source().find(L"bbn:manual") == std::wstring::npos)){
			//learned relations frequently have future/conditional

			EDTOffset start = (*nonTemporalArgs.begin())->get_start();
			EDTOffset end = (*nonTemporalArgs.begin())->get_end();
			int sentNo = 0;
			BOOST_FOREACH(ElfRelationArg_ptr arg, nonTemporalArgs){
				int t = EIUtils::getSentenceNumberForArg( docData->getDocTheory(), (*nonTemporalArgs.begin()));
				if(t > 0){
					sentNo = t; 
					start = std::min(arg->get_start(), start);
					end =  std::max(arg->get_end(), end);
				}
			}
			const SentenceTheory* sTheory = docData->getDocTheory()->getSentenceTheory(sentNo);
			const PropositionSet* pSet = sTheory->getPropositionSet();
			const MentionSet* mSet = sTheory->getMentionSet();
			const TokenSequence* toks = sTheory->getTokenSequence();
			for(int i =0; i< pSet->getNPropositions(); i++){
				const Proposition* p = pSet->getProposition(i);		 
				int pStart = sTheory->getTokenSequence()->getNTokens()+1;
				int pEnd = -1;
				p->getStartEndTokenProposition(mSet, pStart, pEnd);
				EDTOffset pEDTStart = toks->getToken(pStart)->getStartEDTOffset();
				EDTOffset pEDTEnd = toks->getToken(pEnd)->getEndEDTOffset();
				if((pEDTStart>= start && pEDTEnd <=end) || 
					(start>= pEDTStart && end <= pEDTEnd)){
					std::wstring status = pSet->getProposition(i)->getStatus().toString();
					if(futureConditional.find(status) != futureConditional.end()){
						std::wcout<<"Found Conditional: Status: "<<status<<std::endl;
						deletedRelation = true;
						ret = ret | handleRemoveRelation(true, relation);
						break;
					}
				}
			}
		}
		*/
		//std::cout<<"2 KBPTemporalFilter() temporal arg size: "<<temporalArgs.size()<<std::endl;
		//Does this relation contain temporal arguments
		if(temporalArgs.size() == 0 || deletedRelation )
			continue;
		//MRF: This is a hack, for attchments that come from the learned attachment module, we want to remove the date but not the relation
		//     currently, such temporal arguments have a value, while arguments from the predicate attchment code have a date range
		bool isLearnedAttachment = false; 
		//remove future dates
		const DocTheory* docTheory = docData->getDocTheory();
		BOOST_FOREACH(ElfRelationArg_ptr arg, temporalArgs){
			const SentenceTheory* sTheory = docTheory->getSentenceTheory(EIUtils::getSentenceNumberForArg(docTheory, arg));
			const ValueMentionSet* vMentionSet = sTheory->getValueMentionSet();
			ElfIndividual_ptr tempInd = arg->get_individual();
			ValueMention* v1 = 0;
			ValueMention* v2 = 0;
			bool v1IsFuture = false;
			bool v2IsFuture = false;
			if(tempInd->has_date_range()){
				if(tempInd->get_date_range().first != -1)
					v1 = vMentionSet->getValueMention(ValueMentionUID(tempInd->get_date_range().first));
				
				if(tempInd->get_date_range().second != -1)
					v2 = vMentionSet->getValueMention(ValueMentionUID(tempInd->get_date_range().second));
			}
			if(tempInd->has_value_mention_id()){
				v1 = vMentionSet->getValueMention(ValueMentionUID(tempInd->get_value_mention_id()));
			}
			if(v1){
				v1IsFuture = EIUtils::isFutureDate(docTheory, v1->getDocValue());
			}
			if(v2){
				v2IsFuture = EIUtils::isFutureDate(docTheory, v2->getDocValue());
			}
			if(v1IsFuture || v2IsFuture){
				if(arg->get_individual()->has_value_mention_id())
					isLearnedAttachment = true;
				futureTemporalArgs.insert(arg);
			}
		}
		//second loop to decide what to keep (e.g. everything that didn't get put somewhere else
		BOOST_FOREACH(ElfRelationArg_ptr arg, temporalArgs){
			if(futureTemporalArgs.find(arg) != futureTemporalArgs.end())
				continue;
			goodTemporalArgs.insert(arg);			
		}
		//are all of the temporal args good temporal args
		std::set<ElfRelationArg_ptr> allTemporalArgs(temporalArgs);
		if(allTemporalArgs.size() != goodTemporalArgs.size()){
			if(goodTemporalArgs.size() != 0 || futureTemporalArgs.size() == 0 || isLearnedAttachment){
				//otherwise: since there is only future information, we don't keep anything
				//delete old relation, make new relation with only good temporal agrs
				std::vector<ElfRelationArg_ptr> argsToInclude;
				argsToInclude.insert(argsToInclude.begin(), goodTemporalArgs.begin(), goodTemporalArgs.end());
				argsToInclude.insert(argsToInclude.begin(), nonTemporalArgs.begin(), nonTemporalArgs.end());
				//argsToInclude.insert(argsToInclude.begin(), subjArgs.begin(), subjArgs.end());
				//argsToInclude.insert(argsToInclude.begin(), objArgs.begin(), objArgs.end());
				ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
					argsToInclude, relation->get_text(), relation->get_start(), relation->get_end(), 
					relation->get_confidence(), relation->get_score_group());
				new_relation->set_source(relation->get_source());
				new_relation->add_source(L"eru:removeBadTemporal");
				_relationsToAdd.insert(new_relation);
			}
			_relationsToRemove.insert(relation);
		}
	}
}

void KBPConflictingDateTemporalFilter::processAll(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> allRelations = docData->get_relations();
	const ValueSet* vSet = docData->getDocTheory()->getValueSet();
	std::vector<ElfRelation_ptr> deathRelations;
	std::vector<ElfRelation_ptr> birthRelations;
	//Remove temporal information that is also a birth/death date
	set<int> valueMentionIds;
	std::map<int, int> valueMentionIdCounts;
	//If a relation has multiple dates, only allow it if 1 is clipped forward and the other is clipped backwards (otherwise they are usually wrong)
	BOOST_FOREACH(ElfRelation_ptr relation, allRelations){
		bool vmToWatch = false;
		int nVargs = 0;
		std::vector<ElfRelationArg_ptr> temporalArgs;
		std::vector<ElfRelationArg_ptr> nonTemporalArgs;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
			if(arg->get_role().find(L"t:") == 0){
				temporalArgs.push_back(arg);
			} 
			else{
				nonTemporalArgs.push_back(arg);
			}	
		}
		bool removeArgs = false;
		if(temporalArgs.size() > 2)
			removeArgs = true;
		if(temporalArgs.size() == 2){
			if(!((temporalArgs[0]->get_role() == L"t:ClippedForward" && temporalArgs[1]->get_role() == L"t:ClippedBackward") || 
				(temporalArgs[1]->get_role() == L"t:ClippedForward" && temporalArgs[0]->get_role() == L"t:ClippedBackward"))){
					removeArgs = true;
			}
		}
		if(removeArgs){
			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
				nonTemporalArgs, relation->get_text(), relation->get_start(), relation->get_end(), 
				relation->get_confidence(), relation->get_score_group());
			new_relation->set_source(relation->get_source());
			new_relation->add_source(L"ProblemDates");
			_relationsToAdd.insert(new_relation);
			_relationsToRemove.insert(relation);
		}
	}
	/*
	BOOST_FOREACH(ElfRelation_ptr relation, allRelations){
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
			if(arg->get_individual() != NULL && arg->get_individual()->has_value_mention_id()){
				int vmid = (arg->get_individual()->get_value_mention_id());
				valueMentionIds.insert(vmid);
				if(valueMentionIdCounts.find(vmid) == valueMentionIdCounts.end())
					valueMentionIdCounts[vmid] = 1;
				else
					valueMentionIdCounts[vmid] += 1;
			}
		}
	}
	BOOST_FOREACH(ElfRelation_ptr relation, allRelations){
		bool vmToWatch = false;
		int nVargs = 0;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
			if(arg->get_individual() != NULL && arg->get_individual()->has_value_mention_id()){
				int vmid = (arg->get_individual()->get_value_mention_id());
				if(valueMentionIdCounts.find(vmid) != valueMentionIdCounts.end() && valueMentionIdCounts[vmid] > 1)
					vmToWatch = true;
				nVargs+=1;
			}
		}
		if(vmToWatch || (nVargs > 1)){
			std::vector<ElfRelationArg_ptr> argsToKeep = relation->get_args();
			ElfRelationArg_ptr newarg = boost::make_shared<ElfRelationArg>(L"WarningArg", L"WarningType", L"DefaultValue", L"DefaultText");
			argsToKeep.push_back(newarg);
			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
				argsToKeep, relation->get_text(), relation->get_start(), relation->get_end(), 
				relation->get_confidence(), relation->get_score_group());
			new_relation->set_source(relation->get_source());
			new_relation->add_source(L"ProblemDates");
			_relationsToAdd.insert(new_relation);
			ret = ret | handleRemoveRelation(true, relation);			
		}
	}
	*/
}

bool containsYear(std::wstring s){
	const boost::wregex year_regex(L"([012][0-9][0-9][0-9])");
	return boost::regex_search(s, year_regex);
}
void KBPBirthDeathTemporalFilter::processAll(EIDocData_ptr docData){
	const DocTheory* docTheory = docData->getDocTheory();
	std::set<std::wstring> handledPersonArgs;
	BOOST_FOREACH(ElfRelation_ptr r, docData->get_relations()){
		if(r->get_name() != L"eru:DeathEvent")
			continue;
		std::vector<ElfRelationArg_ptr> datelessArgs;
		std::vector<ElfRelationArg_ptr> personArgs;
		std::vector<ElfRelationArg_ptr> dateArgs;
		std::vector<ElfRelationArg_ptr> predicateArgs;
		dateArgs = r->get_args_with_role(L"t:OccursAt");
		predicateArgs = r->get_args_with_role(L"KBP:TRIGGER");
		if(r->get_name()== L"eru:DeathEvent"){
			datelessArgs = r->get_args_with_role(L"eru:hasDatelessDecedent");
			personArgs =  r->get_args_with_role(L"eru:hasDecedent");
		}
		else if (r->get_name() == L"eru:BirthEvent"){
			datelessArgs = r->get_args_with_role(L"eru:hasDatelessPersonBorn");
			personArgs =  r->get_args_with_role(L"eru:hasPersonBorn");
		}
		std::wstring personURI = L"None";
		std::vector<ElfRelation_ptr> samePersonRelations;
		if(datelessArgs.size() == 0 && personArgs.size() == 1){
			if((*personArgs.begin())->get_individual() != NULL){
				personURI = (*personArgs.begin())->get_individual()->get_best_uri();
			}
		} 
		else if(datelessArgs.size() == 1 && personArgs.size() == 0){
			if((*datelessArgs.begin())->get_individual() != NULL){
				personURI = (*datelessArgs.begin())->get_individual()->get_best_uri();
			}
		}
		if(handledPersonArgs.find(personURI) != handledPersonArgs.end()){
			continue;
		}
		handledPersonArgs.insert(personURI);
		BOOST_FOREACH(ElfRelation_ptr other, docData->get_relations()){
			if(other->get_name() == r->get_name()){
				std::vector<ElfRelationArg_ptr> args = other->get_args();
				BOOST_FOREACH(ElfRelationArg_ptr arg, args){
					if((arg->get_individual() != NULL) && ((arg->get_individual()->get_best_uri() == personURI))){
						samePersonRelations.push_back(other);
					}
				}
			}
		}
		//split samePersonRelations into (a) relations with resolvable dates; (b) relations withun resolveable dates; (c) dateless relations
		std::vector<ElfRelation_ptr> goodDateRelations;
		std::vector<ElfRelation_ptr> badDateRelations;
		std::vector<ElfRelation_ptr> datelessRelations;
		BOOST_FOREACH(ElfRelation_ptr other, samePersonRelations){
			std::vector<ElfRelationArg_ptr> dateArgs = other->get_args_with_role(L"t:OccursAt");
			std::vector<ElfRelationArg_ptr> datelessPersonArgs;
			if(other->get_name()== L"eru:DeathEvent"){
				datelessPersonArgs = other->get_args_with_role(L"eru:hasDatelessDecedent");
			}
			else if (other->get_name() == L"eru:BirthEvent"){
				datelessPersonArgs = other->get_args_with_role(L"eru:hasDatelessPersonBorn");
			}
			if(datelessPersonArgs.size() > 0){
				datelessRelations.push_back(other);
			}
			else{
				bool foundDate = false;
				BOOST_FOREACH(ElfRelationArg_ptr arg, dateArgs){
					if(arg->get_individual() && arg->get_individual()->has_value_mention_id()){	
						ValueMentionUID id = ValueMentionUID(arg->get_individual()->get_value_mention_id());
						const SentenceTheory* sTheory = docTheory->getSentenceTheory(id.sentno());
						const ValueMentionSet* vmSet = sTheory->getValueMentionSet();
						const ValueMention* vm = vmSet->getValueMention(id);
						std::wstring dateString = EIUtils::getKBPSpecString(docTheory, vm);
						if(containsYear(dateString)){
							goodDateRelations.push_back(other);
							foundDate = true;
						}
					}
				}
				if(!foundDate){
					badDateRelations.push_back(other);
				}
			}
		}
		bool handledRelation =  false;
		if(goodDateRelations.size() > 0){	//we've found good dates, so remove any extra bad dates and remove all other relations
			BOOST_FOREACH(ElfRelation_ptr other, datelessRelations){ //remove dateless versions
				//ret = ret | handleRemoveRelation(true, other);
				_relationsToRemove.insert(other);
			}
			BOOST_FOREACH(ElfRelation_ptr other, badDateRelations){ //remove bad date versions
				//ret = ret | handleRemoveRelation(true, other);
				_relationsToRemove.insert(other);
			}			
			BOOST_FOREACH(ElfRelation_ptr other, goodDateRelations){ //select only the good dates
				std::vector<ElfRelationArg_ptr> argsToKeep;
				if(other->get_name()== L"eru:DeathEvent"){
					argsToKeep =  other->get_args_with_role(L"eru:hasDecedent");
				}
				else if (other->get_name() == L"eru:BirthEvent"){
					argsToKeep =  other->get_args_with_role(L"eru:hasPersonBorn");
				}
				std::vector<ElfRelationArg_ptr> dateArgs = other->get_args_with_role(L"t:OccursAt");
				BOOST_FOREACH(ElfRelationArg_ptr arg, dateArgs){
					if(arg->get_individual() && arg->get_individual()->has_value_mention_id()){	
						ValueMentionUID id = ValueMentionUID(arg->get_individual()->get_value_mention_id());
						const SentenceTheory* sTheory = docTheory->getSentenceTheory(id.sentno());
						const ValueMentionSet* vmSet = sTheory->getValueMentionSet();
						const ValueMention* vm = vmSet->getValueMention(id);
						std::wstring dateString = EIUtils::getKBPSpecString(docTheory, vm);
						if(containsYear(dateString)){
							arg->get_individual()->set_value(dateString);
							argsToKeep.push_back(arg);
						}
					}
				}
				//make a copy of the argsToKeep to be sure that deleting them doesn't cause problems
				std::vector<ElfRelationArg_ptr> copiedArgsToKeep;
				BOOST_FOREACH(ElfRelationArg_ptr arg, argsToKeep){
					copiedArgsToKeep.push_back(boost::make_shared<ElfRelationArg>(arg));
				}
				ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(other->get_name(), 
					copiedArgsToKeep, other->get_text(), other->get_start(), other->get_end(), other->get_confidence(), other->get_score_group());
				new_relation->set_source(other->get_source());
				new_relation->set_source(L"EIFilter-resolvedDates");
				_relationsToAdd.insert(new_relation);
				//ret = ret | handleRemoveRelation(true, other);
				_relationsToRemove.insert(other);
				handledRelation = true;
			}	
		}
		else if(datelessRelations.size() > 0){
			BOOST_FOREACH(ElfRelation_ptr other, datelessRelations){
				std::vector<ElfRelationArg_ptr> personToKeepArgs;
				if(other->get_name()== L"eru:DeathEvent"){
					personToKeepArgs = other->get_args_with_role(L"eru:hasDatelessDecedent");
				}
				else if (other->get_name() == L"eru:BirthEvent"){
					personToKeepArgs = other->get_args_with_role(L"eru:hasDatelessPersonBorn");
				}
				if(personToKeepArgs.size() != 1){
					_relationsToRemove.insert(other);
					continue;
				}
				if((*personToKeepArgs.begin())->get_individual() == NULL){
					_relationsToRemove.insert(other);
					continue;
				}
				MentionUID muid = MentionUID((*personToKeepArgs.begin())->get_individual()->get_mention_uid());
				const SentenceTheory* sentTheory = docTheory->getSentenceTheory(muid.sentno());
				const MentionSet* mSet = sentTheory->getMentionSet();
				const ValueMentionSet* vmSet = sentTheory->getValueMentionSet();
				std::vector<int> dates;
				for(int i =0; i< vmSet->getNValueMentions(); i++){
					if(vmSet->getValueMention(i)->isTimexValue()){
						std::wstring dateString = EIUtils::getKBPSpecString(docTheory, vmSet->getValueMention(i));
						if(containsYear(dateString)){
							dates.push_back(i);
						}
					}
				}
			
				ElfIndividual_ptr individual;
				ElfRelationArg_ptr temporalArg;
				std::wstring defaultDateString = L"[,]";
				std::wstring dateString = defaultDateString;
				if(dates.size() == 1){	//create date individual from the date that was found 
					individual = boost::make_shared<ElfIndividual>(docTheory, L"xsd:string", vmSet->getValueMention(dates[0]));
					dateString = EIUtils::getKBPSpecString(docTheory, vmSet->getValueMention(dates[0]));
					individual->set_value(dateString);
					temporalArg = boost::make_shared<ElfRelationArg>(L"t:OccursAt", individual);
					_individualsToAdd.insert(individual);
				}
				if(dateString == defaultDateString || !containsYear(dateString)){ //no good date, so just assume publication
					std::wstring origDocID = docTheory->getDocument()->getName().to_string();
					std::wstring dociddate = origDocID.substr(8,8).c_str();
					std::wstring YYYY = dociddate.substr(0,4);
					std::wstring MM = dociddate.substr(4,2);
					std::wstring DD = dociddate.substr(6,2);
					dateString = L"[," + YYYY + L"-" + MM + L"-" + DD +L"]";
					individual = boost::make_shared<ElfIndividual>(L"xsd:string", dateString, dateString, 
						sentTheory->getTokenSequence()->getToken(0)->getStartEDTOffset(),
						sentTheory->getTokenSequence()->getToken(sentTheory->getTokenSequence()->getNTokens()-1)->getEndEDTOffset());
					individual->set_value(dateString); //might be redundant
					temporalArg = boost::make_shared<ElfRelationArg>(L"t:OccursWithin", individual);
					_individualsToAdd.insert(individual);
				}	
				ElfIndividual_ptr personInd = boost::make_shared<ElfIndividual>((*personToKeepArgs.begin())->get_individual());
				ElfRelationArg_ptr person = boost::make_shared<ElfRelationArg>(L"eru:hasDecedent", personInd);
				std::vector<ElfRelationArg_ptr> args;
				args.push_back(temporalArg);
				args.push_back(person);
				ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(other->get_name(), 
					args, other->get_text(), other->get_start(), other->get_end(), 
					other->get_confidence(), other->get_score_group());
				new_relation->set_source(other->get_source());
				new_relation->add_source(L"EIFilter-addedDate");
				_relationsToAdd.insert(new_relation);
				_individualsToAdd.insert(personInd);
				//ret = ret | handleRemoveRelation(true, other);
				_relationsToRemove.insert(other);
				handledRelation = true;
			}
		}
		if(!handledRelation && badDateRelations.size() > 0){
			BOOST_FOREACH(ElfRelation_ptr other, badDateRelations){ //select only the good dates
				std::vector<ElfRelationArg_ptr> argsToKeep;
				if(other->get_name()== L"eru:DeathEvent"){
					argsToKeep =  other->get_args_with_role(L"eru:hasDecedent");
				}
				else if (other->get_name() == L"eru:BirthEvent"){
					argsToKeep =  other->get_args_with_role(L"eru:hasPersonBorn");
				}
				std::wstring origDocID = docTheory->getDocument()->getName().to_string();
				std::wstring dociddate = origDocID.substr(8,8).c_str();
				std::wstring YYYY = dociddate.substr(0,4);
				std::wstring MM = dociddate.substr(4,2);
				std::wstring DD = dociddate.substr(6,2);
				std::wstring dateString = L"[," + YYYY + L"-" + MM + L"-" + DD +L"]";
				ElfIndividual_ptr individual = boost::make_shared<ElfIndividual>(L"xsd:string", dateString, dateString, 
					other->get_start(), other->get_end());
				individual->set_value(dateString); //might be redundant
				ElfRelationArg_ptr temporalArg = boost::make_shared<ElfRelationArg>(L"t:OccursWithin", individual);
				_individualsToAdd.insert(individual);
					
				std::vector<ElfRelationArg_ptr> copiedArgsToKeep;
				copiedArgsToKeep.push_back(temporalArg);
				BOOST_FOREACH(ElfRelationArg_ptr arg, argsToKeep){
					copiedArgsToKeep.push_back(boost::make_shared<ElfRelationArg>(arg));
				}
				ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(other->get_name(), 
					copiedArgsToKeep, other->get_text(), other->get_start(), other->get_end(), 
					other->get_confidence(), other->get_score_group());
				new_relation->set_source(other->get_source());
				new_relation->add_source(L"EIFilter-addedDate");
				_relationsToAdd.insert(new_relation);
				_relationsToRemove.insert(other);
				handledRelation = true;
			}
		}
	}
}

void KBPConflictingBirthDeathTemporalFilter::processAll(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> allRelations = docData->get_relations();
	const ValueSet* vSet = docData->getDocTheory()->getValueSet();
	std::vector<ElfRelation_ptr> deathRelations;
	std::vector<ElfRelation_ptr> birthRelations;
	//Remove temporal information that is also a birth/death date
	set<int> birthDeathValueMentionIds;
	set<int> deathOnlyValueMentionIds;
	BOOST_FOREACH(ElfRelation_ptr relation, allRelations){
		if((relation->get_name() == L"eru:BirthEvent") || (relation->get_name() == L"eru:DeathEvent")){
			ElfRelationArg_ptr arg = relation->get_arg(L"t:OccursAt");
			if(arg == NULL)
				arg = relation->get_arg(L"OccursWithin");
			if(arg == NULL){
				std::wcout<<"EIFilter::KBPConflictingBirthDeathTemporalFilter() Warning: No t:OccursAt role for DeathEvent/BirthEvent (possibly removed as future): "
					<<relation->get_text()<<std::endl;
				continue;
			}
			if(arg->get_individual() != NULL && arg->get_individual()->has_value_mention_id()){
				birthDeathValueMentionIds.insert(arg->get_individual()->get_value_mention_id());
			}
		}
	}
	if(birthDeathValueMentionIds.size() == 0)
		return;
	BOOST_FOREACH(ElfRelation_ptr relation, allRelations){
		if((relation->get_name() == L"eru:BirthEvent") || (relation->get_name() == L"eru:DeathEvent"))
			continue;
		std::vector<ElfRelationArg_ptr> argsToKeep;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
			if(arg->get_individual()->has_value_mention_id()){
				int value_mention_id =  arg->get_individual()->get_value_mention_id();
				if(birthDeathValueMentionIds.find(value_mention_id) == birthDeathValueMentionIds.end()){
					argsToKeep.push_back(arg);
				}
			}else{
				argsToKeep.push_back(arg);
			}
		}
		if(argsToKeep.size() != relation->get_args().size()){
			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
				argsToKeep, relation->get_text(), relation->get_start(), relation->get_end(), 
				relation->get_confidence(), relation->get_score_group());
			new_relation->set_source(relation->get_source());
			new_relation->add_source(L"RemoveBirthDeathDate");
			_relationsToAdd.insert(new_relation);
			_relationsToRemove.insert(relation);		
		}
	}
}

void InferHoldsThroughout::processAll(EIDocData_ptr docData) {
	using namespace boost::gregorian;
	std::set<ElfRelation_ptr> allRelations = docData->get_relations();

	static const Symbol elected(L"elected");
	static const Symbol effective(L"effective");

	// ClippedBackwards events in election sentences are unreliabled
	BOOST_FOREACH(ElfRelation_ptr rel, allRelations) {
		const wstring name = rel->get_name();
		if (name == L"eru:HasEmployer" 
				|| name == L"eru:BelongsToHumanOrganization"
				|| name == L"eru:HasTopMemberOrEmployee"
				|| name == L"eru:PersonTitleInOrganization")
		{
			BOOST_FOREACH(ElfRelationArg_ptr arg, rel->get_args()) {
				if (arg->get_role() == L"t:ClippedBackward") {
					if (EIUtils::argSentenceContainsWord(arg, docData, elected)
							&& !EIUtils::argSentenceContainsWord(arg, docData, effective)) {
						rel->add_source(L"deleted-ClippedBackward-in-elected-sentence");
						ElfIndividual_ptr indiv = arg->get_individual();
						if (indiv) {
							date_period d = EIUtils::DatePeriodFromValue(indiv->get_value());
							if (!d.is_null()) {
								wstringstream str;
								str << L"[[" << TimexUtils::dateToTimex(d.begin()) 
									<< L",],]";
								indiv->set_value(str.str());
								arg->set_role(L"t:HoldsThroughout");
							}
						}
					}
				}
			}
		}
	}

	BOOST_FOREACH(ElfRelation_ptr rel1, allRelations){
		BOOST_FOREACH(ElfRelation_ptr rel2, allRelations){
			if (rel1 == rel2) {
				continue;
			}
			if (rel1->get_name() == rel2->get_name() &&
					nonTemporalArgumentsMatch(rel1, rel2)) 
			{
				copyTemporalArguments(docData, rel1, rel2);
			}
		}
	}

	BOOST_FOREACH(ElfRelation_ptr rel1, allRelations) {
		ElfRelationArg_ptr arg = rel1->get_arg(L"t:HoldsAt");
		if (arg) {
			ElfIndividual_ptr indiv = arg->get_individual();
			if (indiv) {
				date_period dp = TimexUtils::datePeriodFromTimex(indiv->get_value());
				if (!dp.is_null() && dp.length() > date_duration(1)) {
					ElfRelationArg_ptr replacement =
						boost::make_shared<ElfRelationArg>(arg);
					replacement->set_role(L"t:HoldsWithin");
					rel1->remove_argument(arg);
					rel1->insert_argument(replacement);
					rel1->add_source(L"non-specific-HoldsAt-to-HoldsWithin");
				}
			}
		}

		ElfRelationArg_ptr cb = rel1->get_arg(L"t:ClippedBackward");
		ElfRelationArg_ptr cf = rel1->get_arg(L"t:ClippedForward");
		//ElfRelationArg_ptr hw = rel1->get_arg(L"t:HoldsWithin");

		// if a relation has ClippedBackward and ClippedForward
		// but not HoldsWithin, construct a HoldsWithin from their bounds
		if (cb && cf ) {
			ElfIndividual_ptr cb_i = cb->get_individual();
			ElfIndividual_ptr cf_i = cf->get_individual();
			if (cb_i && cf_i) {
				const std::wstring cb_v = cb_i->get_value();
				const std::wstring cf_v = cf_i->get_value();
				bool bad_clipping = false;

				EIUtils::DatePeriodPair newHWSpan =
					calcHWSpan(cb_v,cf_v, bad_clipping);
				if (bad_clipping) {
					// in my test set, every time there is a conflict
					// between ClippedForward and ClippedBackward,
					// ClippedForward wins, so go with it for now. ~ RMG
					bool removed = false;
					BOOST_FOREACH(ElfRelationArg_ptr badArg, 
							rel1->get_args_with_role(L"t:ClippedBackward")) 
					{
						rel1->remove_argument(badArg);
						removed = true;
					}
					if (removed) {
						rel1->add_source(L"clipping-conflict-dropped-clippedBackwards");
					}
				} else {
					if (!newHWSpan.first.is_null() 
							|| !newHWSpan.second.is_null())
					{
						EDTOffset cb_start, cb_end;
						EDTOffset cf_start, cf_end;

						cb_i->get_spanning_offsets(cb_start, cb_end);
						cf_i->get_spanning_offsets(cf_start, cf_end);

						EDTOffset new_start = (std::min)(cb_start, cf_start);
						EDTOffset new_end = (std::max)(cb_end, cf_end);
						int cb_sent = EIUtils::getSentenceNumberForIndividualStart(docData->getDocTheory(), cb_i);
						int cf_sent = EIUtils::getSentenceNumberForIndividualStart(docData->getDocTheory(), cf_i);

						const TokenSequence* ts = docData->getDocTheory()->getSentenceTheory(cb_sent)->getTokenSequence();
						EDTOffset cb_sent_start = ts->getToken(0)->getStartEDTOffset();
						EDTOffset cb_sent_end = ts->getToken(ts->getNTokens()-1)->getEndEDTOffset();

						ts = docData->getDocTheory()->getSentenceTheory(cf_sent)->getTokenSequence();
						EDTOffset cf_sent_start = ts->getToken(0)->getStartEDTOffset();
						EDTOffset cf_sent_end = ts->getToken(ts->getNTokens()-1)->getEndEDTOffset();

						LocatedString* mention_text = 
							MainUtilities::substringFromEdtOffsets(
									docData->getDocTheory()->getDocument()->getOriginalText(), 
									cb_sent_start, cb_sent_end);
						std::wstring cb_text = std::wstring(mention_text->toString());
						delete mention_text; 

						mention_text = 
							MainUtilities::substringFromEdtOffsets(
									docData->getDocTheory()->getDocument()->getOriginalText(), 
									cf_sent_start, cf_sent_end);
						std::wstring cf_text = std::wstring(mention_text->toString());
						delete mention_text; 

						std::wstringstream txt;
						txt << cb_text << L"..." << cf_text;

						ElfIndividual_ptr newIndiv = EIUtils::dateIndividualFromSpecString(
								EIUtils::DatePeriodPairToKBPSpecString(newHWSpan), 
								txt.str(), new_start, new_end);
						_individualsToAdd.insert(newIndiv);
						ElfRelationArg_ptr newArg = boost::make_shared<ElfRelationArg>(
								L"t:HoldsThroughout", newIndiv);
						rel1->insert_argument(newArg);
						rel1->add_source(L"holds-throughout-from-clipping");
					}
				}
			}
		}
	}
}

EIUtils::DatePeriodPair InferHoldsThroughout::calcHWSpan(
		const std::wstring& cb, const std::wstring& cf, bool& bad_clipping)
{
	using namespace boost::gregorian;
	static const date_period NULL_DATE_PERIOD(date(2000,Jan,2),date(2000,Jan,1));
	bad_clipping = false;
	date_period start_period = NULL_DATE_PERIOD;
	date_period end_period = NULL_DATE_PERIOD;

	date_period cb_dp = EIUtils::DatePeriodFromValue(cb);
	if (!cb_dp.is_null()) {
		start_period = cb_dp;
	} else {
		EIUtils::DatePeriodPair cb_dpp = EIUtils::DatePeriodPairFromKBPSpecString(cb);
		if (!cb_dpp.first.is_null()) {
			start_period = cb_dpp.first;
		} else {
			bad_clipping = true;
		}
	}
	
	date_period cf_dp = EIUtils::DatePeriodFromValue(cf);
	if (!cf_dp.is_null()) {
		end_period = cf_dp;
	} else {
		EIUtils::DatePeriodPair cf_dpp = EIUtils::DatePeriodPairFromKBPSpecString(cf);
		if (!cf_dpp.first.is_null()) {
			end_period = cf_dpp.second;
		} else {
			bad_clipping = true;
		}
	}

	if (!start_period.is_null() && !end_period.is_null()) {
		if (!(start_period < end_period)) {
			bad_clipping = true;
		}
	} else {
		bad_clipping = true;
	}

	return make_pair(start_period, end_period);
}

bool InferHoldsThroughout::nonTemporalArgumentsMatch(
		ElfRelation_ptr rel1, ElfRelation_ptr rel2)
{
	std::vector<ElfRelationArg_ptr> args1, args2;

	args1 = nonTemporalArguments(rel1);
	args2 = nonTemporalArguments(rel2);

	return !args1.empty()
		&& containsArguments(args1, args2) && containsArguments(args2,args1);
}

bool InferHoldsThroughout::containsArguments(std::vector<ElfRelationArg_ptr>& args1,
		std::vector<ElfRelationArg_ptr>& args2)
{
	bool ret = false;
	if (args1.size() == args2.size()) {
		ret = true;
		BOOST_FOREACH(ElfRelationArg_ptr arg1, args1) {
			bool found_match = false;
			ElfIndividual_ptr indiv1 = arg1->get_individual();
			if (indiv1) {
				BOOST_FOREACH(ElfRelationArg_ptr arg2, args2) {
					if (arg1->get_role() == arg2->get_role()) {
						ElfIndividual_ptr indiv2 = arg2->get_individual();
						if (indiv2 
								&& indiv1->get_best_uri() == indiv2->get_best_uri())
						{
							found_match = true;
							break;
						}
					}
				}
			}
			if (!found_match) {
				ret = false;
				break;
			}
		}
	}

	return ret;
}

void InferHoldsThroughout::copyTemporalArguments(EIDocData_ptr docData,
		ElfRelation_ptr rel1, ElfRelation_ptr rel2) 
{
	std::vector<ElfRelationArg_ptr> tmps1;
	std::vector<ElfRelationArg_ptr> tmps2;

	tmps1 = temporalArguments(rel1);
	tmps2 = temporalArguments(rel2);

	if(addMissingArguments(tmps1,tmps2,rel2)) {
		rel2->add_source(L"received-copied-temporals");
	}

	if (addMissingArguments(tmps2,tmps1,rel1)) {
		rel1->add_source(L"received-copied-temporals");
	}
}

bool InferHoldsThroughout::addMissingArguments(
		const std::vector<ElfRelationArg_ptr> sourceArgs,
		const std::vector<ElfRelationArg_ptr> destArgs,
		ElfRelation_ptr destRel)
{
	bool copied = false;
	BOOST_FOREACH(ElfRelationArg_ptr sourceArg, sourceArgs) {
		ElfIndividual_ptr sourceIndiv = sourceArg->get_individual();
		if (sourceIndiv) {
			bool found = false;
			BOOST_FOREACH(ElfRelationArg_ptr destArg, destArgs)  {
				if (sourceArg->get_role() == destArg->get_role()) {
					ElfIndividual_ptr destIndiv = destArg->get_individual();
					if (destIndiv && sourceIndiv->get_value() == destIndiv->get_value()) {
						found = true;
						break;
					}
				}
			}
			if (!found) {
				copied = true;
				ElfRelationArg_ptr addArg = boost::make_shared<ElfRelationArg>(
						sourceArg);
				destRel->insert_argument(addArg);
			}
		}
	}

	return copied;
}

std::wstring InferHoldsThroughout::stringRep(EIDocData_ptr docData,
		ElfRelationArg_ptr arg) 
{
	ElfIndividual_ptr indiv = arg->get_individual();
	const DocTheory* dt = docData->getDocTheory();

	if (indiv) {
		std::wstringstream ret;
		if (indiv->has_value_mention_id()) {
			const SentenceTheory* sTheory = dt->getSentenceTheory(
					EIUtils::getSentenceNumberForArg(dt, arg));
			const ValueMentionSet* vms = sTheory->getValueMentionSet();
			/*const ValueMention* vm = vms->getValueMention(indiv->get_value_mention_id());
			const Value* val = vm->getDocValue();*/

			ret << arg->get_role() <<  L": VM(" << indiv->get_value() << L")";
			//ret << L"VM(" << val->getTimexVal().to_string() << L")";
		} else {
			ret << arg->get_role() << L": Other(" << indiv->get_value() << L")";
		}
		return ret.str();
	} else {
		return L"null";
	}
}

std::vector<ElfRelationArg_ptr> InferHoldsThroughout::temporalArguments(ElfRelation_ptr rel)
{
	return argsOfType(rel, true);
}

std::vector<ElfRelationArg_ptr> InferHoldsThroughout::nonTemporalArguments(ElfRelation_ptr rel) {
	return argsOfType(rel, false);
}

std::vector<ElfRelationArg_ptr> InferHoldsThroughout::argsOfType(
		ElfRelation_ptr rel, bool temporalOrNot) 
{
	std::vector<ElfRelationArg_ptr> ret;
	BOOST_FOREACH(ElfRelationArg_ptr arg, rel->get_args()) {
		if (arg->get_role().find(L"t:") != 0) {
			if (!temporalOrNot) {
				ret.push_back(arg);
			}
		} else if (temporalOrNot) {
			ret.push_back(arg);
		}
	}
	return ret;
}

bool preferTemporalArg1(ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2){
	if(!arg1->get_individual())
		return false;
	if(!arg2->get_individual())
		return false;
	if((arg1->get_individual()->get_value().find(L"[") == 0) && 
		(arg2->get_individual()->get_value().find(L"[") == std::wstring::npos))
	{
		return true;
	}
	return false;
}
bool alwaysPreferTemporalArg1(std::set<ElfRelationArg_ptr> args1, std::set<ElfRelationArg_ptr> args2){
	std::vector<ElfRelationArg_ptr> args1V(args1.begin(), args1.end());
	std::vector<ElfRelationArg_ptr> args2V(args2.begin(), args2.end());

	for(size_t i =0; i < args1V.size(); i++){
		for(size_t j = i+1; j < args2V.size(); j++){
			if(!preferTemporalArg1(args1V[i], args2V[j]))
				return false;
		}
	}
	return true;
}
void KBPMatchLearnedAndManualDatesFilter::processAll(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> allRelationsSet = docData->get_relations();
	std::vector<ElfRelation_ptr> allRelations(allRelationsSet.begin(), allRelationsSet.end());
	//really want a nested for loop here
	for(size_t i =0; i < allRelations.size(); i++){
		ElfRelation_ptr r1 = allRelations[i];
		std::set<ElfRelationArg_ptr> r1TemporalArgs;
		std::set<ElfRelationArg_ptr> r1OtherArgs;
		BOOST_FOREACH(ElfRelationArg_ptr arg, r1->get_args()){
			if(arg->get_role().find(L"t:") == 0){
				r1TemporalArgs.insert(arg);
			} else{
				r1OtherArgs.insert(arg);
			}
		}
		if(r1TemporalArgs.size() == 0)
			continue;
		for(size_t j = i+1; j < allRelations.size(); j++){
			ElfRelation_ptr r2 = allRelations[i];
			if(r1->get_name() != r2->get_name())
				continue;
			std::set<ElfRelationArg_ptr> r2TemporalArgs;
			std::set<ElfRelationArg_ptr> r2OtherArgs;
			BOOST_FOREACH(ElfRelationArg_ptr arg, r2->get_args()){
				if(arg->get_role().find(L"t:") == 0){
					r2TemporalArgs.insert(arg);
				} else{
					r2OtherArgs.insert(arg);
				}
			}
			if(r2TemporalArgs.size()== 0)
				continue;
			if(r1OtherArgs.size() != r2OtherArgs.size())
				continue;
			std::set<ElfRelationArg_ptr> alignedArgs =EIUtils::alignArgSets(docData->getDocTheory(), r1OtherArgs, r2OtherArgs);
			if(alignedArgs.size() != r1OtherArgs.size())
				continue;
			bool alwaysPrefer1 = alwaysPreferTemporalArg1(r1TemporalArgs, r2TemporalArgs);
			if(alwaysPrefer1){
				_relationsToRemove.insert(r2);
			} else{
				bool alwaysPrefer2 = alwaysPreferTemporalArg1(r2TemporalArgs, r1TemporalArgs);
				if(alwaysPrefer2){
					_relationsToRemove.insert(r1);
				}
			}
		}
	}
}

void CopyPTIOEmploymentMembershipFilter::processRelation(
		EIDocData_ptr docData, ElfRelation_ptr relation)
{
	// copies PersonInOrganization and PersonTitleInOrganization relations
	// to be IsAffiliateOf relations, which are then processed by later
	// into HasEmployer and BelongsToHumanOrganization relations

	bool copy = false;
	std::wstring personRole = L"";
	std::wstring orgRole = L"";
	std::wstring source = L"";

	if (relation->get_name() == L"eru:PersonTitleInOrganization"
			|| relation->get_name() == L"eru:PersonInOrganization")
	{
		copy = true;
		personRole = L"eru:personOfPTIO";
		orgRole = L"eru:organizationOfPTIO";
		source = L"copied-from-PTIOOrPIO";
	} else if (relation->get_name() == L"eru:HasTopMemberOrEmployee") {
		copy = true;
		personRole = L"eru:objectOfHasTopMemberOrEmployee";
		orgRole = L"eru:subjectOfHasTopMemberOrEmployee";
		source = L"copied-from-HasTopMemberOrEmployee";
	}
		
	if (copy) {
		std::vector<ElfRelationArg_ptr> newArgs;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			if (arg->get_role() == personRole) {
				ElfRelationArg_ptr newArg =
					boost::make_shared<ElfRelationArg>(arg);
				newArg->set_role(L"eru:subjectOfIsAffiliateOf");
				newArgs.push_back(newArg);
			} else if (arg->get_role() == orgRole) {
				ElfRelationArg_ptr newArg =
					boost::make_shared<ElfRelationArg>(arg);
				newArg->set_role(L"eru:objectOfIsAffiliateOf");
				newArgs.push_back(newArg);
			} else if (arg->get_role() == L"t:HoldsAt" 
					|| arg->get_role() == L"t:HoldsWithin"
					|| arg->get_role() == L"t:HoldsThroughout")
			{
				// we don't want to copy Clipping temporal relationships
				// because you may gain or lose a title within the
				// same organization
				newArgs.push_back(boost::make_shared<ElfRelationArg>(arg));
			}
		}
		ElfRelation_ptr newRel = boost::make_shared<ElfRelation>(
				L"eru:IsAffiliateOf", newArgs, relation->get_text(), relation->get_start(),
				relation->get_end(), relation->get_confidence(), 
				relation->get_score_group());
		newRel->add_source(source);
		_relationsToAdd.insert(newRel);
	}
}

void BoundTitleFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	BOOST_FOREACH(const ElfRelationArg_ptr& arg, relation->get_args()) {
		if (arg->get_role() != L"eru:titleOfPTIO") {
			continue;
		}
		ElfIndividual_ptr individual = arg->get_individual();
		ElfType_ptr old_type = individual->get_type();
		if (individual) {
			wstring lookup_string = arg->get_text();
		}
		if (individual && titleSubClass(individual->get_type())) { 
			wstring lookup_string = arg->get_text();
			std::transform(lookup_string.begin(), lookup_string.end(),
					lookup_string.begin(), ::tolower);
			if (!lookup_string.empty()) {
				ElfMultiDoc::BoundTypeURIs boundURIs = 
					ElfMultiDoc::title_bound_uri(old_type->get_value(), lookup_string);
				if (!boundURIs.empty()) {
					EDTOffset start, end;
					old_type->get_offsets(start, end);

					// if there are multiple possibilities...
					for (size_t i =0; i<boundURIs.size(); ++i) {
						ElfMultiDoc::BoundTypeURI typeURI = boundURIs[i];
						ElfType_ptr new_type = boost::make_shared<ElfType>(
								typeURI.type, old_type->get_string(), start, end);

						// replace the current type with the first one
						if (i == 0) {
							individual->set_type(new_type);
							individual->set_bound_uri(typeURI.uri);
						} else {
							// and add all the others to the document
							ElfIndividual_ptr new_indiv = 
								boost::make_shared<ElfIndividual>(individual);
							new_indiv->set_type(new_type);
							new_indiv->set_bound_uri(typeURI.uri);
							_individualsToAdd.insert(new_indiv);
						}
					}
				}
			}
		}
	}
}

bool BoundTitleFilter::titleSubClass(ElfType_ptr type) {
	wstring typeString = type->get_value();
	return L"kbp:Title" == typeString ||
		L"kbp:HeadOfCompanyTitle" == typeString
		|| L"kbp:HeadOfNationStateTitle" == typeString
		|| L"kbp:HeadOfCityTownOrVillageTitle" == typeString
		|| L"kbp:MinisterTitle" == typeString;
}

void DuplicateRelationFilter::processAll(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> allRelations = docData->get_relations();
	std::set<ElfRelation_ptr> coveredRelationsToRemove;
	std::vector<ElfRelation_ptr> newCoveringRelations;
	BOOST_FOREACH(ElfRelation_ptr relation1, allRelations){
		bool dump = false;
		if (relation1->get_args_with_role(L"eru:personOfPTIO").size() > 1) {
			SessionLogger::err("DuplicateRelationFilter") << L"BEFORE: eru:personOfPTIO has multiple args with that role!\n";
			dump = true;
		}
		if (relation1->get_args_with_role(L"eru:titleOfPTIO").size() > 1) {
			SessionLogger::err("DuplicateRelationFilter") << L"BEFORE: eru:titleOfPTIO has multiple args with that role!\n";
			dump = true;
		}
		if (relation1->get_args_with_role(L"eru:organizationOfPTIO").size() > 1) {
			SessionLogger::err("DuplicateRelationFilter") << L"BEFORE: eru:organizationOfPTIO has multiple args with that role!\n";
			dump = true;
		}
		if (dump)
			SessionLogger::err("DuplicateRelationFilter") << "BEFORE:" << relation1->toDebugString(0) << "\n";

		std::vector<ElfRelation_ptr> currentCoveringRelation;
		if(coveredRelationsToRemove.find(relation1) != coveredRelationsToRemove.end()){ //this relation will be removed, so don't let it serve as a cover
			continue;
		}
		std::vector<ElfRelation_ptr> coveringRelation;
		BOOST_FOREACH(ElfRelation_ptr relation2, allRelations){
			if(coveredRelationsToRemove.find(relation2) != coveredRelationsToRemove.end()){ //this relation will be removed, so don't let it serve as a cover
				continue;
			}
			if(currentCoveringRelation.size() == 0){ //relation1 is still an option, so check it
				coveringRelation = EIUtils::makeBestCoveringRelation(docData->getDocTheory(), relation1, relation2);
			} else{
				coveringRelation = EIUtils::makeBestCoveringRelation(docData->getDocTheory(), (*currentCoveringRelation.begin()), relation2);
			}
			if(coveringRelation.size() > 0){//new covering relation
				//std::wcout<<"found covering relation"<<std::endl;
				currentCoveringRelation.clear();
				currentCoveringRelation.push_back((*coveringRelation.begin()));
				coveredRelationsToRemove.insert(relation1);
				coveredRelationsToRemove.insert(relation2);
			}
		}
		if(currentCoveringRelation.size() > 0){
			(*currentCoveringRelation.begin())->add_source(L"COVERING_RELATION");
			newCoveringRelations.push_back((*currentCoveringRelation.begin()));
		}
	}
	//std::wcout<<"Duplicate Fitler: remove: "<<coveredRelationsToRemove.size()<<" REPLACE WITH "<<newCoveringRelations.size()<<std::endl;
	BOOST_FOREACH(ElfRelation_ptr relation, coveredRelationsToRemove){
		_relationsToRemove.insert(relation);
	}
	_relationsToAdd.insert(newCoveringRelations.begin(), newCoveringRelations.end());

	BOOST_FOREACH(ElfRelation_ptr relation, newCoveringRelations) {
		bool dump2 = false;
		if (relation->get_args_with_role(L"eru:personOfPTIO").size() > 1) {
			SessionLogger::err("DuplicateRelationFilter") << L"AFTER: eru:personOfPTIO has multiple args with that role!\n";
			dump2 = true;
		}
		if (relation->get_args_with_role(L"eru:titleOfPTIO").size() > 1) {
			SessionLogger::err("DuplicateRelationFilter") << L"AFTER: eru:titleOfPTIO has multiple args with that role!\n";
			dump2 = true;
		}
		if (relation->get_args_with_role(L"eru:organizationOfPTIO").size() > 1) {
			SessionLogger::err("DuplicateRelationFilter") << L"AFTER: eru:organizationOfPTIO has multiple args with that role!\n";
			dump2 = true;
		}
		if (dump2)
			SessionLogger::err("DuplicateRelationFilter") << "AFTER: " << relation->toDebugString(0) << "\n";
	}
}

void DuplicateArgFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	//std::wcout<<"Duplicate argument filter"<<std::endl;
	std::set<ElfRelationArg_ptr> argsToKeep;
	std::set<ElfRelationArg_ptr> argsToRemove;
	//first just make a set of the arguments
	std::vector<ElfRelationArg_ptr> argsVector = relation->get_args();
	std::set<ElfRelationArg_ptr> relationArgs(argsVector.begin(), argsVector.end());
	BOOST_FOREACH(ElfRelationArg_ptr arg1, relationArgs){
		if((argsToKeep.find(arg1) != argsToKeep.end()) ||(argsToRemove.find(arg1) != argsToRemove.end()))
			continue;
		std::vector<ElfRelationArg_ptr> otherArgs = relation->get_args_with_role(arg1->get_role());
		if(otherArgs.size() == 1)
			continue;
		BOOST_FOREACH(ElfRelationArg_ptr arg2, otherArgs){
			if((arg1 == arg2) || (argsToKeep.find(arg2) != argsToKeep.end()) 
				|| (argsToRemove.find(arg2) != argsToRemove.end()))
				continue;
			if(arg1->isSameAndContains(arg2, docData->getDocTheory())){
				argsToKeep.insert(arg1);
				argsToRemove.insert(arg2);
			}
		}
	}
	if(argsToRemove.size() != 0){
		std::wcout<<"DuplicateArgFilter()ArgsToRemove Size: "<<argsToRemove.size()<<", Args To Keep "<<argsToKeep.size()<<std::endl;
		std::vector<ElfRelationArg_ptr> args;
		BOOST_FOREACH(ElfRelationArg_ptr arg1, relation->get_args()){
			if(argsToRemove.find(arg1) == argsToRemove.end())
				args.push_back(arg1);
			else if(argsToKeep.find(arg1) != argsToKeep.end())
				args.push_back(arg1);
		}
		ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
					args, relation->get_text(), relation->get_start(), relation->get_end(), 
					relation->get_confidence(), relation->get_score_group());
		new_relation->set_source(relation->get_source());
		new_relation->add_source(L"DuplicateArgFilter");

		_relationsToAdd.insert(new_relation);
		_relationsToRemove.insert(relation);
	}
}

void KBPStartEndFilter::processRelation(EIDocData_ptr docData,const ElfRelation_ptr relation){
	std::vector<ElfRelationArg_ptr> argsToKeep;
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
		if ((arg->get_role() != L"KBP:START") && (arg->get_role() != L"KBP:END") &&
			(arg->get_role() != L"KBP:START_SUB") && (arg->get_role() != L"KBP:END_SUB")){
				argsToKeep.push_back(arg);
		} 
	}
	if(argsToKeep.size() != relation->get_args().size() ){
		ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
			argsToKeep, relation->get_text(), relation->get_start(), relation->get_end(), 
			relation->get_confidence(), relation->get_score_group());
		new_relation->set_source(relation->get_source());
		new_relation->add_source(L"FilterKBPStartEnd");
		_relationsToAdd.insert(new_relation);
		_relationsToRemove.insert(relation);	
	}
}

void KBPCompletePTIOFilter::processRelation(EIDocData_ptr docData,const ElfRelation_ptr relation) {
	if (relation->get_name() == L"eru:PersonInOrganization") {
		// these originate from LearnIt, which does not know 
		// how to find titles
		relation->set_name(L"eru:PersonTitleInOrganization");
		relation->add_source(L"rename-PersonInOrganization");
	}

	std::vector<ElfRelationArg_ptr> personOfPTIOs = relation->get_args_with_role(L"eru:personOfPTIO");
	std::vector<ElfRelationArg_ptr> titleOfPTIOs = relation->get_args_with_role(L"eru:titleOfPTIO");
	std::vector<ElfRelationArg_ptr> organizationOfPTIOs = relation->get_args_with_role(L"eru:organizationOfPTIO");

	BOOST_FOREACH(ElfRelationArg_ptr org, organizationOfPTIOs) {
		if (titleOfPTIOs.empty() && !personOfPTIOs.empty()) {
			BOOST_FOREACH(ElfRelationArg_ptr personOfPTIO, personOfPTIOs) {
				if (addMissingTitleToPTIO(relation, personOfPTIO, docData)) {
					relation->add_source(L"added-title-to-PTIO");
				} else {
					relation->add_source(L"PTIO-missing-title");
				}
			}
		} else if (!titleOfPTIOs.empty() && personOfPTIOs.empty()) {
			BOOST_FOREACH(ElfRelationArg_ptr titleOfPTIO, titleOfPTIOs) {
				if (addMissingPersonToPTIO(relation, titleOfPTIO, docData)) {
					relation->add_source(L"added-person-to-PTIO");
				} else {
					relation->add_source(L"PTIO-missing-person");
				}
			}
		}
	}

	titleOfPTIOs = relation->get_args_with_role(L"eru:titleOfPTIO"); 
	BOOST_FOREACH(ElfRelationArg_ptr titleOfPTIO, titleOfPTIOs) {
		trimTitleOfPTIO(relation, titleOfPTIO, docData);
		//ElfRelationArg_ptr arg = relation->get_arg(L"eru:titleOfPTIO");
		//if (arg) {
		//	ElfIndividual_ptr indiv = arg->get_individual();

			/*if (indiv->has_mention_uid()) {
				SessionLogger::info("foo") << L"Result mention: " << EIUtils::findMentionForRelationArg(docData, arg)->getNode()->toTextString();
			} else {
				SessionLogger::info("foo") << L"Result value : " << arg->get_text();
			}*/
		//} else {
		//	SessionLogger::info("foo") << L"Got no title";
		//}
		deleteBadTitle(titleOfPTIO, relation, docData);
	}
	
}

bool KBPCompletePTIOFilter::addMissingPersonToPTIO(ElfRelation_ptr rel,
		ElfRelationArg_ptr title, EIDocData_ptr docData)
{
	const Mention* titleMention = 
		EIUtils::findMentionForRelationArg(docData, title);
	if (titleMention) {
		const Mention* nameMention = 0;
		int name_ment_len = -1;

		const Entity* e = docData->getEntityByMention(titleMention);		
		for (int i=0; i<e->getNMentions(); ++i) {
			const Mention* m = docData->getMention(e->getMention(i));
			if (m!=titleMention 
				&& m->getSentenceNumber() == titleMention->getSentenceNumber()) 
			{
				if (m->getMentionType() == Mention::NAME) {
					int len = m->getNode()->getEndToken() - m->getNode()->getStartToken();
					if (len > name_ment_len) {
						name_ment_len = len;
						nameMention = m;
					}
				}

			}
		}

		if (nameMention) {
			ElfIndividual_ptr nameIndiv = boost::make_shared<ElfIndividual>(
				docData->getDocTheory(),L"kbp:Person", nameMention);
			rel->insert_argument(boost::make_shared<ElfRelationArg>(
						L"eru:personOfPTIO", nameIndiv));
		} else {
			ElfIndividual_ptr titleIndiv = boost::make_shared<ElfIndividual>(
					docData->getDocTheory(), L"kbp:Person", titleMention);
			rel->insert_argument(boost::make_shared<ElfRelationArg>(
						L"eru:personOfPTIO", titleIndiv));
		}
		return true;
	}
	return false;
}

bool KBPCompletePTIOFilter::addMissingTitleToPTIO(ElfRelation_ptr rel,
		ElfRelationArg_ptr person, EIDocData_ptr docData)
{
	const Mention* personMention =
		EIUtils::findMentionForRelationArg(docData, person);
	if (personMention) {
		const Mention* titleMention = 0;
		int title_ment_len = -2;

		const Entity* e = docData->getEntityByMention(personMention);
		for (int i=0; i<e->getNMentions(); ++i) {
			const Mention* m = docData->getMention(e->getMention(i));
			if (m->getSentenceNumber() == personMention->getSentenceNumber()
					&& m->getMentionType() == Mention::DESC)
			{
				int len = m->getNode()->getEndToken() - m->getNode()->getStartToken();
				if (m == personMention) {
					// use this mention if we can find nothing else...
					len = -1;
				}
				if (len > title_ment_len) {
					title_ment_len = len;
					titleMention = m;
				}
			}
		}

		if (titleMention) {
			std::vector<ElfRelationArg_ptr> args;
			args = EIUtils::createRelationArgFromMention(docData, 
					L"eru:titleOfPTIO", L"kbp:Title", titleMention);

			BOOST_FOREACH(ElfRelationArg_ptr arg, args) {
				rel->insert_argument(arg);
			}

			return true;
		}
	}
	return false;
}

void KBPCompletePTIOFilter::trimTitleOfPTIO(ElfRelation_ptr rel,
		ElfRelationArg_ptr title, EIDocData_ptr docData)
{
	const Mention* titleMention =
		EIUtils::findMentionForRelationArg(docData, title);

	int start_token = -1, end_token = -1;
	if (titleMention ) {
		bool valid_title= EIUtils::titleFromMention(docData, titleMention, 
				start_token, end_token); 
		if (valid_title) {
			if (titleMention->getNode()->getStartToken() != start_token 
					|| titleMention->getNode()->getEndToken() != end_token)
			{
				rel->remove_argument(title);
				std::vector<ElfRelationArg_ptr> args =
					EIUtils::createRelationArgFromTokenSpan(docData,
							L"eru:titleOfPTIO", L"kbp:Title",
							titleMention->getSentenceNumber(), 
							start_token, end_token);
				BOOST_FOREACH(ElfRelationArg_ptr arg, args) {
					rel->insert_argument(arg);
				}
				rel->add_source(L"trim-title");
				//SessionLogger::info("foo") << L"Trimmed title.";
			} else {
				//SessionLogger::info("foo") << L"Leaving title alone.";
			}
		} else {
			//SessionLogger::info("foo") << L"Deleting title";
			//rel->remove_argument(title);
			// it's about 50-50 junk-removed vs. good stuff lost on 
			// deleting these titles, so we'll leave them in
		}
	} else {
		//SessionLogger::info("foo") << L"Null title!";
	}
}

void KBPCompletePTIOFilter::deleteBadTitle(ElfRelationArg_ptr titleArg, ElfRelation_ptr rel, EIDocData_ptr docData) {
	const Mention* m = EIUtils::findMentionForRelationArg(docData, titleArg);
	bool del = false;

	if (m) {
		if (m->getNode()->getStartToken() == m->getNode()->getEndToken()) {
			Symbol sym = m->getNode()->getHeadWord();
			del = EIUtils::isBadTitleWord(sym.to_string());
		}
	} else {
		std::wstring txt = titleArg->get_text();
		del = EIUtils::isBadTitleWord(txt);
	}

	if (del) {
		rel->remove_argument(titleArg);
	}
}

void MakePTIOsFilter::processRelation(EIDocData_ptr docData,
		const ElfRelation_ptr relation)
{
	bool added_relation = false;

	if (relation->get_name() == L"eru:HasEmployer") {

		ElfRelation_ptr newRel = boost::make_shared<ElfRelation>(relation);
		newRel->set_name(L"eru:PersonTitleInOrganization");
		std::vector<ElfRelationArg_ptr> employerArgs = newRel->get_args_with_role(L"eru:objectOfHasEmployer");
		std::vector<ElfRelationArg_ptr> employeeArgs = newRel->get_args_with_role(L"eru:subjectOfHasEmployer");

		if (!employerArgs.empty() && !employeeArgs.empty()) {
			BOOST_FOREACH(ElfRelationArg_ptr employerArg, employerArgs) {
				newRel->remove_argument(employerArg);
				employerArg->set_role(L"eru:organizationOfPTIO");
				newRel->insert_argument(employerArg);
			}
			BOOST_FOREACH(ElfRelationArg_ptr employeeArg, employeeArgs) {
				newRel->remove_argument(employeeArg);
				employeeArg->set_role(L"eru:personOfPTIO");
				newRel->insert_argument(employeeArg);
			}
			
			newRel->add_source(L"copied-from-HasEmployer");
			_relationsToAdd.insert(newRel);
		}
	} else if (relation->get_name() == L"eru:BelongsToHumanOrganization") {
		ElfRelation_ptr newRel = boost::make_shared<ElfRelation>(relation);
		newRel->set_name(L"eru:PersonTitleInOrganization");
		ElfRelationArg_ptr orgArg = newRel->get_arg(L"eru:objectOfBelongsToHumanOrganization");
		ElfRelationArg_ptr memberArg = newRel->get_arg(L"eru:subjectOfBelongsToHumanOrganization");

		if (orgArg && memberArg) {
			newRel->remove_argument(orgArg);
			newRel->remove_argument(memberArg);
			orgArg->set_role(L"eru:organizationOfPTIO");
			memberArg->set_role(L"eru:personOfPTIO");
			newRel->insert_argument(orgArg);
			newRel->insert_argument(memberArg);
			newRel->add_source(L"copied-from-BelongsToHumanOrganization");
			_relationsToAdd.insert(newRel);
		}
	} else if (relation->get_name() == L"eru:HasTopMemberOrEmployee") {
		ElfRelation_ptr newRel = boost::make_shared<ElfRelation>(relation);
		newRel->set_name(L"eru:PersonTitleInOrganization");
		ElfRelationArg_ptr orgArg = newRel->get_arg(L"eru:subjectOfHasTopMemberOrEmployee");
		ElfRelationArg_ptr topArg = newRel->get_arg(L"eru:objectOfHasTopMemberOrEmployee");

		if (orgArg && topArg) {
			newRel->remove_argument(orgArg);
			newRel->remove_argument(topArg);
			orgArg->set_role(L"eru:organizationOfPTIO");
			topArg->set_role(L"eru:personOfPTIO");
			newRel->insert_argument(orgArg);
			newRel->insert_argument(topArg);
			newRel->add_source(L"copied-from-HasTopMemberOrEmployee");
			_relationsToAdd.insert(newRel);
		}
	}
}

void BrokenArgFilter::processRelation(EIDocData_ptr docData,const ElfRelation_ptr relation){
	std::vector<ElfRelationArg_ptr> argsToKeep;
	ElfRelationArg_ptr arg1;
	ElfRelationArg_ptr arg2;
	std::wstring role1;
	std::wstring role2;
	std::vector<ElfRelationArg_ptr> coArg1;
	std::vector<ElfRelationArg_ptr> notCoArg1;
	std::vector<ElfRelationArg_ptr> coArg2;
	std::vector<ElfRelationArg_ptr> notCoArg2;
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
		if (arg->get_role() == L"ARG1") {
			arg1 = arg;
		} else if (arg->get_role() == L"ARG2") {
			arg2 = arg;
		} else if (arg->get_role() == L"COREF_ARG1"){
			coArg1.push_back(arg);
		} else if (arg->get_role() == L"NOT_COREF_ARG1") {
			notCoArg1.push_back(arg);
		} else if (arg->get_role() == L"COREF_ARG2"){
			coArg2.push_back(arg);
		} else if (arg->get_role() == L"NOT_COREF_ARG2") {
			notCoArg2.push_back(arg);
		} else if (arg->get_role() == L"ARG1_ROLE"){
			role1 = arg->get_type()->get_value();
		} else if (arg->get_role() == L"ARG2_ROLE"){
			role2 = arg->get_type()->get_value();
		} else {
			argsToKeep.push_back(arg);
		}
	}
	if (arg1 != NULL) {
		if (BrokenArgFilter::checkCoref(arg1,coArg1,notCoArg1)) {
			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
				argsToKeep, relation->get_text(), relation->get_start(), relation->get_end(), 
				relation->get_confidence(), relation->get_score_group());
			new_relation->set_source(relation->get_source());
			new_relation->add_source(L"BrokenArgFilter");
			arg1->set_role(role1);
			new_relation->insert_argument(arg1);
			if (arg2 == NULL || BrokenArgFilter::checkCoref(arg2,coArg2,notCoArg2)) {
				if (arg2 != NULL) {
					arg2->set_role(role2);
					new_relation->insert_argument(arg2);
				}
				_relationsToAdd.insert(new_relation);
			}
		}
		_relationsToRemove.insert(relation);
	}
}

bool BrokenArgFilter::checkCoref(ElfRelationArg_ptr target, std::vector<ElfRelationArg_ptr> coArgs, std::vector<ElfRelationArg_ptr> notCoArgs) {
	BOOST_FOREACH(ElfRelationArg_ptr arg, coArgs) {
		if (arg->get_individual()->get_entity_id() != target->get_individual()->get_entity_id()) {
			return false;
		}
	}
	BOOST_FOREACH(ElfRelationArg_ptr arg, notCoArgs) {
		if (arg->get_individual()->get_entity_id() == target->get_individual()->get_entity_id()) {
			return false;
		}
	}
	return true;
}

/*This filter is not currently used.  Before using check recall/precision balance of filtering rules */
void MarriageWithSetOrOfficialFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation){
	int sentNum = -1;
	const DocTheory* docTheory = docData->getDocTheory();
	const EntitySet* eSet = docTheory->getEntitySet();
	std::wcout<<"**** Call ProcessRelation: ";
	relation->dump(std::cout);
	std::set<ElfRelationArg_ptr> nonTemporalArgs;
	std::set<ElfRelationArg_ptr> temporalArgs;
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()){
		if(sentNum == -1)
			sentNum = EIUtils::getSentenceNumberForArg(docData->getDocTheory(), arg);
		else if(EIUtils::getSentenceNumberForArg(docData->getDocTheory(), arg) != sentNum)
			return;
		if(boost::starts_with(arg->get_role(), L"t:")){
			temporalArgs.insert(arg);
		}
		else{
			nonTemporalArgs.insert(arg);
		}
	}
	std::wcout<<"Found Args: "<<nonTemporalArgs.size()<<std::endl;
	std::set<ElfRelationArg_ptr> setArgs;
	std::set<ElfRelationArg_ptr> officialArgs;
	std::set<std::wstring> weddingOfficials = makeStrSet(L"rev reverend minister priest rabbi");
	BOOST_FOREACH(ElfRelationArg_ptr arg, nonTemporalArgs){
		if((arg->get_individual() != NULL) && (arg->get_individual()->has_mention_uid())){
			const Mention* mention =  eSet->getMention(arg->get_individual()->get_mention_uid());
			if(mention->getParent() != 0 && mention->getParent()->getMentionType() == Mention::LIST){
				const Entity* entity = eSet->getEntityByMention(mention->getUID());
				if(entity == 0){
					; //confused, allow these
				}
				else{
					int eid = entity->ID;
					bool foundOther = false;
					const Mention* mentionParent = mention->getParent();
					const Mention* childMention = mentionParent->getChild();
					while(childMention != 0){
						const Entity* other = eSet->getEntityByMention(mention->getUID());
						if((other != 0) && (other->getID() != eid)){
							setArgs.insert(arg);
							break;
						}
						childMention = childMention->getNext();						
					}
				}
			}
			Symbol mentionWords[51];
			int nWords = mention->getNode()->getTerminalSymbols(mentionWords, 50);
			for(int i = 0; i < nWords; i++){
				if(weddingOfficials.find(mentionWords[i].to_string()) != weddingOfficials.end()){
					officialArgs.insert(arg);
				}
			}
		}	
	}
	std::wcout<<"#Officials: "<<officialArgs.size()<<" #Set: "<<setArgs.size()<<std::endl;
	if(officialArgs.size() == 0 && setArgs.size() == 0){
		std::wcout<<"No sets/no officials"<<std::endl;
		return;
	}
	std::set<std::wstring> marryWords = makeStrSet(
		L"marry marrying married marries wed wedded weds wedding remarry remmarying remarried remarries re-marry re-married re-marries");

	const PropositionSet* pSet = docTheory->getSentenceTheory(sentNum)->getPropositionSet();
	const MentionSet* mentionSet = docTheory->getSentenceTheory(sentNum)->getMentionSet();
	bool foundProblem = false;
	std::set<ElfRelationArg_ptr> argsToRemove;
	bool handledRelation = false;
	for(int i = 0; i < pSet->getNPropositions(); i++){
		if(handledRelation)
			break;
		const Proposition*  p = pSet->getProposition(i);
		if(p->getPredHead() == 0)
			continue;
		if(marryWords.find(p->getPredHead()->getHeadWord().to_string()) != marryWords.end()){
			//std::set<ElfRelationArg_ptr> argsToRemove;
			std::wcout<<"Filter: Looking at Proposition: "<<p->getPredHead()->toTextString()<<std::endl;
			const Mention* subjectMention = 0;
			const Mention* objectMention = 0;
			const Entity* subjectEntity = 0;
			const Entity* objectEntity = 0;
			for(int ano = 0; ano < p->getNArgs(); ano++){
				Argument* pArg = p->getArg(ano);
				if(pArg->getRoleSym() == Argument::SUB_ROLE){
					subjectMention = pArg->getMention(mentionSet);
					subjectEntity =  eSet->getEntityByMention(subjectMention->getUID());
				}
				if(pArg->getRoleSym() == Argument::OBJ_ROLE){
					objectMention = pArg->getMention(mentionSet);
					objectEntity =  eSet->getEntityByMention(subjectMention->getUID());
				}
			}	

			bool officialIsSubject = false;
			bool officialIsObject = false;
			bool setIsSubject = false;
			bool setIsObject = false;

			BOOST_FOREACH(ElfRelationArg_ptr arg, officialArgs){
				if((arg->get_individual() != NULL) && (arg->get_individual()->has_mention_uid())){
					const Mention* mention =  eSet->getMention(arg->get_individual()->get_mention_uid());
					const Entity* argEntity = eSet->getEntityByMention(mention->getUID());
					if(argEntity != 0){
						if(subjectEntity != 0 && argEntity->ID == subjectEntity->ID){
							officialIsSubject = true;
						}
						if(objectEntity != 0 && argEntity->ID == subjectEntity->ID){
							officialIsObject = true;
						}
					}
				}
			}
			std::wcout<<"Official is object: "<<officialIsObject<<" Official is subject: "<<officialIsSubject<<std::endl;
			BOOST_FOREACH(ElfRelationArg_ptr arg, setArgs){
				if((arg->get_individual() != NULL) && (arg->get_individual()->has_mention_uid())){
					const Mention* mention =  eSet->getMention(arg->get_individual()->get_mention_uid());
					const Mention* mentionParent = mention->getParent();
					const Entity* argEntity = eSet->getEntityByMention(mention->getUID());
					if(argEntity != 0){
						if(subjectEntity != 0 && argEntity->ID == subjectEntity->ID){
							setIsSubject = true;
						}
						if(objectEntity != 0 && argEntity->ID == subjectEntity->ID){
							setIsObject = true;
						}
					}
					if(mentionParent != 0){
						if(subjectMention != 0 && mentionParent->getUID() == subjectMention->getUID()){
							setIsSubject = true;
						}
						if(objectMention != 0 &&  mentionParent->getUID() == objectMention->getUID()){
							setIsObject = true;
						}
					}
				}
			}
			std::set<ElfRelationArg_ptr> argsToKeep;
			std::wcout<<"Set is object: "<<setIsObject<<" Set is subject: "<<setIsSubject<<std::endl;

			BOOST_FOREACH(ElfRelationArg_ptr arg, nonTemporalArgs){
				if((officialArgs.find(arg) == officialArgs.end()) && (setArgs.find(arg) == setArgs.end()))
					argsToKeep.insert(arg);
				else{
					if((officialArgs.find(arg) != officialArgs.end()) && officialIsSubject && (setIsSubject || setIsObject)){
						;//official is subject, there is a set, we don't want to include the official
					}
					else if((setArgs.find(arg) != setArgs.end()) && setIsSubject && (objectMention != 0) && (objectMention->getEntityType().matchesPER())){
						;//the set is a subject and there is a person object, don't keep any members of the set
					}
					else{
						argsToKeep.insert(arg);
					}
				}
			}
			std::wcout<<"Args Tok Keep Size: "<<argsToKeep.size()<<std::endl;
			
			if(argsToKeep.size() == nonTemporalArgs.size()){
				std::wcout<<"--------- Relation is OK"<<std::endl;
				continue;
			}
			else if(argsToKeep.size() == 1){
				std::wcout<<"--------- Only delete relation"<<std::endl;
				std::wcout<<"***Remove Simple Relation: ";
				relation->dump(std::cout);

				_relationsToRemove.insert(relation);
				handledRelation = true;
			} 
			else{
				std::wcout<<"--------- Add and delete relation"<<std::endl;
				std::vector<ElfRelationArg_ptr> newArgs;
				newArgs.insert(newArgs.begin(), nonTemporalArgs.begin(), nonTemporalArgs.end());
				newArgs.insert(newArgs.begin(), temporalArgs.begin(), temporalArgs.end());

				ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(relation->get_name(), 
					newArgs, relation->get_text(), relation->get_start(), relation->get_end(), relation->get_confidence(), relation->get_score_group());
				new_relation->set_source(relation->get_source());
				new_relation->add_source(L"removeOfficialOrSet");
				std::wcout<<"***Add Relation: ";
				new_relation->dump(std::cout);
				_relationsToAdd.insert(new_relation);
				std::wcout<<"***Remove Relation: ";
				relation->dump(std::cout);
				_relationsToRemove.insert(relation);
			}
		}
	}
}

void KBPMinisterGPEFilter::processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
	static EntitySubtype nationSubtype;
	static bool init = false;

	if (!init) {
		init = true;
		try {
			nationSubtype = EntitySubtype(Symbol(L"GPE.Nation"));
		} catch ( ... ) {
			SessionLogger::err("bad_subtype") << "Cann't find subtype GPE.Nation!";
			throw;
		}
	}

	std::vector<ElfRelationArg_ptr> personArgs = relation->get_args_with_role(L"eru:personOfPTIO");
	std::vector<ElfRelationArg_ptr> orgArgs = relation->get_args_with_role(L"eru:organizationOfPTIO");
	std::vector<ElfRelationArg_ptr> titleArgs = relation->get_args_with_role(L"eru:titleOfPTIO");
	
	bool doAdd = false;
	std::vector<ElfRelationArg_ptr> args;
	std::wstring text;
	EDTOffset start, end;

	BOOST_FOREACH(ElfRelationArg_ptr personArg, personArgs) {
		BOOST_FOREACH(ElfRelationArg_ptr titleArg, titleArgs) {
			bool bad = false;
			if (!bad && personArg && titleArg && personArg->get_individual()
					&& titleArg->individual_has_type(L"kbp:MinisterTitle"))
			{
				BOOST_FOREACH(ElfRelationArg_ptr orgArg, orgArgs) {
					ElfIndividual_ptr orgIndiv = orgArg->get_individual();
					if (orgIndiv->has_entity_id()) {
						const Entity* e = docData->getEntity(orgIndiv->get_entity_id());
						if (e != 0 && e->guessEntitySubtype(docData->getDocTheory()) == nationSubtype)
						{
							bad = true;
							SessionLogger::info("foo") << "Has nation org";
						}
					}
				}

				if (!bad) {
					int num_gpes = 0;
					const Entity* gpe_ent = 0;
					const EntitySet* es = docData->getDocTheory()->getEntitySet();

					for (int i=0; i<es->getNEntities(); ++i) {
						const Entity* e = es->getEntity(i);
						if (e->guessEntitySubtype(docData->getDocTheory()) == nationSubtype) {
							++num_gpes;
							gpe_ent = e;
						}
					}

					if (num_gpes == 1) {
						// since there is exactly one GPE in the document, but we aren't
						// already in PTIO with a GPE, create an employment relation
						// with that GPE
						ElfIndividual_ptr personIndiv = personArg->get_individual();
						ElfRelationArg_ptr personArg2 = boost::make_shared<ElfRelationArg>(
								L"eru:subjectOfHasEmployer", personIndiv);
						args.push_back(personArg2);
						ElfIndividual_ptr orgIndiv;
						BOOST_FOREACH(ElfIndividual_ptr ind, docData->get_individuals_by_type()) {
							if (ind->get_entity_id() == gpe_ent->getID()) {
								orgIndiv = ind;
								break;
							}
						}
						if (orgIndiv) {
							ElfRelationArg_ptr orgArg = boost::make_shared<ElfRelationArg>(
									L"eru:objectOfHasEmployer", orgIndiv);
							args.push_back(orgArg);
							text = EIUtils::textFromIndividualSentences(docData, personIndiv, orgIndiv);  //Note this text will probably be inconsistent when there are multiple persons
							EIUtils::sentenceSpanOfIndividuals(docData, personIndiv, 
									orgIndiv, start, end); //Note these spans will probably be inconsistent when there are multiple persons
							doAdd = true;
						}
						break;
					}
				}
			}
		}
	}
	if (doAdd) {
		ElfRelation_ptr rel = boost::make_shared<ElfRelation>(
				L"eru:HasEmployer", args, text, start, end, 
				relation->get_confidence() * 0.9, relation->get_score_group());
		rel->set_source(relation->get_source());
		rel->add_source(L"minister-employment-from-context");
		_relationsToAdd.insert(rel);
	}
}

void FocusGPEResidenceFilter::processAll(EIDocData_ptr docData)
{
	// if the document is dominantly about a single country, assume everyone
	// mentioned in it lives in that country
	const Mention* focusNationState = 
		EIUtils::identifyFocusNationState(docData, false);
	
	if (focusNationState) {
		ElfIndividual_ptr nationIndiv = boost::make_shared<ElfIndividual>(
				docData->getDocTheory(), L"kbp:GPE-spec", focusNationState);

		BOOST_FOREACH(ElfIndividual_ptr ind, docData->getElfDoc()->get_individuals_by_type()){
			if (ind->has_type(L"kbp:Person")) {
				if (!docData->getElfDoc()->individual_in_relation(ind, L"eru:ResidesInGPE-spec") ) {
					ElfRelation_ptr rel = EIUtils::createBinaryRelationWithSentences(docData,
							L"eru:ResidesInGPE-spec", 
							L"eru:subjectOfResidesInGPE-spec", ind,
							L"eru:objectOfResidesInGPE-spec", nationIndiv,
							Pattern::UNSPECIFIED_SCORE);
					//SessionLogger::info("foo") << L"RET text is " << rel->get_text();
					_relationsToAdd.insert(rel);
				}
			}
		}
	}
}

CrazyBoundIDFilter::CrazyBoundIDFilter() : EIFilter(L"CrazyBoundIDFilter",false,false,true),
	_same_sentence_only(true), _displace_xdoc(false), _even_if_already_found_one(false)
{
	static bool init = false;
	static int craziness_level = -1;

	if (!init) {
		init = true;
		craziness_level = 
			ParamReader::getOptionalIntParamWithDefaultValue("bound_id_craziness_level",0);

		if (craziness_level == 0) {
			SessionLogger::info("craziness") << "Running in sane mode.";
		} else {
			SessionLogger::info("craziness") << "Running at craziness level "
				<< craziness_level;
		}
	}

	switch (craziness_level) {
		case 0:
			_relation_filter = false;
			_individual_filter = false;
			_process_all = false;
			break;
		case 1:
			_same_sentence_only = true;
			_displace_xdoc = false;
			break;
		case 2:
			_same_sentence_only = true;
			_displace_xdoc = true;
			break;
		case 3:
			_same_sentence_only = true;
			_displace_xdoc = true;
			_even_if_already_found_one = true;
		case 4:
			_same_sentence_only = false;
			_displace_xdoc = false;
			break;
		case 5:
			_same_sentence_only = false;
			_displace_xdoc = true;
			break;
		case 6:
			_same_sentence_only = false;
			_displace_xdoc = true;
			_even_if_already_found_one = true;
			break;
		default:
			std::wstringstream str;
			str << "Unknown PredFinder craziness level " << craziness_level;
			throw UnexpectedInputException("CrazyBoundIDFilter::CrazyBoundIDFilter",
					str);
	}
}

void CrazyBoundIDFilter::processAll(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> allRelations = docData->get_relations();
	ElfIndividualSet individuals = docData->get_individuals_by_type();

	// build a map from the bound URIs in the document to all relation types
	// it is found it
	typedef std::map<std::wstring, std::set<std::wstring> > RelationTypesSeen;
	RelationTypesSeen relTypesSeen;


	if (!_even_if_already_found_one) {
		BOOST_FOREACH(ElfRelation_ptr relation, allRelations) {
			BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
				ElfIndividual_ptr indiv = arg->get_individual();
				if (indiv && indiv->has_bound_uri()) {
					RelationTypesSeen::iterator probe =
						relTypesSeen.find(indiv->get_bound_uri());
					if (probe == relTypesSeen.end()) {
						std::set<std::wstring> s;
						s.insert(relation->get_name());
						relTypesSeen.insert(make_pair(indiv->get_bound_uri(), s));
					} else {
						probe->second.insert(relation->get_name());
					}
				}
			}
		}
	}

	std::map<std::wstring,std::wstring> boundURIs;
	BOOST_FOREACH(ElfIndividual_ptr indiv, individuals) {
		if (indiv->has_bound_uri()) {
			//SessionLogger::info("crazy") << L"Saw bound URI " 
			//	<< indiv->get_bound_uri() << L" with type " 
			//	<< indiv->get_type()->get_value();
			boundURIs.insert(std::make_pair(indiv->get_bound_uri(), 
						indiv->get_type()->get_value()));
		}
	}

	if (_same_sentence_only || boundURIs.size() <= 10) {
		// this could be done more efficiently but is probably fast enough...
		// candidate relations for bound id hijacking are those where the bound id
		// type matches one of the argument, but the bound id is never seen with
		// any relation of that type
		BOOST_FOREACH_PAIR(const std::wstring& boundURI, const std::wstring& type,
				boundURIs) 
		{
			RelationTypesSeen::const_iterator probe =
				relTypesSeen.find(boundURI);
			std::set<std::wstring> relsSeenForURI;
			if (probe != relTypesSeen.end()) {
				relsSeenForURI = probe->second;
			}

			BOOST_FOREACH(ElfRelation_ptr relation, allRelations) {
				if (relsSeenForURI.find(relation->get_name()) == relsSeenForURI.end()) {
					BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
						ElfIndividual_ptr indiv = arg->get_individual();
						if (indiv->get_type()->get_value() == type) {
							candidateCrazyRelation(docData, boundURI, relation, arg);
						}
					}
				}
			}
		}
	}
}

void CrazyBoundIDFilter::candidateCrazyRelation(EIDocData_ptr docData,
		const std::wstring& boundID, ElfRelation_ptr relation, 
		ElfRelationArg_ptr arg)
{
	if (_displace_xdoc || !arg->get_individual()->has_xdoc_cluster_id()) {
		if (!_same_sentence_only 
				|| boundIDFoundInSentenceWith(docData, boundID, arg)) 
		{
			ElfRelation_ptr newRel = 
				boost::make_shared<ElfRelation>(relation);
			ElfRelationArg_ptr newArg = newRel->get_arg(arg->get_role());
			newArg->get_individual()->set_bound_uri(boundID);
			newRel->add_source(L"crazy-relation");
			_relationsToAdd.insert(newRel);
			//SessionLogger::info("crazy") << L"Relation of type " << relation->get_name()
			//	<< L" was hijacked by " << boundID;
		}
	}
}

bool CrazyBoundIDFilter::boundIDFoundInSentenceWith(EIDocData_ptr docData,
		const std::wstring& boundID, ElfRelationArg_ptr arg)
{
	int sent_for_arg = EIUtils::getSentenceNumberForArg(docData->getDocTheory(),
			arg);
	BOOST_FOREACH(ElfIndividual_ptr indiv, docData->get_individuals_by_type()) {
		if (indiv->has_bound_uri() && indiv->get_bound_uri() == boundID 
				&& EIUtils::getSentenceNumberForIndividualStart(
					docData->getDocTheory(), indiv) == sent_for_arg)
		{
			return true;
		}
	}
	return false;
}

void AddTitleXDocIDsFilter::processAll(EIDocData_ptr docData) {
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

//----------------------------------------------------------------------------
// ELF INFERENCE METHODS (For backwards compatibility)

// If necessary, these ElfInference methods can be augmented by EIFilter methods
// that sweep over all the filters (e.g., by using a vector of pointers to EIFilter 
// subclasses).

// These methods apply multiple filters sequentially. They are all called from do_inference().
void ElfInference::applyRelationSpecificFilters(EIDocData_ptr docData) {
	// must precede applyRenameMembershipEmploymentFilter
	applyCopyPTIOEmploymentMembershipFilter(docData);
	applyRemovePersonGroupFilter(docData);
	applyRenameMembershipEmployment(docData);
	applyLeadershipFilter(docData);
	applyEmploymentFilter(docData);
	applyLocationFilter(docData);
	applyMarriageFilter(docData);
	applyPersonnelHiredFiredFilter(docData);
	applyPersonnelHeldPositionFilter(docData);
}

void ElfInference::applySpecialBoundIDFilters(EIDocData_ptr docData) {
	applyMilitaryAttackFilter(docData);
	applyBoundIDMemberishFilter(docData);
	applyNatBoundIDFilter(docData);
}

void ElfInference::applyPerLocationFilters(EIDocData_ptr docData){
	applyPerLocationFromDescriptorFilter(docData); // this should come first, it's more reliable
	applyPerLocationFromAttackFilter(docData);
}

// These methods apply single filters.

// Disallows relations with the same entity filling more than one slot unless
// we are told it's okay in this case
void ElfInference::applyDoubleEntityFilter(EIDocData_ptr docData) {
	DoubleEntityFilter filt;
	filt.apply(docData);
}

void ElfInference::applyCopyPTIOEmploymentMembershipFilter(EIDocData_ptr docData) {
	CopyPTIOEmploymentMembershipFilter filt;
	filt.apply(docData);
}

void ElfInference::applyUnmappedNFLTeamFilter(EIDocData_ptr docData) {
	UnmappedNFLTeamFilter filt;
	filt.apply(docData);
}

void ElfInference::applyLeadershipFilter(EIDocData_ptr docData) {
	LeadershipFilter filt;
	filt.apply(docData);
}

void ElfInference::applyEmploymentFilter(EIDocData_ptr docData) {
	EmploymentFilter filt;
	filt.apply(docData);
}

void ElfInference::applyLocationFilter(EIDocData_ptr docData) {
	LocationFilter filt;
	filt.apply(docData);
}

void ElfInference::applyMarriageFilter(EIDocData_ptr docData) {
	MarriageFilter filt;
	filt.apply(docData);
}

void ElfInference::applyAddTitleSubclassFilter(EIDocData_ptr docData) {
	AddTitleSubclassFilter filt;
	filt.apply(docData);
}

void ElfInference::applyPersonnelHiredFiredFilter(EIDocData_ptr docData) {
	PersonnelHiredFiredFilter filt;
	filt.apply(docData);
}

void ElfInference::applyPersonnelHeldPositionFilter(EIDocData_ptr docData) {
	PersonnelHeldPositionFilter filt;
	filt.apply(docData);
}

void ElfInference::applyGenericViolenceFilter(EIDocData_ptr docData) {
	GenericViolenceFilter filt;
	// Note that this calls applyIndividuals() as well as applyRelations().
	filt.apply(docData);
}

void ElfInference::applyGenderFilter(EIDocData_ptr docData){
	GenderFilter filt;
	filt.apply(docData);
}

void ElfInference::applyMilitaryAttackFilter(EIDocData_ptr docData) {
	MilitaryAttackFilter filt;
	filt.apply(docData);
}

void ElfInference::applyBoundIDMemberishFilter(EIDocData_ptr docData) {
	BoundIDMemberishFilter filt;
	filt.apply(docData);
}

void ElfInference::applyNatBoundIDFilter(EIDocData_ptr docData) {
	NatBoundIDFilter filt;
	filt.apply(docData);
}

void ElfInference::applyInformalMemberFilter(EIDocData_ptr docData){
	InformalMemberFilter filt;
	filt.apply(docData);
}

void ElfInference::applyOrgLocationFilter(EIDocData_ptr docData) {
	OrgLocationFilter filt;
	filt.apply(docData);
}

void ElfInference::applyPerLocationFromDescriptorFilter(EIDocData_ptr docData){
	PerLocationFromDescriptorFilter filt;
	filt.apply(docData);
}

void ElfInference::applyPerLocationFromAttackFilter(EIDocData_ptr docData) {
	PerLocationFromAttackFilter filt;
	filt.apply(docData);
} 

//void ElfInference::applyLeadershipPerOrgFilter(EIDocData_ptr docData) {
void ElfInference::applyLeadershipPerOrgFilter(EIDocData_ptr docData,
		const std::wstring& ontology) {
	LeadershipPerOrgFilter filt(ontology);
	filt.apply(docData);
} 

void ElfInference::applyEmploymentPerOrgFilter(EIDocData_ptr docData) {
	EmploymentPerOrgFilter filt;
	filt.apply(docData);
}

void ElfInference::applyLocationFromSubSuperFilter(EIDocData_ptr docData){
	LocationFromSubSuperFilter filt;
	filt.apply(docData);
}

void ElfInference::applyPersonGroupEntityTypeFilter(EIDocData_ptr docData){
	PersonGroupEntityTypeFilter filt;
	filt.apply(docData);
}

void ElfInference::applyRemovePersonGroupFilter(EIDocData_ptr docData){
	RemovePersonGroupRelations filt;
	filt.apply(docData);
}

void ElfInference::applyRenameMembershipEmployment(EIDocData_ptr docData){
	RenameMembershipEmployment filt;
	filt.apply(docData);
}

void ElfInference::applyKBPTemporalFilter(EIDocData_ptr docData){
	KBPTemporalFilter filt;
	filt.apply(docData);
}
void ElfInference::applyDuplicateRelationFilter(EIDocData_ptr docData){
	DuplicateRelationFilter filt;
	filt.apply(docData);
}

void ElfInference::applyKBPCompletePTIOFilter(EIDocData_ptr docData) {
	KBPCompletePTIOFilter filt;
	filt.apply(docData);
}
void ElfInference::applyKBPConflictingBirthDeathTemporalFilter(EIDocData_ptr docData){
	KBPConflictingBirthDeathTemporalFilter filter;
	filter.apply(docData);
}
void ElfInference::applyKBPConflictingDateTemporalFilter(EIDocData_ptr docData){
	KBPConflictingDateTemporalFilter filter;
	filter.apply(docData);
}
void ElfInference::applyKBPStartEndFilter(EIDocData_ptr docData){
	KBPStartEndFilter filter;
	filter.apply(docData);
}
void ElfInference::applyKBPMatchLearnedAndManualDatesFilter(EIDocData_ptr docData){
	KBPMatchLearnedAndManualDatesFilter filter;
	filter.apply(docData);
}

void ElfInference::applyMakePTIOsFilter(EIDocData_ptr docData) {
	MakePTIOsFilter filt;
	filt.apply(docData);
}

void ElfInference::applyBoundTitleFilter(EIDocData_ptr docData) {
	BoundTitleFilter filt;
	filt.apply(docData);
}

void ElfInference::applyRemoveUnusedIndividualsFilter(EIDocData_ptr docData){
	RemoveUnusedIndividualsFilter filt;
	filt.apply(docData);
}

void ElfInference::applyInferHoldsThroughout(EIDocData_ptr docData) {
	InferHoldsThroughout filt;
	filt.apply(docData);
}

void ElfInference::applyBrokenArgFilter(EIDocData_ptr docData){
	BrokenArgFilter filter;
	filter.apply(docData);
}

void ElfInference::applyMarriageWithSetOrOfficialFilter(EIDocData_ptr docData) {
	MarriageWithSetOrOfficialFilter filt;
	filt.apply(docData);
}

void ElfInference::applyGPEMinisterFilter(EIDocData_ptr docData) {
	KBPMinisterGPEFilter filt;
	filt.apply(docData);
}

void ElfInference::applyFocusGPEResidence(EIDocData_ptr docData) {
	FocusGPEResidenceFilter filt;
	filt.apply(docData);
}

void ElfInference::applyCrazyBoundIDFilter(EIDocData_ptr docData) {
	CrazyBoundIDFilter filt;
	filt.apply(docData);
}

void ElfInference::applyKBPBirthDeathTemporalFilter(EIDocData_ptr docData){
	KBPBirthDeathTemporalFilter filt;
	filt.apply(docData);
}
