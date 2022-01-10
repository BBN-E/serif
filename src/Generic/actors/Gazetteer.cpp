// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/actors/Gazetteer.h"
#include "Generic/icews/ICEWSDB.h"
#include "Generic/actors/AWAKEDB.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/database/DatabaseConnection.h"
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <math.h>
#include <cctype>
#include <boost/scoped_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

Gazetteer::Gazetteer(Mode mode) : _logString("Gazetteer"), _last_read_i(0), _last_written_i(0)
{
	// load blocked entries
	std::set<std::wstring> blockedSet = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("blocked_gazetteer_entries"), true, false);
	BOOST_FOREACH(const std::wstring &blocked, blockedSet)
		_blockedEntries.push_back(boost::wregex(blocked, boost::regex_constants::icase));

	// Lockheed Martin is now using geonames/altnames for something else. We need to call our something SERIF-specific.
	std::string tablename_prefix;
	if (mode == AWAKE)
		tablename_prefix = ParamReader::getRequiredParam("awake_gazetteer_tablename_prefix");
	else if (mode == ICEWS)
		tablename_prefix = ParamReader::getParam("icews_gazetteer_tablename_prefix");
	else 
		throw UnexpectedInputException("Gazetteer::Gazetteer", "Unknown mode");

	_geonamesTablename = L"geonames";
	_altnamesTablename = L"altnames";
	if (tablename_prefix == "serif") {
		_geonamesTablename = L"serif_geonames";
		_altnamesTablename = L"serif_altnames";
	} else if (tablename_prefix != "NONE" && tablename_prefix != "none") {
		std::ostringstream msg;
		msg << "Parameter 'gazetteer_tablename_prefix' must be set to 'serif' or 'none', not " << tablename_prefix;
		throw UnexpectedInputException("Gazetteer::Gazetteer", msg.str().c_str());
	}
	
	std::string db_url = "";
	/*
	if (mode == AWAKE)
		db_url = ParamReader::getParam("bbn_actor_db");
	else if (mode == ICEWS)
		db_url = ParamReader::getParam("gazetteer_db");
	else 
		throw UnexpectedInputException("Gazetteer::Gazetteer", "Unknown mode");

	if (db_url.empty())
		db_url = ICEWSDB::getDefaultDbUrl();
*/

	db_url = ParamReader::getParam("gazetteer_db");

	if (db_url.empty()) {
		if (mode == AWAKE)
			db_url = AWAKEDB::getDefaultDbUrl();
		else if (mode == ICEWS)
			db_url = ICEWSDB::getDefaultDbUrl();
		else
			throw UnexpectedInputException("Gazetteer::Gazetteer", "Unknown mode");
	}

	if (db_url.find("oracle") == 0 || db_url.find("sqlite") == 0 || db_url.find("postgres") == 0) {
		_nameCompareString = L"lower_name = ";
		_asciinameCompareString = L"lower_asciiname = ";
		_altnameCompareString = L"lower_alternate_name = ";
	} else if (db_url.find("mysql") == 0) {
		// mysql search is case-insensitive
		_nameCompareString = L"name = ";
		_asciinameCompareString = L"asciiname = ";
		_altnameCompareString = L"alternate_name = ";
	} else {		
		std::ostringstream msg;
		msg << "Parameter 'gazetteer_db' (or icews_db if icews_stories_db unspecified) must start with 'oracle', 'mysql', 'sqlite' or 'postgres' " << db_url;
		throw UnexpectedInputException("Gazetteer::Gazetteer", msg.str().c_str());
	}

	// load nationality to nation and state abbreviation tables
	loadSubstitutions("gazetteer_nationality_to_nation", _nationalityToNation);
	loadSubstitutions("gazetteer_state_abbreviations", _stateAbbreviations);
}

Gazetteer::~Gazetteer() { }

