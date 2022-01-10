// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/actors/LocationMentionResolver.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolConstants.h"
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <stdlib.h>


LocationMentionResolver::LocationMentionResolver(Gazetteer *extGazetteer)
{ 
	_gazetteer = extGazetteer;
	_max_ambiguity_for_gazetteer_actors = 3;
	_verbosity = 3;

	std::vector <std::wstring> fields;
	std::set<std::wstring> all_rules = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("gazetteer_location_rules"), true, false);
	BOOST_FOREACH(std::wstring rule, all_rules)
	{
		boost::algorithm::split( fields, rule, boost::algorithm::is_any_of( "\t" ) );
		std::pair<boost::wregex, std::wstring> ruleResolutionPair;
		ruleResolutionPair.first = boost::wregex(fields[0], boost::regex_constants::icase);
		ruleResolutionPair.second = fields[1];
		_rules.push_back(ruleResolutionPair); 
	}
}

//
// GET LISTS OF RESOLUTIONS
//

std::vector<Gazetteer::GeoResolution_ptr> LocationMentionResolver::getBestResolutionSetForCanonicalNames(std::vector<std::wstring>& canonicalNames) const {
	std::vector<Gazetteer::GeoResolution_ptr> resolutions;
	for (std::vector<std::wstring>::iterator iter = canonicalNames.begin(); iter != canonicalNames.end(); iter++) {
		resolutions = _gazetteer->geonameLookup(*iter);
		if (resolutions.size() > 0)
			break;
	}
	return resolutions;
}


// Get all possible resolutions from gazetteer table (NOT countries table)
std::map<Symbol, Gazetteer::GeoResolution_ptr> LocationMentionResolver::getAllPossibleResolutionsOnePerCountry(std::vector<std::wstring>& canonicalNames) const {
	std::vector<Gazetteer::GeoResolution_ptr> resolutions = getBestResolutionSetForCanonicalNames(canonicalNames);
	std::map<Symbol, Gazetteer::GeoResolution_ptr> best_country_resolutions;
	BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, resolutions) {
		size_t population = candidateResolution->population;
		Symbol country = SymbolConstants::nullSymbol;
		if (candidateResolution->countryInfo)
			country = candidateResolution->countryInfo->isoCode;
		if (best_country_resolutions[country] == Gazetteer::GeoResolution_ptr() || population > best_country_resolutions[country]->population)
			best_country_resolutions[country] = candidateResolution;	
	}
	return best_country_resolutions;
}

// Get all possible resolutions from COUNTRIES table
std::vector<Gazetteer::GeoResolution_ptr> LocationMentionResolver::getAllPossibleCountryResolutions(std::vector<std::wstring>& canonicalNames) const {
	BOOST_FOREACH(std::wstring name, canonicalNames) {
		std::vector<Gazetteer::GeoResolution_ptr> resolutions = _gazetteer->countryLookup(name);
		if (resolutions.size() > 0)
			return resolutions;
	}
	return std::vector<Gazetteer::GeoResolution_ptr>();
}

// Get all possible gazetteer resolutions from a particular country
std::vector<Gazetteer::GeoResolution_ptr> LocationMentionResolver::getAllResolutionsInCountry(std::vector<std::wstring>& canonicalNames, const Symbol countryCode) const {
	for(std::vector<std::wstring>::iterator iter = canonicalNames.begin(); iter != canonicalNames.end(); iter++) {
		std::vector<Gazetteer::GeoResolution_ptr> result;	
		std::vector<Gazetteer::GeoResolution_ptr> resolutions = _gazetteer->geonameLookup(*iter);
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, resolutions) {
			if (candidateResolution->countrycode == countryCode) 
				result.push_back(candidateResolution);
		}
		if (!result.empty())
			return result;
	}
	return std::vector<Gazetteer::GeoResolution_ptr>();
}

//
// MANUALLY-SPECIFIED RULES
//

