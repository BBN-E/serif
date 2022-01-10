#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/Offset.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/bsp_declare.h"
#include "boost/tuple/tuple.hpp"
#include <set>
#include <vector>
#include <map>
#include "PredFinder/common/ContainerTypes.h"

class Mention;
class ElfIndividual;
class EIDocData;
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(EIDocData);
#pragma once


class EIBlitz {
public:
	static void manageBlitzCoreference(EIDocData_ptr docData, const std::wstring & type, int& id_counter);
	static void filterBlitzRelations(EIDocData_ptr docData);	
	static void cleanupBlitzIndividualTypes(EIDocData_ptr docData, const std::wstring & type);
	static void addBlitzNamesToMap(EIDocData_ptr docData, const ElfIndividualSet& individuals, 
		std::map<std::wstring, std::wstring>& corefMap, int& id_counter);
	static void addBlitzDescriptorsToMap(EIDocData_ptr docData, const std::wstring & type, const ElfIndividualSet& individuals, 
		std::map<std::wstring, std::wstring>& corefMap, int& id_counter);
	static void addBlitzPossiblesToMapOrig(EIDocData_ptr docData, const std::wstring & type, 
		const ElfIndividualSet& individuals, std::map<std::wstring, std::wstring>& corefMap, int& id_counter);
	static void addBlitzPossiblesToMapPron(EIDocData_ptr docData, const std::wstring & type, 
		const ElfIndividualSet& individuals, std::map<std::wstring, std::wstring>& corefMap, int& id_counter);

	static void transitionBlitzIndividual(EIDocData_ptr docData, const ElfIndividual_ptr original_individual, 
		const std::wstring & coref_id, ElfIndividualUriMap & newIndividualIDMap,
		ElfIndividualSet & individualsToRemove);
	static void transitionBlitzIndividualsByTypeAndSuffix(EIDocData_ptr docData, const std::map<std::wstring, std::wstring> & corefMap,
												const std::wstring & type,
												const std::wstring & suffix,
												ElfIndividualUriMap & newIndividualIDMap,
												ElfIndividualSet & individualsToRemove);
	//void linkToOnlyName(std::map<std::wstring, std::wstring> corefMap, std::wstring type);
	static const Mention *findBlitzMentionByMentionID(EIDocData_ptr docData, MentionUID id);	
	static const Mention *findBestMentionForBlitzIndividual(EIDocData_ptr docData, const ElfIndividual_ptr ind);
	static ElfIndividual_ptr findBestIndividualForBlitzMention(EIDocData_ptr docData, const Mention *ment, 
		std::map<std::wstring, std::wstring>& corefMap,	bool print_near_misses = false);
	static ElfIndividual_ptr findBlitzAntecedent(EIDocData_ptr docData, const std::wstring & type, const ElfIndividual_ptr ind, 
		std::map<std::wstring, std::wstring>& corefMap, bool high_confidence_only); 
	static bool blitzIndividualHasName(EIDocData_ptr docData, const std::wstring & blitz_id, 
		std::map<std::wstring, std::wstring>& corefMap);
	static ElfIndividual_ptr findBlitzPronAntecedent(EIDocData_ptr docData, const Mention* ment, 
		std::map<std::wstring, std::wstring>& corefMap);
	static bool killBlitzIndividual(EIDocData_ptr docData, const ElfIndividual_ptr ind);
};