std::vector<Gazetteer::GeoResolution_ptr> Gazetteer::geonameLookup(std::wstring locationName) {
	// canonicalize location text
	locationName = toCanonicalForm(locationName);

	if (locationName.size() == 0) {
		// bail out!
		std::vector<GeoResolution_ptr> result;
		return result;
	}

	geo_lookup_count += 1;
	//use 16-place cache to prevent redundant goenames queries
  
    int i = _last_read_i;
    do {
        if (_cache[i].loc == locationName) 
		{
			cache_count += 1;
            return _cache[i].result;
        }
        i = (i + 1) % _n_buckets;
    } while (i != _last_read_i);
    _last_read_i = _last_written_i = (_last_written_i + 1) % _n_buckets;
    _cache[_last_written_i].loc = locationName;

	std::vector<GeoResolution_ptr> result;
	boost::to_lower(locationName);
	if (locationName.size() > 200) {
		_cache[_last_written_i].result = result;
		return _cache[_last_written_i].result;
	}
	
	BOOST_FOREACH(const boost::wregex& block_re, _blockedEntries)
	{
		if (boost::regex_match(locationName, block_re)) {
			SessionLogger::info(_logString.c_str()) << "    Gazetteer entry blocked: " << locationName;
			_cache[_last_written_i].result = result;
			return _cache[_last_written_i].result;
		}
	}
	DatabaseConnection_ptr geonames_db(getSingletonGeonamesDb());

	// Get any names directly from the geonames tables
	std::wostringstream query;
	query << L"SELECT geonameid, asciiname, population, country_code, latitude, longitude "
		<< L"FROM " << _geonamesTablename
		<< L" WHERE " << _nameCompareString << DatabaseConnection::quote(locationName)
		<< L" OR " << _asciinameCompareString << DatabaseConnection::quote(locationName)
		<< L" AND country_code IS NOT NULL" 
		<< L" ORDER BY geonameid DESC";

	//SessionLogger::info("Gazetteer") << "Geoname query " << query.str();

	for (DatabaseConnection::RowIterator row = geonames_db->iter(query); row!=geonames_db->end(); ++row) {
		GeoResolution_ptr info = boost::make_shared<GeoResolution>();
		info->geonameid = row.getCellAsWString(0);
		info->cityname = row.getCellAsWString(1);
		info->population = static_cast<size_t>(row.getCellAsInt64(2));
		info->latitude = row.isCellNull(4) ? std::numeric_limits<float>::quiet_NaN() : boost::lexical_cast<float>(row.getCellAsWString(4));
		info->longitude = row.isCellNull(5) ? std::numeric_limits<float>::quiet_NaN() : boost::lexical_cast<float>(row.getCellAsWString(5));
		info->isEmpty = false;
		info->countrycode = row.getCellAsSymbol(3);
		info->countryInfo = _countryInfo[row.getCellAsSymbol(3)];
		result.push_back(info);
	}

	std::wostringstream query2;
	query2 << L"SELECT DISTINCT geonameid FROM " << _altnamesTablename << L" WHERE " << _altnameCompareString << DatabaseConnection::quote(locationName)
			<< L" ORDER BY geonameid DESC";

	// Use two loops here because getGeoResolution() also calls a db query, and MySQL can't have that while
	//  still iterating through a different query.
	std::vector<std::wstring> query2_results;
	for (DatabaseConnection::RowIterator row = geonames_db->iter(query2); row!=geonames_db->end(); ++row) {
		query2_results.push_back(row.getCellAsWString(0));
	}
	BOOST_FOREACH(std::wstring res, query2_results) {
		GeoResolution_ptr info = getGeoResolution(res);
		// if something with the same geonameid has already been added to the result vector, ignore it
		bool already_in_result = 0;
		for (size_t i1 = 0; i1 < result.size(); i1++) {
			Gazetteer::GeoResolution_ptr candidateResolution = result[i1];
			if (haveSameGeonameId(candidateResolution, info)){
				already_in_result = 1;
				i1 = result.size(); // can't use a break here because of commit hook
			}
		}	
		if (already_in_result) continue;
		result.push_back(info);
	}

	//update cache
	_cache[_last_written_i].result = result;
    return _cache[_last_written_i].result;
}

