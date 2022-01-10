#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"

#pragma warning(disable: 4996)

#include <boost/algorithm/string.hpp>
#include <vector>
#include "PredFinder/inference/EIBlitz.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/inference/EIDocData.h"
#include "PredFinder/inference/EIUtils.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Argument.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/elf/ElfIndividualFactory.h"

///////////////////
//               //
// BLITZ METHODS //
//               //
///////////////////

/**
 *  Perform coreference for Blitz classes. Modifies the document in place. Called from do_inference.
 *  @param docData DocTheory + ElfDocument pointer.
 *  @param type Ontology type (e.g., "ic:PhysiologicalCondition").
 *  @param id_counter Counts number of IDs created.
 */
void EIBlitz::manageBlitzCoreference(EIDocData_ptr docData, const std::wstring & type, int& id_counter) {
	
	// Goal: Create a mapping from individual IDs to new coref IDs. 
	// Empty string means that it is an individual that should be removed.
	std::map<std::wstring, std::wstring> corefMap;

	addBlitzNamesToMap(docData, docData->get_individuals_by_type(type + L"NameList"), corefMap, id_counter);
	addBlitzNamesToMap(docData, docData->get_individuals_by_type(type + L"NameModel"), corefMap, id_counter);
	
	addBlitzDescriptorsToMap(docData, type, docData->get_individuals_by_type(type + L"DescList"), corefMap, id_counter);
	addBlitzDescriptorsToMap(docData, type, docData->get_individuals_by_type(type + L"DescPattern"), corefMap, id_counter);
	addBlitzDescriptorsToMap(docData, type, docData->get_individuals_by_type(type + L"DescModel"), corefMap, id_counter);

	// The ones that come from the name model will already have been captured. This leaves only
	//   those coming from the descriptor model.
	addBlitzDescriptorsToMap(docData, type, docData->get_individuals_by_type(type + L"-learnit"), corefMap, id_counter);

	// These are "possible" substances and conditions created by the patterns, which should only be kept
	//   if they map to a "real" substance or condition. 
	// This function takes care of the mapping and also removes the ones that don't map and have no other types.
	addBlitzPossiblesToMapPron(docData, type, docData->get_individuals_by_type(type + L"Possible"), corefMap, id_counter);

	ElfIndividualUriMap newIndividualIDMap;
	ElfIndividualSet individualsToRemove;

	// Create new individuals with the right IDs (containing all of the stuff from the old ones)
	// Do this in a nice order so that we keep the ones with the most exact offsets (i.e. not mentions, booo)
	// I'm not sure if that matters, but it sure makes things easier to look at.
	transitionBlitzIndividualsByTypeAndSuffix(docData, corefMap, type, L"NameList", newIndividualIDMap, individualsToRemove);
	transitionBlitzIndividualsByTypeAndSuffix(docData, corefMap, type, L"NameModel", newIndividualIDMap, individualsToRemove);
	transitionBlitzIndividualsByTypeAndSuffix(docData, corefMap, type, L"DescPattern", newIndividualIDMap, individualsToRemove);
	transitionBlitzIndividualsByTypeAndSuffix(docData, corefMap, type, L"DescList", newIndividualIDMap, individualsToRemove);
	transitionBlitzIndividualsByTypeAndSuffix(docData, corefMap, type, L"-learnit", newIndividualIDMap, individualsToRemove);
	transitionBlitzIndividualsByTypeAndSuffix(docData, corefMap, type, L"Possible", newIndividualIDMap, individualsToRemove);

	// Now remove all of the old individuals with the bad old IDs, plus any relations that we decided were bad (cascades if the individual matches)
	docData->remove_individuals(individualsToRemove);	

	// Update individuals with their new coreference URIs
	docData->getElfDoc()->add_individual_coref_uris(newIndividualIDMap);
}

/**
 *  Take a Blitz individual and transition it to the new coreffed individual.
 *  The original individuals, which should be removed, are appropriately collected in individualsToRemove
 */
void EIBlitz::transitionBlitzIndividual(EIDocData_ptr docData, const ElfIndividual_ptr original_individual, const std::wstring & coref_id, 
											 ElfIndividualUriMap & newIndividualIDMap,
											 ElfIndividualSet & individualsToRemove) 
{
	// If the individual wasn't coreffed, drop it
	if (coref_id == L"") {
		individualsToRemove.insert(original_individual);
		return;
	}

	// Generate a new ID
	std::wstringstream new_id;
	new_id << L"bbn:blitz-" << std::wstring(docData->getDocument()->getName().to_string());
	new_id << "-" << coref_id;

	// store mapping from old ID to new ID
	newIndividualIDMap[original_individual->get_generated_uri()] = new_id.str();
}