bool LocationMentionResolver::isBlockedLocation(const SentenceTheory *sentTheory, const Mention* ment) const
{
	std::vector<std::wstring> temp = _gazetteer->toCanonicalForms(sentTheory, ment);
	return isBlockedLocation(temp);
}
bool LocationMentionResolver::isBlockedLocation(std::vector<std::wstring>& names) const
{	
	BOOST_FOREACH(std::wstring str, names) {
		if (isBlockedLocation(str))
			return true;
	}
	return false;
}
bool LocationMentionResolver::isBlockedLocation(std::wstring canonical_name) const
{
	BOOST_FOREACH(const boost::wregex& block_re, _gazetteer->getBlockedEntries())
	{
		if (boost::regex_match(canonical_name, block_re)) 
		{
			return true;
		}
	}
	return false;
}

Gazetteer::GeoResolution_ptr LocationMentionResolver::getRuleBasedResolution(std::vector<std::wstring>& canonicalNames) const
{
	std::pair<boost::wregex, std::wstring> rule;
	BOOST_FOREACH(rule, _rules)	{
		// Avoid nested BOOST_FOREACHes
		for (std::vector<std::wstring>::iterator iter = canonicalNames.begin(); iter != canonicalNames.end(); iter++) {
			if (boost::regex_match(*iter, rule.first)) {
				std::vector <std::wstring> fields;
				boost::algorithm::split( fields, rule.second, boost::algorithm::is_any_of( ":" ) );
				boost::trim(fields[1]);
				if (fields[0] == L"geo_id") return _gazetteer->getGeoResolution(fields[1]);
				else return _gazetteer->getCountryResolution(fields[1]);
			}
		}
	}

	return boost::make_shared<Gazetteer::GeoResolution>();
}

bool LocationMentionResolver::isUSGeorgia(ActorMentionSet *documentActorMentionSet, const Mention* ment, 
										  ActorId::HashMap<float>& countryCounts, ActorId usaActorId) const 
{

	// !!NOTE!!: This function assumes that what is being passed in as the actor mention set (documentActorMentionSet)
	//  is really the actor mention set for the WHOLE DOCUMENT (not just a sentence). Otherwise the assumptions/heuristics
	//  contained here will not work. Also countryCounts must be filled in!

	bool has_non_us_country = false;	
	for (ActorId::HashMap<float>::iterator iter = countryCounts.begin(); iter != countryCounts.end(); ++iter) {
		ActorId country = (*iter).first;
		if (country != usaActorId && (*iter).second > 0)
			has_non_us_country = true;
	}

	// Look for places in the US state of Georgia (e.g. Atlanta)
	static Symbol georgiaRegionCode(L"GA");
	BOOST_FOREACH(ActorMention_ptr actorMention, documentActorMentionSet->getAll()) {
		if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
			Gazetteer::GeoResolution_ptr resolution = pn_actor->getGeoResolution();
			if (!resolution)
				continue;
			if (_gazetteer->getGeoRegion(resolution->geonameid) == georgiaRegionCode)
				return true;
		}
	}
	if (has_non_us_country)
		return false;
	return true;
}

// 
// MASTER RESOLUTION FUNCTION
//

