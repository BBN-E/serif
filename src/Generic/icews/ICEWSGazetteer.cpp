// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/icews/ICEWSGazetteer.h"
#include "Generic/icews/ICEWSDB.h"
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


ICEWSGazetteer::~ICEWSGazetteer() { }

ICEWSGazetteer::ICEWSGazetteer() : Gazetteer(Gazetteer::ICEWS) {
	std::ostringstream query;
	query << "SELECT c.id, c.ISOCode, a.actor_id, a.unique_code"
		<< " FROM countries c, dict_actors a"
		<< " WHERE a.country_id=c.id";
	DatabaseConnectionMap dbs = ICEWSDB::getNamedDbs();
	for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
		for (DatabaseConnection::RowIterator row = i->second->iter(query); row!=i->second->end(); ++row) {
			CountryInfo_ptr info = boost::make_shared<CountryInfo>();
			info->countryId = CountryId(row.getCellAsInt32(0), i->first);
			info->isoCode = row.getCellAsSymbol(1);
			info->actorId = ActorId(row.getCellAsInt32(2), i->first);
			info->actorCode = row.getCellAsSymbol(3);
			_countryInfo[info->isoCode] = info;
		}
	}
	SessionLogger::info("ICEWS") << "Loaded info for " << _countryInfo.size() << " countries.";

	_logString = "ICEWS";
}

std::vector<Gazetteer::GeoResolution_ptr> ICEWSGazetteer::countryLookup(std::wstring location) 
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
			SessionLogger::info("ICEWS") << "    Gazetteer entry blocked: " << location;
			return result;
		}
	}
	std::ostringstream query;
	query << "SELECT DISTINCT ISOCode FROM countries "
		  << "WHERE lower_name = " << DatabaseConnection::quote(location)
		  << " AND ISOCode IS NOT NULL";
	std::vector<Symbol> isoCodes;
	DatabaseConnectionMap dbs = ICEWSDB::getNamedDbs();
	for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
		for (DatabaseConnection::RowIterator row = i->second->iter(query); row!=i->second->end(); ++row) 
		{
			Symbol s = row.getCellAsSymbol(0);
			if (s.is_null())
				continue;
			isoCodes.push_back(s);
		}
	}

	// We have to do this in a separate loop, because we can't have two DB queries open at the same time
	BOOST_FOREACH(Symbol iso_code, isoCodes) {
		Gazetteer::GeoResolution_ptr info = boost::make_shared<GeoResolution>();
		info->isEmpty = false;
		info->countrycode = iso_code;
		info->countryInfo = _countryInfo[iso_code];
		//set default population, lat, long for country to largest entry in geonames
		Gazetteer::GeoResolution_ptr largestCityInCountry = getLargestCity(iso_code.to_string());
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

DatabaseConnection_ptr ICEWSGazetteer::getSingletonGeonamesDb() {
	return ICEWSDB::getGeonamesDb();
}