void EIBlitz::transitionBlitzIndividualsByTypeAndSuffix(EIDocData_ptr docData, const std::map<std::wstring, std::wstring> & corefMap,
														const std::wstring & type, const std::wstring & suffix,
														ElfIndividualUriMap & newIndividualIDMap,
														ElfIndividualSet & individualsToRemove) {
	typedef std::pair<std::wstring, std::wstring> coref_pair_t;
	BOOST_FOREACH(coref_pair_t pair, corefMap) {
		if (newIndividualIDMap.find(pair.first) != newIndividualIDMap.end())
			continue;
		ElfIndividual_ptr original_individual = docData->getElfDoc()->get_individual_by_generated_uri(pair.first);
		if (original_individual.get() == NULL)
			// This shouldn't happen
			continue;
		if (original_individual->has_type(type + suffix)) {
			transitionBlitzIndividual(docData, original_individual, pair.second, newIndividualIDMap, individualsToRemove);
		}
	}
}

bool EIBlitz::killBlitzIndividual(EIDocData_ptr docData, const ElfIndividual_ptr ind) {

	static std::set< std::wstring > HUMAN_BLOOD = makeStrSet(
		L"human|blood|heart function|adult|congress|c|head|bone|heart|kidney|neck|prostate|human cloning|multiple fronts|one-a-day",
		L"|"); // L("|") is the token delimiter

	std::wstring this_value = ind->get_name_or_desc()->get_value();

	if (this_value.find(L"the ") == 0)
		this_value = this_value.substr(4);
	if (this_value.size() == 0)
		return true;
	this_value = UnicodeUtil::normalizeTextString(this_value);

	if (this_value.find(L"levels") != std::wstring::npos &&  this_value.find(L"cholesterol") == std::wstring::npos) {
		SessionLogger::info("LEARNIT") << L"Proposed individual contains the word 'levels' but no 'cholesterol', killing: " << this_value << L"\n";
		return true;
	}
	
	const Mention *ment = findBestMentionForBlitzIndividual(docData, ind);
	if (ment != 0) {
		Symbol headword = ment->getNode()->getHeadWord();
		if (headword == Symbol(L"system") || headword == Symbol(L"systems") ||
			headword == Symbol(L"program") || headword == Symbol(L"programs"))
		{
			SessionLogger::info("LEARNIT") << L"Proposed individual has headword 'system'/'program', killing: " << this_value << L"\n";
			return true;
		}
	}

	if (HUMAN_BLOOD.find(this_value) != HUMAN_BLOOD.end())
		return true;

	if (this_value.find(L"blood flow") == 0)
		return true;

	return false;
}

/**
 *  Try to coreference Blitz names. String match, basically.
 */
void EIBlitz::addBlitzNamesToMap(EIDocData_ptr docData, const ElfIndividualSet& individuals, 
									  std::map<std::wstring, std::wstring>& corefMap, int& id_counter) {
	std::list<ElfIndividual_ptr> sortedList;
	EIUtils::sortIndividualMapByOffsets(individuals, sortedList);

	BOOST_FOREACH(ElfIndividual_ptr ind, sortedList) {
		// skip things that already have a blitz coref ID
		if (corefMap.find(ind->get_generated_uri()) != corefMap.end())
			continue;

		if (killBlitzIndividual(docData, ind)) {
			// Mark this individual as killed
			corefMap[ind->get_generated_uri()] = L"";
			continue;
		}

		// Get rid of things which match GPEs... these seem to occur a lot, at least with the initial name model
		const Mention *ment = findBestMentionForBlitzIndividual(docData, ind);
		if (ment != 0 && ment->getEntityType().matchesGPE()) {
			SessionLogger::info("LEARNIT") << L"Killing Blitz individual found as GPE: " 
				<< ind->get_name_or_desc()->get_value() << L" from mention: " << ment->getNode()->toTextString() << L"\n";
			corefMap[ind->get_generated_uri()] = L"";
			ment = findBestMentionForBlitzIndividual(docData, ind);
			continue;
		}

		std::wstring this_value = UnicodeUtil::normalizeTextString(ind->get_name_or_desc()->get_value());
		bool found_name_match = false;
		typedef std::pair<std::wstring, std::wstring> coref_pair_t;
		BOOST_FOREACH(coref_pair_t prev_pair, corefMap) {
			ElfIndividual_ptr prevInd = docData->getElfDoc()->get_individual_by_generated_uri(prev_pair.first);
			std::wstring coref_value = UnicodeUtil::normalizeTextString(prevInd->get_name_or_desc()->get_value());
			// poor-man's plural matching (e.g. ulcer ~ ulcers)
			if (this_value == coref_value || this_value + L"s" == coref_value || this_value == coref_value + L"s")
			{
				corefMap[ind->get_generated_uri()] = prev_pair.second;
				found_name_match = true;
			}
		}
		// if no name match found, create a new ID
		if (!found_name_match) {
			std::wstringstream s;
			s<<id_counter++;
			corefMap[ind->get_generated_uri()] = s.str();
			//SessionLogger::info("LEARNIT") << L"New name id: " << this_value << L"\n";
		}
	}
}

