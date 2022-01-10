// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOCATIONMENTIONRESOLVER_H
#define LOCATIONMENTIONRESOLVER_H

#include "ICEWS/Gazetteer.h"
#include "ICEWS/ActorMention.h"
#include "ICEWS/ActorMentionSet.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>

namespace ICEWS {


/* 
Base class LocationMentionResolver - extended by multiple classes to combine different approaches to 
resolving location mentions in a document to location entries in database (entries are split between the countries table and
geonames table)
*/
class LocationMentionResolver
{
public:
	LocationMentionResolver(Gazetteer *extGazetteer)
	{ 
		_gazetteer = extGazetteer;
	}
	virtual ~LocationMentionResolver() { }
	
	void setActorMentionSet(ActorMentionSet *extActorMentionSet) { _actorMentionSet = extActorMentionSet; }
	// getLocationResolution is called to resolve a location mention (must be specified in subclasses)
	virtual Gazetteer::GeoResolution_ptr getLocationResolution(const Mention* mention) const =0;
	// getAllPossibleResolutions returns a vector of candidate location resolutions found in the geonames table 
	// (excluding countries, which are in a separate table)
	std::vector<Gazetteer::GeoResolution_ptr> getAllPossibleResolutions(const Mention* mention) const;

protected:
	ActorMentionSet *_actorMentionSet;
	Gazetteer *_gazetteer;
};


/* 
Attempts to resolve only country named location mentions (which in the baseline take presidence over resolutions in the geonames table)
*/
class CountryLMR : public LocationMentionResolver
{
public:
	CountryLMR(Gazetteer *extGazetteer)
		: LocationMentionResolver(extGazetteer)
	{	}

	Gazetteer::GeoResolution_ptr getLocationResolution(const Mention* mention) const;
	std::vector<Gazetteer::GeoResolution_ptr> getAllPossibleResolutions(const Mention* mention) const throw( invalid_argument );
	std::vector<Gazetteer::GeoResolution_ptr> getAllResolutionsInCountry(const Mention* ment, const Symbol countryCode);
};


/* 
RuleBasedLMR resolves named location mentions based on a set of rules defined in two files: 
--> One at the directory specified by the "icews_location_rules" parameter
	Rules in here can be of two forms:
	1) instructions to resolve a text span to a specific entry in the geonames table, of the form <location text>,geo_id:<geonames_table_id> (example: washington,geo_id:4138106)
	2) instructions to resolve a text span to a specific country in the countries table (countries are NOT included in the geonames table, and require separate treatment)
	   <location_text>,country_id:<country_table_iso_code> (example: india,country_id:in)
--> One at the directory specified by the "icews_blocked_gazetteer_entries" parameter
	Rules here take the form of regular expressions, which when matched prevent the LMR from resolving to any location
	(This data in this file also gets used elsewhere by the Gazetteer)
*/
class RuleBasedLMR : public LocationMentionResolver
{
public:
	RuleBasedLMR(Gazetteer *extGazetteer);
	
	Gazetteer::GeoResolution_ptr getLocationResolution(const Mention* mention) const;
	bool hasRuleForLocation(std::wstring location_mention_text) const;
	bool isBlockedLocation(std::wstring location_mention_text) const;

private:
	std::vector<std::pair<boost::wregex, std::wstring> > _rules;
};


/*
Resolves named location mentions to the potential resolvant with the highest population
*/
class PopulationLMR : public LocationMentionResolver
{
public:
	PopulationLMR(Gazetteer *extGazetteer)
		: LocationMentionResolver(extGazetteer)
	{	}

	Gazetteer::GeoResolution_ptr getLocationResolution(const Mention* mention) const;
};


/* 
Combines CountryLMR, RuleBasedLMR and PopulationLMR in the following order to provide a baseline resolution level for named locations:
1) Run the rule based LMR, and if the left hand side of a rule matches the text span of the location mention, this rule is fired and 
and corresponding value is returned.
2) If no matching rule is found, attempt to resolve a location mention to a country.  This is necessary because countries listed in the country table of 
the icews database don't have population data listed alongside them, which means by default they will be overlooked in the population based resolver.
3) If no resolution is found in the CountryLMR, use PopulationLMR and attempt to resolve entries against the geonames table based on 
the population of the potential resolvent.
4) If no resolution can be found, the location mention is left unresolved.
*/
class BaselineLMR : public LocationMentionResolver
{
public:
	BaselineLMR(Gazetteer *extGazetteer)
		: LocationMentionResolver(extGazetteer), rLMR(extGazetteer), pLMR( extGazetteer), cLMR(extGazetteer)
	{	}

