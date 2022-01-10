// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GAZETTEER_H
#define GAZETTEER_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/database/DatabaseConnection.h"
#include <vector>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include <boost/make_shared.hpp>
#include <boost/cstdint.hpp>
class SentenceTheory;
class Mention;


class Gazetteer: private boost::noncopyable {
public:

	enum Mode { ICEWS, AWAKE };

	Gazetteer(Mode mode);
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

	struct GeoResolution {
		/* the unique id of the geo in the geonames table */
		std::wstring geonameid;

		/* the string of the geo in the geonames table */
		Symbol cityname;

		/* population of the geo */
		size_t population;

		/* the exact coordinates of the geo */
		float latitude;
		float longitude;
		Symbol countrycode;

		/* a boolean indicating if the resolution is empty */
		bool isEmpty;

		/* Information about the country that contains this location */
		CountryInfo_ptr countryInfo;

		GeoResolution(): geonameid(), cityname(), population(1), latitude(), longitude(), isEmpty(true) {}
	};
	typedef boost::shared_ptr<GeoResolution> GeoResolution_ptr;
	typedef std::pair<double, GeoResolution_ptr> ScoredGeoResolution;
	typedef std::set<ScoredGeoResolution> SortedGeoResolutions;

	/* Look up a location by name in the icews database geonames table
	and return a GeoResolution pointer for each potential match */
	virtual std::vector<GeoResolution_ptr> geonameLookup(std::wstring locationName);

	/* Look up a location by name in the icews database country table
	and return a GeoResolution pointer for each potential match */
	virtual std::vector<GeoResolution_ptr> countryLookup(std::wstring locationName);

	/* getLargestCity returns the largest entry (by population) in a country, to use as default 
	for population/lat/long for the country */
	virtual GeoResolution_ptr getLargestCity(std::wstring country_iso_code);

	/* getCountryResolution (one arg) returns the exact country GeoResolution_ptr associated
	with an iso country code */
	virtual GeoResolution_ptr getCountryResolution(std::wstring country_iso_code);

	/* getGeoResolution returns the exact geonames GeoResolution_ptr associated with a 
	geonames unique id (as found in the geonames table)*/
	virtual GeoResolution_ptr getGeoResolution(std::wstring geonames_id);

	/* getGeoRegion returns the top region (as found in the admin1 field of the geonames table) 
	associated with a geonames id (ex: Minneapolis --> Minnesota)*/
	virtual Symbol getGeoRegion(std::wstring geonames_id);

	//// helper functions
	virtual std::vector<std::wstring> toCanonicalForms(const SentenceTheory *sentTheory, const Mention* mention);
	virtual bool isOKStateAbbreviation(const SentenceTheory *sentTheory, const Mention* mention);
	virtual std::wstring toCanonicalForm(std::wstring location_string, const SentenceTheory *sentTheory = 0, const Mention* mention = 0);

	bool haveSameGeonameId(GeoResolution_ptr a, GeoResolution_ptr b) { return a->geonameid == b->geonameid; }

	/* joinResolutions takes two vectors of GeoResolution_ptr's and returns the union of the two vectors (as another vector) */
	static std::vector<Gazetteer::GeoResolution_ptr> joinResolutions(const std::vector<Gazetteer::GeoResolution_ptr> & A, const std::vector<Gazetteer::GeoResolution_ptr> & B);

	/* loadSubstitutions */
	virtual void loadSubstitutions(const std::string & paramFile, std::map<std::wstring, std::wstring> & mapping);
	std::vector<boost::wregex> getBlockedEntries() const { return _blockedEntries; }
	int cache_count;
	int geo_lookup_count;

protected:
	Symbol::HashMap<CountryInfo_ptr> _countryInfo;
	void initCountryInfo();
	std::vector<boost::wregex> _blockedEntries;
	std::map<std::wstring, std::wstring> _nationalityToNation;
	std::map<std::wstring, std::wstring> _stateAbbreviations;

	std::wstring _nameCompareString;
	std::wstring _asciinameCompareString;
	std::wstring _altnameCompareString;
	std::wstring _geonamesTablename;
	std::wstring _altnamesTablename;

	std::string _logString;

	virtual DatabaseConnection_ptr getSingletonGeonamesDb();

	std::map<std::wstring, Gazetteer::GeoResolution_ptr> _largestCityCache;

	static const int _n_buckets = 16;
	struct {
		std::wstring loc; std::vector<GeoResolution_ptr> result;
    } _cache[_n_buckets];
	int _last_read_i;
	int _last_written_i;

};

typedef boost::shared_ptr<Gazetteer> Gazetteer_ptr;

#endif
