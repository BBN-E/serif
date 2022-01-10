// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "LocationMentionResolver.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <stdlib.h>

namespace ICEWS {


//// LocationMentionResolver (LMR) -- base class


std::vector<Gazetteer::GeoResolution_ptr> LocationMentionResolver::getAllPossibleResolutions(const Mention* ment) const
{
	EntityType entityType = ment->getEntityType();
	if (!((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())))
	{
		throw invalid_argument("getAllPossibleResolutions called on an actor mention that isn't a location or GPE");
	}
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);
	// (optimization) to prevent querying the database twice, check if head string == full string
	std::vector<Gazetteer::GeoResolution_ptr> head_resolutions = _gazetteer->geonameLookup(head_string);
	if (head_string == full_string || head_resolutions.size() > 0)
	{
		return head_resolutions;
	}
	else 
	{
		std::vector<Gazetteer::GeoResolution_ptr> full_resolutions = _gazetteer->geonameLookup(full_string);
		return full_resolutions;
	}
}


////	RuleBasedLMR


RuleBasedLMR::RuleBasedLMR(Gazetteer *extGazetteer)
	: LocationMentionResolver(extGazetteer)
{	
	vector <std::wstring> fields;
	std::set<std::wstring> all_rules = InputUtil::readFileIntoSet(ParamReader::getParam("icews_location_rules"), true, false);
	BOOST_FOREACH(std::wstring rule, all_rules)
	{
		boost::algorithm::split( fields, rule, boost::algorithm::is_any_of( "\t" ) );
		std::pair<boost::wregex, std::wstring> ruleResolutionPair;
		ruleResolutionPair.first = boost::wregex(fields[0], boost::regex_constants::icase);
		ruleResolutionPair.second = fields[1];
		_rules.push_back(ruleResolutionPair); 
	}
}


Gazetteer::GeoResolution_ptr RuleBasedLMR::getLocationResolution(const Mention* ment) const
{
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);

	// initialize to empty resolution
	Gazetteer::GeoResolution_ptr emptyResolution = boost::make_shared<Gazetteer::GeoResolution>();

	// check to see if the string is blocked
	BOOST_FOREACH(const boost::wregex& block_re, _gazetteer->getBlockedEntries())
	{	
		if (boost::regex_match(full_string, block_re) || boost::regex_match(head_string, block_re)) 
		{
			// change isEmpty flag to false so that blocked entries aren't considered further down the pipeline
			emptyResolution->isEmpty = false;
			return emptyResolution;
		}
	}

	// check to see if we have a rule for the string
	std::pair<boost::wregex, std::wstring> rule;
	BOOST_FOREACH(rule, _rules)
	{
		if (boost::regex_match(head_string, rule.first) || boost::regex_match(full_string, rule.first))  
		{
			vector <std::wstring> fields;
			boost::algorithm::split( fields, rule.second, boost::algorithm::is_any_of( ":" ) );
			boost::trim(fields[1]);
			if (fields[0] == L"geo_id") return _gazetteer->getGeoResolution(fields[1]);
			else return _gazetteer->getCountryResolution(fields[1]);
		}
	}

	return emptyResolution;
}


bool RuleBasedLMR::hasRuleForLocation(std::wstring location_mention_text) const
{
	location_mention_text = _gazetteer->toCanonicalForm(location_mention_text);
	// check to see if we have a regexp resolution rule
	std::pair<boost::wregex, std::wstring> rule;
	BOOST_FOREACH(rule, _rules)
	{
		if (boost::regex_match(location_mention_text, rule.first)) 
		{
			return true;
		}
	}
	return false;
}

bool RuleBasedLMR::isBlockedLocation(std::wstring location_mention_text) const
{
	location_mention_text = _gazetteer->toCanonicalForm(location_mention_text);
	// check to see if entry is blocked
	BOOST_FOREACH(const boost::wregex& block_re, _gazetteer->getBlockedEntries())
	{
		if (boost::regex_match(location_mention_text, block_re)) 
		{
			return true;
		}
	}
	return false;
}


//// CountryLMR


std::vector<Gazetteer::GeoResolution_ptr> CountryLMR::getAllPossibleResolutions(const Mention* ment) const throw( invalid_argument )
{
	EntityType entityType = ment->getEntityType();
	if (!((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())))
	{
		throw invalid_argument("getAllPossibleResolutions called on an actor mention that isn't a location or GPE");
	}
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);
	// (optimization) to prevent querying the database twice, check if head string == full string and if there are resolutions based only on the head
	std::vector<Gazetteer::GeoResolution_ptr> head_resolutions = _gazetteer->countryLookup(head_string);
	if (head_string == full_string || head_resolutions.size() > 0)
	{
		return head_resolutions;
	}
	else 
	{
		std::vector<Gazetteer::GeoResolution_ptr> full_resolutions = _gazetteer->countryLookup(full_string);
		return full_resolutions;
	}
}

