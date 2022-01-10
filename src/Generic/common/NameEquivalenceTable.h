#ifndef NAME_EQUIVALENCE_TABLE_H
#define NAME_EQUIVALENCE_TABLE_H

#include <string>
#include <vector>
#include <set>
#include <map>

class NameEquivalenceTable {
public:

	NameEquivalenceTable(std::string filename);

	std::set<std::wstring> getEquivalentNames(std::wstring originalName, double min_score = 0.0);
	bool isInTable(std::wstring originalName, double min_score);

	typedef enum {
		UNKNOWN,
		SEED,
		WK_ALIAS,
		WK_GEONAME_ALTERNATE,
		WEB_ALIAS,
		WK_ACRONYM,
		ORG_EDIST_WITH_MODIFIER,
		WK_DERIVED_ALIAS,
		WK_ALTERNATE,
		MIPT_ALTERNATE,
		EDIST_HIGH_SIM,
		TOKEN_SUBSET_TREE,
		WK_COUNTRY_CAPITAL,
		TST_INITIALS,
		EDIST_SHARED_PREMOD,
		EDIST_WITH_MODIFIER,
		EDIST_ACRONYM,
		EDIST_SINGLETON,
		EDIST_GENERAL,
		NAME_COREF,
		COUNTRY_ALIASES
	} EquivNameSource_t;
	
	typedef struct {
		std::wstring name;
		double score;
		EquivNameSource_t source;
		unsigned int ent_type;
	} EquivNameRecord_t;

private:

	typedef std::map<std::wstring, std::vector<EquivNameRecord_t> > NameEquivalenceTable_t;
	NameEquivalenceTable_t _table;

	EquivNameSource_t sourceStringAsEnum(std::wstring sourceString);

};

#endif
