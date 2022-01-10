#include "common/leak_detection.h" // This must be the first #include

#include <ctime>
#include <boost/algorithm/string/trim.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Database.h"
#include "ResultSet.h"
#include "ValueVector.h"
#include "XDocRetriever.h"



static int CHAR_TYPE = 0;
static int INT_TYPE = 1;
static int DOUBLE_TYPE = 2;


XDocRetriever::XDocRetriever(string server, std::string xdoc_database, string username, string password) {

	_username = username;
	_password = password;

	// Create a database connection string
	_conn_str = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=";
	_conn_str += username;
	_conn_str += ";Initial Catalog=";
	_conn_str += xdoc_database;
	_conn_str += ";Data Source=";
	_conn_str += server;

	verbose = true;

	_db = _new Database(_username.c_str(),_password.c_str(),_conn_str.c_str());
}

XDocRetriever::~XDocRetriever(){
	delete _db;
}

/*
void XDocRetriever::getEquivalentNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results) {
	/* The function returns results for a query in the form:
		SELECT name_string, freq_count FROM NAME_CLUSTERS WHERE cluster_id IN (
		SELECT B.cluster_id FROM NAME_CLUSTERS A, NAME_CLUSTERS B, SERIF_STATS C WHERE A.name_id = C.name_1_id AND
		B.name_id = C.name_2_id AND A.name_string = 'abbas' AND A.entity_type = 'PER' AND 
		A.entity_type = B.entity_type AND score > 0.1 
		UNION SELECT cluster_id FROM NAME_CLUSTERS WHERE name_string = 'abbas' AND entity_type = 'PER')
		ORDER BY FREQ_COUNT DESC
	*/
/*
 

	std::wstring query = L"SELECT name_string, freq_count FROM NAME_CLUSTERS WHERE cluster_id IN ("\
		L"SELECT B.cluster_id FROM NAME_CLUSTERS A, NAME_CLUSTERS B, SERIF_STATS C "\
		L"WHERE A.name_id = C.name_1_id AND B.name_id = C.name_2_id AND A.name_string = '";
	
	std::wstring name_str = name;	
	std::wstring sql_value = UnicodeUtil::normalizeNameForSql(name_str);

	query.append(sql_value);
	query.append(L"' AND A.ENTITY_TYPE = '");
	query.append(entity_type);
	query.append(L"' AND A.entity_type = B.entity_type AND score > 0.1 "\
		L"UNION SELECT cluster_id FROM NAME_CLUSTERS WHERE name_string = '");
	query.append(sql_value);
	query.append(L"' AND ENTITY_TYPE = '");
	query.append(entity_type);
	query.append(L"') ORDER BY FREQ_COUNT DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("Name_String");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("FREQ_COUNT");
	column_types.push_back(INT_TYPE);
	printQuery(L"getEquivalentNames", query.c_str());
	getResults(query, column_names, column_types, results);
}

*/

int XDocRetriever::getXDocID(const wchar_t * full_doc_id, int entity_id) {
    // The function returns XDoc ID from the new XDoc v2 results

	string extraction_db_char = ParamReader::getRequiredParam("db_en_database");
	wstring extraction_db_wchar(extraction_db_char.begin(), extraction_db_char.end());

	wstringstream wss;
	wss <<  L"SELECT cluster_id FROM ENTITIES E, " << extraction_db_wchar << L".dbo.DOCUMENTS D WHERE E.docID = D.docID AND D.brandyID = '";
	wss << full_doc_id << L"' AND entityID = " << entity_id;

	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("cluster_id");
	// add column data type
    column_types.push_back(INT_TYPE);
	//printQuery(L"getXDocID", wss.str().c_str());
    ResultSet * results = _new ResultSet();
	getResults(wss.str(), column_names, column_types, results);
    size_t n_rows = results->getColumnValues(0)->getNValues();
    int cluster_id = 0;
    if (n_rows > 0)
        cluster_id = _wtoi(results->getColumnValues(0)->getValue(0));
    delete results;
    return cluster_id;
}