std::vector<Gazetteer::GeoResolution_ptr> CountryLMR::getAllResolutionsInCountry(const Mention* ment, const Symbol countryCode)
{
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);
	std::vector<Gazetteer::GeoResolution_ptr> result;
	// (optimization) to prevent querying the database twice, check if head string == full string and if there are resolutions based only on the head
	std::vector<Gazetteer::GeoResolution_ptr> head_resolutions = _gazetteer->geonameLookup(head_string);
	BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, head_resolutions) 
	{
		if (candidateResolution->countrycode == countryCode) result.push_back(candidateResolution);
	}				
	if (result.empty())
	{
		std::vector<Gazetteer::GeoResolution_ptr> full_resolutions = _gazetteer->geonameLookup(full_string);
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, full_resolutions) 
		{
			if (candidateResolution->countrycode == countryCode) result.push_back(candidateResolution);
		}	
	}
	return result;
}

Gazetteer::GeoResolution_ptr CountryLMR::getLocationResolution(const Mention* ment) const
{
	// get list of all possible resolutions based on actorMention text
	std::vector<Gazetteer::GeoResolution_ptr> resolutions = getAllPossibleResolutions(ment);
	Gazetteer::GeoResolution_ptr bestResolution = boost::make_shared<Gazetteer::GeoResolution>();
	// if more than one country might match, do not resolve
	if (resolutions.size() > 1)
	{
		return bestResolution;	
	}
	else if (resolutions.size() == 0) 
	{
		return bestResolution;
	}
	else 
	{
		bestResolution->geonameid = resolutions[0]->geonameid;
		bestResolution->cityname = resolutions[0]->cityname;
		bestResolution->population = resolutions[0]->population;
		bestResolution->latitude = resolutions[0]->latitude;
		bestResolution->longitude = resolutions[0]->longitude;
		bestResolution->isEmpty = false;
		bestResolution->countrycode = resolutions[0]->countrycode;
		bestResolution->countryInfo = resolutions[0]->countryInfo;
		return bestResolution;
	}
}


//// PopulationLMR


Gazetteer::GeoResolution_ptr PopulationLMR::getLocationResolution(const Mention* ment) const
{
	// get list of all possible resolutions based on actorMention text
	std::vector<Gazetteer::GeoResolution_ptr> resolutions = getAllPossibleResolutions(ment);
	// initialize to empty resolution
	Gazetteer::GeoResolution_ptr bestResolution = boost::make_shared<Gazetteer::GeoResolution>();
	BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, resolutions) 
	{
		if (candidateResolution->population > bestResolution->population) 
		{
			bestResolution = candidateResolution;
		}
	}
	return bestResolution;
}


//// BaselineLMR


Gazetteer::GeoResolution_ptr BaselineLMR::getLocationResolution(const Mention* ment) const
{
	EntityType entityType = ment->getEntityType();
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);
	if (rLMR.hasRuleForLocation(head_string) || rLMR.hasRuleForLocation(full_string))
	{
		return rLMR.getLocationResolution(ment);
	}
	else
	{
		// try resolving to country first
		Gazetteer::GeoResolution_ptr resolution = cLMR.getLocationResolution(ment);
		// if resolution is empty, look at geonames using the population LMR
		if (resolution)
		{
			resolution = pLMR.getLocationResolution(ment);
		}
		return resolution;
	}
}


//// ComboLMR


Gazetteer::GeoResolution_ptr ComboLMR::getUnambiguousResolution(const Mention* ment)
{
	EntityType entityType = ment->getEntityType();
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);

	// see if we've already resolved this location
	Gazetteer::GeoResolution_ptr registered = getRegisteredResolution(head_string);
	if (!registered->isEmpty) return registered;
	registered = getRegisteredResolution(full_string);
	if (!registered->isEmpty) return registered;

	// check to see if location is blocked
	if (rLMR.isBlockedLocation(head_string) || rLMR.isBlockedLocation(full_string))
	{
		registerResolution(head_string, registered);
		registerResolution(full_string, registered);
		return boost::make_shared<Gazetteer::GeoResolution>();
	}

	if (rLMR.hasRuleForLocation(head_string) || rLMR.hasRuleForLocation(full_string))
	{
		registered = rLMR.getLocationResolution(ment);
		registerResolution(head_string, registered);
		registerResolution(full_string, registered);
		return registered;
	}
	
	// first look for a country match
	registered = cLMR.getLocationResolution(ment);
	if (!registered->isEmpty)
	{
		registerResolution(head_string, registered);
		registerResolution(full_string, registered);
		return registered;
	}

	// resolution is ambiguous, return empty resolution
	return boost::make_shared<Gazetteer::GeoResolution>();
}