	Gazetteer::GeoResolution_ptr getLocationResolution(const Mention* mention) const;

private:
	RuleBasedLMR rLMR;
	CountryLMR cLMR;
	PopulationLMR pLMR;
};

/*
Highest performing LMR, used in ActorMentionFinder for georesolution
*/ 
class ComboLMR : public LocationMentionResolver
{
public:
	ComboLMR(Gazetteer *extGazetteer)
		: LocationMentionResolver(extGazetteer), rLMR(extGazetteer), cLMR(extGazetteer)
	{ 
		_countryCounts.clear(); 
		_maxAmbiguityForGazetteerActors = 3;
		_verbosity = 3;
	}

	/* geUnambiguousResolution attempts to resolve location mentions that are unambiguous, meaning
	they have a predefined resolution rule or have only one potential resolution in the
	icews locational database tables. Returns an empty resolution if no unambiguous match
	is found. */
	Gazetteer::GeoResolution_ptr getUnambiguousResolution(const Mention* mention);

	/* getLocationResolution attempts to resolve location mentions that haven't been resolved 
	by the getUnambiguousResolution method above, taking into account document level counts
	and other pertinant information to determine a location resolution.  This is intended to be
	run as the second phase of determining location resolutions.  */
	Gazetteer::GeoResolution_ptr getLocationResolution(const Mention* mention) const;

	/* attempts to resolve a location to a geo in a specific country */
	Gazetteer::GeoResolution_ptr getResolutionInCountry(const Mention* mention, Symbol countryCode);

	/* getAssociationScore returns a float indicating how closely associated a candidate
	resolution is with locations that have been resolved unambiguously in a document. */
	float getAssociationScore(Gazetteer::GeoResolution_ptr candidateResolution) const;

	/* set the actor mention set for the LMR and also build document association counts */
	void setActorMentionSet(ActorMentionSet *extActorMentionSet);

	void setMaxAmbiguity(int ambiguity) { _maxAmbiguityForGazetteerActors = ambiguity; };
	void setVerbosity(size_t verbosity) { _verbosity = verbosity; };

	double getDistanceBetweenResolutions(const Gazetteer::GeoResolution_ptr loc1, const Gazetteer::GeoResolution_ptr loc2) const;
	double distanceToNearestResolution(const Gazetteer::GeoResolution_ptr location) const;

	bool isBlockedLocation(const Mention* ment);

	// registerResolution adds a mapping from a location string that has been resolved to the geoResolution_ptr 
	// that the string was resolved to --> used to make sure the mentions with the same underlying text get 
	// resolved to the same thing (there are some conditions when this might not normally happen)
	void registerResolution(std::wstring location_text, Gazetteer::GeoResolution_ptr resolution) { _resolutions[location_text] = resolution; }
	Gazetteer::GeoResolution_ptr getRegisteredResolution(const std::wstring location_text) const { 
		std::map<std::wstring, Gazetteer::GeoResolution_ptr>::const_iterator it;
		it = _resolutions.find(location_text);
		if (it == _resolutions.end()) return boost::make_shared<Gazetteer::GeoResolution>();
		else return it->second;
	}

	void clear(){	
		_countryCounts.clear();
		_resolutions.clear();
	}

private:
	RuleBasedLMR rLMR;
	CountryLMR cLMR;
	std::map<Symbol, float> _countryCounts;
	int _maxAmbiguityForGazetteerActors;
	size_t _verbosity;
	std::map<std::wstring, Gazetteer::GeoResolution_ptr> _resolutions;
};


//// Helper functions

} // end of ICEWS namespace

#endif