/**
 *  Try to coreference Blitz descriptors. Outsourced to findBlitzAntecedent().
 */
void EIBlitz::addBlitzDescriptorsToMap(EIDocData_ptr docData, const std::wstring & type, const ElfIndividualSet& individuals, 
											std::map<std::wstring, std::wstring>& corefMap, int& id_counter) 
{	
	std::list<ElfIndividual_ptr> sortedList;
	EIUtils::sortIndividualMapByOffsets(individuals, sortedList);

	// Do it once for high-confidence stuff. Which is currently everything. Whatev! :)
	BOOST_FOREACH(ElfIndividual_ptr ind, sortedList) {
		
		// skip things that already have a blitz coref ID
		if (corefMap.find(ind->get_generated_uri()) != corefMap.end())
			continue;

		if (killBlitzIndividual(docData, ind)) {
			// Mark this individual as killed
			corefMap[ind->get_generated_uri()] = L"";
			continue;
		}

		// see if this is identical to some previous linked individual,
		// this will happen if we are stored as token offsets but the previous one was a mention, for example
		if (!ind->has_value()) {
			const Mention *ment = findBestMentionForBlitzIndividual(docData, ind);
			if (ment != 0) {
				ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, ment, corefMap, false);
				if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
					//SessionLogger::info("LEARNIT") << L"Matched value to mention: " << ind->get_name_or_desc()->get_value() 
					//	<< L" --> " << match->get_name_or_desc()->get_value() << L"\n";
					corefMap[ind->get_generated_uri()] = corefMap[match->get_generated_uri()];
					continue;
				} 
			}
		}

		// find antecedent for this descriptor, and if you can, take its ID 
		ElfIndividual_ptr antecedent = findBlitzAntecedent(docData, type, ind, corefMap, true);
		if (antecedent != ElfIndividual_ptr() && corefMap.find(antecedent->get_generated_uri()) != corefMap.end()) {
			corefMap[ind->get_generated_uri()] = corefMap[antecedent->get_generated_uri()];
		} 
	}

	BOOST_FOREACH(ElfIndividual_ptr ind, sortedList) {
		
		// skip things that already have a blitz coref ID
		if (corefMap.find(ind->get_generated_uri()) != corefMap.end())
			continue;		

		// find antecedent for this descriptor, and if you can, take its ID -- this time being more generous
		ElfIndividual_ptr antecedent = findBlitzAntecedent(docData, type, ind, corefMap, false);
		if (antecedent != ElfIndividual_ptr() && corefMap.find(antecedent->get_generated_uri()) != corefMap.end()) {
			corefMap[ind->get_generated_uri()] = corefMap[antecedent->get_generated_uri()];
		} else {
			std::wstringstream s;
			s<<id_counter++;
			corefMap[ind->get_generated_uri()] = s.str();
		}
	}
}

/**
 *  Take individuals generated by relation patterns and try to match them to a "real" individual generated by the class patterns.
 *  If you can't, remove it and the horse it rode in on (the relation).
 *  This does NOT handle pronouns but probably should. TODO.
 */
