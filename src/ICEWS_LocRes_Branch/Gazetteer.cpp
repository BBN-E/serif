// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ICEWS/Gazetteer.h"
#include "ICEWS/ICEWSDB.h"
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
#include <boost/scoped_ptr.hpp>

namespace ICEWS {

Gazetteer::~Gazetteer() {
}

Gazetteer::Gazetteer() : _stringEqualityPredicate("LIKE")
{
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	std::ostringstream query;
	query << "SELECT c.id, c.ISOCode, a.actor_id, a.unique_code"
		<< " FROM countries c, dict_actors a"
		<< " WHERE a.country_id=c.id";
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
		CountryInfo_ptr info = boost::make_shared<CountryInfo>();
		info->countryId = CountryId(row.getCellAsInt32(0));
		info->isoCode = row.getCellAsSymbol(1);
		info->actorId = ActorId(row.getCellAsInt32(2));
		info->actorCode = row.getCellAsSymbol(3);
		_countryInfo[info->isoCode] = info;
	}
	SessionLogger::info("ICEWS") << "Loaded info for " << _countryInfo.size() << " countries.";

	// load blocked entries
	std::set<std::wstring> blockedSet = InputUtil::readFileIntoSet(ParamReader::getParam("icews_blocked_gazetteer_entries"), true, false);
	BOOST_FOREACH(const std::wstring &blocked, blockedSet)
		_blockedEntries.push_back(boost::wregex(blocked, boost::regex_constants::icase));

	// Check if the geonames database is case normalized (in which case we can use = instead of LIKE)
	if (ParamReader::isParamTrue("icews_geonames_is_case_normalized")) {
		_stringEqualityPredicate = "=";
	}

	// load nationality to nation and state abbreviation tables
	loadSubstitutions("icews_nationality_to_nation", _nationalityToNation);
	loadSubstitutions("icews_state_abbreviations", _stateAbbreviations);

	// number of times cache is used
	cache_count = 0;
	geo_lookup_count = 0;
}

std::vector<Gazetteer::LocationInfo_ptr> Gazetteer::lookup(std::wstring locationName) {
	std::vector<LocationInfo_ptr> result;
	boost::to_lower(locationName);
	BOOST_FOREACH(const boost::wregex& block_re, _blockedEntries)
		if (boost::regex_match(locationName, block_re)) {
			SessionLogger::info("ICEWS") << "    Gazetteer entry blocked: " << locationName;
			return result;
		}
	DatabaseConnection_ptr geonames_db(getSingletonIcewsGeonamesDb());

	Symbol::HashMap<size_t> country_to_population;
	// Get any names directly from the geonames table.
	std::ostringstream query;
	query << "SELECT DISTINCT country_code, population FROM geonames "
		<< "WHERE name " << _stringEqualityPredicate << " " << DatabaseConnection::quote(locationName) << " OR "
		<< "asciiname " << _stringEqualityPredicate << " " << DatabaseConnection::quote(locationName);
	for (DatabaseConnection::RowIterator row = geonames_db->iter(query); row!=geonames_db->end(); ++row) {
		Symbol country = row.getCellAsSymbol(0);
		size_t new_population = 1;
		if (!row.isCellNull(1))
			new_population = static_cast<size_t>(row.getCellAsInt64(1));
		if (new_population > country_to_population[country])
			country_to_population[country] = new_population;
	}
	// Now get any names from the altnames table.
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	std::ostringstream query2;
	query2 << "SELECT DISTINCT country_code, population FROM geonames "
		  << "JOIN altnames USING(geonameid) WHERE alternate_name " << _stringEqualityPredicate << " "
		  << DatabaseConnection::quote(locationName);
	for (DatabaseConnection::RowIterator row = icews_db->iter(query2); row!=icews_db->end(); ++row) {
		Symbol country = row.getCellAsSymbol(0);
		size_t new_population = static_cast<size_t>(row.getCellAsInt64(1));
		if (new_population > country_to_population[country])
			country_to_population[country] = new_population;
	}
	// Convert it to a list of LocationInfo objects.
	for(Symbol::HashMap<size_t>::iterator it=country_to_population.begin(); it!=country_to_population.end(); ++it) {
		if (_countryInfo.find((*it).first) != _countryInfo.end()) {
			LocationInfo_ptr info = boost::make_shared<LocationInfo>();
			info->countryInfo = _countryInfo[(*it).first];
			info->population = (*it).second;
			result.push_back(info);
		}
	}
	return result;
}

