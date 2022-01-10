#include "common/leak_detection.h"

#include "Generic/common/NameEquivalenceTable.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SessionLogger.h"
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>   
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

NameEquivalenceTable::NameEquivalenceTable(std::string filename) {

	// ALL NAMES ARE LOWERCASED

	if (filename.size() == 0)
		return;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(filename.c_str());
	if (in.fail()) {
		std::string str = "Could not open " + filename;
		throw UnexpectedInputException("NameEquivalenceTable::NameEquivalenceTable()", str.c_str());
	}

	while (!in.eof()) {
		std::wstring line;	
		in.getLine(line);		
		if (line.size() == 0)
			continue;
		std::vector<std::wstring> fields;
		boost::split(fields, line, boost::is_any_of(L"\t"));
		if (fields.size() != 5)
			continue;
		EquivNameRecord_t rec;
		std::wstring name = fields[0];		
		std::transform(name.begin(), name.end(), name.begin(), tolower);
		rec.ent_type = _wtoi(fields[1].c_str());
		rec.name = fields[2];
		std::transform(rec.name.begin(), rec.name.end(), rec.name.begin(), tolower);
		rec.source = sourceStringAsEnum(fields[3]);
		rec.score = boost::lexical_cast<double>(fields[4].c_str());
		NameEquivalenceTable_t::iterator iter = _table.find(name);
		if (iter == _table.end())
			_table[name] = std::vector<EquivNameRecord_t>();
		_table[name].push_back(rec);
	}
	in.close();
}

// Return set includes the originalName
std::set<std::wstring> NameEquivalenceTable::getEquivalentNames(std::wstring originalName, double min_score) {
	std::transform(originalName.begin(), originalName.end(), originalName.begin(), tolower);
	std::set<std::wstring> names;
	NameEquivalenceTable_t::iterator iter = _table.find(originalName);
	if (iter == _table.end()) {
		names.insert(originalName);
		return names;
	}
	std::vector<EquivNameRecord_t> equivNames = iter->second;
	BOOST_FOREACH(EquivNameRecord_t rec, equivNames) {
		if (rec.score >= min_score)
			names.insert(rec.name);
	}
	if (names.size() == 0)
		names.insert(originalName);
	return names;
}

bool NameEquivalenceTable::isInTable(std::wstring originalName, double min_score) {
	std::transform(originalName.begin(), originalName.end(), originalName.begin(), tolower);
	NameEquivalenceTable_t::iterator iter = _table.find(originalName);
	if (iter == _table.end()) {
		return false;
	}
	std::set<std::wstring> names;
	std::vector<EquivNameRecord_t> equivNames = iter->second;
	BOOST_FOREACH(EquivNameRecord_t rec, equivNames) {
		if (rec.score >= min_score)
			names.insert(rec.name);
	}
	if (names.size() == 0)
		return false;
	return true;
}

NameEquivalenceTable::EquivNameSource_t NameEquivalenceTable::sourceStringAsEnum(std::wstring sourceString) {
	if (sourceString.compare(L"SEED") == 0)
		return SEED;
	if (sourceString.compare(L"WK_ALIAS") == 0)
		return WK_ALIAS;
	if (sourceString.compare(L"WK_GEONAME_ALTERNATE") == 0)
		return WK_GEONAME_ALTERNATE;
	if (sourceString.compare(L"WEB_ALIAS") == 0)
		return WEB_ALIAS;
	if (sourceString.compare(L"WK_ACRONYM") == 0)
		return WK_ACRONYM;
	if (sourceString.compare(L"ORG_EDIST_WITH_MODIFIER") == 0)
		return ORG_EDIST_WITH_MODIFIER;
	if (sourceString.compare(L"WK_DERIVED_ALIAS") == 0)
		return WK_DERIVED_ALIAS;
	if (sourceString.compare(L"WK_ALTERNATE") == 0)
		return WK_ALTERNATE;
	if (sourceString.compare(L"MIPT_ALTERNATE") == 0)
		return MIPT_ALTERNATE;
	if (sourceString.compare(L"EDIST_HIGH_SIM") == 0)
		return EDIST_HIGH_SIM;
	if (sourceString.compare(L"TOKEN_SUBSET_TREE") == 0)
		return TOKEN_SUBSET_TREE;
	if (sourceString.compare(L"WK_COUNTRY_CAPITAL") == 0)
		return WK_COUNTRY_CAPITAL;
	if (sourceString.compare(L"TST_INITIALS") == 0)
		return TST_INITIALS;
	if (sourceString.compare(L"EDIST_SHARED_PREMOD") == 0)
		return EDIST_SHARED_PREMOD;
	if (sourceString.compare(L"EDIST_WITH_MODIFIER") == 0)
		return EDIST_WITH_MODIFIER;
	if (sourceString.compare(L"EDIST_ACRONYM") == 0)
		return EDIST_ACRONYM;
	if (sourceString.compare(L"EDIST_SINGLETON") == 0)
		return EDIST_SINGLETON;
	if (sourceString.compare(L"EDIST_GENERAL") == 0)
		return EDIST_GENERAL;
	if (sourceString.compare(L"NAME_COREF") == 0)
		return NAME_COREF;
	if (sourceString.compare(L"COUNTRY_ALIASES") == 0)
		return COUNTRY_ALIASES;

	SessionLogger::info("BRANDY") << "Warning: Algorithm in NameEquivalenceTable that is not represented in C++ code: " << sourceString;
	return UNKNOWN;
}