void EIBlitz::addBlitzPossiblesToMapOrig(EIDocData_ptr docData, const std::wstring & type, const ElfIndividualSet& individuals, 
											  std::map<std::wstring, std::wstring>& corefMap, int& id_counter) 
{
	std::list<ElfIndividual_ptr> sortedList;
	EIUtils::sortIndividualMapByOffsets(individuals, sortedList);

	std::set<std::wstring> individualIDsToRemove;
	ElfIndividualSet individualsToRemove;

	BOOST_FOREACH(ElfIndividual_ptr ind, sortedList) {
		
		// skip things that already have a blitz coref ID
		if (corefMap.find(ind->get_generated_uri()) != corefMap.end())
			continue;

		if (killBlitzIndividual(docData, ind))
			continue;

		// all Blitz-possibles should be mentions, since they come from Liz's patterns
		if (!ind->has_mention_uid())
			continue;

		const Mention *ment = findBlitzMentionByMentionID(docData, ind->get_mention_uid());
		if (ment == 0)
			continue;

		if (ment->getMentionType() == Mention::PRON) {
			Symbol headword = ment->getNode()->getHeadWord();
			if (headword != Symbol(L"it") || headword != Symbol(L"its"))
				continue;
			// TODO: find confident pronoun antecedent
			continue;
		}

		ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, ment, corefMap, true);
		// By definition this thing is in the coref map if it was returned, but let's be sure
		if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
			corefMap[ind->get_generated_uri()] = corefMap[match->get_generated_uri()];
		} else {
			// We couldn't find a match, so this is a bad individual if all it is is a "Possible" one
			ElfIndividual_ptr original_ind = docData->getElfDoc()->get_individual_by_generated_uri(ind->get_generated_uri());
			if (original_ind->has_type(type + L"Possible")) {
				individualIDsToRemove.insert(ind->get_generated_uri());
				individualsToRemove.insert(original_ind);
			}
		}
	}
	
	// Identify relations that point to unmapped Possibles
	std::set<ElfRelation_ptr> relationsToRemove;	
	BOOST_FOREACH(ElfRelation_ptr elf_relation, docData->get_relations()) {
		bool bad_arg = false;
		BOOST_FOREACH(ElfRelationArg_ptr elf_rel_arg, elf_relation->get_args()) {
			ElfIndividual_ptr rel_ind = elf_rel_arg->get_individual();
			if (rel_ind == ElfIndividual_ptr())
				continue;
			if (individualIDsToRemove.find(rel_ind->get_generated_uri()) != individualIDsToRemove.end()) {
				relationsToRemove.insert(elf_relation);
			}
		}
	}

	// Remove unmapped Possibles (that have no other types) and the relations that point to them
	docData->getElfDoc()->remove_individuals(individualsToRemove);
	docData->remove_relations(relationsToRemove);
}

void EIBlitz::addBlitzPossiblesToMapPron(EIDocData_ptr docData, const std::wstring & type, const ElfIndividualSet& individuals, 
											  std::map<std::wstring, std::wstring>& corefMap, int& id_counter) 
{
	std::list<ElfIndividual_ptr> sortedList;
	EIUtils::sortIndividualMapByOffsets(individuals, sortedList);

	std::set<std::wstring> individualIDsToRemove;
	ElfIndividualSet individualsToRemove;

	BOOST_FOREACH(ElfIndividual_ptr ind, sortedList) {
		
		// skip things that already have a blitz coref ID
		if (corefMap.find(ind->get_generated_uri()) != corefMap.end())
			continue;

		if (killBlitzIndividual(docData, ind))
			continue;

		// all Blitz-possibles should be mentions, since they come from Liz's patterns
		if (!ind->has_mention_uid())
			continue;

		const Mention *ment = findBlitzMentionByMentionID(docData, ind->get_mention_uid());
		if (ment == 0)
			continue;
		ElfIndividual_ptr match = ElfIndividual_ptr();
		if (ment->getMentionType() == Mention::PRON) {
			Symbol headword = ment->getNode()->getHeadWord();
			if (headword != Symbol(L"it") && headword != Symbol(L"its"))
				continue;
			const DocTheory* docTheory = docData->getDocTheory();
			const Entity* entity = docTheory->getEntitySet()->getEntityByMention(ment->getUID());
			if (entity != 0){	//SERIF thinks it knows this type
				continue;
			}
			// TODO: find confident pronoun antecedent
			ElfIndividual_ptr ant = findBlitzPronAntecedent(docData, ment, corefMap);
			if(ant !=  ElfIndividual_ptr()){
				match = ant;
			}
		}
		else{
			match = findBestIndividualForBlitzMention(docData, ment, corefMap, true);
		}
		// By definition this thing is in the coref map if it was returned, but let's be sure
		if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
			corefMap[ind->get_generated_uri()] = corefMap[match->get_generated_uri()];

		} else {
			// We couldn't find a match, so this is a bad individual if all it is is a "Possible" one
			ElfIndividual_ptr original_ind = docData->get_merged_individual_by_uri(ind->get_generated_uri());
			if (original_ind->has_type(type + L"Possible")) {
				individualIDsToRemove.insert(ind->get_generated_uri());
				individualsToRemove.insert(original_ind);
			}
		}
	}
	
	// Identify relations that point to unmapped Possibles
	std::set<ElfRelation_ptr> relationsToRemove;	
	BOOST_FOREACH(ElfRelation_ptr elf_relation, docData->get_relations()) {
		bool bad_arg = false;
		BOOST_FOREACH(ElfRelationArg_ptr elf_rel_arg, elf_relation->get_args()) {
			ElfIndividual_ptr rel_ind = elf_rel_arg->get_individual();
			if (rel_ind == ElfIndividual_ptr() || rel_ind->has_value())
				continue;
			if (individualIDsToRemove.find(rel_ind->get_generated_uri()) != individualIDsToRemove.end()) {
				relationsToRemove.insert(elf_relation);
			}
		}
	}

	// Remove unmapped Possibles (that have no other types) and the relations that point to them
	docData->getElfDoc()->remove_individuals(individualsToRemove);
	docData->remove_relations(relationsToRemove);
}
bool EIBlitz::blitzIndividualHasName(EIDocData_ptr docData, const std::wstring & blitz_id,  
										  std::map<std::wstring, std::wstring>& corefMap){
	typedef std::pair<std::wstring, std::wstring> coref_pair_t;
	BOOST_FOREACH(coref_pair_t pair, corefMap) {
		if(pair.second == blitz_id){
			ElfIndividual_ptr coref_individual = docData->getElfDoc()->get_individual_by_generated_uri(pair.first);	
			if(coref_individual->get_type()->get_value().find(L"Name") != std::wstring::npos)
				return true;
		}
	}
	return false;
}
/**
 *
 */