std::wstring XDocRetriever::getCanonicalName(const wchar_t * full_doc_id, int entity_id) {

	string extraction_db_char = ParamReader::getRequiredParam("db_en_database");
	wstring extraction_db_wchar(extraction_db_char.begin(), extraction_db_char.end());
	
	wstringstream wss;
	wss << L"SELECT canonical_name FROM CLUSTERS C, ENTITIES E, " << extraction_db_wchar;
	wss << L".dbo.DOCUMENTS D WHERE E.docID = D.docID AND C.cluster_id = E.cluster_id AND D.brandyID = '";
	wss << full_doc_id << L"' AND entityID = " << entity_id;
	
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("canonical_name");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	//printQuery(L"getCanonicalName", wss.str().c_str());
    ResultSet * results = _new ResultSet();
	getResults(wss.str(), column_names, column_types, results);
	if (results->getColumnValues(0)->getNValues() > 0) {
		wstring canonical_name(results->getColumnValues(0)->getValue(0));
		boost::trim(canonical_name);
		delete results;
		return canonical_name;
	}
    delete results;
	return L"";
}
void XDocRetriever::getENWikipediaEquivalentNames(const wchar_t * name, ResultSet * results) {
	
	std::wstring name_str = name;
	std::wstring sqlName =  UnicodeUtil::normalizeNameForSql(name_str);

	wstring query = L"SELECT DISTINCT page_title, 1, 1 FROM GetWikiAllEnAliases('"; 

	query.append(sqlName);
	query.append(L"')");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(INT_TYPE);
	getResults(query, column_names, column_types, results);
}
void XDocRetriever::getAREquivalentNames(const wchar_t * name, ResultSet * results) {

	std::wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

	wstring query = L"SELECT DISTINCT page_title, 1, 1 FROM GetWikiAllArAliases(N'"; 

	query.append(sqlName);
	query.append(L"')");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(INT_TYPE);
	getResults(query, column_names, column_types, results);
}

void XDocRetriever::getCHEquivalentNames(const wchar_t * name, ResultSet * results) {

	std::wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

	wstring query = L"SELECT DISTINCT page_title, 1, 1 FROM GetWikiAllZhAliases(N'"; 

	query.append(sqlName);
	query.append(L"')");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(INT_TYPE);
	getResults(query, column_names, column_types, results);
}

void XDocRetriever::getCanonicalNameForCountry(const wchar_t * name, ResultSet * results) {

	std::wstring query = L"SELECT A1.country_name FROM country_aliases A1, country_aliases A2 "\
		L"WHERE A1.country_code = A2.country_code AND A1.canonical = 1 AND A2.country_name = '";

	std::wstring name_str = name;
	std::wstring sql_value = UnicodeUtil::normalizeNameForSql(name_str);

	query.append(sql_value);
	query.append(L"'");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("country_name");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	getResults(query, column_names, column_types, results);
}

void XDocRetriever::getY2EquivalentNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results) {
	getY2EquivalentNames(name, entity_type, 0, results);
}

void XDocRetriever::getY2EquivalentNames(const wchar_t * name, const wchar_t * entity_type, double score_threshold, ResultSet * results) {
	/* The function returns results for a query in the form:
		SELECT C2.name_string, edit_distance_score FROM CORPUS_NAMES C1, CORPUS_NAMES C2, XDOC_SCORES S 
        WHERE C1.id = S.id_1 AND C2.id = S.id_2 AND C1.name_string = 'george bush' AND C1.entity_type = 'PER' 
        AND edit_distance_score <= 0.2
        ORDER BY edit_distance_score
	*/

    wstring query = L"SELECT C2.name_string, C2.freq_count, E.score, E.source, C2.entity_type FROM CORPUS_NAMES C1, CORPUS_NAMES C2, EQUIVALENT_NAMES E "\
        L"WHERE C1.id = E.id AND C2.id = E.eq_id AND C1.name_string = '";

	std::wstring name_str = name;
	wstring sql_value = UnicodeUtil::normalizeNameForSql(name_str);

	query.append(sql_value);
    if (wcslen(entity_type) > 0) {
	    query.append(L"' AND C1.ENTITY_TYPE = '");
	    query.append(entity_type);
    }
	query.append(L"' AND score >= ");
	std::wostringstream wos;
	wos << score_threshold;
	wstring score_str = wos.str();
	query.append(score_str.c_str());
	query.append(L" ORDER BY score DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
	column_names.push_back("source");
    column_types.push_back(INT_TYPE);
	column_names.push_back("entity_type");
	column_types.push_back(CHAR_TYPE);
	getResults(query, column_names, column_types, results);
}
void XDocRetriever::getY2LongerNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results) {
	/*  returns results for a query seeking longer names in the DB that end (or maybe start) with the input
	*/

    wstring query = L"SELECT name_string, freq_count  FROM CORPUS_NAMES "\
        L"WHERE name_string LIKE '% "; // need the blank after % to block original name

	std::wstring name_str = name;
	std::wstring sql_value = UnicodeUtil::normalizeNameForSql(name_str);

	query.append(sql_value);
    if (wcslen(entity_type) > 0) {
	    query.append(L"' AND ENTITY_TYPE = '");
	    query.append(entity_type);
    }
	query.append(L"' ORDER BY freq_count DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
	getResults(query, column_names, column_types, results);
}
void XDocRetriever::getSerifCoreferenceNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results) {
	getSerifCoreferenceNames(name, entity_type, 0, results);
}

