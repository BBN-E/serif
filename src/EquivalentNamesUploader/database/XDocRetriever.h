
#ifndef XDOC_RETRIEVER_H
#define XDOC_RETRIEVER_H

#include <sstream>
#include <vector>
class Database;
class ResultSet;
class ValueVector;

using namespace std;

class XDocRetriever
{
public:
	XDocRetriever(std::string server, std::string xdoc_database, std::string username, std::string password);
	~XDocRetriever();

	void getEquivalentNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results);
	void getSerifEquivalentNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results);
	void getSerifEquivalentNames(const wchar_t * name, const wchar_t * entity_type, double score_threshold, ResultSet * results);
    void getY2EquivalentNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results);
	void getY2LongerNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results);
	void getAREquivalentNames(const wchar_t * name, ResultSet * results);
	void getCHEquivalentNames(const wchar_t * name, ResultSet * results);
	void getENWikipediaEquivalentNames(const wchar_t * name, ResultSet * results);
	void getY2EquivalentNames(const wchar_t * name, const wchar_t * entity_type, double score_threshold, ResultSet * results);
    void getSerifCoreferenceNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results);
    void getSerifCoreferenceNames(const wchar_t * name, const wchar_t * entity_type, double score_threshold, ResultSet * results);
    void getY2EquivalentNamesForTitle(const wchar_t * title, const wchar_t * entity_type, ResultSet * results);
    void getY2EquivalentNamesForNameAndTitle(const wchar_t * name, const wchar_t * title, const wchar_t * entity_type, ResultSet * results);
    void getY2EquivalentNamesForUnknown(const wchar_t * name, const wchar_t * entity_type, ResultSet * results);
    std::wstring getCanonicalName(const wchar_t * name, const wchar_t * entity_type);
	std::wstring getCanonicalName(const wchar_t * name, const wchar_t * entity_type, double score_threshold);
	std::wstring getShortName(const wchar_t * name, const wchar_t * entity_type);
	std::wstring getShortName(const wchar_t * name, const wchar_t * entity_type, double score_threshold);
	void getCanonicalNames(ValueVector ** name_sets, size_t n_name_sets, ValueVector * entity_types, ValueVector ** canonical_names);
	void getCanonicalNames(ValueVector * names, const wchar_t * entity_type, ValueVector * canonical_names, bool ignore_one_word_names);
	void getCanonicalNameForCountry(const wchar_t * name, ResultSet * results);
    void getBestEntityType(const wchar_t * name, wchar_t * result);
    int getFrequencyCount(const wchar_t * name, const wchar_t * entity_type);
    int getFrequencyCountWithoutType(const wchar_t * name);
	int getXDocID(const wchar_t * full_doc_id, int entity_id);
	std::wstring getCanonicalName(const wchar_t * full_doc_id, int entity_id);

private:

	// Database connection parameters
	std::string _conn_str;
	std::string _username;
	std::string _password;

	char _error_str[200];

	Database * _db;

	bool verbose;
	void printQuery(const wchar_t * function_name, const wchar_t * query) const;
	std::wstring getBestName(const wchar_t * name, const wchar_t * entity_type, bool short_name);
	std::wstring getBestName(const wchar_t * name, const wchar_t * entity_type, double score_threshold, bool short_name);
	void getResults(wstring query, vector <string> columns, vector <int> column_types, ResultSet * results);
	bool trimAndCheckForWS(wstring& str);
};

#endif // #ifndef XDOC_RETRIEVER_H