std::vector<Gazetteer::GeoResolution_ptr> Gazetteer::countryLookup(std::wstring location) 
{
	std::cerr << "Generic Gazetteer function countryLookup not implemented\n";
	return std::vector<Gazetteer::GeoResolution_ptr>();
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

	// Cache these resolutions to avoid frequent queries on the same country
	std::map<std::wstring, Gazetteer::GeoResolution_ptr>::iterator cache_iter = _largestCityCache.find(country_iso_code);
	if (cache_iter != _largestCityCache.end())
		return (*cache_iter).second;

	DatabaseConnection_ptr geonames_db(getSingletonGeonamesDb());
	std::ostringstream query;
	query << "SELECT DISTINCT geonameid FROM " << _geonamesTablename
		  << " WHERE country_code = " << DatabaseConnection::quote(country_iso_code)
		  << " AND population = (SELECT Max(population) FROM " << _geonamesTablename << " WHERE country_code = " << DatabaseConnection::quote(country_iso_code) << ")"
		  << " ORDER BY geonameid DESC";
	DatabaseConnection::RowIterator row = geonames_db->iter(query);
	if (row != geonames_db->end()) {
		std::wstring result = row.getCellAsWString(0);
		while (row != geonames_db->end()) ++row; // MySQL wants us to clear out the query before moving on
		Gazetteer::GeoResolution_ptr ret_value = getGeoResolution(result);
		_largestCityCache[country_iso_code] = ret_value;
		return ret_value;
	}
	Gazetteer::GeoResolution_ptr ret_value = boost::make_shared<GeoResolution>();
	_largestCityCache[country_iso_code] = ret_value;
	return ret_value;
}

Gazetteer::GeoResolution_ptr Gazetteer::getGeoResolution(std::wstring geonames_id) 
{
	Gazetteer::GeoResolution_ptr geoResolution = boost::make_shared<GeoResolution>();
	DatabaseConnection_ptr geonames_db(getSingletonGeonamesDb());
	std::ostringstream query;
	query << "SELECT geonameid, asciiname, population, country_code, latitude, longitude "
		<< "FROM " << _geonamesTablename
		<< " WHERE geonameid = " << DatabaseConnection::quote(geonames_id)
		<< " AND country_code IS NOT NULL";
	for (DatabaseConnection::RowIterator row = geonames_db->iter(query); row!=geonames_db->end(); ++row) 
	{
		geoResolution->geonameid = row.getCellAsWString(0);
		geoResolution->cityname = row.getCellAsWString(1);
		geoResolution->population = static_cast<size_t>(row.getCellAsInt64(2));
		geoResolution->latitude = row.isCellNull(4) ? std::numeric_limits<float>::quiet_NaN() : boost::lexical_cast<float>(row.getCellAsWString(4));
		geoResolution->longitude = row.isCellNull(5) ? std::numeric_limits<float>::quiet_NaN() : boost::lexical_cast<float>(row.getCellAsWString(5));
		geoResolution->countrycode = row.getCellAsSymbol(3);
		geoResolution->isEmpty = false;
		geoResolution->countryInfo = _countryInfo[row.getCellAsSymbol(3)];
		while (row != geonames_db->end()) ++row; // MySQL wants us to clear out the query before moving on
		return geoResolution;
	}

	//if there is no resolution found in the db, return empty
	return geoResolution;
}

Symbol Gazetteer::getGeoRegion(std::wstring geonames_id) 
{
	Gazetteer::GeoResolution_ptr geoResolution = boost::make_shared<GeoResolution>();
	DatabaseConnection_ptr geonames_db(getSingletonGeonamesDb());
	std::ostringstream query;
	query << "SELECT admin1 "
		<< "FROM " << _geonamesTablename
		<< " WHERE geonameid = " << DatabaseConnection::quote(geonames_id)
		<< " AND country_code IS NOT NULL";
	for (DatabaseConnection::RowIterator row = geonames_db->iter(query); row!=geonames_db->end(); ++row) 
	{
		Symbol s = row.getCellAsSymbol(0);
		while (row != geonames_db->end()) ++row; // MySQL wants us to clear out the query before moving on
		return s;
	}

	//if there is no resolution found in the db, return empty
	return Symbol();
}


