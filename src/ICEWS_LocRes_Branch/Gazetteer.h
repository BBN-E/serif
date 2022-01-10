// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_GAZETTEER_H
#define ICEWS_GAZETTEER_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Generic/common/Symbol.h"
#include "ICEWS/Identifiers.h"
#include <vector>
#include <boost/regex.hpp>
#include <boost/make_shared.hpp>

// Forward declaration
class DatabaseConnection;

namespace ICEWS {

class Gazetteer: private boost::noncopyable {
public:

	Gazetteer();
	~Gazetteer();

	struct CountryInfo {
		/** Primary key in the "countries" table. */
		CountryId countryId;

		/** Two-digit ISO 3166 code for this country (as used in 
		  * rcdr.geonames.country_code and rcdr.countries.ISOCode). */
		Symbol isoCode;

		/** Primary key in the "dict_actors" table for the actor that
		  * corresponds to this country. */
		ActorId actorId;

		/** The value of "dict_actors.unique_code" for the actor that
		  * corresponds to this country. */
		Symbol actorCode;
	};
	typedef boost::shared_ptr<CountryInfo> CountryInfo_ptr;

	struct LocationInfo {
		/** The population of this location. */
		size_t population;
		/** Information about the country that contains this location. */
		CountryInfo_ptr countryInfo;
	};
	typedef boost::shared_ptr<LocationInfo> LocationInfo_ptr;


	struct GeoResolution {
		/* the unique id of the geo in the geonames table */
		Symbol geonameid;
		
		/* the string of the geo in the geonames table */
		Symbol cityname;
		
		/* population of the geo */
		size_t population;
		
		/* the exact coordinates of the geo */
		Symbol latitude;
		Symbol longitude;
		Symbol countrycode;

		/* a boolean indicating if the resolution is empty */
		bool isEmpty;

		/* Information about the country that contains this location */
		CountryInfo_ptr countryInfo;

		GeoResolution(): geonameid(), cityname(), population(1), latitude(), longitude(), isEmpty(true) {}
	};
	typedef boost::shared_ptr<GeoResolution> GeoResolution_ptr;
	typedef std::pair<float, GeoResolution_ptr> ScoredGeoResolution;
	typedef std::set<ScoredGeoResolution> SortedGeoResolutions;

	/** Look up a location by name.  Return a record for each location
	  * in the gazetteer that matches the given name. */
	std::vector<LocationInfo_ptr> lookup(std::wstring locationName);

	/* Look up a location by name in the icews database geonames table
	and return a GeoResolution pointer for each potential match */
	std::vector<GeoResolution_ptr> geonameLookup(std::wstring locationName);

	/* Look up a location by name in the icews database country table
	and return a GeoResolution pointer for each potential match */
	std::vector<GeoResolution_ptr> countryLookup(std::wstring locationName);

	/* getCountryResolution (two args) returns the exact country GeoResolution_ptr associated
	with an actor_uid / location string*/
	GeoResolution_ptr getCountryResolution(std::wstring actor_uid, std::wstring location_text);

	/* getLargestCity returns the largest entry (by population) in a country, to use as default 
	for population/lat/long for the country */
	GeoResolution_ptr getLargestCity(std::wstring country_iso_code);

	/* getCountryResolution (one arg) returns the exact country GeoResolution_ptr associated
	with an iso country code */
	GeoResolution_ptr getCountryResolution(std::wstring country_iso_code);
	
	/* getGeoResolution returns the exact geonames GeoResolution_ptr associated with a 
	geonames unique id (as found in the geonames table)*/
	GeoResolution_ptr getGeoResolution(std::wstring geonames_id);

	//// helper functions

	static Symbol getIsoCodeForActor(ActorId actor_id);

	std::wstring toCanonicalForm(std::wstring location_string);

	bool haveSameGeonameId(GeoResolution_ptr a, GeoResolution_ptr b) { return a->geonameid == b->geonameid; }

	/* joinResolutions takes two vectors of GeoResolution_ptr's and returns the union of the two vectors (as another vector) */
	static std::vector<Gazetteer::GeoResolution_ptr> joinResolutions(const std::vector<Gazetteer::GeoResolution_ptr> & A, const std::vector<Gazetteer::GeoResolution_ptr> & B);

	/* loadSubstitutions */
	void loadSubstitutions(const std::string & paramFile, std::map<std::wstring, std::wstring> & mapping);
	std::vector<boost::wregex> getBlockedEntries() const { return _blockedEntries; }
	int cache_count;
	int geo_lookup_count;

private:
	Symbol::HashMap<CountryInfo_ptr> _countryInfo;
	void initCountryInfo();
	std::vector<boost::wregex> _blockedEntries;
	std::map<std::wstring, std::wstring> _nationalityToNation;
	std::map<std::wstring, std::wstring> _stateAbbreviations;
	std::string _stringEqualityPredicate;
};

} // end namespace ICEWS

#endif