void LocationMentionResolver::getCandidateResolutions(const SentenceTheory *sentTheory, const Mention* ment, 
													  Gazetteer::SortedGeoResolutions &candidateResolutions,
													  bool one_per_country,
													  std::vector<CountryId> allowableCountries) 
{
	EntityType entityType = ment->getEntityType();
	std::vector<std::wstring> canonicalNames = _gazetteer->toCanonicalForms(sentTheory, ment);

	if (isBlockedLocation(canonicalNames))
		return;

	// If we've manually specified the resolution for something, use only that
	Gazetteer::GeoResolution_ptr ruleResolution = getRuleBasedResolution(canonicalNames);
	if (!ruleResolution->isEmpty) {	
		if (isFromAllowableCountry(ruleResolution, allowableCountries)) {
			candidateResolutions.insert(std::make_pair(100.0F, ruleResolution)); // high score since it's rule-based
			return;
		}
	}	

	// first check to see if there are any matches in the country table
	std::vector<Gazetteer::GeoResolution_ptr> possibleCountryResolutions = getAllPossibleCountryResolutions(canonicalNames);

	if (possibleCountryResolutions.size() > 0) 
	{	
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, possibleCountryResolutions) {
			if (isFromAllowableCountry(candidateResolution, allowableCountries)) {
				addScoredCandidateResolution(candidateResolutions, candidateResolution);
			}
		}

		// Countries trump everything else, so return only countries if we have one
		if (candidateResolutions.size() > 0) {
			// Very special rule for Georgia because it is both a country and a non-country
			if (std::find(canonicalNames.begin(), canonicalNames.end(), L"georgia") == canonicalNames.end())
				return;
		}
	}

	// if no potential matches are found in the countries table (or we are Georgia), look in geonames/altnames
	if (one_per_country) {
		std::map<Symbol, Gazetteer::GeoResolution_ptr> possibleResolutions = getAllPossibleResolutionsOnePerCountry(canonicalNames);
		typedef std::pair<Symbol, Gazetteer::GeoResolution_ptr> country_res_pair_t;
		BOOST_FOREACH(country_res_pair_t my_pair, possibleResolutions) {
			if (isFromAllowableCountry(my_pair.second, allowableCountries)) {
				addScoredCandidateResolution(candidateResolutions, my_pair.second);
			}
		}
	} else {
		std::vector<Gazetteer::GeoResolution_ptr> resolutions = getBestResolutionSetForCanonicalNames(canonicalNames);
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr resolution, resolutions) {
			if (isFromAllowableCountry(resolution, allowableCountries)) {
				addScoredCandidateResolution(candidateResolutions, resolution);
			}
		}
	}


	return;
}

Gazetteer::ScoredGeoResolution LocationMentionResolver::getUnambiguousCountryResolution(const SentenceTheory *sentTheory, const Mention* ment, ActorInfo_ptr actorInfo) {
	std::vector<std::wstring> canonicalNames = _gazetteer->toCanonicalForms(sentTheory, ment);
    std::vector<Gazetteer::GeoResolution_ptr> possibleCountryResolutions;
	std::vector<Gazetteer::GeoResolution_ptr> allResolutions = getBestResolutionSetForCanonicalNames(canonicalNames);
	BOOST_FOREACH(Gazetteer::GeoResolution_ptr geo, allResolutions) {
		ActorId actorId = actorInfo->getActorIdForGeonameId(geo->geonameid);
		if (actorInfo->isACountry(actorId)) 
			possibleCountryResolutions.push_back(geo);
	}
	if (possibleCountryResolutions.size() != 1)
		return createEmptyResolution();
	return std::make_pair(100.0F, possibleCountryResolutions.at(0));
}