// Helper Functions
std::vector<std::wstring> Gazetteer::toCanonicalForms(const SentenceTheory *sentTheory, const Mention* mention) {
	std::vector<std::wstring> result;

	std::wstring full_string = mention->toCasedTextString();
	result.push_back(toCanonicalForm(full_string, sentTheory, mention));

	std::wstring possessives_removed = full_string;
	boost::replace_all(possessives_removed, " 's", "'s");
	if (full_string != possessives_removed) {
		std::wstring temp = toCanonicalForm(possessives_removed, sentTheory, mention);
		result.push_back(toCanonicalForm(possessives_removed, sentTheory, mention));
	}

	// remove punctuation from full string
	std::wstring full_string_no_punct = full_string;
	full_string_no_punct.erase(
		remove_if ( full_string_no_punct.begin(), full_string_no_punct.end(), &iswpunct ),
		full_string_no_punct.end());
	if (full_string != full_string_no_punct) {
		std::wstring temp = toCanonicalForm(full_string_no_punct, sentTheory, mention);
		result.push_back(toCanonicalForm(full_string_no_punct, sentTheory, mention));
	}

	std::wstring head_string = mention->getAtomicHead()->toTextString();
	if (head_string != full_string) {
		result.push_back(toCanonicalForm(head_string, sentTheory, mention));

		// remove punctuation from head string
		std::wstring head_string_no_punct = head_string;
		head_string_no_punct.erase(
			remove_if ( head_string_no_punct.begin(), head_string_no_punct.end(), &iswpunct ),
			head_string_no_punct.end());
		if (head_string != head_string_no_punct) {
			std::wstring temp = toCanonicalForm(head_string_no_punct, sentTheory, mention);
			result.push_back(toCanonicalForm(head_string_no_punct, sentTheory, mention));
		}
	}
	return result;
}

Symbol dashSym(L"-");
Symbol commaSym(L",");
Symbol periodSym(L".");
Symbol rightParenSym(L".");
bool Gazetteer::isOKStateAbbreviation(const SentenceTheory *sentTheory, const Mention* mention) {
	if (sentTheory == 0)
		return false;

	int stoken = mention->getNode()->getStartToken();
	int etoken = mention->getNode()->getEndToken();
	const TokenSequence *ts = sentTheory->getTokenSequence();
	
	if (stoken == 0)
		return false;

	Symbol precToken = ts->getToken(stoken-1)->getSymbol();

	if (precToken != dashSym && precToken != commaSym)
		return false;

	std::wstring thisToken = ts->getToken(etoken)->getSymbol().to_string();
	if (thisToken.find(L".") != std::wstring::npos)
		return true;

	if (etoken == ts->getNTokens() - 1)
		return true;
	Symbol follToken = ts->getToken(etoken+1)->getSymbol();
	if (follToken == periodSym || follToken == rightParenSym)
		return true;

	return false;
}

std::wstring Gazetteer::toCanonicalForm(std::wstring str, const SentenceTheory *sentTheory, const Mention* mention){

	std::wstring origStr(str);

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
	if (_stateAbbreviations.find(str) != _stateAbbreviations.end() && isOKStateAbbreviation(sentTheory, mention)) str = _stateAbbreviations[str];
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
	std::vector <std::wstring> fields;
	std::wstring line;
	std::string mappingFile = ParamReader::getRequiredParam(paramFile);
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(mappingFile.c_str()));
	UTF8InputStream& file(*file_scoped_ptr);
	if (file.is_open())
	{
		while ( file.good() )
		{
			getline (file,line);
			boost::to_lower(line);
			if (line == L"") continue;
			if (line.at(0) == L'#') continue;
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
	else {	
		std::stringstream errStr;
		errStr << "Unable to load mapping file: " << mappingFile << " specified by parameter '" << paramFile << "'";
		throw UnexpectedInputException("Gazetteer::loadSubstitutions", errStr.str().c_str());
	}
}

DatabaseConnection_ptr Gazetteer::getSingletonGeonamesDb() {
	std::cerr << "Generic Gazetteer function getSingletonGeonamesDb not implemented\n";
	return DatabaseConnection_ptr();
}