Gazetteer::GeoResolution_ptr ComboLMR::getLocationResolution(const Mention* ment) const
{
	float baseline_score = 10; 
	Gazetteer::SortedGeoResolutions sortedGeoResolutions;

	EntityType entityType = ment->getEntityType();
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);

	// see if we've already resolved this location
	Gazetteer::GeoResolution_ptr registered = getRegisteredResolution(head_string);
	if (!registered->isEmpty) return registered;
	registered = getRegisteredResolution(full_string);
	if (!registered->isEmpty) return registered;

	// check to see if location is blocked
	if (rLMR.isBlockedLocation(head_string) || rLMR.isBlockedLocation(full_string))
	{
		return boost::make_shared<Gazetteer::GeoResolution>();
	}

	// first check to see if there are any matches in the country table
	std::vector<Gazetteer::GeoResolution_ptr> possibleResolutions = cLMR.getAllPossibleResolutions(ment);
	if (possibleResolutions.size() > 0) 
	{	
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, possibleResolutions) 
		{
			sortedGeoResolutions.insert(Gazetteer::ScoredGeoResolution(baseline_score + getAssociationScore(candidateResolution) + log(static_cast<float>(candidateResolution->population)), 
															candidateResolution));
		}
		Gazetteer::SortedGeoResolutions::reverse_iterator it = sortedGeoResolutions.rbegin();
		const Gazetteer::ScoredGeoResolution& best = *it++;
		if (possibleResolutions.size() > 1) {
			const Gazetteer::ScoredGeoResolution& secondBest = *it;

			// if there is a tie, block resolution
			if (best.first - secondBest.first < 0.02) {
				SessionLogger::info("ICEWS") << "Blocked ambiguous gazetteer location for \""
											 << ment->toCasedTextString() << "\": two different countries (" 
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

	// if no potential matches are found in the countries table, look in geonames/altnames
	possibleResolutions = getAllPossibleResolutions(ment);

	if ((_maxAmbiguityForGazetteerActors>=0) && (static_cast<int>(possibleResolutions.size()) > _maxAmbiguityForGazetteerActors)) 
	{
		if (_verbosity > 3) {
			std::stringstream msg;
			msg << "Blocked ambiguous gazetteer location: \""
				<< full_string << "\" has " << possibleResolutions.size() << " entries.";
			SessionLogger::info("ICEWS") << msg.str();
		}
		return boost::make_shared<Gazetteer::GeoResolution>();
	} 

	BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, possibleResolutions) 
	{
		float associationScore = getAssociationScore(candidateResolution);
		sortedGeoResolutions.insert(Gazetteer::ScoredGeoResolution(baseline_score + associationScore + log(static_cast<float>(candidateResolution->population)), 
														candidateResolution));
	}
	
	if (sortedGeoResolutions.size() > 0) {
		Gazetteer::SortedGeoResolutions::reverse_iterator it = sortedGeoResolutions.rbegin();
		const Gazetteer::ScoredGeoResolution& best = *it++;
		if (sortedGeoResolutions.size() > 1) {
			const Gazetteer::ScoredGeoResolution& secondBest = *it;
			// if there is a tie, block resolution
			if (best.first - secondBest.first < 0.02) {
				SessionLogger::info("ICEWS") << "Blocked ambiguous gazetteer location for \""
											 << ment->toCasedTextString() << "\": two different geos (" 
											 << best.second << " and " << secondBest.second
											 << ") both have the similar scores (" << best.first 
											 << " and " << secondBest.first << ")";
				// return empty resolution
				return boost::make_shared<Gazetteer::GeoResolution>();
			}
		}
		return best.second;
	}


	// if no match found return an empty resolution
	return boost::make_shared<Gazetteer::GeoResolution>();
}