ElfIndividual_ptr EIBlitz::findBlitzPronAntecedent(EIDocData_ptr docData, const Mention* ment, 
														std::map<std::wstring, std::wstring>& corefMap){
	const SentenceTheory* st = docData->getSentenceTheory(ment->getSentenceNumber());
	EDTOffset head_start = st->getTokenSequence()->getToken(ment->getNode()->getHeadPreterm()->getStartToken())->getStartEDTOffset();

	ElfIndividual_ptr nearest_preceding = ElfIndividual_ptr();
	int difference = -1;		
	ElfIndividual_ptr second_nearest_preceding = ElfIndividual_ptr();
	int second_difference = -1;
	typedef std::pair<std::wstring, std::wstring> coref_pair_t;
	BOOST_FOREACH(coref_pair_t pair, corefMap) {
		if(pair.second == L"")
			continue;
		ElfIndividual_ptr coref_individual = docData->getElfDoc()->get_individual_by_generated_uri(pair.first);			
		EDTOffset start, end;
		coref_individual->get_name_or_desc()->get_offsets(start, end);
		if (end < head_start) {
			if (difference == -1 || head_start.value() - end.value() < difference) {
				second_difference = difference;
				second_nearest_preceding = nearest_preceding;
				difference = head_start.value() - end.value();
				nearest_preceding = coref_individual;
			} else if (second_difference == -1 || head_start.value() - end.value() < second_difference) {
				second_difference = head_start.value() - end.value();
				second_nearest_preceding = coref_individual;
			}
		}
	}
	if(nearest_preceding == ElfIndividual_ptr() || second_nearest_preceding == ElfIndividual_ptr())
		return nearest_preceding;
	if(blitzIndividualHasName(docData, corefMap[nearest_preceding->get_generated_uri()], corefMap))
		return nearest_preceding;
	if(blitzIndividualHasName(docData, corefMap[second_nearest_preceding->get_generated_uri()], corefMap))
		return second_nearest_preceding;
	return nearest_preceding;	
}

/**
 *  Standard pre-link stuff, plus we link "the singular-noun" to the nearest antecdent.
 */