Gazetteer::ScoredGeoResolution LocationMentionResolver::getICEWSLocationResolution(ActorMentionSet *documentActorMentionSet, 
		std::vector<ActorMatch> patternActorMatches, ActorId::HashMap<float>& countryCounts, ActorId usaActorId, const SentenceTheory *sentTheory, 
		const Mention* ment, bool allow_ambiguity, std::vector<CountryId> allowableCountries, ActorInfo_ptr _actorInfo) 
{
	EntityType entityType = ment->getEntityType();
	std::vector<std::wstring> canonicalNames = _gazetteer->toCanonicalForms(sentTheory, ment);
	
	// See if we've already resolved this location string
	// Right now the only things that are registered are those that are getting assigned
	//   a special resolution that they would not otherwise get. As of 8/31/2015, this is just
	//   United States cities/states via "Minneapolis, Minnesota" constructions
	BOOST_FOREACH(std::wstring str, canonicalNames) {
		Gazetteer::ScoredGeoResolution registered = getRegisteredResolution(str);
		if (registered.first > 0) {
			return registered;
		}
	}

	// Very special rule for Georgia-- note that this assumes this function is called using a 
	//  documentActorMentionSet that already is filled in with unambiguous actors	
	static Symbol usCountryCode(L"US");
	if (std::find(canonicalNames.begin(), canonicalNames.end(), L"georgia") != canonicalNames.end()) {
		// Georgia is always ambiguous
		if (!allow_ambiguity)
			return createEmptyResolution();
		// If this doesn't return, life will proceed as usual, and this will end up 
		//   being tagged as Georgia the country, since its population is higher
		if (isUSGeorgia(documentActorMentionSet, ment, countryCounts, usaActorId)) {
			Gazetteer::GeoResolution_ptr stateRes = getResolutionInRegion(sentTheory, ment, usCountryCode);		
			if (stateRes && isFromAllowableCountry(stateRes, allowableCountries)) {
				return std::make_pair(100.0F, stateRes);
			} 
		} 
	}

	// Check to see whether we have an exact match from a pattern
	// If so, prefer it. If not, we're going to hope that our gazetteer can beat it
	int stoken1 = ment->getNode()->getStartToken();
	int etoken1 = ment->getNode()->getEndToken();
	int stoken2 = ment->getAtomicHead()->getStartToken();
	int etoken2 = ment->getAtomicHead()->getEndToken();
	BOOST_FOREACH(ActorMatch am, patternActorMatches) {
		if (!_actorInfo->mightBeALocation(am.id))
			continue;
		if (am.start_token == stoken1 && am.end_token == etoken1)
			return createEmptyResolution();
		if (am.start_token == stoken2 && am.end_token == etoken2)
			return createEmptyResolution();
	}

	// Get just the best one per country, since we are in ICEWS-land
	Gazetteer::SortedGeoResolutions sortedGeoResolutions;
	getCandidateResolutions(sentTheory, ment, sortedGeoResolutions, true, allowableCountries);
	
	int max_ambiguity = 1;
	if (allow_ambiguity)
		max_ambiguity = _max_ambiguity_for_gazetteer_actors;

	if (sortedGeoResolutions.size() == 0)
		return createEmptyResolution();

	Gazetteer::SortedGeoResolutions::reverse_iterator it = sortedGeoResolutions.rbegin();

	// Don't bother with anything fancy if there's only one option
	if (sortedGeoResolutions.size() == 1)
		return (*it);

	Gazetteer::GeoResolution_ptr naiveBest = (*it).second;

	// Look at the subset of georesolutions where there is some external
	//  evidence that this country is mentioned in the document (or it's a really big city)
	Gazetteer::SortedGeoResolutions vettedGeoResolutions;
	for (; it != sortedGeoResolutions.rend(); it++) {
		if ((*it).second->countryInfo) {
			ActorId countryActorId = (*it).second->countryInfo->actorId;
			if (countryCounts[countryActorId] > 0 || (*it).second->population > 1000000) {
				vettedGeoResolutions.insert(Gazetteer::ScoredGeoResolution((*it).first + countryCounts[countryActorId], (*it).second));
			}
		} else if ((*it).second->population > 1000000) {
			vettedGeoResolutions.insert(*it);
		}
	}

	if (vettedGeoResolutions.size() > 0) {

		if (static_cast<int>(vettedGeoResolutions.size()) > max_ambiguity) {
			if (_verbosity > 3 && allow_ambiguity) {
				std::stringstream msg;
				msg << "Blocked too-ambiguous gazetteer location: \""
					<< ment->toCasedTextString() << "\" resolves to " << sortedGeoResolutions.size() << " different VETTED countries.";
				SessionLogger::info("ICEWS") << msg.str();
			}
			return createEmptyResolution();
		} 

		Gazetteer::SortedGeoResolutions::reverse_iterator vit = vettedGeoResolutions.rbegin();
		if (naiveBest->geonameid != (*vit).second->geonameid) {
			std::stringstream msg;
			msg << "    Picking lower-population resolution due to document context: " << ment->toCasedTextString() << " -- ";
			if (naiveBest->countryInfo)
				msg << "naive best country = " << naiveBest->countryInfo->isoCode << ", ";
			if ((*vit).second->countryInfo)
				msg << "picked country = " << (*vit).second->countryInfo->isoCode << " ";
			SessionLogger::info("ICEWS") << msg.str();
		}
		return (*vit);
	} else {

		if (static_cast<int>(sortedGeoResolutions.size()) > max_ambiguity) {
			if (_verbosity > 3 && allow_ambiguity) {
				std::stringstream msg;
				msg << "Blocked too-ambiguous gazetteer location: \""
					<< ment->toCasedTextString() << "\" resolves to " << sortedGeoResolutions.size() << " different countries (none of which are vetted).";
				SessionLogger::info("ICEWS") << msg.str();
			}
			return createEmptyResolution();
		} 

		SessionLogger::info("ICEWS") << "    Note: no supporting evidence for any resolution: " << ment->toCasedTextString();
		it = sortedGeoResolutions.rbegin();
		return (*it);
	}

	// if no match found return an empty resolution
	return createEmptyResolution();
}