Gazetteer::GeoResolution_ptr ComboLMR::getResolutionInCountry(const Mention* mention, Symbol countryCode)
{
	float baseline_score = 10;
	Gazetteer::SortedGeoResolutions sortedGeoResolutions;
	std::vector<Gazetteer::GeoResolution_ptr> possibleResolutions = cLMR.getAllResolutionsInCountry(mention, countryCode);
	if (possibleResolutions.size() > 0) 
	{	
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, possibleResolutions) 
		{
			sortedGeoResolutions.insert(Gazetteer::ScoredGeoResolution(baseline_score + log(static_cast<float>(candidateResolution->population)), 
															candidateResolution));
		}
		Gazetteer::SortedGeoResolutions::reverse_iterator it = sortedGeoResolutions.rbegin();
		const Gazetteer::ScoredGeoResolution& best = *it++;
		if (possibleResolutions.size() > 1) {
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

bool ComboLMR::isBlockedLocation(const Mention* ment)
{
	std::wstring head_string = ment->getAtomicHead()->toTextString();
	std::wstring full_string = ment->toCasedTextString();
	head_string = _gazetteer->toCanonicalForm(head_string);
	full_string = _gazetteer->toCanonicalForm(full_string);

	// check to see if location is blocked
	if (rLMR.isBlockedLocation(head_string) || rLMR.isBlockedLocation(full_string))
	{
		return true;
	}
	return false;
}

void ComboLMR::setActorMentionSet(ActorMentionSet *extActorMentionSet)
{
	_actorMentionSet = extActorMentionSet; 
	//update country counts
	BOOST_FOREACH(ActorMention_ptr actor, *extActorMentionSet) {
		if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(actor))
		{
			Symbol isoCode = pnActor->getCountryIsoCode();
			if (isoCode.is_null()) continue;
			if (pnActor->isNamedLocation() && pnActor->isResolvedGeo())
			{
				_countryCounts[isoCode] += 1; // + ((pnActor->getGeoPopulation()) ? log(static_cast<float>(pnActor->getGeoPopulation())) : 0);
			}
			else
			{
				_countryCounts[isoCode] += 0.25;
			}
		}
	}
}


// measures the strength of a candidate resolution relative to the set of georesolutions that were made unambiguously
float ComboLMR::getAssociationScore(Gazetteer::GeoResolution_ptr candidateResolution) const
{
	// if there are no unambiguous mentions in the document, or if the candidate is not affiliated with any specific countries (e.g. "Persian Gulf"), return 1
	if (_countryCounts.empty() || !candidateResolution->countryInfo)
	{
		return 1;
	}
	// for bigger geonames tables, some entries are not 
	double distance_to_nearest_resolution = distanceToNearestResolution(candidateResolution);
	std::map<Symbol, float>::const_iterator it = _countryCounts.find(candidateResolution->countryInfo->isoCode);
	if (it == _countryCounts.end()) return -log(static_cast<float>(distance_to_nearest_resolution));
	else return (*it).second;
}


double ComboLMR::getDistanceBetweenResolutions(const Gazetteer::GeoResolution_ptr loc1, const Gazetteer::GeoResolution_ptr loc2) const {
	if (loc1->longitude == Symbol() || loc1->latitude == Symbol() || loc2->longitude == Symbol() || loc2->latitude == Symbol()) {
		return 12000.0; //half the circum. of the Earth -> no two points on the surface can be further apart.  If we don't have data assume this
	}
	const double PI = 3.141592;
	double degrees_to_radians = PI / 180.0;
	
	double phi1 = (90.0 - wcstod(loc1->latitude.to_string(), NULL))*degrees_to_radians;
	double phi2 = (90.0 - wcstod(loc2->latitude.to_string(), NULL))*degrees_to_radians;
	double theta1 = wcstod(loc1->longitude.to_string(), NULL)*degrees_to_radians;
	double theta2 = wcstod(loc2->longitude.to_string(), NULL)*degrees_to_radians;
	
	double cosi = sin(phi1)*sin(phi2)*cos(theta1 - theta2) + cos(phi1)*cos(phi2);
	double arc = acos(cosi);
	double radius_of_earth = 3960; // in miles = 6373 km
	return arc * radius_of_earth;
}

double ComboLMR::distanceToNearestResolution(const Gazetteer::GeoResolution_ptr location) const {
	double min_distance = 12000;  //half the circum. of the Earth -> no two points on the surface can be further apart
	BOOST_FOREACH(ActorMention_ptr actor, *_actorMentionSet) 
	{
		if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(actor))
		{
			if (pnActor->isNamedLocation() && pnActor->isResolvedGeo()) 
			{
				double distance = getDistanceBetweenResolutions(location, pnActor->getGeoResolution());
				if (distance < min_distance) min_distance = distance;
			}
		}
	}
	return min_distance;
}

} // end of ICEWS namespace