ElfIndividual_ptr EIBlitz::findBlitzAntecedent(EIDocData_ptr docData, const std::wstring & type, const ElfIndividual_ptr ind, 
													std::map<std::wstring, std::wstring>& corefMap, 
													bool high_confidence_only) 
{

	// all things where we search for antecedents should be mentions
	if (!ind->has_mention_uid())
		return ElfIndividual_ptr();

	const Mention *ment = findBlitzMentionByMentionID(docData, ind->get_mention_uid());
	const SentenceTheory *st = docData->getSentenceTheory(ment->getSentenceNumber());
	if (ment == 0)
		return ElfIndividual_ptr();

	// First, look for genuine appositives
	if (ment->getParent() != 0 && ment->getParent()->getMentionType() == Mention::APPO) {
		const Mention *iter = ment->getParent()->getChild();
		while (iter != 0) {
			if (iter != ment) {
				ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, iter, corefMap);
				if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
					//SessionLogger::info("LEARNIT") << L"Found appositive match: " 
					//	<< ind->get_name_or_desc()->get_value() << L" --> " << match->get_name_or_desc()->get_value() << L"\n";
					return match;
				}
			}
			iter = iter->getNext();
		}
	}
	if (ment->getNext() != 0) {
		ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, ment->getNext(), corefMap);
		if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
			//SessionLogger::info("LEARNIT") << L"Found \"next\" match: " 
			//	<< ind->get_name_or_desc()->get_value() << L" --> " << match->get_name_or_desc()->get_value() << L"\n";
			return match;
		}
	}
	for (int p = 0; p < st->getPropositionSet()->getNPropositions(); p++) {
		const Proposition *prop = st->getPropositionSet()->getProposition(p);
		if (prop->getPredType() == Proposition::COPULA_PRED && prop->getNArgs() >= 2 &&
			prop->getArg(0)->getType() == Argument::MENTION_ARG &&
			prop->getArg(1)->getType() == Argument::MENTION_ARG)
		{
			if (prop->getArg(0)->getMentionIndex() == ment->getIndex()) {
				ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, 
					prop->getArg(1)->getMention(st->getMentionSet()), corefMap);
				if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
					//SessionLogger::info("LEARNIT") << L"Found copula match: " 
					//	<< ind->get_name_or_desc()->get_value() << L" --> " << match->get_name_or_desc()->get_value() << L"\n";
					return match;
				}
			}
			if (prop->getArg(1)->getMentionIndex() == ment->getIndex()) {
				ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, 
					prop->getArg(0)->getMention(st->getMentionSet()), corefMap);
				if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end()) {
					//SessionLogger::info("LEARNIT") << L"Found copula match: " 
					//	<< ind->get_name_or_desc()->get_value() << L" --> " << match->get_name_or_desc()->get_value() << L"\n";
					return match;
				}
			}
		}
	}
	// e.g. node = (its ulcer medication) (Zantac)
	const SynNode *parent = ment->getNode()->getParent();
	if (parent != 0) {
		for (int i = 0; i < parent->getNChildren() - 1; i++) {
			if (parent->getChild(i) == ment->getNode()) {
				const SynNode *nextNode = parent->getChild(i+1);
				if (nextNode->hasMention()) {
					ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, 
						st->getMentionSet()->getMention(nextNode->getMentionIndex()), 
						corefMap);
					if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end() &&
						(match->has_type(type + L"NameModel") || match->has_type(type + L"NameList")))
					{
						SessionLogger::info("LEARNIT") << L"Found next-node match (type 1): " 
							<< ind->get_name_or_desc()->get_value() << L" --> " << match->get_name_or_desc()->get_value() << L"\n";
						return match;
					}
				}
			}
		}
	}
	
	// e.g. node = ((Wellcome's anti-herpes drug) (Zovirax))
	const SynNode *node = ment->getNode();
	for (int i = 0; i < node->getNChildren() - 1; i++) {
		if (node->getChild(i) == node->getHead()) {
			const SynNode *nextNode = node->getChild(i+1);
			if (nextNode->hasMention()) {
				ElfIndividual_ptr match = findBestIndividualForBlitzMention(docData, 
					st->getMentionSet()->getMention(nextNode->getMentionIndex()), 
					corefMap);
				if (match != ElfIndividual_ptr() && corefMap.find(match->get_generated_uri()) != corefMap.end() &&
					(match->has_type(type + L"NameModel") || match->has_type(type + L"NameList")))
				{
					SessionLogger::info("LEARNIT") << L"Found next-node match (type 2): " 
						<< ind->get_name_or_desc()->get_value() << L" --> " << match->get_name_or_desc()->get_value() << L"\n";
					return match;
				}
			}
		}
	}

	EDTOffset head_start = st->getTokenSequence()->getToken(ment->getNode()->getHeadPreterm()->getStartToken())->getStartEDTOffset();
	
	std::wstring mentText = ment->getNode()->toTextString();
	if (mentText.find(L"the") == 0 && ment->getNode()->getHeadPreterm()->getTag() == Symbol(L"NN")) {		
		ElfIndividual_ptr nearest_preceding = ElfIndividual_ptr();
		int difference = -1;		
		ElfIndividual_ptr second_nearest_preceding = ElfIndividual_ptr();
		int second_difference = -1;
		typedef std::pair<std::wstring, std::wstring> coref_pair_t;
		BOOST_FOREACH(coref_pair_t pair, corefMap) {
			//if(pair.second == L"")
			//	continue;
			ElfIndividual_ptr coref_individual = docData->getElfDoc()->get_individual_by_generated_uri(pair.first);			
			const Mention *ant_ment = findBestMentionForBlitzIndividual(docData, coref_individual);
			if (ant_ment != 0 && ant_ment->getMentionType() == Mention::PRON)
				continue;

			EDTOffset start, end;
			coref_individual->get_name_or_desc()->get_offsets(start, end);
			if (end < head_start) {
				if (difference == -1 || head_start.value() - end.value() < difference) {
					second_difference = difference;
					second_nearest_preceding = nearest_preceding;
					difference = head_start.value() - end.value();
					nearest_preceding = coref_individual;
				} else if (second_difference == -1 || head_start.value() - end.value() < second_difference) {
					second_difference = head_start.value() - end.value();
					second_nearest_preceding = coref_individual;
				}
			}
		}
		if (nearest_preceding!= ElfIndividual_ptr() && second_nearest_preceding == ElfIndividual_ptr()) {
			//SessionLogger::info("LEARNIT") << L"Found nearest-preceding high-confidence match: " 
			//	<< ind->get_name_or_desc()->get_value() << L" --> " << nearest_preceding->get_name_or_desc()->get_value() << L"\n";
			return nearest_preceding;
		} else if (nearest_preceding != ElfIndividual_ptr() && !high_confidence_only) {
			//SessionLogger::info("LEARNIT") << L"Found nearest-preceding low-confidence match: " 
			//	<< ind->get_name_or_desc()->get_value() << L" --> " << nearest_preceding->get_name_or_desc()->get_value() << L"\n";
			return nearest_preceding;
		}	
	} 
	if (high_confidence_only)
		return ElfIndividual_ptr();
	//SessionLogger::info("LEARNIT") << L"Unmatched descriptor: " << ind->get_name_or_desc()->get_value() << L"\n";
						
	return ElfIndividual_ptr();
}

