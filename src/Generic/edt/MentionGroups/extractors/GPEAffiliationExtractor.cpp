// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Generic/edt/MentionGroups/extractors/GPEAffiliationExtractor.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/actors/AWAKEActorInfo.h"

#include <boost/foreach.hpp>

GPEAffiliationExtractor::GPEAffiliationExtractor() :
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"gpe")) 
{
	validateRequiredParameters();
	_use_awake_info = ParamReader::isParamTrue("do_actor_match");
}

void GPEAffiliationExtractor::validateRequiredParameters() {
	DescLinkFeatureFunctions::loadNationalities();
	DescLinkFeatureFunctions::loadNameGPEAffiliations();
}

std::vector<AttributeValuePair_ptr> GPEAffiliationExtractor::extractFeatures(const Mention& context,
                                                                             LinkInfoCache& cache,
                                                                             const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;
	
	std::set<std::wstring> modifiers;
	DescLinkFeatureFunctions::getGPEModNames(&context, docTheory, modifiers);
	BOOST_FOREACH(std::wstring mod, modifiers) {
		results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"modifier"), Symbol(mod), getFullName()));
	}
	
	std::vector<std::wstring> affiliations = DescLinkFeatureFunctions::getGPEAffiliations(&context, docTheory);
	BOOST_FOREACH(std::wstring aff, affiliations) {
		results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"affiliation"), Symbol(aff), getFullName()));
	}
	
	if (_use_awake_info && context.getEntityType().matchesPER()) {
		ActorInfo_ptr actorInfo = AWAKEActorInfo::getAWAKEActorInfo();

		/* Store mods, but only when they match a country actor */
		BOOST_FOREACH(std::wstring mod, modifiers) {
			if (actorInfo->isCountryActorName(mod))
				results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"country-actor-modifier"), Symbol(mod), getFullName()));
		}

		/* Get country affiliation for high quality actor match for PER mention */
		const SentenceTheory *st = docTheory->getSentenceTheory(context.getSentenceNumber());
		ActorMentionSet *ams = st->getActorMentionSet();
		std::set<ActorId> actorIds;
		BOOST_FOREACH(ActorMention_ptr am, ams->findAll(context.getUID())) {
			ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
			if (pnam && pnam->getPatternMatchScore() > 0) 
				actorIds.insert(pnam->getActorId());
		}
		if (actorIds.size() == 1) {
			BOOST_FOREACH(ActorId aid, actorIds) {
				BOOST_FOREACH(ActorId countryActorId, actorInfo->getAssociatedCountryActorIds(aid)) {
					std::wstring countryName = actorInfo->getActorName(countryActorId);
					std::transform(countryName.begin(), countryName.end(), countryName.begin(), std::towlower);
					results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"country-actor-affiliation"), Symbol(countryName), getFullName()));
				}
			}
		}
	}

	return results;
}