void XDocRetriever::getSerifCoreferenceNames(const wchar_t * name, const wchar_t * entity_type, double score_threshold, ResultSet * results) {
    /* The function returns results for a query in the form:
    SELECT C2.name_string, C2.freq_count, S.coref_score 
    FROM SERIF_STATS S, CORPUS_NAMES C1, CORPUS_NAMES C2 
    WHERE S.name_1_id = C1.id AND S.name_2_id = C2.id AND C1.entity_type = 'ORG'
    AND C1.name_string = 'United Nations' ORDER BY coref_score DESC
    */
 
	wstring name_str = name;

    wstring query = L"SELECT C2.name_string, C2.freq_count, S.coref_score AS score FROM SERIF_STATS S, CORPUS_NAMES C1, CORPUS_NAMES C2 "\
        L"WHERE S.name_1_id = C1.id AND S.name_2_id = C2.id AND C1.name_string = '";

	wstring sql_value = UnicodeUtil::normalizeNameForSql(name_str);

	query.append(sql_value);
    if (wcslen(entity_type) > 0) {
	    query.append(L"' AND C1.entity_type = '");
	    query.append(entity_type);
    }
	query.append(L"' AND coref_score >= ");
	std::wostringstream wos;
	wos << score_threshold;
	wstring score_str = wos.str();
	query.append(score_str.c_str());
	query.append(L" ORDER BY coref_score DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
	getResults(query, column_names, column_types, results);
}

void XDocRetriever::getY2EquivalentNamesForTitle(const wchar_t * title, const wchar_t * entity_type, ResultSet * results) {
	std::wstring title_str = title;

	std::wstring query = L"SELECT name_string, freq_count, score FROM GetNamesForTitle('";
   
	std::wstring sql_title = UnicodeUtil::normalizeNameForSql(title_str);

	query.append(sql_title);
	query.append(L"', '");
	query.append(entity_type);
	query.append(L"')");
	query.append(L" ORDER BY freq_count DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
	getResults(query, column_names, column_types, results);
}

void XDocRetriever::getY2EquivalentNamesForNameAndTitle(const wchar_t * name, const wchar_t * title, const wchar_t * entity_type, ResultSet * results) {
    wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

    wstring title_str = title;
	std::wstring sqlTitle = UnicodeUtil::normalizeNameForSql(title_str);

    wstring query = L"SELECT name_string, freq_count, score FROM GetNamesForNameAndTitle('";
   	
	query.append(sqlName);
	query.append(L"', '");
    query.append(sqlTitle);
	query.append(L"', '");
	query.append(entity_type);
	query.append(L"')");
	query.append(L" ORDER BY freq_count DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
	getResults(query, column_names, column_types, results);
}

void XDocRetriever::getY2EquivalentNamesForUnknown(const wchar_t * name, const wchar_t * entity_type, ResultSet * results) {
    wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

    wstring query = L"SELECT name_string, freq_count, score FROM GetNamesForUnknown('";
   	
	query.append(sqlName);
	query.append(L"', '");
	query.append(entity_type);
	query.append(L"')");
	query.append(L" ORDER BY score DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
    column_types.push_back(INT_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
	getResults(query, column_names, column_types, results);
}
int XDocRetriever::getFrequencyCount(const wchar_t * name, const wchar_t * entity_type) {
    /* The function returns results for a query in the form:
		SELECT freq_count FROM CORPUS_NAMES WHERE name_string = 'george bush' AND entity_type = 'PER' 
    */
    // Normalize the name
	wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

	wstring query = L"SELECT freq_count FROM CORPUS_NAMES WHERE name_string = '";	
	query.append(sqlName);
	query.append(L"' AND entity_type = '");
	query.append(entity_type);
	query.append(L"'");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("freq_count");
	// add column data type
    column_types.push_back(INT_TYPE);
    ResultSet * results = _new ResultSet();
	getResults(query, column_names, column_types, results);
    size_t n_rows = results->getColumnValues(0)->getNValues();
    int count = 0;
    if (n_rows > 0)
        count = _wtoi(results->getColumnValues(0)->getValue(0));
    delete results;
    return count;
}

int XDocRetriever::getFrequencyCountWithoutType(const wchar_t * name) {
    /* The function returns results for a query in the form:
		SELECT SUM(freq_count) AS freq_count FROM CORPUS_NAMES WHERE name_string = 'george bush' 
    */
    // Normalize the name
	wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

	wstring query = L"SELECT SUM(freq_count) AS freq_count FROM CORPUS_NAMES WHERE name_string = '";	
	query.append(sqlName);
	query.append(L"'");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("freq_count");
	// add column data type
    column_types.push_back(INT_TYPE);
    ResultSet * results = _new ResultSet();
	getResults(query, column_names, column_types, results);
    size_t n_rows = results->getColumnValues(0)->getNValues();
    int count = 0;
    if (n_rows > 0)
        count = _wtoi(results->getColumnValues(0)->getValue(0));
    delete results;
    return count;
}

/*
void XDocRetriever::getSerifEquivalentNames(const wchar_t * name, const wchar_t * entity_type, ResultSet * results) {
	getSerifEquivalentNames(name, entity_type, 0.1, results);
}

void XDocRetriever::getSerifEquivalentNames(const wchar_t * name, const wchar_t * entity_type, double score_threshold, ResultSet * results) {
	/* The function returns results for a query in the form:
		SELECT B.name_string, B.freq_count FROM NAMES A, NAMES B, SERIF_STATS C WHERE A.id = C.name_1_id AND
		B.id = C.name_2_id AND A.name_string = 'hillary clinton' AND A.entity_type = 'PER' AND 
		A.entity_type = B.entity_type AND score > 0.1 
		UNION SELECT name_string, freq_count FROM NAMES WHERE name_string = 'hillary clinton' AND entity_type = 'PER'
		ORDER BY B.FREQ_COUNT DESC
	*/
/*
	// Normalize the name
	std::wstring name_str = name;
	std::wstring sqlName =  UnicodeUtil::normalizeNameForSql(name_str);

	// Special case: George Bush
	if ((wcscmp(sqlName, L"george bush") == 0) ||
		(wcscmp(sqlName, L"george w bush") == 0))
		score_threshold = 0.01;

	wstring query = L"SELECT DISTINCT B.name_string, B.freq_count FROM NAMES A, NAMES B, SERIF_STATS C "\
		L"WHERE A.id = C.name_1_id AND B.id = C.name_2_id AND A.name_string = '";
	
	query.append(sqlName);
	query.append(L"' AND A.ENTITY_TYPE = '");
	query.append(entity_type);
	query.append(L"' AND A.entity_type = B.entity_type AND score > ");
	std::wostringstream wos;
	wos << score_threshold;
	wstring score_str = wos.str();
	query.append(score_str.c_str());
	query.append(L" UNION SELECT name_string, freq_count FROM NAMES WHERE name_string = '");
	query.append(sql_value);
	query.append(L"' AND ENTITY_TYPE = '");
	query.append(entity_type);
	query.append(L"' ORDER BY B.FREQ_COUNT DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("name_string");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	column_names.push_back("freq_count");
	column_types.push_back(INT_TYPE);
	//printQuery(L"getSerifEquivalentNames", query.c_str());
	getResults(query, column_names, column_types, results);
}
*/

void XDocRetriever::printQuery(const wchar_t * function_name, const wchar_t * query) const {
	if (verbose)
		SessionLogger::info("BRANDY") << function_name << L":\n" << query << L"\n";
}

std::wstring XDocRetriever::getBestName(const wchar_t * name, const wchar_t * entity_type, bool short_name) {
	return getBestName(name, entity_type, 0, short_name);
}

std::wstring XDocRetriever::getBestName(const wchar_t * name, const wchar_t * entity_type, double score_threshold, bool short_name) {
	ResultSet * results = _new ResultSet();
    getY2EquivalentNames(name, entity_type, score_threshold, results);
	//getSerifEquivalentNames(name, entity_type, score_threshold, results);
	size_t n_rows = results->getColumnValues(0)->getNValues();
	if (n_rows == 0) {
		delete results;
		return L"";
	}
	size_t row_index = 0;
	size_t name_len = wcslen(results->getColumnValues(0)->getValue(0));
	int top_freq_count = _wtoi(results->getColumnValues(1)->getValue(0));
	for (size_t i = 1; i < n_rows; i++) {
		const wchar_t * name_str = results->getColumnValues(0)->getValue(i);
		if (short_name && (wcslen(name_str) >= name_len))
			continue;
		if (!short_name && (wcslen(name_str) <= name_len))
			continue;
		int freq_count = _wtoi(results->getColumnValues(1)->getValue(i));
		if ((freq_count * 1.0 / top_freq_count) >= 0.1) {
			row_index = i;
			name_len = wcslen(name_str);
		}
	}
	std::wstring return_value(results->getColumnValues(0)->getValue(row_index));
	delete results;
	return return_value;
}


std::wstring XDocRetriever::getCanonicalName(const wchar_t * name, const wchar_t * entity_type) {
	return getCanonicalName(name, entity_type, 0.1);
}

std::wstring XDocRetriever::getCanonicalName(const wchar_t * name, const wchar_t * entity_type, double score_threshold) {
	return getBestName(name, entity_type, score_threshold, false);
}

std::wstring XDocRetriever::getShortName(const wchar_t * name, const wchar_t * entity_type) {
	return getShortName(name, entity_type, 0.1);
}

std::wstring XDocRetriever::getShortName(const wchar_t * name, const wchar_t * entity_type, double score_threshold) {
	return getBestName(name, entity_type, score_threshold, true);
}

void XDocRetriever::getCanonicalNames(ValueVector ** name_sets, size_t n_name_sets, ValueVector * entity_types, ValueVector ** canonical_names) {
	for (size_t i = 0; i < n_name_sets; i++) {
		ValueVector * names = name_sets[i];
		ValueVector * results = _new ValueVector();
		if (entity_types->getNValues() >= 1)
			getCanonicalNames(names, entity_types->getValue(0), results, true);
		canonical_names[i] = results;
	}
}

void XDocRetriever::getCanonicalNames(ValueVector * names, const wchar_t * entity_type, ValueVector * canonical_names, bool ignore_one_word_names) {
	// If ignore_one_word_names is true, names consisting of one word will be ignored only if there are longer names
	if ((names == 0) || (names->getNValues() == 0))
		return;
	ValueVector * one_word_names = _new ValueVector();
	ValueVector * longer_names = _new ValueVector();
	for (size_t i = 0; i < names->getNValues(); i++) {
		wstring name = names->getValue(i);
		bool is_one_word = trimAndCheckForWS(name);
		if (is_one_word && ignore_one_word_names) {
			if (!one_word_names->contains(name.c_str()))
				one_word_names->addValue(name.c_str());
		} else if (!longer_names->contains(name.c_str()))
			longer_names->addValue(name.c_str());
	}
	ValueVector * selected_names = longer_names;
	if (selected_names->getNValues() > 0)
		delete one_word_names;
	else {
		selected_names = one_word_names;
		delete longer_names;
	}
	
	for (size_t i = 0; i < selected_names->getNValues(); i++) {
		wstring name = selected_names->getValue(i);
		wstring canonical_name = getCanonicalName(name.c_str(), entity_type);
		if (canonical_name.size() > 0 && (!canonical_names->contains(canonical_name.c_str())))
			canonical_names->addValue(canonical_name);
	}
	delete selected_names;
}

void XDocRetriever::getBestEntityType(const wchar_t * name, wchar_t * result) {
    wstring name_str = name;
	std::wstring sqlName = UnicodeUtil::normalizeNameForSql(name_str);

    wstring query = L"SELECT TOP 1 entity_type FROM CORPUS_NAMES WHERE name_string = '";

    query.append(sqlName);
    query.append(L"' ORDER BY freq_count DESC");
	vector <string> column_names;
	vector <int> column_types;
	// add column name 
	column_names.push_back("entity_type");
	// add column data type
	column_types.push_back(CHAR_TYPE);
	printQuery(L"getBestEntityType", query.c_str());
    ResultSet * results = _new ResultSet();
	getResults(query, column_names, column_types, results);
    wcscpy(result, L"");
    if (results->getColumnValues(0)->getNValues() > 0)
        wcscpy(result, results->getColumnValues(0)->getValue(0));
    delete results;
}

bool XDocRetriever::trimAndCheckForWS(wstring& str)
{
	wstring::size_type pos = str.find_last_not_of(L' ');
	if(pos != wstring::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(L' ');
		if(pos != wstring::npos) 
			str.erase(0, pos);
	}
	else 
		str.erase(str.begin(), str.end());
	
	// check for whitespace
	pos = str.find_first_of(L' ');
	return (pos == wstring::npos);
}

void XDocRetriever::getResults(wstring query, vector <string> columns, vector <int> column_types, ResultSet * results) {

	try{
		
		Table tbl;
		
		time_t start_t = time(0);
        _db->Execute(query.c_str(), tbl);
		time_t elapsed_t = time(0) - start_t;
		
        if( elapsed_t >= 5 ) {
			ostringstream ostr;
            ostr << L"\nLONG_SQL_QUERY (" << static_cast<unsigned long>(elapsed_t) << L") : " << query << endl;
			SessionLogger::info("BRANDY") << ostr.str();
        }
        
		ValueVector ** values = _new ValueVector*[columns.size()];
		for (size_t i = 0; i < columns.size(); i++) {
			values[i] = _new ValueVector();
		}
		wchar_t value_char[1025];
		int value_int = 0;
		double value_double = 0;
		if(!tbl.ISEOF())
			tbl.MoveFirst();

		bool error = false;
		while(!tbl.ISEOF())
		{
			for (size_t i = 0; i < columns.size(); i++) {
				const char * column_name = columns.at(i).c_str();
				int column_type = column_types.at(i);
				error = false;		
				if (column_type == CHAR_TYPE) {
					if(tbl.Get(column_name, value_char))
						values[i]->addValue(value_char);
					else
						error = true;
				} else if (column_type == INT_TYPE) {
					if(tbl.Get(column_name, value_int))
						values[i]->addValue(_itow(value_int, value_char, 10));
					else
						error = true;
				} else if (column_type == DOUBLE_TYPE) {
					if(tbl.Get(column_name, value_double)) {
						std::wostringstream wos;
						wos << value_double;
						wstring wstr = wos.str();
						values[i]->addValue(wstr.c_str());
						//values[i]->addValue(_gcvt(value_double, 10, value_char));
					} else
						error = true;
				}
				if (error)
				{
					tbl.GetErrorErrStr(_error_str);
					SessionLogger::info("BRANDY")<<"\nXDocRetriever::getResults(): "<<_error_str<<"\n";
					break;
				}
			}
			tbl.MoveNext();
		}
		
		for (size_t i = 0; i < columns.size(); i++) {
			results->addColumn(columns.at(i).c_str(), values[i]);
		}
		delete [] values;
	
		return;
	}
	catch( ... ){
		SessionLogger::info("BRANDY") << "<in getResults for SQL: '" << query << "'>\n" << endl;
		return;
	}
}