/** 
 *  Blitz helper function. Annoyingly, we have to search by rote for this.
 **/
const Mention *EIBlitz::findBlitzMentionByMentionID(EIDocData_ptr docData, MentionUID id) {
	const DocTheory *docTheory = docData->getDocTheory();
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		MentionSet *ms = docTheory->getSentenceTheory(sentno)->getMentionSet();
		for (int m = 0; m < ms->getNMentions(); m++) {
			const Mention *ment = ms->getMention(m);
			if (ment->getUID() == id)
				return ment;
		}
	}
	return 0;
}

/** 
 *  Blitz helper function. Annoyingly, we have to search by rote for this.
 **/

/** 
 *  Blitz helper function. Finding the Serif mention for an ElfMention is easy. Finding one for what comes 
 *  from a ValueMention is hard. We play games with offsets and are relatively conservative.
 **/
const Mention *EIBlitz::findBestMentionForBlitzIndividual(EIDocData_ptr docData, const ElfIndividual_ptr ind) {
	// we don't handle these in the BLitz.
	if (ind->has_entity_id()) 
		return 0;

	if (ind->has_mention_uid()) 
		return findBlitzMentionByMentionID(docData, ind->get_mention_uid());

	EDTOffset start, end;
	ind->get_name_or_desc()->get_offsets(start, end);

	const DocTheory *docTheory = docData->getDocTheory();
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		TokenSequence *ts = docTheory->getSentenceTheory(sentno)->getTokenSequence();
		EDTOffset end_of_sent = ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset();
		if (start < end_of_sent) {
			// then this is our sentence
			MentionSet *ms = docTheory->getSentenceTheory(sentno)->getMentionSet();
			// Allow exact matches to node extent, or places where the atomic head contains us
			for (int m = 0; m < ms->getNMentions(); m++) {
				const Mention *ment = ms->getMention(m);
				if (ment->getMentionType() != Mention::NAME && ment->getMentionType() != Mention::DESC &&
					ment->getMentionType() != Mention::PRON && ment->getMentionType() != Mention::PART)
					continue;				
				if (ts->getToken(ment->getNode()->getStartToken())->getStartEDTOffset() == start &&
					ts->getToken(ment->getNode()->getEndToken())->getEndEDTOffset() == end)
					return ment;
				const SynNode* atomicHead = ment->getAtomicHead();
				if (ts->getToken(atomicHead->getStartToken())->getStartEDTOffset() <= start &&
					ts->getToken(atomicHead->getEndToken())->getEndEDTOffset() >= end)
					return ment;
			}
			break;
		}
	}
	return 0;
}

/** 
 *  Blitz helper function. Try to find an individual (that already has a blitz coref id) that 
 *  matches this mention. Used when we are trying to do coref-- we can find that something is in a copula
 *  with some other Mention, and then we have to find out whether that's an ElfIndividual we can use.
 **/
