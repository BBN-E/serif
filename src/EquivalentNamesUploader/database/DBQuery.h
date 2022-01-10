#ifndef DBQUERY_H
#define DBQUERY_H


#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
class Database;
class ResultSet;
class UTF8InputStream;
class UTF8Token;
class ValueVector;

using namespace std;

/* Copied from distill_limits.h */
const int MAX_SOURCES=100;

class DBQuery
{
public:
    DBQuery(const char * server, const char * database, const char * username, const char * password, const std::wstring& sqlQueryPhrasePrefix);  // The latter should be L"N'" if the db has nvarchar and ntext, L"'" otherwise
	~DBQuery();
	bool isDBAlive();
	bool getDocumentsForNames(ValueVector * names, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForNamesWithEventTypes(ValueVector * names, ValueVector * event_types, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForNamesInEventArgs(ValueVector * names, ValueVector * event_types, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForNamesInRelations(ValueVector* names, ValueVector* relations, ValueVector* sources, ValueVector* dates, ValueVector* ingestDates, ValueVector* locations, int max_results, ResultSet* results);
	bool getDocumentsForNamesInTwoTieredRelation(ValueVector* names, ValueVector* relationsMain, ValueVector* relationsSecondary, ValueVector* sources, ValueVector* dates, ValueVector* ingestDates, ValueVector* locations, int max_results, ResultSet* results);
	bool getDocumentsForNamesInRelationEventIntersection(ValueVector* names, ValueVector* relationsMain, ValueVector* eventsSecondary, ValueVector* sources, ValueVector* dates, ValueVector* ingestDates, ValueVector* locations, int max_results, ResultSet* results);
	bool getDocumentsForNameSets(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForNameSetsWithRelations(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForNameSetsWithRelationsAndNameSet(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * name_set_3, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForTwoRelations(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * name_set_3, ValueVector * name_set_4, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForNameAndPatterns(ValueVector * name_set, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForStringAndPatterns(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results, bool isANDSearch);
    bool getPERWithRelationToORG(ValueVector * org_names, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForStrings(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results, bool isANDSearch);
    bool getDocumentsForStringUsingLike(std::wstring text, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	int getDocumentCountForStrings(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, bool isANDSearch);
	bool getDocumentsForExactFuzzySearch(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	bool getDocumentsForFuzzySearch(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
    bool getRelationsForNameSets(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * relations, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	
	bool getRelationsForNameSetAndTypeSet(ValueVector * name_set_1, ValueVector * type_set, ValueVector * role_1_relations, ValueVector * role_2_relations, ValueVector * role_any_relations, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	
	bool getEventsForNames(ValueVector * names, ValueVector * events, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
    bool getDocumentsForKeywords(ValueVector * required, ValueVector * excluded, ValueVector * optional, const wchar_t * free_text, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
    bool getDocumentsForNamesAndEvents(ValueVector * names, ValueVector * events, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results);
	
	bool getEnglishSources(ResultSet * results);

	// for a given document, updates the pair( short name, canonical name ) value type for all keys in 'names' with DB resolutions
	void getDocumentResolutions( std::wstring brandyID, std::map< int, std::pair<Symbol, Symbol> > & names );

	double getConfidenceForSegment(const std::wstring& brandyID, int official_segment_id);
	std::wstring getDatelineForDocument(const std::wstring& brandyID);
	
	void printResults(ResultSet * results);
	
	bool getResults(wstring query, vector <string> columns, vector <int> column_types, ResultSet * results);

private:
	// Database connection parameters
	char _conn_str[500];
	char _username[100];
	char _password[100];

	char _error_str[1000];
	map<wstring, int> _stop_words;
	Database * _db;
    bool _failOnDbFailure;
	bool _verbose;
    std::wstring _sqlQueryPhrasePrefix;  // This will be N' for unicode and ' for ascii.  N' is bad for speed if the db has varchar instead of nvarchar.

	void printQuery(const wchar_t * function_name, const wchar_t * query) const;
	wstring getQueryForStrings(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, bool isANDSearch,
        wstring table = L"DOCUMENTS", wstring textColumn = L"SOURCE_TEXT", wstring keyColumn = L"docID");
    ValueVector pruneValueVectorForOrQuery(const ValueVector& strings);
    ValueVector splitWstringIntoTokens(const wstring& text);
	std::pair<std::wstring,std::wstring> getLocationsSQL(ValueVector * locations);
	std::wstring getLocationsSQLForInnerJoin(ValueVector * locations);

	// Note: It is very inefficient to have a CONTAINS() call in the where clause after a join.  It appears that MSSQL is evaluating the CONTAINS() clause
	// once for each row, and does not cache the results.  As a result, using a CONTAINSTABLE() in the join itself is better.  Please use the above
	// getLocationsSQL() or getLocationsSQLForInnerJoin() instead.
	std::wstring getSourcesDatesLocationsSQL(ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations);
    static std::wstring getTemplate18Patterns();
	bool isStopWord(wstring word);
};

#endif //#ifndef DBQUERY_H