std::vector<Gazetteer::GeoResolution_ptr> Gazetteer::geonameLookup(std::wstring locationName) {
	// canonicalize location text
	locationName = toCanonicalForm(locationName);
	geo_lookup_count += 1;
	//use 16-place cache to prevent redundant goenames queries
    static const int n_buckets = 16; 
    static struct {
		std::wstring loc; std::vector<GeoResolution_ptr> result;
    } cache[n_buckets];
    static int last_read_i = 0;
    static int last_written_i = 0;
    int i = last_read_i;
    do {
        if (cache[i].loc == locationName) 
		{
			cache_count += 1;
            return cache[i].result;
        }
        i = (i + 1) % n_buckets;
    } while (i != last_read_i);
    last_read_i = last_written_i = (last_written_i + 1) % n_buckets;
    cache[last_written_i].loc = locationName;

	std::vector<GeoResolution_ptr> result;
	boost::to_lower(locationName);
	if (locationName.size() > 200) {
		return result;
	}
	
	BOOST_FOREACH(const boost::wregex& block_re, _blockedEntries)
	{
		if (boost::regex_match(locationName, block_re)) {
			SessionLogger::info("ICEWS") << "    Gazetteer entry blocked: " << locationName;
			return result;
		}
	}
	DatabaseConnection_ptr geonames_db(getSingletonIcewsGeonamesDb());

	// Get any names directly from the geonames tables
	std::ostringstream query;
	query << "SELECT geonameid, asciiname, CAST(AVG(population) AS BIGINT), country_code, AVG(latitude), AVG(longitude) "
		<< "FROM geonames "
		<< "WHERE name "  << _stringEqualityPredicate << " " << DatabaseConnection::quote(locationName)
		<< " OR asciiname " << _stringEqualityPredicate << " " << DatabaseConnection::quote(locationName)
		<< " AND country_code IS NOT NULL" 
		<< " GROUP BY geonameid";
	for (DatabaseConnection::RowIterator row = geonames_db->iter(query); row!=geonames_db->end(); ++row) {
		GeoResolution_ptr info = boost::make_shared<GeoResolution>();
		info->geonameid = row.getCellAsSymbol(0);
		info->cityname = row.getCellAsWString(1);
		info->population = static_cast<size_t>(row.getCellAsInt64(2));
		info->latitude = row.getCellAsSymbol(4);
		info->longitude = row.getCellAsSymbol(5);
		info->isEmpty = false;
		info->countrycode = row.getCellAsSymbol(3);
		info->countryInfo = _countryInfo[row.getCellAsSymbol(3)];
		result.push_back(info);
	}

	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	std::ostringstream query2;
	query2 << "SELECT DISTINCT geonameid FROM altnames WHERE alternate_name "  << _stringEqualityPredicate << " " << DatabaseConnection::quote(locationName);
	for (DatabaseConnection::RowIterator row = icews_db->iter(query2); row!=icews_db->end(); ++row) {
		GeoResolution_ptr info = getGeoResolution(row.getCellAsWString(0));
		// if something with the same geonameid has already been added to the result vector, ignore it
		std::vector<GeoResolution_ptr>::iterator it;
		bool already_in_result = 0;
		BOOST_FOREACH(Gazetteer::GeoResolution_ptr candidateResolution, result) 
		{
			if (haveSameGeonameId(candidateResolution, info)){
				already_in_result = 1;
				break;
			}
		}	
		if (already_in_result) continue;
		result.push_back(info);
	}

	//update cache
	cache[last_written_i].result = result;
    return cache[last_written_i].result;
}


std::vector<Gazetteer::GeoResolution_ptr> Gazetteer::countryLookup(std::wstring location) 
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
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	std::ostringstream query;
	query << "SELECT DISTINCT ISOCode FROM countries "
		  << "WHERE Name "  << _stringEqualityPredicate << " " << DatabaseConnection::quote(location)
		  << " AND ISOCode IS NOT NULL";
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) 
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


Gazetteer::GeoResolution_ptr Gazetteer::getCountryResolution(std::wstring country_iso_code) {
	Gazetteer::GeoResolution_ptr countryResolution = boost::make_shared<GeoResolution>();
	std::transform(country_iso_code.begin(), country_iso_code.end(), country_iso_code.begin(), ::toupper);
	countryResolution->countryInfo = _countryInfo[Symbol(country_iso_code)];
	countryResolution->isEmpty = false;
	countryResolution->countrycode = country_iso_code;
	//set default population, lat, long for country to largest entry in geonames
	Gazetteer::GeoResolution_ptr largestCityInCountry = getLargestCity(country_iso_code);
	if (!largestCityInCountry->isEmpty) 
	{
		countryResolution->population = largestCityInCountry->population;
		countryResolution->latitude = largestCityInCountry->latitude;
		countryResolution->longitude = largestCityInCountry->longitude;
	}
	return countryResolution;
}

Gazetteer::GeoResolution_ptr Gazetteer::getLargestCity(std::wstring country_iso_code) {
	DatabaseConnection_ptr icews_db(getSingletonIcewsGeonamesDb());
	std::ostringstream query;
	query << "SELECT DISTINCT geonameid FROM geonames "
		  << "WHERE country_code = " << DatabaseConnection::quote(country_iso_code)
		  << " AND population = (SELECT Max(population) FROM geonames WHERE country_code = " << DatabaseConnection::quote(country_iso_code) << ")";
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) 
	{
		return getGeoResolution(row.getCellAsWString(0));
	}
	return boost::make_shared<GeoResolution>();
}