ElfIndividual_ptr EIBlitz::findBestIndividualForBlitzMention(
	EIDocData_ptr docData, const Mention *ment, std::map<std::wstring, std::wstring>& corefMap, bool print_near_misses) {

	TokenSequence *ts = docData->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();
	EDTOffset head_start = ts->getToken(ment->getNode()->getStartToken())->getStartEDTOffset();
	EDTOffset head_end = ts->getToken(ment->getNode()->getEndToken())->getEndEDTOffset();

	typedef std::pair<std::wstring, std::wstring> coref_pair_t;
	// first try to find a mention that literally matches
	BOOST_FOREACH(coref_pair_t pair, corefMap) {
		ElfIndividual_ptr coref_individual = docData->getElfDoc()->get_individual_by_generated_uri(pair.first);
		if (coref_individual->has_mention_uid()) {
			if (coref_individual->get_mention_uid() == ment->getUID())
				return coref_individual;			
		}
	}
	// now try to find what is presumably a value mention that matches closely
	BOOST_FOREACH(coref_pair_t pair, corefMap) {
		ElfIndividual_ptr coref_individual = docData->getElfDoc()->get_individual_by_generated_uri(pair.first);
		if (!coref_individual->has_mention_uid() && ! coref_individual->has_entity_id()) {
			EDTOffset start, end;
			coref_individual->get_name_or_desc()->get_offsets(start, end);
			if (head_start == start && head_end == end) {
				//SessionLogger::info("LEARNIT") << L"Matched possible to real: " << ment->getNode()->toTextString() 
				// << L" --> " << coref_individual->get_name_or_desc()->get_value() << L"\n";
				return coref_individual;
			} else if (head_start >= start && head_end <= end) {
				// if this thing is the best mention for a blitz individual, allow the match
				const Mention *coref_ment = findBestMentionForBlitzIndividual(docData, coref_individual);
				if (coref_ment == ment)
					return coref_individual;
				//else if (print_near_misses)
					//SessionLogger::info("LEARNIT") << L"Near miss: " << ment->getNode()->toTextString() << L" --> " 
					// << coref_individual->get_name_or_desc()->get_value() << L"\n";
			}
		}
	}
	return ElfIndividual_ptr();
}


/** 
 *  Remove Blitz relations that are trying to point at a substance or condition, 
 *  but can't find a real one to point at.
 *  Also remove any of those arguments that don't point at a real substance/condition.
 **/
void EIBlitz::filterBlitzRelations(EIDocData_ptr docData) {
	std::set<ElfRelation_ptr> relationsToRemove;

	BOOST_FOREACH(ElfRelation_ptr elf_relation, docData->get_relations()) {
		bool bad_arg = false;
		BOOST_FOREACH(ElfRelationArg_ptr elf_rel_arg, elf_relation->get_args()) {
			ElfIndividual_ptr relation_ind = elf_rel_arg->get_individual();
			if (boost::starts_with(relation_ind->get_type()->get_value(), L"ic:PharmaceuticalSubstance") && (!relation_ind->has_coref_uri() || elf_rel_arg->get_role() != L"eru:substance"))
				bad_arg = true;
			if (boost::starts_with(relation_ind->get_type()->get_value(), L"ic:PhysiologicalCondition") && (!relation_ind->has_coref_uri() || elf_rel_arg->get_role() != L"eru:condition"))
				bad_arg = true;
		}
		if (bad_arg) {
			relationsToRemove.insert(elf_relation);
		} 
	}
	docData->remove_relations(relationsToRemove);
}

/** 
 *  Blitz types are funny when created, so that they carry meaning. But that meaning needs to go away
 *  before we output things. So, change the types, both in the individuals table and in the relation arguments.
 **/
void EIBlitz::cleanupBlitzIndividualTypes(EIDocData_ptr docData, const std::wstring & type_string) {
	BOOST_FOREACH(ElfRelation_ptr elf_relation, docData->get_relations()) {
		BOOST_FOREACH(ElfRelationArg_ptr elf_rel_arg, elf_relation->get_args()) {
			ElfIndividual_ptr individual = elf_rel_arg->get_individual();
			if (individual.get() == NULL || individual->has_value())
				continue;
			ElfType_ptr old_type = individual->get_type();
			if (boost::starts_with(old_type->get_value(), type_string) && old_type->get_value() != type_string) {
				EDTOffset start, end;
				old_type->get_offsets(start, end);
				if (old_type->get_value() != type_string) {
					ElfType_ptr new_type = boost::make_shared<ElfType>(type_string, old_type->get_string(), start, end);
					individual->set_type(new_type);
				}
			}
		}
	}

	ElfIndividualSet individualsToRemove;
	ElfIndividualSet individualsToAdd;
	BOOST_FOREACH(ElfIndividual_ptr individual, docData->get_individuals_by_type()) {
		if (individual.get() == NULL || individual->has_value())
			continue;
		ElfType_ptr old_type = individual->get_type();
		if (boost::starts_with(old_type->get_value(), type_string) && old_type->get_value() != type_string) {
			EDTOffset start, end;
			old_type->get_offsets(start, end);
			if (old_type->get_value() != type_string) {
				ElfType_ptr new_type = boost::make_shared<ElfType>(type_string, old_type->get_string(), start, end);
				ElfIndividual_ptr new_individual = boost::make_shared<ElfIndividual>(individual);
				new_individual->set_type(new_type);
				individualsToRemove.insert(individual);
				individualsToAdd.insert(new_individual);
			}
		}
	}	
	docData->remove_individuals(individualsToRemove);
	BOOST_FOREACH(ElfIndividual_ptr individual, individualsToAdd) {
		docData->getElfDoc()->insert_individual(individual);
	}
}
