// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/actors/AWAKEGazetteer.h"
#include "Generic/actors/AWAKEDB.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/database/DatabaseConnection.h"
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <math.h>
#include <cctype>
#include <boost/scoped_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>


AWAKEGazetteer::~AWAKEGazetteer() { }

AWAKEGazetteer::AWAKEGazetteer() : Gazetteer(Gazetteer::AWAKE) {
	DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());

	_actorDBName = ParamReader::getParam(Symbol(L"bbn_actor_db_name"));
	if (_actorDBName.is_null())
		_actorDBName = Symbol(L"default");

	std::ostringstream query;
	query << "SELECT Actor.ActorId, IsoCode FROM Actor, ActorIsoCode"
		  << " WHERE Actor.ActorId = ActorIsoCode.ActorId";
	for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) {
		CountryInfo_ptr info = boost::make_shared<CountryInfo>();
		info->countryId = CountryId(row.getCellAsInt32(0), _actorDBName);
		info->isoCode = row.getCellAsSymbol(1);
		info->actorId = ActorId(row.getCellAsInt32(0), _actorDBName);
		info->actorCode = Symbol();
		_countryInfo[info->isoCode] = info;
	}
	SessionLogger::info("Gazetteer") << "Loaded info for " << _countryInfo.size() << " countries.";
}

std::vector<Gazetteer::GeoResolution_ptr> AWAKEGazetteer::countryLookup(std::wstring location) 
{
	// canonicalize location text
	location = toCanonicalForm(location);

	std::vector<GeoResolution_ptr> result;
	// if location string is too long, return empty list
	if (location.size() > 45) {
		return result;
	}
	BOOST_FOREACH(const boost::wregex& block_re, _blockedEntries) {
		if (boost::regex_match(location, block_re)) {
			SessionLogger::info("Gazetteer") << "    Gazetteer entry blocked: " << location;
			return result;
		}
	}
	DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());
	std::ostringstream query;
	query << "SELECT DISTINCT IsoCode FROM Actor a, ActorIsoCode aic"
		  << " WHERE a.ActorId = aic.ActorID "
		  << " AND lower(CanonicalName) = " << DatabaseConnection::quote(location);
	for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) 
	{
		Gazetteer::GeoResolution_ptr info = boost::make_shared<GeoResolution>();
		Symbol iso_code = row.getCellAsSymbol(0);
		if (iso_code.is_null()) continue;
		info->isEmpty = false;
		info->countrycode = iso_code;
		info->countryInfo = _countryInfo[iso_code];
		//set default population, lat, long for country to largest entry in geonames
		Gazetteer::GeoResolution_ptr largestCityInCountry = getLargestCity(row.getCellAsWString(0));
		if (!largestCityInCountry->isEmpty) 
		{
			info->population = largestCityInCountry->population;
			info->latitude = largestCityInCountry->latitude;
			info->longitude = largestCityInCountry->longitude;
		}
		result.push_back(info);
	}
	return result;
}

/*
DatabaseConnection_ptr AWAKEGazetteer::getSingletonGeonamesDb() {
	static DatabaseConnection_ptr geonames_db;
	if (!geonames_db) {
		std::string db_url = ParamReader::getRequiredParam("bbn_actor_db");
		geonames_db = DatabaseConnection::connect(db_url);
	}
	return geonames_db;
}
*/
DatabaseConnection_ptr AWAKEGazetteer::getSingletonGeonamesDb() {
	return AWAKEDB::getGeonamesDb();
}

Gazetteer::GeoResolution_ptr AWAKEGazetteer::getCountryResolution(std::wstring country_iso_code) {
	std::transform(country_iso_code.begin(), country_iso_code.end(), country_iso_code.begin(), ::toupper);

	std::map<std::wstring, Gazetteer::GeoResolution_ptr>::iterator cache_iter = _countryResolutionCache.find(country_iso_code);
	if (cache_iter != _countryResolutionCache.end())
		return (*cache_iter).second;

	DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());
	std::ostringstream query;
	query << "SELECT geonameid FROM actor a, actorisocode aic"
		  << " WHERE a.actorid = aic.actorid"
		  << " AND aic.isocode = " << DatabaseConnection::quote(country_iso_code);
	DatabaseConnection::RowIterator row = bbn_db->iter(query);
	if (row != bbn_db->end()) {
		std::wstring result = row.getCellAsWString(0);
		while (row != bbn_db->end()) ++row; // MySQL wants us to clear out the query before moving on
		Gazetteer::GeoResolution_ptr ret_value = getGeoResolution(result);
		_countryResolutionCache[country_iso_code] = ret_value;
		return ret_value;
	}

	Gazetteer::GeoResolution_ptr ret_value = Gazetteer::getCountryResolution(country_iso_code);
	_countryResolutionCache[country_iso_code] = ret_value;
	return ret_value;
}