Gazetteer::GeoResolution_ptr Gazetteer::getGeoResolution(std::wstring geonames_id) 
{
	Gazetteer::GeoResolution_ptr geoResolution = boost::make_shared<GeoResolution>();
	DatabaseConnection_ptr icews_db(getSingletonIcewsGeonamesDb());
	std::ostringstream query;
	query << "SELECT geonameid, asciiname, CAST(AVG(population) AS BIGINT), country_code, AVG(latitude), AVG(longitude) "
		<< "FROM geonames "
		<< "WHERE geonameid = " << DatabaseConnection::quote(geonames_id)
		<< " AND country_code IS NOT NULL";
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) 
	{
		geoResolution->geonameid = row.getCellAsSymbol(0);
		geoResolution->cityname = row.getCellAsWString(1);
		geoResolution->population = static_cast<size_t>(row.getCellAsInt64(2));
		geoResolution->latitude = row.getCellAsSymbol(4);
		geoResolution->longitude = row.getCellAsSymbol(5);
		geoResolution->countrycode = row.getCellAsSymbol(3);
		geoResolution->isEmpty = false;
		geoResolution->countryInfo = _countryInfo[row.getCellAsSymbol(3)];
		return geoResolution;
	}

	//if there is no resolution found in the db, return empty
	return geoResolution;
}


// Helper Functions

std::wstring Gazetteer::toCanonicalForm(std::wstring str){
	//remove punctuation
	str.erase(
    remove_if ( str.begin(), str.end(), static_cast<int(*)(int)>(&ispunct) ),
    str.end());

	//lowercase
	boost::to_lower(str);

	//remove stopwords
	boost::wregex e(L"^the ");
	std::wstring e_replacement = L"";
	str = boost::regex_replace(str, e, e_replacement, boost::match_default | boost::format_perl);
	boost::wregex f(L" prefecture ");
	std::wstring f_replacement = L" ";
	str = boost::regex_replace(str, f, f_replacement, boost::match_default | boost::format_perl);

	//strip whitespace
	boost::trim(str);

	// clean state abbreviations / nationalities
	if (_stateAbbreviations.find(str) != _stateAbbreviations.end()) str = _stateAbbreviations[str];
	if (_nationalityToNation.find(str) != _nationalityToNation.end()) str = _nationalityToNation[str];

	//strip whitespace
	boost::trim(str);

	return str;
}

std::vector<Gazetteer::GeoResolution_ptr> Gazetteer::joinResolutions(const std::vector<Gazetteer::GeoResolution_ptr> & A, const std::vector<Gazetteer::GeoResolution_ptr> & B)
{
	std::vector<Gazetteer::GeoResolution_ptr> AB;
	AB.reserve( A.size() + B.size() ); // preallocate memory
	AB.insert( AB.end(), A.begin(), A.end() );
	AB.insert( AB.end(), B.begin(), B.end() );
	std::sort( AB.begin(), AB.end() );
	AB.erase( std::unique( AB.begin(), AB.end() ), AB.end() );
	return AB;
}

void Gazetteer::loadSubstitutions(const std::string & paramFile, std::map<std::wstring, std::wstring> & mapping)
{
	vector <std::wstring> fields;
	std::wstring line;
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(ParamReader::getRequiredParam(paramFile).c_str()));
	UTF8InputStream& file(*file_scoped_ptr);
	if (file.is_open())
	{
		while ( file.good() )
		{
			getline (file,line);
			boost::to_lower(line);
			if (line == L"") continue;
			boost::algorithm::split( fields, line, boost::algorithm::is_any_of( L"," ) );
			if (fields.size() < 2) {
				std::wstringstream errStr;
				errStr << L"Bad line in gazetteer substitution file (" << UnicodeUtil::toUTF16StdString(paramFile) << L"): " << line << L"\n";
				throw UnexpectedInputException("Gazetteer::loadSubstitutions", errStr);
			}
			mapping[fields[0]] = fields[1]; 
		}
		file.close();
	}
	else cout << "Unable to load mapping specified by the " << paramFile << " parameter."; 
}

Symbol Gazetteer::getIsoCodeForActor(ActorId actor_id) 
{
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	std::ostringstream query;
	/*<< "SELECT c.id, c.ISOCode, a.actor_id, a.unique_code"
		<< " FROM countries c, dict_actors a"
		<< " WHERE a.country_id=c.id";*/
	query << "SELECT DISTINCT ISOCode FROM countries as c "
		  << "JOIN dict_actors as d "
		  << "ON country_id=id "
		  << "WHERE actor_id = " << actor_id.getId();
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) 
	{
		return row.getCellAsSymbol(0);
	}
	return Symbol();
}

} // end namespace ICEWS
