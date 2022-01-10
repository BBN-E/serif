// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOCATIONMENTIONRESOLVER_H
#define LOCATIONMENTIONRESOLVER_H

#include "Generic/icews/ICEWSGazetteer.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/actors/JabariTokenMatcher.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/theories/ActorMentionSet.h"
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>


class LocationMentionResolver
{
public:
	LocationMentionResolver(Gazetteer *extGazetteer);
	virtual ~LocationMentionResolver() { }

	// Public helpers
	void setMaxAmbiguity(int ambiguity) { _max_ambiguity_for_gazetteer_actors = ambiguity; };
	void setVerbosity(size_t verbosity) { _verbosity = verbosity; };
	void clear(){	
		_resolutions.clear();
	}

	// Get all possible resolutions (possibly constrained by allowable countries)	
	void getCandidateResolutions(const SentenceTheory *sentTheory, const Mention* ment, 
		Gazetteer::SortedGeoResolutions &candidateResolutions, bool one_per_country, std::vector<CountryId> allowableCountries = std::vector<CountryId>());

	// Get best resolution, ICEWS-style; assumes that documentActorMentionSet really does have document actors in it, and that
	//  countryCounts is likewise informative
	Gazetteer::ScoredGeoResolution getICEWSLocationResolution(ActorMentionSet *documentActorMentionSet, 
		std::vector<ActorMatch> patternActorMatches, ActorId::HashMap<float>& countryCounts, ActorId usaActorId,
		const SentenceTheory *sentTheory, const Mention* ment, bool allow_ambiguity, 
		std::vector<CountryId> allowableCountries, ActorInfo_ptr actorInfo);

	Gazetteer::ScoredGeoResolution getUnambiguousCountryResolution(const SentenceTheory *sentTheory, const Mention* ment, ActorInfo_ptr actorInfo);

	// Gets best resolution from within a prespecified country and region
	Gazetteer::GeoResolution_ptr getResolutionInRegion(const SentenceTheory *sentTheory, const Mention* mention, Symbol countryCode, Symbol regionCode = Symbol());

	// Walks through vector and gets the results for the first one that has resolutions
	std::vector<Gazetteer::GeoResolution_ptr> getBestResolutionSetForCanonicalNames(std::vector<std::wstring>& canonicalNames) const;
	
	std::map<Symbol, Gazetteer::GeoResolution_ptr> getAllPossibleResolutionsOnePerCountry(std::vector<std::wstring>& canonicalNames) const;
	static void addScoredCandidateResolution(Gazetteer::SortedGeoResolutions& sortedGeoResolutions, Gazetteer::GeoResolution_ptr candidateResolution);
		
	// registerResolution adds a mapping from a location string that has been resolved to the geoResolution_ptr 
	// that the string was resolved to --> used to make sure the mentions with the same underlying text get 
	// resolved to the same thing (there are some conditions when this might not normally happen)
	void registerResolutions(std::vector<std::wstring>& names, Gazetteer::ScoredGeoResolution resolution) {
		BOOST_FOREACH(std::wstring name, names) { 
			registerResolution(name, resolution);
		}
	}
	
	// Test whether location resolution is blocked by some rule
	bool isBlockedLocation(const SentenceTheory *sentTheory, const Mention* mention) const;	
	
private:
	Gazetteer *_gazetteer;
	int _max_ambiguity_for_gazetteer_actors;
	size_t _verbosity;
	std::map<std::wstring, Gazetteer::ScoredGeoResolution> _resolutions;	

	// Convenience functions
	Gazetteer::ScoredGeoResolution createEmptyResolution() const {
		return std::make_pair(0.0F, boost::make_shared<Gazetteer::GeoResolution>());
	}
	bool isFromAllowableCountry(Gazetteer::GeoResolution_ptr candidateResolution, std::vector<CountryId> allowableCountries);
	
	// Get various lists of resolutions
	std::vector<Gazetteer::GeoResolution_ptr> getAllPossibleCountryResolutions(std::vector<std::wstring>& canonicalNames) const;
	std::vector<Gazetteer::GeoResolution_ptr> getAllResolutionsInCountry(std::vector<std::wstring>& canonicalNames, const Symbol countryCode) const;

	// Caching
	void registerResolution(std::wstring location_text, Gazetteer::ScoredGeoResolution resolution) { _resolutions[location_text] = resolution; }
	Gazetteer::ScoredGeoResolution getRegisteredResolution(const std::wstring location_text) const { 
		std::map<std::wstring, Gazetteer::ScoredGeoResolution>::const_iterator it;
		it = _resolutions.find(location_text);
		if (it == _resolutions.end()) 
			return createEmptyResolution();
		else return it->second;
	}

	// Rules resolve named location mentions based on a set of rules defined in two files: 
	// --> One at the directory specified by the "icews_location_rules" parameter
	//     Rules in here can be of two forms:
	//     1) instructions to resolve a text span to a specific entry in the geonames table, of the form <location text>,geo_id:<geonames_table_id> (example: washington,geo_id:4138106)
	//     2) instructions to resolve a text span to a specific country in the countries table (countries are NOT included in the geonames table, and require separate treatment)
	//     <location_text>,country_id:<country_table_iso_code> (example: india,country_id:in)
	// --> One at the directory specified by the "icews_blocked_gazetteer_entries" parameter
	//     Rules here take the form of regular expressions, which when matched prevent the LMR from resolving to any location
	//     (This data in this file also gets used elsewhere by the Gazetteer)
	bool isBlockedLocation(std::wstring canonical_name) const;
	bool isBlockedLocation(std::vector<std::wstring>& names) const;
	Gazetteer::GeoResolution_ptr getRuleBasedResolution(std::vector<std::wstring>& canonicalNames) const;	
	std::vector<std::pair<boost::wregex, std::wstring> > _rules;

	// Special cases
	bool isUSGeorgia(ActorMentionSet *documentActorMentionSet, const Mention* ment, ActorId::HashMap<float>& countryCounts, ActorId usaActorId) const;

	
};

typedef boost::shared_ptr<LocationMentionResolver> LocationMentionResolver_ptr;


#endif