//
// HELPER FUNCTIONS
//

bool LocationMentionResolver::isFromAllowableCountry(Gazetteer::GeoResolution_ptr candidateResolution, std::vector<CountryId> allowableCountries) {
	if (allowableCountries.size() == 0)
		return true;
	if (!candidateResolution->countryInfo)
		return false;
	return (std::find(allowableCountries.begin(), allowableCountries.end(), candidateResolution->countryInfo->countryId) != allowableCountries.end());
}

void LocationMentionResolver::addScoredCandidateResolution(Gazetteer::SortedGeoResolutions& sortedGeoResolutions, 
														   Gazetteer::GeoResolution_ptr candidateResolution) 
{
	double score = 20.0;

	if (candidateResolution->population > 0)
		score += log(static_cast<double>(candidateResolution->population));
		
	// We need a way to break ties when the entries have the same population 
	// (happens when they have no population, typically). Currently the maximum 
	// geonameid is 8,647,700. So, we multiply by geonameid / 10,000,000.
	// _wtoi should return 0 if the geonameid is not an integer, so this
	// is safe despite the DB not enforcing this.
	int geonameid = _wtoi(candidateResolution->geonameid.c_str());
	score += geonameid / 10000000.0F;
	
	sortedGeoResolutions.insert(Gazetteer::ScoredGeoResolution(score, candidateResolution));
}

Gazetteer::GeoResolution_ptr LocationMentionResolver::getResolutionInRegion(const SentenceTheory *sentTheory, const Mention* mention, Symbol countryCode, Symbol regionCode)
{
	
	Gazetteer::SortedGeoResolutions sortedGeoResolutions;	
	std::vector<std::wstring> canonicalNames = _gazetteer->toCanonicalForms(sentTheory, mention);
	std::vector<Gazetteer::GeoResolution_ptr> possibleResolutions = getAllResolutionsInCountry(canonicalNames, countryCode);
	if (possibleResolutions.size() > 0) 
	{	
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, possibleResolutions) 
		{
			if (regionCode.is_null() || _gazetteer->getGeoRegion(candidateResolution->geonameid) == regionCode) {
				sortedGeoResolutions.insert(Gazetteer::ScoredGeoResolution(log(static_cast<double>(candidateResolution->population)), candidateResolution));
			}
		}
		// if we have not matches after filtering
		if (sortedGeoResolutions.size() == 0) 
			return boost::make_shared<Gazetteer::GeoResolution>();

		Gazetteer::SortedGeoResolutions::reverse_iterator it = sortedGeoResolutions.rbegin();
		const Gazetteer::ScoredGeoResolution& best = *it++;
		if (sortedGeoResolutions.size() > 1) {
			const Gazetteer::ScoredGeoResolution& secondBest = *it;

			// if there is a tie, block resolution
			if (best.first - secondBest.first < 0.02) {
				SessionLogger::info("ICEWS") << "Blocked ambiguous gazetteer location for \""
											 << mention->toCasedTextString() << "\": two different locations in a country (" 
											 << best.second << " and " << secondBest.second
											 << ") both have the similar scores (" << best.first 
											 << " and " << secondBest.first << ")";
				// return empty resolution
				return boost::make_shared<Gazetteer::GeoResolution>();
			}
			else return best.second;
		}
		else return best.second;
	}
	return boost::make_shared<Gazetteer::GeoResolution>();
}

