#include "Generic/common/leak_detection.h" // This must be the first #include

#include <ctime>

#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/algorithm/string/trim.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/TimedSection.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnicodeUtil.h"
#include "Database.h"
#include "DBQuery.h"
#include "ResultSet.h"
#include "ValueVector.h"
#include <boost/scoped_ptr.hpp>

static int CHAR_TYPE = 0;
static int INT_TYPE = 1;
static int DOUBLE_TYPE = 2;


DBQuery::DBQuery(const char * server, const char * database, const char * username, const char * password, const std::wstring& sqlQueryPhrasePrefix) : _sqlQueryPhrasePrefix(sqlQueryPhrasePrefix) {
    string conn_str = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=";
    conn_str.append(username);
    conn_str.append(";Password=");
    conn_str.append(password);
    conn_str.append(";Initial Catalog=");
    conn_str.append(database);
    conn_str.append(";Data Source=");
    conn_str.append(server);
    conn_str.append(";");
    strcpy(_conn_str, conn_str.c_str());
    strcpy(_username, username);
    strcpy(_password, password);
    _failOnDbFailure = ParamReader::getRequiredTrueFalseParam("fail_on_db_failure");
    _verbose = ParamReader::getRequiredTrueFalseParam("verbose");
    _db = _new Database(_username,_password,_conn_str);
}

DBQuery::~DBQuery(){

    delete _db;
}

bool DBQuery::isDBAlive() {

    // the constructor now throws if it can't acquire a DB connection, so this is a tautology

    return true;
}

bool DBQuery::getDocumentsForNamesInEventArgs(ValueVector * names, ValueVector * event_types, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNamesInEventArgs");
    /* Sped things up by making execution plan more explicit.  Creates queries of the form:
select distinct top 50000 d.brandyID, F.mentions_mention_cnt_ratio as score from 
  (
    select C.docID, C.mentions_mention_cnt_ratio from
      (
        select distinct B.docID, B.mentions_mention_cnt_ratio, ea.eventID from
          (
            select A.docID, A.entityID, ec.mentions_mention_cnt_ratio from
              (
                select distinct em.docID, em.entityID from Entity_mentions em
                where em.head_norm in ('baghdad','bagdad','bagghdad','baghadi','bahgdad','bghdad','bghdadg') 
              ) A
            inner join
              Entity_counts ec
            on A.docID = ec.docID and A.entityID = ec.entityID
          ) B
        inner join
          Event_arguments ea
        on B.docID = ea.docID and ea.entityID = B.entityID
      ) C
    inner join
      Events e
    on C.docID = e.docID and C.eventID = e.eventID
    where ((e.event_type = 'Conflict' and e.event_subtype = 'Attack'))
  ) F
inner join
  Documents d
on F.docID = d.docID
inner join CONTAINSTABLE(Documents, source_text, N'"american" OR "boston" OR "chicago"') as source_hits on source_hits.[key] = d.docID
inner join
  Sources s
on d.sourceid = s.sourceid
and d.DOCDATE BETWEEN '20050901' AND '20060831' order by score desc, brandyID asc
    */

    wstringstream query;
    if ((names == 0) || (names->getNValues() == 0))
        return false;
    if ((event_types == 0) || (event_types->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "select distinct top " << max_results << " d.brandyID, F.mentions_mention_cnt_ratio as score from ";
    query << "  ( ";
    query << "    select C.docID, C.mentions_mention_cnt_ratio from ";
    query << "      ( ";
    query << "        select distinct B.docID, B.mentions_mention_cnt_ratio, ea.eventID from ";
    query << "          ( ";
    query << "            select A.docID, A.entityID, ec.mentions_mention_cnt_ratio from ";
    query << "            ( ";
    query << "                select distinct em.docID, em.entityID from Entity_mentions em ";
    query << "                where em.head_norm in ( ";
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << "                                      ) ";
    query << "              ) A ";
    query << "            inner join ";
    query << "              Entity_counts ec ";
    query << "            on A.docID = ec.docID and A.entityID = ec.entityID ";
    query << "          ) B ";
    query << "        inner join ";
    query << "          Event_arguments ea ";
    query << "        on B.docID = ea.docID and ea.entityID = B.entityID ";
    query << "      ) C ";
    query << "    inner join ";
    query << "      Events e ";
    query << "    on C.docID = e.docID and C.eventID = e.eventID ";
    query << "    where ( ";
    for( size_t i = 0; i < event_types->getNValues(); i++ ){
        if (i > 0) query << " OR ";
        wstring val = event_types->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
        query << "(e.event_type = '" << base << "' and e.event_subtype = '" << sub << "')";
    }
    query << "          ) ";
    query << "  ) F ";
    query << "inner join ";
    query << "  Documents d ";
    query << "on F.docID = d.docID ";
	query << getLocationsSQLForInnerJoin(locations);
	query << "inner join ";
    query << "  Sources s ";
    query << "on d.sourceid = s.sourceid ";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, 0);
    query << "order by score desc, brandyID asc";
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNamesInEventArgs", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

// Gets all documents in which one of the provided names participated in one of the provided relations
// Relations are specified as a vector of TYPE.Subtype strings
bool DBQuery::getDocumentsForNamesInRelations(ValueVector* names, ValueVector* relations, ValueVector* sources, ValueVector* dates, ValueVector* ingestDates, ValueVector* locations, int max_results, ResultSet* results) {
    //TimedSection ts("DBQuery::getDocumentsForNamesInRelations");
    /* Creates a query of the form:
    select distinct top 100 d.brandyID, ec1.mentions_mention_cnt_ratio as score from
    (select distinct docID, entityID from Entity_mentions where head_norm in ('spencer abraham', 'spencer avraham')) as em1,
    Entity_counts ec1, Relations r, Documents d, Sources s where
    d.docID = em1.docID and d.docID = ec1.docID and em1.docID = r.docID and
    em1.entityID = ec1.entityID and d.sourceid = s.sourceid and
    (
    (r.entityID_left = em1.entityID OR
    r.entityID_right = em1.entityID) AND
    ((r.relation_type = 'PHYS' AND r.relation_subtype = 'Located') OR
    (r.relation_type = 'ORG-AFF' AND r.relation_subtype ='Membership'))
    )
    AND CONTAINS(source_text, N'"Baghdad"')
    AND s.CODE IN ('AFE', 'APE')
    AND d.DOCDATE BETWEEN '1 January 2003' AND '27 January 2004'
    order by score desc, brandyID asc
    */		
    wstringstream query;
    if ((!names) || (names->getNValues() == 0)) { return false; }
    if ((!relations) || (relations->getNValues() == 0)) { return false; }
    if (!results) { return false; }

    // Set up our query
    query << "select distinct top " << max_results << " d.brandyID, ec1.mentions_mention_cnt_ratio as score from ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";

    // Add in our equivalent names
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << ")) as em1, ";
    query << "Entity_counts ec1, Relations r, Documents d, Sources s where ";
    query << "d.docID = em1.docID and d.docID = ec1.docID and em1.docID = r.docID and ";
    query << "em1.entityID = ec1.entityID and d.sourceid = s.sourceid and ";
    query << "(";
    query << "    (r.entityID_left = em1.entityID OR ";
    query << "     r.entityID_right = em1.entityID) AND ";
    query << "    (";

    // Loop over our relations and split them into type and subtype		
    for (size_t i = 0; i < relations->getNValues(); i++) {
        wstring relation = relations->getValueForSQL(i);
        size_t pos = relation.find(L'.');
        if (pos <= 0) { 
            SessionLogger::info("BRANDY") << "Could not parse relation " << relation << "\n"; 
            return false;
        }
        wstring type = relation.substr(0, pos);
        wstring subtype = relation.substr(pos+1);

        // Add the type and subtype to our query
        if (i > 0) { query << " OR "; }
        query << "(r.relation_type = '" << type << "' AND r.relation_subtype = '" << subtype << "')";
    }
    query << "    )";
    query << ")";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by score desc, brandyID asc";

    // Get our results
    vector<string> column_names;
    vector<int> column_types;

    // add column name 
    column_names.push_back("brandyID");

    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    SessionLogger::info("BRANDY") << L"Query:\n" << query.str().c_str() << L"\n";  // DEBUGGING - REMOVE ME
    printQuery(L"getDocumentsForNamesInRelations", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

// Gets all documents in which the entity is in the second slot of a relation
// in relationsMain, and the first slot of the relation is the first slot of a 
// relation in relationsSecondary
// Relations are specified as a vector of TYPE.Subtype strings
bool DBQuery::getDocumentsForNamesInTwoTieredRelation(ValueVector* names, ValueVector* relationsMain, ValueVector* relationsSecondary, ValueVector* sources, ValueVector* dates, ValueVector* ingestDates, ValueVector* locations, int max_results, ResultSet* results) {
    //TimedSection ts("DBQuery::getDocumentsForNamesInRelations");
    /* Creates a query of the form:
	select distinct top 100 d.brandyID, ec1.mentions_mention_cnt_ratio as score from
    (select distinct docID, entityID from Entity_mentions where head_norm in ('Microsoft', 'Microsoft Inc.')) as em1,
    Entity_counts ec1, Relations r1, Relations r2, Documents d, Sources s where
    d.docID = em1.docID and d.docID = ec1.docID and em1.docID = r1.docID and
    em1.entityID = ec1.entityID and d.sourceid = s.sourceid and r1.docID = r2.docID and
	r1.entityID_right = em1.entityID AND r1.entityID_left = r2.entityID_left AND
    ((r1.relation_type = 'ORG-AFF' AND r1.relation_subtype = 'Employment') OR
    (r1.relation_type = 'ORG-AFF' AND r1.relation_subtype ='Membership'))
	and 
	((r2.relation_type = 'PHYS' AND r2.relation_subtype = 'Located'))
    AND CONTAINS(source_text, N'"Baghdad"')
    AND s.CODE IN ('AFE', 'APE')
    AND d.DOCDATE BETWEEN '1 January 2003' AND '27 January 2004'
    order by score desc, brandyID asc
    */		
    wstringstream query;
    if ((!names) || (names->getNValues() == 0)) { return false; }
    if ((!relationsMain) || (relationsMain->getNValues() == 0)) { return false; }
	if ((!relationsSecondary) || (relationsSecondary->getNValues() == 0)) { return false; }
    if (!results) { return false; }

    // Set up our query
    query << "select distinct top " << max_results << " d.brandyID, ec1.mentions_mention_cnt_ratio as score from ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";

    // Add in our equivalent names
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << ")) as em1, ";
    query << "Entity_counts ec1, Relations r1, Relations r2, Documents d, Sources s where ";
    query << "d.docID = em1.docID and d.docID = ec1.docID and em1.docID = r1.docID and ";
    query << "em1.entityID = ec1.entityID and d.sourceid = s.sourceid and ";
    query << "r1.docID = r2.docID and r1.entityID_right = em1.entityID and ";
	query << "r1.entityID_left = r2.entityID_left AND ";
    query << "    (";

    // Loop over our relationsMain and split them into type and subtype		
    for (size_t i = 0; i < relationsMain->getNValues(); i++) {
        wstring relation = relationsMain->getValueForSQL(i);
        size_t pos = relation.find(L'.');
        if (pos <= 0) { 
            SessionLogger::info("BRANDY") << "Could not parse relation " << relation << "\n"; 
            return false;
        }
        wstring type = relation.substr(0, pos);
        wstring subtype = relation.substr(pos+1);

        // Add the type and subtype to our query
        if (i > 0) { query << " OR "; }
        query << "(r1.relation_type = '" << type << "' AND r1.relation_subtype = '" << subtype << "')";
    }
    query << "    ) AND ";
	query << "    (";

    // Loop over our relationsSecondary and split them into type and subtype		
    for (size_t i = 0; i < relationsSecondary->getNValues(); i++) {
        wstring relation = relationsSecondary->getValueForSQL(i);
        size_t pos = relation.find(L'.');
        if (pos <= 0) { 
            SessionLogger::info("BRANDY") << "Could not parse relation " << relation << "\n"; 
            return false;
        }
        wstring type = relation.substr(0, pos);
        wstring subtype = relation.substr(pos+1);

        // Add the type and subtype to our query
        if (i > 0) { query << " OR "; }
        query << "(r2.relation_type = '" << type << "' AND r2.relation_subtype = '" << subtype << "')";
    }

    query << ")";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by score desc, brandyID asc";

    // Get our results
    vector<string> column_names;
    vector<int> column_types;

    // add column name 
    column_names.push_back("brandyID");

    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    SessionLogger::info("BRANDY") << L"Query:\n" << query.str().c_str() << L"\n";  // DEBUGGING - REMOVE ME
    printQuery(L"getDocumentsForNamesInTwoTieredRelation", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

// Gets all documents in which the entity is in the second slot of a relation
// in relationsMain, and the first slot of the relation fills a role 
// in eventsSecondary
// Relations and events are specified as a vector of TYPE.Subtype strings
bool DBQuery::getDocumentsForNamesInRelationEventIntersection(ValueVector* names, ValueVector* relationsMain, ValueVector* eventsSecondary, ValueVector* sources, ValueVector* dates, ValueVector* ingestDates, ValueVector* locations, int max_results, ResultSet* results) {
    //TimedSection ts("DBQuery::getDocumentsForNamesInRelations");
    /* Creates a query of the form:
	select distinct top 100 d.brandyID, ec1.mentions_mention_cnt_ratio as score from
    (select distinct docID, entityID from Entity_mentions where head_norm in ('Microsoft', 'Microsoft Inc.')) as em1,
    Entity_counts ec1, Relations r1, Events v, Event_Arguments va, Documents d, Sources s where
    d.docID = em1.docID and d.docID = ec1.docID and d.docID = r1.docID and
    d.docID = va.docID and d.docID = v.docID and em1.entityID = ec1.entityID and d.sourceid = s.sourceid and 
    r1.entityID_right = em1.entityID and and va.entityID = r1.entityID_left and va.eventID = v.eventID and 
	em1.entityID = ec1.entityID and d.sourceid = s.sourceid AND
    ((r1.relation_type = 'ORG-AFF' AND r1.relation_subtype = 'Employment') OR
    (r1.relation_type = 'ORG-AFF' AND r1.relation_subtype ='Membership'))
	and 
	((v.event_type = 'Movement' AND v.event_subtype = 'Transport'))
    AND CONTAINS(source_text, N'"Baghdad"')
    AND s.CODE IN ('AFE', 'APE')
    AND d.DOCDATE BETWEEN '1 January 2003' AND '27 January 2004'
    order by score desc, brandyID asc
    */		
    wstringstream query;
    if ((!names) || (names->getNValues() == 0)) { return false; }
    if ((!relationsMain) || (relationsMain->getNValues() == 0)) { return false; }
	if ((!eventsSecondary) || (eventsSecondary->getNValues() == 0)) { return false; }
    if (!results) { return false; }

    // Set up our query
    query << "select distinct top " << max_results << " d.brandyID, ec1.mentions_mention_cnt_ratio as score from ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";

    // Add in our equivalent names
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << ")) as em1, ";
    query << "Entity_counts ec1, Relations r1, Events v, Event_Arguments va, Documents d, Sources s where ";
    query << "d.docID = em1.docID and d.docID = ec1.docID and d.docID = r1.docID and ";
	query << "d.docID = v.docID and d.docID = va.docID and ";
	query << "r1.entityID_right = em1.entityID and va.entityID = r1.entityID_left and ";
	query << "va.eventID = v.eventID and ";
    query << "em1.entityID = ec1.entityID and d.sourceid = s.sourceid AND ";
    query << "    (";

    // Loop over our relationsMain and split them into type and subtype		
    for (size_t i = 0; i < relationsMain->getNValues(); i++) {
        wstring relation = relationsMain->getValueForSQL(i);
        size_t pos = relation.find(L'.');
        if (pos <= 0) { 
            SessionLogger::info("BRANDY") << "Could not parse relation " << relation << "\n"; 
            return false;
        }
        wstring type = relation.substr(0, pos);
        wstring subtype = relation.substr(pos+1);

        // Add the type and subtype to our query
        if (i > 0) { query << " OR "; }
        query << "(r1.relation_type = '" << type << "' AND r1.relation_subtype = '" << subtype << "')";
    }
    query << "    ) AND ";
	query << "    (";

    // Loop over our relationsSecondary and split them into type and subtype		
    for (size_t i = 0; i < eventsSecondary->getNValues(); i++) {
        wstring ev = eventsSecondary->getValueForSQL(i);
        size_t pos = ev.find(L'.');
        if (pos <= 0) { 
            SessionLogger::info("BRANDY") << "Could not parse event " << ev << "\n"; 
            return false;
        }
        wstring type = ev.substr(0, pos);
        wstring subtype = ev.substr(pos+1);

        // Add the type and subtype to our query
        if (i > 0) { query << " OR "; }
        query << "(v.event_type = '" << type << "' AND v.event_subtype = '" << subtype << "')";
    }

    query << ")";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by score desc, brandyID asc";

    // Get our results
    vector<string> column_names;
    vector<int> column_types;

    // add column name 
    column_names.push_back("brandyID");

    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    SessionLogger::info("BRANDY") << L"Query:\n" << query.str().c_str() << L"\n";  // DEBUGGING - REMOVE ME
    printQuery(L"getDocumentsForNamesInRelationEventIntersection", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}


void DBQuery::printQuery(const wchar_t * function_name, const wchar_t * query) const {
    std::ostringstream ostr;
    if (_verbose) {
        ostr << function_name;
        char* utf8 = UnicodeUtil::toUTF8String(query);
        ostr << utf8 << "\n";
        delete[] utf8;
    }
    SessionLogger::info("BRANDY") << ostr.str();
}

bool DBQuery::getDocumentsForNamesWithEventTypes(ValueVector * names, ValueVector * event_types, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNamesWithEventTypes");
    /* The function creates and returns results for queries of the form:
select distinct top 50000 d.brandyID, F.mentions_mention_cnt_ratio as score from
  (
    select C.docID, C.mentions_mention_cnt_ratio from 
      (
        select A.docID, ec.mentions_mention_cnt_ratio from 
        (
          select distinct em1.docID, em1.entityID from Entity_mentions em1
          where em1.head_norm in ('al qaeda','al qa eda','al qa ida','al qaeda department','al qaeda movement','al qaeda organization','al qaeda s','al qaedas','al qaiada','al qaida','al qaida organization','al qaieda','al qayda','al qeada','al quaeda','al quaida','al queda','ll qaeda','organization of the al qaeda','talqada') 
        ) A 
        inner join
          Entity_counts ec
        on A.docID = ec.docID and A.entityID = ec.entityID
      ) C
    inner join
      Events e
    on C.docID = e.docID
    where ((e.event_type = 'Justice' and e.event_subtype = 'Arrest-Jail'))
  ) F
inner join
  Documents d
on F.docID = d.docID 
inner join CONTAINSTABLE(Documents, source_text, N'"american" OR "boston" OR "chicago"') as source_hits on source_hits.[key] = d.docID
inner join 
  Sources s
on d.sourceid = s.sourceid
order by score desc, brandyID asc
    */

    wstringstream query;
    if ((names == 0) || (names->getNValues() == 0))
        return false;
    if ((event_types == 0) || (event_types->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "select distinct top " << max_results << " d.brandyID, F.mentions_mention_cnt_ratio as score from ";
    query << "  ( ";
    query << "    select C.docID, C.mentions_mention_cnt_ratio from ";
    query << "      ( ";
    query << "        select A.docID, ec.mentions_mention_cnt_ratio from ";
    query << "        ( ";
    query << "          select distinct em1.docID, em1.entityID from Entity_mentions em1 ";
    query << "          where em1.head_norm in ( ";
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << "                                 ) ";
    query << "        ) A ";
    query << "        inner join ";
    query << "          Entity_counts ec ";
    query << "        on A.docID = ec.docID and A.entityID = ec.entityID ";
    query << "      ) C ";
    query << "    inner join ";
    query << "      Events e ";
    query << "    on C.docID = e.docID ";
    query << "    where ( ";
    for( size_t i = 0; i < event_types->getNValues(); i++ ){
        if (i > 0) query << " OR ";
        wstring val = event_types->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
        query << "(e.event_type = '" << base << "' and e.event_subtype = '" << sub << "')";
    }
    query <<"           ) ";
    query << "  ) F ";
    query << "inner join ";
    query << "  Documents d ";
    query << "on F.docID = d.docID ";
	query << getLocationsSQLForInnerJoin(locations);
	query << "inner join ";
    query << "  Sources s ";
    query << "on d.sourceid = s.sourceid ";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, 0);
    query << "order by score desc, brandyID asc";

    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNamesWithEventTypes", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForNames(ValueVector * names, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNames");
    /* The function creates and returns results for queries of the form:
select distinct top 50000 d.brandyID, ec.mentions_mention_cnt_ratio as score 
from Entity_mentions em, Entity_counts ec, Documents d, Sources s, 
CONTAINSTABLE(Documents, source_text, N'"wyoming" OR "wisconsin" OR "washington"') as source_hits
where em.head_norm in (N'mitt romney',N'matt romney',N'mitt rooney',N'willard mitt romney') 
and d.docID = em.docID and d.docID = ec.docID and d.docID = source_hits.[key] and em.entityID = ec.entityID and d.sourceid = s.sourceid
AND s.CODE IN ('CLE', 'CLF', 'CLI', 'CLS', 'CLV', 'CLG', 'CLN', 'CLD', 'CLA', 'CLC', 'ACO', 'ACT', 'ACR', 'ACF', 'ACI', 'ACS', 'ACV', 'ACE', 'LKO', 'LKT', 'LKR', 'LKF', 'LKI', 'LKS', 'RSO', 'RST', 'EGO', 'HBO', 'AFE', 'APE', 'CNE', 'LAT', 'NYT', 'XIE', 'EUT') 
AND d.DOCDATE BETWEEN '20071215' AND '20080201' order by score desc, brandyID asc
    */

    wstringstream query;
    if ((names == 0) || (names->getNValues() == 0))
        return false;
    if (results == 0)
        return false;
	
	std::pair<std::wstring,std::wstring> locations_sql_pair = getLocationsSQL(locations);

    query << "select distinct top " << max_results << " d.brandyID, ec.mentions_mention_cnt_ratio as score ";
    query << "from Entity_mentions em, Entity_counts ec, Documents d, Sources s ";
	query << locations_sql_pair.first;
	query << "where em.head_norm in (";

    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }

    query << ") and d.docID = em.docID and d.docID = ec.docID ";
	query << locations_sql_pair.second;
	query << "and em.entityID = ec.entityID and d.sourceid = s.sourceid ";

    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, 0);

    query << "order by score desc, brandyID asc";


    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNames", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForNameSets(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNameSets");
    /* The function creates and returns results for queries of the following form.  Note that SQL's execution plan was crap, which is why we have the more explicit SQL statement below.
    select distinct top 50000 d.brandyID, E.score from 
      (
        select B.docID, (B.mentions_mention_cnt_ratio + C.mentions_mention_cnt_ratio) as score from 
          (
            select A.docID, A.entityID, ec.mentions_mention_cnt_ratio from 
            (
              select distinct em1.docID, em1.entityID from Entity_mentions em1
              where em1.head_norm in ('george bush','bush george','bush jr','bush junior','george baush','george bush junior','george bush s','george h bush','george h w bush','george h walker bush','george p bush','george w','george w bush','george w bush s','george walker bush','george woukr bush','geroge w bush','gorge w bush','jeorge w bush','v george w bush','walker bush') 
            ) A
            inner join
              Entity_counts ec
            on A.docID = ec.docID and A.entityID = ec.entityID
          ) B
        inner join
          (
            select A.docID, A.entityID, ec.mentions_mention_cnt_ratio from 
            (
              select distinct em2.docID, em2.entityID from Entity_mentions em2
              where em2.head_norm in ('united nations','a united nations','committee of the international committee of the united nations','committee of the united nations','committee on the united nations','department of the united nations','group united nations','house of the united nations','international committee of the united nations','organization of the united nations','the united nations','un','un','un commission','un corp','un international organization','un organization','united natiions','united nation','united nations agency','united nations and organization','united nations commission','united nations commission on international','united nations committee','united nations department','united nations house','united nations international','united nations international school','united nations organization','united nations school','united nations university','university of the united nations') 
            ) A
            inner join
              Entity_counts ec
            on A.docID = ec.docID and A.entityID = ec.entityID
          ) C
        on B.docID = C.docID
      ) E
    inner join 
      Documents d
    on E.docID = d.docID
	inner join 
	  Sources s
	on d.docID = s.docid
    order by score desc, brandyID asc
    */

    wstringstream query;
    if ((name_set_1 == 0) || (name_set_1->getNValues() == 0) || (name_set_2 == 0) || (name_set_2->getNValues() == 0) )
        return false;
    if (results == 0)
        return false;

    query << "select distinct top " << max_results << " d.brandyID, E.score from ";
    query << "  ( ";
    query << "    select B.docID, (B.mentions_mention_cnt_ratio + C.mentions_mention_cnt_ratio) as score from ";
    query << "      ( ";
    query << "        select A.docID, A.entityID, ec.mentions_mention_cnt_ratio from ";
    query << "        ( ";
    query << "          select distinct em1.docID, em1.entityID from Entity_mentions em1 ";
    query << "          where em1.head_norm in ( ";
    for (size_t i = 0; i < name_set_1->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_1->getValueForSQL(i) << "'";
    }
    query << "                                 ) ";
    query << "        ) A ";
    query << "        inner join ";
    query << "          Entity_counts ec ";
    query << "        on A.docID = ec.docID and A.entityID = ec.entityID ";
    query << "      ) B ";
    query << "    inner join ";
    query << "      ( ";
    query << "        select A.docID, A.entityID, ec.mentions_mention_cnt_ratio from ";
    query << "        ( ";
    query << "          select distinct em2.docID, em2.entityID from Entity_mentions em2 ";
    query << "          where em2.head_norm in ( ";
    for (size_t i = 0; i < name_set_2->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_2->getValueForSQL(i) << "'";
    }    
    query << "                                 ) ";
    query << "        ) A ";
    query << "        inner join ";
    query << "          Entity_counts ec ";
    query << "        on A.docID = ec.docID and A.entityID = ec.entityID ";
    query << "      ) C ";
    query << "    on B.docID = C.docID ";
    query << "  ) E ";
    query << "inner join ";
    query << "  Documents d ";
    query << "on E.docID = d.docID ";
	query << "inner join ";
    query << "  Sources s ";
    query << "on d.sourceid = s.sourceid ";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by score desc, brandyID asc";
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNameSets", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getEnglishSources(ResultSet * results)
{
	wstringstream query; 
	query << "SELECT DISTINCT TOP " << MAX_SOURCES << " code FROM Sources where language = \'English\'";
	vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("code");
    // add column data type
    column_types.push_back(CHAR_TYPE);

	printQuery(L"getEnglishSources", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForNameAndPatterns(ValueVector * name_set, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNameAndPatterns");
/* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 100 D.brandyid, C.mentions_mention_cnt_ratio AS SCORE 
    FROM DOCUMENTS D, ENTITY_MENTIONS M, ENTITY_COUNTS C WHERE 
    D.docid = C.docid AND C.entityid = M.entityid AND D.docid = M.docid AND 
    M.head_norm IN ('Phillip Morris') AND
    CONTAINS(D.source_text, '
    "aka" OR "changed his name to" OR "changed her name to" OR
    "changed its name to" OR
    "changed their name to" OR
    "known as" OR
    " nee " OR
    "referred to as" OR
    "maiden name" OR
    "goes by" OR
    "alias" OR
    "spelled as" OR
    "misspelled as" OR
    "new name" OR
    "old name" OR
    "name change" OR
    "name changed" OR "also called"')
    ORDER BY SCORE DESC, BRANDYID ASC
    */

    wstringstream query;
    if ((name_set == 0) || (name_set->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "SELECT DISTINCT TOP " << max_results << " D.brandyID, C.mentions_mention_cnt_ratio AS SCORE FROM ";
    query << "DOCUMENTS D, SOURCES S, ENTITY_MENTIONS M, ENTITY_COUNTS C WHERE ";
    query << "D.docid = C.docid AND C.entityid = M.entityid AND D.docid = M.docid AND D.sourceid = S.sourceid AND ";
    query << "CONTAINS(D.source_text, '" << getTemplate18Patterns().c_str() << "') AND ";
    query << "M.head_norm IN (";
    for (size_t i = 0; i < name_set->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set->getValueForSQL(i) << "'";
    }
    query << ") " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);

    query << "order by score desc, brandyID asc";
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNameAndPatterns", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getRelationsForNameSets(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * relations, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getRelationsForNameSets");
    /* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 1000 A.DOC_ID, C.EXTENT_START, C.EXTENT_END,
    B.RELATION_TYPE + '.' + B.RELATION_SUBTYPE AS RELATION_TYPE, B.RELATION_ID, 
    A.CANONICAL_NAME AS NAME_1,	A.ENTITY_TYPE AS TYPE_1, 
    ROLE_1 = CASE WHEN A.ENTITY_ID = B.ENTITY_ID_1 THEN 'Arg-1' ELSE 'Arg-2' END,
    D.CANONICAL_NAME AS NAME_2, D.ENTITY_TYPE AS TYPE_2,
    ROLE_2 = CASE WHEN D.ENTITY_ID = B.ENTITY_ID_1 THEN 'Arg-1' ELSE 'Arg-2' END
    FROM ENTITIES A, ENTITIES D, RELATIONS B, RELATION_MENTIONS C, DOCUMENTS 
    WHERE A.DOC_ID = B.DOC_ID AND A.DOC_ID = D.DOC_ID
    AND A.DOC_ID = DOCUMENTS.DOC_ID AND B.DOC_ID = C.DOC_ID AND B.RELATION_ID = C.RELATION_ID
    AND ((A.ENTITY_ID = B.ENTITY_ID_1 AND D.ENTITY_ID = B.ENTITY_ID_2) OR 
    (A.ENTITY_ID = B.ENTITY_ID_2 AND D.ENTITY_ID = B.ENTITY_ID_1)) 
    AND EXISTS (SELECT TOP 1 mention_type FROM ENTITY_MENTIONS F 
    WHERE F.DOC_ID = A.DOC_ID AND F.ENTITY_ID = A.ENTITY_ID AND F.MENTION_TYPE = 'NAM') 
    AND EXISTS (SELECT TOP 1 mention_type FROM ENTITY_MENTIONS E 
    WHERE E.DOC_ID = D.DOC_ID AND E.ENTITY_ID = D.ENTITY_ID AND E.MENTION_TYPE = 'NAM') 
    AND B.RELATION_TYPE + '.' + B.RELATION_SUBTYPE IN ('PER-SOC.BUSINESS', 'PHYS.LOCATED')
    AND A.CANONICAL_NAME IN ('bill clinton', 'clinton')
    AND D.CANONICAL_NAME IN ('al gore', 'gore', 'bush')
    AND CONTAINS(source_text, '"Iraq"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '1 January 2000' AND '27 January 2004'
    ORDER BY RELATION_ID

    New form:

    select distinct top 1000 d.brandyID, rm.extent_start, rm.extent_end, 
    (r.relation_type + '.' + r.relation_subtype) as relation_type, r.brandyRelationID,
    ent1.canonical_name as name_1, ent1.entity_type as type_1,
    role_1 = case when ent1.entityID = r.entityID_left then 'Arg-1' else 'Arg-2' end,
    ent2.canonical_name as name_2, ent2.entity_type as type_2,
    role_2 = case when ent2.entityID = r.entityID_left then 'Arg-1' else 'Arg-2' end
    from Entities ent1, Entities ent2, Relations r, Relation_mentions rm, Documents d, Sources s where
    r.docID = ent1.docID and r.docID = ent2.docID and r.docID = rm.docID and r.docID = d.docID and
	d.sourceid = s.sourceid and 
    (
    (ent1.entityID = r.entityID_left and ent2.entityID = r.entityID_right) OR
    (ent2.entityID = r.entityID_left and ent1.entityID = r.entityID_right)
    ) and
    (
    (r.relation_type = 'PER-SOC' and r.relation_subtype = 'BUSINESS') OR
    (r.relation_type = 'PHYS'	and r.relation_subtype = 'LOCATED')
    ) and
    ent1.canonical_name in ('bill clinton', 'clinton') and
    ent2.canonical_name in ('al gore','gore')
    order by relationID


    */

    wstringstream query;
    if ((name_set_1 == 0) || (name_set_1->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "select distinct top " << max_results << " d.brandyID, rm.extent_start, rm.extent_end, ";
    query << "(r.relation_type + '.' + r.relation_subtype) as relation_type, r.brandyRelationID, ";
    query << "ent1.canonical_name as name_1, ent1.entity_type as type_1, ";
    query << "role_1 = case when ent1.entityID = r.entityID_left then 'Arg-1' else 'Arg-2' end, ";
    query << "ent2.canonical_name as name_2, ent2.entity_type as type_2, ";
    query << "role_2 = case when ent2.entityID = r.entityID_left then 'Arg-1' else 'Arg-2' end ";
    query << "from Entities ent1, Entities ent2, Relations r, Relation_mentions rm, Documents d, Sources s where ";
    query << "r.docID = ent1.docID and r.docID = ent2.docID and r.docID = rm.docID and r.docID = d.docID and ";
	query << "d.sourceid = s.sourceid and ";
    query << "( ";
    query << "	(ent1.entityID = r.entityID_left and ent2.entityID = r.entityID_right) OR ";
    query << "	(ent2.entityID = r.entityID_left and ent1.entityID = r.entityID_right) ";
    query << ") and ent1.canonical_name in (";	
    for (size_t i = 0; i < name_set_1->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_1->getValueForSQL(i) << "'";
    }
    query << ") and ent2.canonical_name in (";
    for (size_t i = 0; i < name_set_2->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_2->getValueForSQL(i) << "'";
    }
    query << ") and ( ";
    for( size_t i = 0; i < relations->getNValues(); i++ ){
        if (i > 0) query << " OR ";

        wstring val = relations->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
        query << "(r.relation_type = '" << base << "' and r.relation_subtype = '" << sub << "')";
    }
    query << ") " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);

    query << " order by brandyRelationID";


    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("extent_start");
    column_types.push_back(INT_TYPE);
    column_names.push_back("extent_end");
    column_types.push_back(INT_TYPE);
    column_names.push_back("relation_type");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("brandyRelationID");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("name_1");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("type_1");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("role_1");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("name_2");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("type_2");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("role_2");
    column_types.push_back(CHAR_TYPE);
    printQuery(L"getRelationsForNameSets", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}



bool DBQuery::getRelationsForNameSetAndTypeSet(ValueVector * name_set_1, ValueVector * type_set, ValueVector * role_1_relations, ValueVector * role_2_relations,
                                               ValueVector * role_any_relations, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getRelationsForNameSetAndTypeSet");
    wstringstream query;
    if ((name_set_1 == 0) || (name_set_1->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "select top " << max_results << " d.brandyID, rm.extent_start, rm.extent_end, ";
    query << "(r.relation_type + '.' + r.relation_subtype) as relation_type, r.brandyRelationID, ";
    query << "ent1.canonical_name as name_1, ent1.entity_type as type_1, ";
    query << "role_1 = case when ent1.entityID = r.entityID_left then 'Arg-1' else 'Arg-2' end, ";
    query << "ent2.canonical_name as name_2, ent2.entity_type as type_2, ";
    query << "role_2 = case when ent2.entityID = r.entityID_left then 'Arg-1' else 'Arg-2' end ";
    query << "from Entities ent1, Entities ent2, Entity_counts ec1, Entity_counts ec2, Relations r, Relation_mentions rm, Documents d, Sources s where ";
    query << "r.docID = ent1.docID and r.docID = ent2.docID and r.docID = rm.docID and r.docID = d.docID and r.relationID = rm.relationID and ";
    query << "r.docID = ec1.docID and r.docID = ec2.docID and ent1.entityID = ec1.entityID and ent2.entityID = ec2.entityID and d.sourceid = s.sourceid and ";

    query << "( ";

    bool role_1 = role_1_relations->getNValues() > 0;
    bool role_2 = role_2_relations->getNValues() > 0;
    bool role_a = role_any_relations->getNValues() > 0;

    if( role_a ){
        query << "( (";

        query << "	(ent1.entityID = r.entityID_left and ent2.entityID = r.entityID_right) or ";
        query << "	(ent2.entityID = r.entityID_left and ent1.entityID = r.entityID_right) ";

        query << ") and ( ";
        for( size_t i = 0; i < role_any_relations->getNValues(); i++ ){
            if (i > 0) query << " OR ";

            wstring val = role_any_relations->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
            query << "(r.relation_type = '" << base << "' and r.relation_subtype = '" << sub << "')";
        }
        query << " ) ) " << ((role_1 || role_2) ? "OR " : "") ;
    }

    if( role_1 ){
        query << "( (ent1.entityID = r.entityID_left and ent2.entityID = r.entityID_right) ";
        query << "and ( ";
        for( size_t i = 0; i < role_1_relations->getNValues(); i++ ){
            if (i > 0) query << " OR ";

            wstring val = role_1_relations->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
            query << "(r.relation_type = '" << base << "' and r.relation_subtype = '" << sub << "')";
        }
        query << " ) ) " << ( role_2 ? "OR " : "" );
    }

    if( role_2 ){
        query << "( (ent2.entityID = r.entityID_left and ent1.entityID = r.entityID_right) ";
        query << "and ( ";
        for( size_t i = 0; i < role_2_relations->getNValues(); i++ ){
            if (i > 0) query << " OR ";

            wstring val = role_2_relations->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
            query << "(r.relation_type = '" << base << "' and r.relation_subtype = '" << sub << "')";
        }
        query << " ) ) ";
    }

    query << ") and ent1.canonical_name in (";	
    for (size_t i = 0; i < name_set_1->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_1->getValueForSQL(i) << "'";
    }
    query << ") and ent2.entity_type in (";
    for (size_t i = 0; i < type_set->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << type_set->getValueForSQL(i) << "'";
    }
    // filter out unnamed entities
    query << ") and ent2.canonical_name != " << _sqlQueryPhrasePrefix << "' " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);

    query << " order by (ec1.mentions_mention_cnt_ratio + ec2.mentions_mention_cnt_ratio) desc";

    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("extent_start");
    column_types.push_back(INT_TYPE);
    column_names.push_back("extent_end");
    column_types.push_back(INT_TYPE);
    column_names.push_back("relation_type");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("brandyRelationID");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("name_1");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("type_1");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("role_1");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("name_2");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("type_2");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("role_2");
    column_types.push_back(CHAR_TYPE);
    printQuery(L"getRelationsForNameSetAndTypeSet", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}



bool DBQuery::getEventsForNames(ValueVector * names, ValueVector * events, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getEventsForNames");
    /* The function creates and returns results for queries of the form:
select d.brandyID, I.extent_start, I.extent_end, I.event_type, I.brandyEventID, I.role, I.entity_name, I.entity_type from 
  (
    select H.docID, H.eventID, H.eventMentID, H.extent_start, H.extent_end, H.event_type, H.brandyEventID, H.role, case when H.canonical_name != '' then H.canonical_name else em.head_norm end as entity_name, H.entity_type from 
      (
        select G.docID, G.eventID, G.eventMentID, G.entityID, G.entityMentID, ev_m.extent_start, ev_m.extent_end, G.event_type, G.brandyEventID, G.role, G.canonical_name, G.entity_type from 
          (
            select F.docID, F.eventID, F.eventMentID, F.entityID, F.entityMentID, F.event_type, F.brandyEventID, F.role, ent2.canonical_name, ent2.entity_type from 
              (
	            select C.docID, C.eventID, C.eventMentID, ea2.entityMentID, ea2.entityID, C.event_type, C.brandyEventID, ea2.role from 
                  (    
                    select B.docID, B.entityID, B.eventID, ea.eventMentID, B.event_type, B.brandyEventID from 
                      (
                        select A.docID, A.entityID, e.eventID, e.event_type + '.' + e.event_subtype as event_type, e.brandyEventID from 
                          (
                            select docID, entityID, entity_type, canonical_name from Entities s_ent 
                            where s_ent.canonical_name in ('gordon brown')
                          ) A
                        inner join
                          Events e
                        on A.docID = E.docID 
                        and ((e.event_type = 'Contact' and e.event_subtype = 'Meet')) 
                      ) B
                    inner join
                      Event_arguments ea
                    on ea.docID = B.docID and ea.eventID = B.eventID and ea.entityID = B.entityID
                  ) as C
                inner join
                  Event_arguments ea2
                on ea2.docID = C.docID and ea2.eventID = C.eventID and ea2.eventMentID = C.eventMentID
              ) as F
            inner join Entities ent2
            on ent2.docID = F.docID and ent2.entityID = F.entityID
          ) as G
        inner join 
          Event_mentions ev_m
        on ev_m.docID = G.docID and ev_m.eventID = G.eventID and ev_m.mentionID = G.eventMentID
      ) as H
    inner join
      Entity_mentions em
    on em.docID = H.docID and em.entityID = H.entityID and em.mentionID = H.entityMentID
  ) as I
inner join
  Documents d
on d.docID = I.docID
inner join
  Sources s
on d.docID = s.docID
AND s.CODE IN ('TAA', 'AFE', 'TAC', 'TBE', 'TBC', 'TBD', 'TAD', 'APE', 'APW', 'TAE', 'TAF', 'CNE', 'CNN', 'CNT', 'TAG', 'TAH', 'TBF', 'TAI', 'TAJ', 'TAK', 'TAL', 'TAM', 'TAN', 'TAO', 'TAP', 'TAQ', 'TAY', 'TAR', 'TBH', 'TAS', 'TBG', 'LAT', 'TAT', 'MNB', 'MSN', 'MST', 'TBA', 'NBC', 'NYT', 'TAU', 'TAV', 'TAW', 'TAX', 'TAZ', 'TBB', 'UME', 'TAB', 'XIE', 'AFC', 'AHN') 
order by d.docID, I.eventID, I.eventMentID 
    */

    wstringstream query;
    if ((names == 0) || (names->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    // Build our query
    query << "select d.brandyID, I.extent_start, I.extent_end, I.event_type, I.brandyEventID, I.role, I.entity_name, I.entity_type from ";
    query << "  ( ";
    query << "    select H.docID, H.eventID, H.eventMentID, H.extent_start, H.extent_end, H.event_type, H.brandyEventID, H.role, case when H.canonical_name != '' then H.canonical_name else em.head_norm end as entity_name, H.entity_type from ";
    query << "      ( ";
    query << "        select G.docID, G.eventID, G.eventMentID, G.entityID, G.entityMentID, ev_m.extent_start, ev_m.extent_end, G.event_type, G.brandyEventID, G.role, G.canonical_name, G.entity_type from ";
    query << "          ( ";
    query << "            select F.docID, F.eventID, F.eventMentID, F.entityID, F.entityMentID, F.event_type, F.brandyEventID, F.role, ent2.canonical_name, ent2.entity_type from ";
    query << "              ( ";
    query << "	            select C.docID, C.eventID, C.eventMentID, ea2.entityMentID, ea2.entityID, C.event_type, C.brandyEventID, ea2.role from ";
    query << "                  ( ";
    query << "                    select B.docID, B.entityID, B.eventID, ea.eventMentID, B.event_type, B.brandyEventID from ";
    query << "                      ( ";
    query << "                        select A.docID, A.entityID, e.eventID, e.event_type + '.' + e.event_subtype as event_type, e.brandyEventID from ";
    query << "                          ( ";
    query << "                            select docID, entityID, entity_type, canonical_name from Entities s_ent ";
    query << "                            where s_ent.canonical_name in ( ";
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << "                                                          ) ";
    query << "                          ) A ";
    query << "                        inner join ";
    query << "                          Events e ";
    query << "                        on A.docID = E.docID ";
    query << "                        and ( ";
    for( size_t i = 0; i < events->getNValues(); i++ ){
        if (i > 0) query << " or ";
        wstring val = events->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
        query << "(e.event_type = '" << base << "' and e.event_subtype = '" << sub << "')";
    }
    query << "                            ) "; 
    query << "                      ) B ";
    query << "                    inner join ";
    query << "                      Event_arguments ea ";
    query << "                    on ea.docID = B.docID and ea.eventID = B.eventID and ea.entityID = B.entityID ";
    query << "                  ) as C ";
    query << "                inner join ";
    query << "                  Event_arguments ea2 ";
    query << "                on ea2.docID = C.docID and ea2.eventID = C.eventID and ea2.eventMentID = C.eventMentID ";
    query << "              ) as F ";
    query << "            inner join Entities ent2 ";
    query << "            on ent2.docID = F.docID and ent2.entityID = F.entityID ";
    query << "          ) as G ";
    query << "        inner join ";
    query << "          Event_mentions ev_m ";
    query << "        on ev_m.docID = G.docID and ev_m.eventID = G.eventID and ev_m.mentionID = G.eventMentID ";
    query << "      ) as H ";
    query << "    inner join ";
    query << "      Entity_mentions em ";
    query << "    on em.docID = H.docID and em.entityID = H.entityID and em.mentionID = H.entityMentID ";
    query << "  ) as I ";
    query << "inner join ";
    query << "  Documents d ";
    query << "on d.docID = I.docID ";
	query << "inner join ";
    query << "  Sources s ";
    query << "on d.sourceid = s.sourceid ";
    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by d.docID, I.eventID, I.eventMentID";

    // Get our results
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("extent_start");
    column_types.push_back(INT_TYPE);
    column_names.push_back("extent_end");
    column_types.push_back(INT_TYPE);
    column_names.push_back("event_type");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("brandyEventID");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("role");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("entity_name");
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("entity_type");
    column_types.push_back(CHAR_TYPE);
    printQuery(L"getEventsForNames", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForNamesAndEvents(ValueVector * names, ValueVector * events, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNamesAndEvents");
    /* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 1000 A.DOC_ID, C.MENTIONS_MENTION_CNT_RATIO AS SCORE
    FROM ENTITIES A, ENTITY_COUNTS C, DOCUMENTS, EVENTS B
    WHERE A.DOC_ID = DOCUMENTS.DOC_ID AND A.DOC_ID = C.DOC_ID AND A.ENTITY_ID = C.ENTITY_ID
    AND A.CANONICAL_NAME IN ('saddam hussein', 'putin')
    AND A.DOC_ID = B.DOC_ID AND B.EVENT_TYPE + '.' + B.EVENT_SUBTYPE IN 
    ('Movement.Transport', 'Personnel.End-Position')
    AND CONTAINS(source_text, '"Iraq"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '1 January 2000' AND '27 January 2004'
    ORDER BY SCORE DESC, BRANDYID ASC
    */

    wstringstream query;
    if ((names == 0) || (names->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    bool use_e = ( events && events->getNValues() );

    query << "select distinct top " << max_results;
    query << " d.brandyID, ec.mentions_mention_cnt_ratio as score from Entity_mentions em, Entity_counts ec, Documents d, Sources s" << (use_e ? ", Events e " : " ");
    query << "where d.docID = em.docID and d.sourceid = s.sourceid and d.docID = ec.docID and " << ( use_e ? "d.docID = e.docID and" : "" );
    query << "em.entityID = ec.entityID and em.head_norm in (";
    for (size_t i = 0; i < names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << names->getValueForSQL(i) << "'";
    }
    query << ") ";

    if( use_e ){
        query << " and (";
        for( size_t i = 0; i < events->getNValues(); i++ ){
            if (i > 0) query << " or ";

            wstring val = events->getValue(i), base = val.substr(0,val.find(L'.')), sub = val.substr( val.find(L'.')+1 );
            query << "(e.event_type = '" << base << "' and e.event_subtype = '" << sub << "')";
        }
        query << ") ";

    }

    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);

    query << "order by score desc, brandyID asc";

    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNamesAndEvents", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForKeywords(ValueVector * required, ValueVector * excluded, ValueVector * optional, const wchar_t * free_text, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForKeywords");
    /* The function creates and returns results for queries of the form:

    select top 1000 source_text, d.docID, req_docs.rank + opt_docs.rank from Documents d
    JOIN (select [KEY] as docID, [RANK] as rank from containstable( Documents, source_text, '"bush" AND "iraq"' )) AS req_docs on d.docID = req_docs.docID
    left outer join (SELECT [KEY] as docID, [RANK] as rank from containstable( Documents, source_text, '"province" OR "haliburton"')) as opt_docs on d.docID = opt_docs.docID
	inner join Sources s on d.sourceid = s.sourceid
    where not contains( source_text, '""' ) order by req_docs.rank + opt_docs.rank desc, brandyID asc

    */

    wstringstream query;
    bool has_req = required->getNValues() > 0;
    bool has_opt = optional->getNValues() > 0;
    bool has_exc = excluded->getNValues() > 0;

    wstring ranker;
    if( has_req && has_opt ) ranker = L"req_docs.rank + opt_docs.rank";
    else if( has_req ) ranker = L"req_docs.rank";
    else ranker = L"opt_docs.rank";

    // sanity checks
    if( !has_req && !has_opt ) return false;

    if (results == 0) return false;

    query << "select top " << max_results << " d.brandyID, " << ranker << " as rank from Documents d ";

    // add required keywords
    if( has_req ){
        query << "join (select [KEY] as docID, [RANK] as rank from containstable( Documents, source_text, "<< _sqlQueryPhrasePrefix <<"\"";
        for (size_t i = 0; i < required->getNValues(); i++) {
            if (i > 0) query << "\" AND \"";
            query << required->getValueForSQL(i);
        }
        query << "\"')) as req_docs on d.docID = req_docs.docID ";
    }

    // add optional keywords
    if( has_opt ){
        query << "left outer join (select [KEY] as docID, [RANK] as rank from containstable( Documents, source_text, " << _sqlQueryPhrasePrefix << "\"";
        for (size_t i = 0; i < optional->getNValues(); i++) {
            if (i > 0) query << "\" OR \"";
            query << optional->getValueForSQL(i);
        }				
        query << "\"')) as opt_docs on d.docID = opt_docs.docID ";
    }

	query << "inner join Sources s on d.sourceid = s.sourceid ";

    // add excluded keywords - this could generate a 'contains( source_text, '""' )',
    // but this won't match any documents, so no harm
    query << "where not contains( source_text, " << _sqlQueryPhrasePrefix << "\"";
    for( size_t i = 0; i < excluded->getNValues(); i++ ) {
        if (i > 0) query << "\" OR \"";
        query << excluded->getValueForSQL(i);
    }
    query << "\"' ) and " << ranker << " > 0 and d.sourceid = s.sourceid ";

    query << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by " << ranker << " desc, brandyID asc";

    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("rank");
    column_types.push_back(INT_TYPE);

    printQuery(L"getDocumentsForKeywords", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForNameSetsWithRelations(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNameSetsWithRelations");
    /* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 100 A.DOC_ID, (C.MENTIONS_MENTION_CNT_RATIO + D.MENTIONS_MENTION_CNT_RATIO) AS SCORE 
    FROM ENTITY_MENTIONS A, ENTITY_MENTIONS B, ENTITY_COUNTS C, ENTITY_COUNTS D, RELATIONS E, DOCUMENTS
    WHERE A.DOC_ID = B.DOC_ID AND A.DOC_ID = E.DOC_ID AND A.DOC_ID = DOCUMENTS.DOC_ID 
    AND C.DOC_ID = D.DOC_ID AND A.ENTITY_ID = C.ENTITY_ID AND B.ENTITY_ID = D.ENTITY_ID 
    AND ((A.ENTITY_ID = E.ENTITY_ID_1 AND B.ENTITY_ID = E.ENTITY_ID_2) OR
    (A.ENTITY_ID = E.ENTITY_ID_2 AND B.ENTITY_ID = E.ENTITY_ID_1))
    AND CONTAINS(A.HEAD_NORM, '"woshington" OR "washington"')
    AND CONTAINS(B.HEAD_NORM, '"vladimir putin" OR "putin"')
    AND CONTAINS(source_text, '"Afghanistan"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '1 January 2000' AND '27 January 2001' 
    ORDER BY SCORE DESC, BRANDYID ASC
    */

    wstringstream query;
    if ((name_set_1 == 0) || (name_set_1->getNValues() == 0) || (name_set_2 == 0) || (name_set_2->getNValues() == 0) )
        return false;
    if (results == 0)
        return false;

	std::pair<std::wstring,std::wstring> locations_sql_pair = getLocationsSQL(locations);

    query << "select distinct top " << max_results << " d.brandyID, (ec1.mentions_mention_cnt_ratio + ec2.mentions_mention_cnt_ratio) as score from ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_1->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_1->getValueForSQL(i) << "'";
    }
    query << ")) as em1, ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_2->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_2->getValueForSQL(i) << "'";
    }
    query << ")) as em2, ";
    query << "Entity_counts ec1, Entity_counts ec2, Relations r, Documents d, Sources s ";
	query << locations_sql_pair.first;
	query << "where ";
    query << "d.docID = em1.docID and d.docID = em2.docID and d.docID = ec1.docID and d.docID = ec2.docID and em1.docID = r.docID ";
	query << locations_sql_pair.second;
    query << "and em1.entityID = ec1.entityID and em2.entityID = ec2.entityID and d.sourceid = s.sourceid and ";
    query << "( ";
    query << "	(r.entityID_left = em1.entityID and r.entityID_right = em2.entityID) OR ";
    query << "	(r.entityID_left = em2.entityID and r.entityID_right = em1.entityID) ";
    query << ") " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, 0);
    query << "order by score desc, brandyID asc";


    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNameSetsWithRelations", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}


bool DBQuery::getDocumentsForNameSetsWithRelationsAndNameSet(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * name_set_3, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForNameSetsWithRelationsAndNameSet");
    /* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 100 A.DOC_ID, (C.MENTIONS_MENTION_CNT_RATIO + D.MENTIONS_MENTION_CNT_RATIO) AS SCORE 
    FROM ENTITY_MENTIONS A, ENTITY_MENTIONS B, ENTITY_MENTIONS F, ENTITY_COUNTS C, ENTITY_COUNTS D, RELATIONS E, DOCUMENTS
    WHERE A.DOC_ID = B.DOC_ID AND A.DOC_ID = E.DOC_ID AND A.DOC_ID = DOCUMENTS.DOC_ID 
    AND C.DOC_ID = D.DOC_ID AND A.DOC_ID = F.DOC_ID AND A.ENTITY_ID = C.ENTITY_ID AND B.ENTITY_ID = D.ENTITY_ID 
    AND ((A.ENTITY_ID = E.ENTITY_ID_1 AND B.ENTITY_ID = E.ENTITY_ID_2) OR
    (A.ENTITY_ID = E.ENTITY_ID_2 AND B.ENTITY_ID = E.ENTITY_ID_1))
    AND CONTAINS(A.HEAD_NORM, '"russia" OR "russian"')
    AND CONTAINS(B.HEAD_NORM, '"vladimir putin" OR "putin"')
    AND CONTAINS(F.HEAD_NORM, 'clinton')
    AND CONTAINS(source_text, '"Afghanistan"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '1 January 2000' AND '27 January 2001' 
    ORDER BY SCORE DESC, BRANDYID ASC
    */

    wstringstream query;
    if ((name_set_1 == 0) || (name_set_1->getNValues() == 0) 
        || (name_set_2 == 0) || (name_set_2->getNValues() == 0)
        || (name_set_3 == 0) || (name_set_3->getNValues() == 0))
        return false;
    if (results == 0)
        return false;


    query << "select distinct top " << max_results << " d.brandyID, (ec1.mentions_mention_cnt_ratio + ec2.mentions_mention_cnt_ratio) as score from ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_1->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_1->getValueForSQL(i) << "'";
    }
    query << ")) as em1, ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_2->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_2->getValueForSQL(i) << "'";
    }
    query << ")) as em2, ";
    query << "(select distinct docID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_3->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_3->getValueForSQL(i) << "'";
    }
    query << ")) as em3, ";
    query << "Entity_counts ec1, Entity_counts ec2, Relations r, Documents d, Sources s where ";
    query << "d.docID = em1.docID and d.docID = em2.docID and d.docID = em3.docID and d.docID = ec1.docID and d.docID = ec2.docID and em1.docID = r.docID and ";
    query << "em1.entityID = ec1.entityID and em2.entityID = ec2.entityID and d.sourceid = s.sourceid and ";
    query << "( ";
    query << "	(r.entityID_left = em1.entityID and r.entityID_right = em2.entityID) OR ";
    query << "	(r.entityID_left = em2.entityID and r.entityID_right = em1.entityID) ";
    query << ") " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << "order by score desc, brandyID asc";


    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForNameSetsWithRelationsAndNameSet", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}


bool DBQuery::getDocumentsForTwoRelations(ValueVector * name_set_1, ValueVector * name_set_2, ValueVector * name_set_3, ValueVector * name_set_4, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForTwoRelations");
    /* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 100 A.DOC_ID, (C.MENTIONS_MENTION_CNT_RATIO + D.MENTIONS_MENTION_CNT_RATIO) AS SCORE 
    FROM ENTITY_MENTIONS A, ENTITY_MENTIONS B, ENTITY_MENTIONS F, ENTITY_MENTIONS G, ENTITY_COUNTS C, ENTITY_COUNTS D, RELATIONS E, RELATIONS H, DOCUMENTS
    WHERE A.DOC_ID = B.DOC_ID AND A.DOC_ID = E.DOC_ID AND A.DOC_ID = DOCUMENTS.DOC_ID 
    AND C.DOC_ID = D.DOC_ID AND A.DOC_ID = F.DOC_ID AND A.DOC_ID = H.DOC_ID AND A.DOC_ID = G.DOC_ID 
    AND A.ENTITY_ID = C.ENTITY_ID AND B.ENTITY_ID = D.ENTITY_ID 
    AND ((A.ENTITY_ID = E.ENTITY_ID_1 AND B.ENTITY_ID = E.ENTITY_ID_2) OR
    (A.ENTITY_ID = E.ENTITY_ID_2 AND B.ENTITY_ID = E.ENTITY_ID_1))
    AND ((F.ENTITY_ID = H.ENTITY_ID_1 AND G.ENTITY_ID = H.ENTITY_ID_2) OR
    (F.ENTITY_ID = H.ENTITY_ID_2 AND G.ENTITY_ID = H.ENTITY_ID_1))
    AND CONTAINS(A.HEAD_NORM, '"russia" OR "russian"')
    AND CONTAINS(B.HEAD_NORM, '"vladimir putin" OR "putin"')
    AND CONTAINS(F.HEAD_NORM, '"clinton"')
    AND CONTAINS(G.HEAD_NORM, '"united states" or "washington"')
    AND CONTAINS(source_text, '"Afghanistan"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '1 January 2000' AND '27 January 2001' 
    ORDER BY SCORE DESC, BRANDYID ASC
    */

    wstringstream query;
    if ((name_set_1 == 0) || (name_set_1->getNValues() == 0) 
        || (name_set_2 == 0) || (name_set_2->getNValues() == 0)
        || (name_set_3 == 0) || (name_set_3->getNValues() == 0)
        || (name_set_4 == 0) || (name_set_4->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "select distinct top " << max_results << " d.brandyID, (ec1.mentions_mention_cnt_ratio + ec2.mentions_mention_cnt_ratio) as score from ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_1->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_1->getValueForSQL(i) << "'";
    }
    query << ")) as em1, ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_2->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_2->getValueForSQL(i) << "'";
    }
    query << ")) as em2, ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_3->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_3->getValueForSQL(i) << "'";
    }
    query << ")) as em3, ";
    query << "(select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < name_set_4->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << name_set_4->getValueForSQL(i) << "'";
    }
    query << ")) as em4, ";

    query << "Entity_counts ec1, Entity_counts ec2, Relations r1, Relations r2, Documents d, Sources s ";
    query << "where d.docID = em1.docID and d.docID = em2.docID and d.docID = em3.docID and d.docID = em4.docID and ";
    query << "d.docID = ec1.docID and d.docID = ec2.docID and d.docID = r1.docID and d.docID = r2.docID and ";
    query << "em1.entityID = ec1.entityID and em2.entityID = ec2.entityID and d.sourceid = s.sourceid and ";
    query << "( ";
    query << "	(r1.entityID_left = em1.entityID and r1.entityID_right = em2.entityID) or ";
    query << "	(r1.entityID_left = em2.entityID and r1.entityID_right = em1.entityID) ";
    query << ") and ";
    query << "( ";
    query << "	(r2.entityID_left = em3.entityID and r2.entityID_right = em4.entityID) or ";
    query << "	(r2.entityID_left = em4.entityID and r2.entityID_right = em3.entityID) ";
    query << ") " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);
    query << " order by score desc, brandyID asc";


    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForTwoRelations", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getPERWithRelationToORG(ValueVector * org_names, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getPERWithRelationToORG");
    /* The function creates and returns results for queries of the form:
    SELECT DISTINCT TOP 100 F.HEAD_NORM, SUM(D.MENTIONS_MENTION_CNT_RATIO) AS SCORE
    FROM ENTITIES A, ENTITIES B, RELATIONS C, ENTITY_COUNTS D, ENTITY_MENTIONS E, ENTITY_MENTIONS F, DOCUMENTS
    WHERE A.DOC_ID = B.DOC_ID AND A.DOC_ID = C.DOC_ID AND A.DOC_ID = D.DOC_ID
    AND A.ENTITY_ID = D.ENTITY_ID AND A.DOC_ID = E.DOC_ID AND B.ENTITY_ID = E.ENTITY_ID
    AND A.DOC_ID = F.DOC_ID AND A.ENTITY_ID = F.ENTITY_ID AND F.MENTION_TYPE = 'NAM'
    AND A.DOC_ID = DOCUMENTS.DOC_ID
    AND CONTAINS(E.HEAD_NORM, '"united nations" OR "un"')
    AND B.ENTITY_TYPE = 'ORG' AND A.ENTITY_TYPE = 'PER'
    AND ((A.ENTITY_ID = C.ENTITY_ID_1 AND B.ENTITY_ID = C.ENTITY_ID_2) 
    OR (A.ENTITY_ID = C.ENTITY_ID_2 AND B.ENTITY_ID = C.ENTITY_ID_1))
    AND CONTAINS(source_text, '"Afghanistan"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '1 January 2000' AND '27 January 2001' 
    GROUP BY F.HEAD_NORM
    ORDER BY SCORE DESC
    */

    wstringstream query;
    if ((org_names == 0) || (org_names->getNValues() == 0))
        return false;
    if (results == 0)
        return false;

    query << "select distinct top " << max_results << " p_em.head_norm, sum(p_ec.mentions_mention_cnt_ratio) as score from Entities o_ent, ";
    query << "( select distinct docID, entityID from Entity_mentions where head_norm in (";
    for (size_t i = 0; i < org_names->getNValues(); i++) {
        if (i > 0) query << ",";
        query << _sqlQueryPhrasePrefix << org_names->getValueForSQL(i) << "'";
    }		
    query << ") ) as o_em, Entities p_ent, Entity_mentions p_em, Entity_counts p_ec, Relations r, Documents d, Sources s where ";
    query << "o_ent.docID = o_em.docID and o_em.docID = p_ent.docID and p_ent.docID = p_em.docID and p_em.docID = p_ec.docID and ";
    query << "o_ent.entityID = o_em.entityID and o_ent.entity_type = 'ORG' and ";
    query << "p_ent.entityID = p_em.entityID and p_em.entityID = p_ec.entityID and ";
    query << "p_ent.entity_type = 'PER' and p_em.mention_type = 'NAM' and d.sourceid = s.sourceid and ";
    query << "( ";
    query << "	(r.docID = p_ent.docID and r.entityID_left = p_ent.entityID and r.entityID_right = o_ent.entityID) or ";
    query << "	(r.docID = p_ent.docID and r.entityID_left = o_ent.entityID and r.entityID_right = p_ent.entityID) ";
    query << ") " << getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations);

    query << "group by p_em.head_norm order by score desc ";

    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("head_norm");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getPERWithRelationToORG", query.str().c_str());
    bool success = getResults(query.str(), column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForStrings(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results, bool isANDSearch) {
    //TimedSection ts("DBQuery::getDocumentsForStrings");
    /* The function creates and returns results for queries of the form:
    SELECT TOP 50000 merged.brandyID, sum(merged.RANK) as rank from 
      (
        SELECT TOP 50000 brandyID, KEY_TBL.RANK FROM DOCUMENTS d INNER JOIN CONTAINSTABLE(DOCUMENTS, SOURCE_TEXT, N'formsof(inflectional, "The Trial of Saddam Hussein")') AS KEY_TBL ON d.docID = KEY_TBL.[KEY]  
      ) as merged 
        inner join 
      Documents d on d.brandyID = merged.brandyID  
	  inner join Sources s on d.sourceid = s.sourceid
    AND s.CODE IN ('CTV', 'NTD', 'RFA', 'AHN', 'CTT', 'DFT', 'JST', 'NTT', 'PHN', 'XIN', 'MUT', 'AFC', 'CNA', 'ZBN')  AND d.DOCDATE BETWEEN '20051019' AND '20060930'  
    group by merged.brandyID order by rank desc, brandyID asc
    */
    // TODO: Fix query so we don't use "d" in three different ways, depending on scoping to keep them distinct.

    // Build our subquery for the documents table.  Note that we defer source/date/location restriction until after the join.
    wstring documentsQuery = getQueryForStrings(strings, 0, 0, 0, 0, isANDSearch, L"DOCUMENTS", L"SOURCE_TEXT", L"docID");

    // For the headline, we want to do an ANDed search of the individual tokens.  Thus we need to create a new ValueVector with one token per value.
    wstring headlinesQuery;
    if (strings->size() == 1) {
        
        // Split the string into tokens
        ValueVector tokens = splitWstringIntoTokens(strings->getValueForSQL(0));
        headlinesQuery = getQueryForStrings(&tokens, 0, 0, 0, 0, true, L"Headlines", L"headline", L"brandyID");
    } else {
        headlinesQuery = getQueryForStrings(strings, 0, 0, 0, 0, isANDSearch, L"Headlines", L"headline", L"brandyID");
    }
    wchar_t max_results_str[10];
    _itow(max_results, max_results_str, 10);
    if (documentsQuery.length() == 0) { return false; }
    wstring query = L"SELECT TOP ";
    query.append(max_results_str);
    query.append(L" merged.brandyID, sum(merged.RANK) as rank from (");
    query.append(L"SELECT TOP ");  // BEGIN Documents query
    query.append(max_results_str);
    query.append(L" brandyID, KEY_TBL.RANK ");
    query.append(documentsQuery);  // END Documents query
//    query.append(L" union all ");
//    query.append(L"SELECT TOP ");  // BEGIN Headlines query
//    query.append(max_results_str);
//    query.append(L" brandyID, KEY_TBL.RANK ");
//    query.append(headlinesQuery);  // END Headlines query
    query.append(L") as merged ");
    query.append(L"inner join Documents d on d.brandyID = merged.brandyID "); // join with the documents table so we can restrict by source/date/location
	query.append(L"inner join Sources s on d.sourceid = s.sourceid ");
    query.append(getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations));
    query.append(L" group by merged.brandyID order by rank desc, brandyID asc");  // Group by brandyID and order by rank

    // Set up our column names and types
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("rank");
    column_types.push_back(INT_TYPE);
    printQuery(L"getDocumentsForStrings", query.c_str());
    bool success = getResults(query, column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForStringUsingLike(std::wstring text, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForStringUsingLike");
    wstring query = L"select d.brandyID, 1 as score from Documents d, Sources s where d.sourceid = s.sourceid and source_text like ";
    query.append(text);
    query.append(getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations));

    // Set up our column names and types
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("score");
    column_types.push_back(INT_TYPE);
    printQuery(L"getDocumentsForStringUsingLike", query.c_str());
    bool success = getResults(query, column_names, column_types, results);
    return success;
}

bool DBQuery::getDocumentsForStringAndPatterns(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results, bool isANDSearch) {
    //TimedSection ts("DBQuery::getDocumentsForStringAndPatterns");
    /* The function creates and returns results for queries of the form:
    SELECT TOP 50000 merged.brandyID, sum(merged.RANK) as rank from 
      (
        SELECT TOP 50000 brandyID, KEY_TBL.RANK FROM DOCUMENTS d INNER JOIN CONTAINSTABLE(DOCUMENTS, SOURCE_TEXT, N'formsof(inflectional, "The Trial of Saddam Hussein")') AS KEY_TBL ON d.docID = KEY_TBL.[KEY]  
      ) as merged 
        inner join 
      Documents d on d.brandyID = merged.brandyID  
	  inner join Sources s on d.sourceid = s.sourceid
    where CONTAINS(D.source_text, '
    "aka" OR "changed his name to" OR "changed her name to" OR
    "changed its name to" OR
    "changed their name to" OR
    "known as" OR
    " nee " OR
    "referred to as" OR
    "maiden name" OR
    "goes by" OR
    "alias" OR
    "spelled as" OR
    "misspelled as" OR
    "new name" OR
    "old name" OR
    "name change" OR
    "name changed" OR "also called"')
    AND s.CODE IN ('CTV', 'NTD', 'RFA', 'AHN', 'CTT', 'DFT', 'JST', 'NTT', 'PHN', 'XIN', 'MUT', 'AFC', 'CNA', 'ZBN')  AND d.DOCDATE BETWEEN '20051019' AND '20060930'  
    group by merged.brandyID order by rank desc, brandyID asc
    */
    // TODO: Fix query so we don't use "d" in three different ways, depending on scoping to keep them distinct.

    // Build our subquery for the documents table.  Note that we defer source/date/location restriction until after the join.
    wstring documentsQuery = getQueryForStrings(strings, 0, 0, 0, 0, isANDSearch, L"DOCUMENTS", L"SOURCE_TEXT", L"docID");

    // For the headline, we want to do an ANDed search of the individual tokens.  Thus we need to create a new ValueVector with one token per value.
    wstring headlinesQuery;
    if (strings->size() == 1) {
        
        // Split the string into tokens
        ValueVector tokens = splitWstringIntoTokens(strings->getValueForSQL(0));
        headlinesQuery = getQueryForStrings(&tokens, 0, 0, 0, 0, true, L"Headlines", L"headline", L"brandyID");
    } else {
        headlinesQuery = getQueryForStrings(strings, 0, 0, 0, 0, isANDSearch, L"Headlines", L"headline", L"brandyID");
    }
    wchar_t max_results_str[10];
    _itow(max_results, max_results_str, 10);
    if (documentsQuery.length() == 0) { return false; }
    wstring query = L"SELECT TOP ";
    query.append(max_results_str);
    query.append(L" merged.brandyID, sum(merged.RANK) as rank from (");
    query.append(L"SELECT TOP ");  // BEGIN Documents query
    query.append(max_results_str);
    query.append(L" brandyID, KEY_TBL.RANK ");
    query.append(documentsQuery);  // END Documents query
//    query.append(L" union all ");
//    query.append(L"SELECT TOP ");  // BEGIN Headlines query
//    query.append(max_results_str);
//    query.append(L" brandyID, KEY_TBL.RANK ");
//    query.append(headlinesQuery);  // END Headlines query
    query.append(L") as merged ");
    query.append(L"inner join Documents d on d.brandyID = merged.brandyID "); // join with the documents table so we can restrict by source/date/location
	query.append(L"inner join Sources s on d.sourceid = s.sourceid "); 
    query.append(L"where CONTAINS(d.source_text, '");
    query.append(getTemplate18Patterns());
    query.append(L"')");
    query.append(getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations));
    query.append(L" group by merged.brandyID order by rank desc, brandyID asc");  // Group by brandyID and order by rank

    // Set up our column names and types
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("rank");
    column_types.push_back(INT_TYPE);
    printQuery(L"getDocumentsForStrings", query.c_str());
    bool success = getResults(query, column_names, column_types, results);
    return success;
}

int DBQuery::getDocumentCountForStrings(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, bool isANDSearch) {
    /* The function creates and returns results for queries of the form:
    SELECT COUNT(*)
    FROM DOCUMENTS AS A INNER JOIN
    CONTAINSTABLE(DOCUMENTS, SOURCE_TEXT, N'"vladimir putin"') AS KEY_TBL
    ON A.doc_id = KEY_TBL.[KEY]
    WHERE A.SOURCE IN ('NYT', 'APW') 
    AND A.DOCDATE BETWEEN '7 January 2001' AND '27 January 2002'
    AND CONTAINS(SOURCE_TEXT, '"Moscow"')
    */
    wstring raw_query = getQueryForStrings(strings, sources, dates, ingestDates, locations, isANDSearch);
    if (raw_query.length() == 0)
        return -1;
    wstring query = L"SELECT COUNT(*) AS DOC_COUNT ";
    query.append(raw_query);
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("DOC_COUNT");
    // add column data type
    column_types.push_back(INT_TYPE);
    //printQuery(L"getDocumentCountForStrings", query.c_str());
    ResultSet * results = _new ResultSet();
    bool success = getResults(query, column_names, column_types, results);
    if(!success){
        return -1;
    }
    size_t n_rows = results->getColumnValues(0)->getNValues();
    int doc_count = 0;
    if (n_rows > 0)
        doc_count = _wtoi(results->getColumnValues(0)->getValue(0));
    delete results;
    return doc_count;
}

wstring DBQuery::getQueryForStrings(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, bool isANDSearch, wstring table, wstring textColumn, wstring keyColumn) {
    /* The function creates and returns an ***INCOMPLETE*** query string of the form:
    FROM DOCUMENTS AS A INNER JOIN
    CONTAINSTABLE(DOCUMENTS, SOURCE_TEXT, N'formsof(inflectional, "vladimir putin")') AS KEY_TBL
    ON A.doc_id = KEY_TBL.[KEY]
    WHERE A.SOURCE IN ('NYT', 'APW') 
    AND A.DOCDATE BETWEEN '7 January 2001' AND '27 January 2002'
    AND CONTAINS(SOURCE_TEXT, '"Moscow"')
    */
    try {
        if ((strings == 0) || (strings->getNValues() == 0)) {
            return L"";
        }
        wstring condition;
        ValueVector pruned;
        if (isANDSearch) {
            condition = L"AND";
            pruned = *strings;  // Don't do any pruning for an AND search
        } else {
            condition = L"OR";
            pruned = pruneValueVectorForOrQuery(*strings);  // Prune out redundant superstrings
        }
        wstring query = L"FROM " + table + L" d INNER JOIN CONTAINSTABLE(" + table + L", " + textColumn + L", " + _sqlQueryPhrasePrefix;
        for (size_t i = 0; i < pruned.getNValues(); i++) {
            if (i > 0) {
                query.append(L" ");
                query.append(condition);
                query.append(L" ");
            }
            query.append(L"formsof(inflectional, \"");
            query.append(pruned.getValueForSQL(i));
            query.append(L"\")");
            if (query.length() > 3000) {
                break;
            }
        }
        query.append(L"') AS KEY_TBL ON d." + keyColumn + L" = KEY_TBL.[KEY] ");
		query.append(L"INNER JOIN Sources s on d.sourceid = s.sourceid ");
        query.append( getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations) );
        return query;
    } catch (...) {
        return L"";
    }
}

// Removes any reduandant entries.  ie, "hong kong" OR "hong kong island" can be simplified to just "hong kong"
ValueVector DBQuery::pruneValueVectorForOrQuery(const ValueVector& strings) {
    //SessionLogger::info("BRANDY") << "Entering DBQuery::pruneValueVectorForOrQuery\n";
    if (strings.size() <= 1) { return strings; }  // We only need to prune if there is more than one string
    const int maxOrder = 20;
    ValueVector ngrams[maxOrder];

    // Loop through our strings and put them in the appropriate ngram buckets.  Drop those with > 20 tokens.  (Who searches for 21-grams?)
    for (size_t i = 0; i < strings.getNValues(); i++) {
        wstring name = strings.getValue(i);

        // Count the number of spaces in the string.
        int numSpaces = 0;
        for (size_t j = 0; j < name.length(); j++) {
            if (name.at(j) == ' ') {
                numSpaces++;
            }
        }
        ngrams[numSpaces].addValue(name);
    }

    // Starting with 1-grams, loop through and remove any higher order n-grams that contain the lower order n-gram's token(s)
    for (int lowerOrderNGramIndex = 0; lowerOrderNGramIndex < maxOrder; lowerOrderNGramIndex++) {
        for (size_t lowerOrderEntryIndex = 0; lowerOrderEntryIndex < ngrams[lowerOrderNGramIndex].size(); lowerOrderEntryIndex++) {
            wstring lowerOrderEntry = ngrams[lowerOrderNGramIndex].getValue(lowerOrderEntryIndex);
            const boost::wregex pattern(L"\\b" + lowerOrderEntry + L"\\b", boost::regex_constants::perl);
            //SessionLogger::info("BRANDY") << "Examining lower order entry: " << lowerOrderEntry << ".  Using wregex (" << pattern << ")\n";
            boost::match_results<std::wstring::const_iterator> matchResult;
            for (int higherOrderNGramIndex = lowerOrderNGramIndex + 1; higherOrderNGramIndex < maxOrder; higherOrderNGramIndex++) {
                for (size_t higherOrderEntryIndex = 0; higherOrderEntryIndex < ngrams[higherOrderNGramIndex].size(); higherOrderEntryIndex++) {
                    wstring higherOrderEntry = ngrams[higherOrderNGramIndex].getValue(higherOrderEntryIndex);
                    if (boost::regex_search(higherOrderEntry, matchResult, pattern)) {
                        //SessionLogger::info("BRANDY") << "Removing string " << higherOrderEntry << " as it is a superset of " << lowerOrderEntry << "\n";
                        ngrams[higherOrderNGramIndex].eraseValue(higherOrderEntryIndex);
                        higherOrderEntryIndex--;
                    }
                }
            }
        }
    }

    // Accumulate our remaining n-grams
    ValueVector pruned;
    for (int i = 0; i < maxOrder; i++) {
        for (size_t j = 0; j < ngrams[i].size(); j++) {
            pruned.addValue(ngrams[i].getValue(j));
        }
    }
    return pruned;
}

ValueVector DBQuery::splitWstringIntoTokens(const wstring& text) {
    ValueVector tokens;
    size_t str_len = text.length();
    wstring token = L"";
    for (size_t i = 0; i < str_len; i++) {
        wchar_t symbol = text.at(i);
        if ((symbol == L' ') && (token.length() == 0)) {  // We have a whitespace char and a zero length token
            continue;
        } else if (symbol != L' ') {  // We have a non-whitespace char
            token += towlower(symbol);
        } else {
            if (!isStopWord(token)) {  // We have a whitespace char and a non-zero length token
                tokens.addValue(token.c_str());
            }
            token.clear();
        }
    }
    if (!isStopWord(token)) {  // Don't forget to add in the last token
        tokens.addValue(token.c_str());
    }
    return tokens;
}

bool DBQuery::getDocumentsForExactFuzzySearch(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForExactFuzzySearch");
    /* The function creates and returns results for queries of the form:
    SELECT TOP 1000 DOC_ID, RANK FROM DOCUMENTS INNER JOIN
    CONTAINSTABLE(DOCUMENTS, SOURCE_TEXT, '"10th" AND "anniversary" AND "Gulf" AND "War"', 5000) AS KEY_TBL
    ON DOCUMENTS.DOC_ID = KEY_TBL.[KEY] WHERE DOC_ID = DOC_ID AND 
    CONTAINS(source_text, '"Iraq"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '7 January 2001' AND '27 January 2002'
    ORDER BY RANK DESC, BRANDYID ASC
    */
    if ((strings == 0) || (strings->getNValues() == 0))
        return false;
    if (results == 0)
        return false;
    wstring query = L"SELECT TOP ";
    wchar_t max_results_str[10];
    query.append(_itow(max_results, max_results_str, 10));
    query.append(L" brandyID, RANK FROM DOCUMENTS d INNER JOIN CONTAINSTABLE(DOCUMENTS, SOURCE_TEXT, ");
    query.append(_sqlQueryPhrasePrefix);
    query.append(L"\"");

    // Split the string into tokens
    ValueVector tokens = splitWstringIntoTokens(strings->getValueForSQL(0)); 
    for (size_t i = 0; i < tokens.getNValues(); i++) {
        if (i > 0) {
            query.append(L"\" AND \"");
        }
        query.append(tokens.getValueForSQL(i));
    }
    query.append(L"\"', 5000) AS KEY_TBL ON d.docID = KEY_TBL.[KEY] ");
	query.append(L"INNER JOIN Sources s on d.sourceid = s.sourceid ");
	query.append(L"WHERE docID = docID ");
    query.append( getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations) );
    query.append(L" ORDER BY RANK DESC, BRANDYID ASC");
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("RANK");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForExactFuzzySearch", query.c_str());
    bool success = getResults(query, column_names, column_types, results);
    return success;
}

bool DBQuery::isStopWord(wstring word) {
    if (_stop_words.size() > 0) { // See if we have already initialized the stop words
        return (_stop_words.find(word) != _stop_words.end());
    } else {
        char filename[500];
        ParamReader::getParam("db_stop_words", filename, 500);
        boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
        UTF8InputStream& uis(*uis_scoped_ptr);
        try {
            uis.open(filename);
            if ( _verbose )
                SessionLogger::info("BRANDY") << "Loading stop words from " << filename << "...\n";
            UTF8Token token;
            while ( ! uis.eof() ) {
                uis >> token;
                _stop_words[token.symValue().to_string()] = 1;
            }
            uis.close();
            return (_stop_words.find(word) != _stop_words.end());
        } catch (UnrecoverableException& ){
            if ( _verbose )
                SessionLogger::info("BRANDY") << "ERROR: Loading stop words from " << filename << " failed.\n";
            return false;        
        }
    }
}

bool DBQuery::getDocumentsForFuzzySearch(ValueVector * strings, ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations, int max_results, ResultSet * results) {
    //TimedSection ts("DBQuery::getDocumentsForFuzzySearch");
    /* The function creates and returns results for queries of the form:
    SELECT TOP 1000 DOC_ID, RANK FROM DOCUMENTS INNER JOIN
    FREETEXTTABLE(documents, source_text, 'the Galapagos Island oil spill', 5000) AS KEY_TBL
    ON DOCUMENTS.DOC_ID = KEY_TBL.[KEY] WHERE RANK > 100 AND
    CONTAINS(source_text, '"Galapagos"')
    AND DOCUMENTS.SOURCE IN ('NYT', 'APW') 
    AND DOCUMENTS.DOCDATE BETWEEN '7 January 2001' AND '27 January 2002'
    ORDER BY RANK DESC, BRANDYID ASC
    */
    if ((strings == 0) || (strings->getNValues() == 0))
        return false;
    if (results == 0)
        return false;
    wstring query = L"SELECT TOP ";
    wchar_t max_results_str[10];
    query.append(_itow(max_results, max_results_str, 10));
    query.append(L" brandID, RANK FROM DOCUMENTS d INNER JOIN FREETEXTTABLE(DOCUMENTS, SOURCE_TEXT, ");
    query.append(_sqlQueryPhrasePrefix);
    query.append(strings->getValueForSQL(0));
    query.append(L"', 5000) AS KEY_TBL ON d.docID = KEY_TBL.[KEY] ");
	query.append(L"INNER JOIN Sources s ON d.sourceid = s.sourceid ");
	query.append(L"WHERE RANK > 100 ");
    query.append( getSourcesDatesLocationsSQL(sources, dates, ingestDates, locations) );
    query.append(L" ORDER BY RANK DESC, BRANDYID ASC");
    vector <string> column_names;
    vector <int> column_types;
    // add column name 
    column_names.push_back("brandyID");
    // add column data type
    column_types.push_back(CHAR_TYPE);
    column_names.push_back("RANK");
    column_types.push_back(DOUBLE_TYPE);
    printQuery(L"getDocumentsForFuzzySearch", query.c_str());
    bool success = getResults(query, column_names, column_types, results);
    return success;
}

void DBQuery::printResults(ResultSet * results) {
    if (!_verbose)
        return;
    if (results->getNColumns() == 0)
        return;
    std::ostringstream ostr;
    for (size_t i = 0; i < results->getNColumns(); i++) {
        const char * column_name = results->getColumnName(i);
        ostr << column_name << "\t\t\t\t";
    }
    SessionLogger::info("BRANDY") << ostr.str();
    std::ostringstream ostr2;
    size_t n_values = results->getColumnValues(0)->getNValues();
    int row_index = 0;
    for (size_t k = 0; k < n_values; k++) {
        for (size_t i = 0; i < results->getNColumns(); i++) {
            ValueVector * values = results->getColumnValues(i);
            const wchar_t * value = values->getValue(k);
            ostr2 << value << "\t\t";
        }
        ostr2 << endl;
    }
    SessionLogger::info("BRANDY") << ostr2.str();
}

std::pair<wstring, wstring> DBQuery::getLocationsSQL(ValueVector * locations) {
    wstringstream query;
    if ((locations > 0) && (locations->getNValues() > 0)) {
        query << ", CONTAINSTABLE(Documents, source_text, " << _sqlQueryPhrasePrefix << "\"";

        // Prune the locations to eliminate the redundant entries
        ValueVector pruned = pruneValueVectorForOrQuery(*locations);	
        wstringstream loc_res;

        // MS SQL has a hard limit of 4000 char's for CONTAINS() search strings
        // It also has a more nebulous limit that results in "Too many full-text columns or the full-text query is too complex to be executed."
        // Empirically, the latter limit appears to be 245 OR'd phrases
        for (size_t i = 0; i < pruned.getNValues() && (loc_res.str().size() + 6 + pruned.getValueForSQL(i).size()) < 4000 && i < 245; i++) {
            if (i > 0) loc_res << L"\" OR \"";
            loc_res << pruned.getValueForSQL(i);
        }
        query << loc_res.str() << L"\"') as source_hits ";
		return std::make_pair(query.str(), L"and d.docID = source_hits.[key] ");
	} else {
		return std::make_pair(L"", L"");
	}	
}

std::wstring DBQuery::getLocationsSQLForInnerJoin(ValueVector * locations) {
	wstringstream query;
    if ((locations > 0) && (locations->getNValues() > 0)) {
        query << "inner join CONTAINSTABLE(Documents, source_text, " << _sqlQueryPhrasePrefix << "\"";

        // Prune the locations to eliminate the redundant entries
        ValueVector pruned = pruneValueVectorForOrQuery(*locations);	
        wstringstream loc_res;

        // MS SQL has a hard limit of 4000 char's for CONTAINS() search strings
        // It also has a more nebulous limit that results in "Too many full-text columns or the full-text query is too complex to be executed."
        // Empirically, the latter limit appears to be 245 OR'd phrases
        for (size_t i = 0; i < pruned.getNValues() && (loc_res.str().size() + 6 + pruned.getValueForSQL(i).size()) < 4000 && i < 245; i++) {
            if (i > 0) loc_res << L"\" OR \"";
            loc_res << pruned.getValueForSQL(i);
        }
        query << loc_res.str() << L"\"') as source_hits on source_hits.[key] = d.docID ";
		return query.str();
	} else {
		return L"";
	}	
}

wstring DBQuery::getSourcesDatesLocationsSQL( ValueVector * sources, ValueVector * dates, ValueVector * ingestDates, ValueVector * locations ){

    wstringstream query;

    if ((locations > 0) && (locations->getNValues() > 0)) {
        query << " AND CONTAINS(source_text, " << _sqlQueryPhrasePrefix << "\"";

        // Prune the locations to eliminate the redundant entries
        ValueVector pruned = pruneValueVectorForOrQuery(*locations);	
        wstringstream loc_res;

        // MS SQL has a hard limit of 4000 char's for CONTAINS() search strings
        // It also has a more nebulous limit that results in "Too many full-text columns or the full-text query is too complex to be executed."
        // Empirically, the latter limit appears to be 245 OR'd phrases
        for (size_t i = 0; i < pruned.getNValues() && (loc_res.str().size() + 6 + pruned.getValueForSQL(i).size()) < 4000 && i < 245; i++) {
            if (i > 0) loc_res << L"\" OR \"";
            loc_res << pruned.getValueForSQL(i);
        }
        query << loc_res.str() << L"\"') ";
    }
    if ((sources > 0) && (sources->getNValues() > 0)) {
        query << " AND s.CODE IN ('";
        for (size_t i = 0; i < sources->getNValues(); i++) {
            if (i > 0)
                query << L"', '";
            query << sources->getValue(i);
        }
        query << L"') ";
    }
    if ((dates > 0) && (dates->getNValues() == 2)) {
        query << L" AND d.DOCDATE BETWEEN '";
        query << dates->getValue(0);
        query << L"' AND '";
        query << dates->getValue(1);
        query << L"' ";
    }
    if ((ingestDates > 0) && (ingestDates->getNValues() == 2)) {
        query << L" AND d.INGESTDATE BETWEEN '";
        query << ingestDates->getValue(0);
        query << L"' AND '";
        query << ingestDates->getValue(1);
        query << L"' ";
    }

    return query.str();
}

bool DBQuery::getResults(wstring query, vector <string> columns, vector <int> column_types, ResultSet * results) {
    try {
        Table tbl;

		time_t start_t = time(0);
        _db->Execute(query.c_str(), tbl);
		time_t elapsed_t = time(0) - start_t;

        if( elapsed_t >= 5.0 ) {
            SessionLogger::info("BRANDY") << L"\nLONG_SQL_QUERY (" << static_cast<size_t>(elapsed_t) << L") : " << query << endl;
		} else {

			// Prefix long queries with a newline for readability
			if (query.length() > 160) { SessionLogger::info("BRANDY") << "\n"; }
			SessionLogger::info("BRANDY") << query << std::endl;
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
                    if(tbl.Get(column_name, value_char)) {
                        values[i]->addValue(value_char);
                        //SessionLogger::info("BRANDY") << L"getResults CHAR_TYPE " << i << L" " << value_char << L"\n";
                    } else {
                        error = true;
                    }
                } else if (column_type == INT_TYPE) {
                    if(tbl.Get(column_name, value_int)) {
                        values[i]->addValue(_itow(value_int, value_char, 10));
                        //SessionLogger::info("BRANDY") << L"getResults INT_TYPE " << i << L" " << _itow(value_int, value_char, 10) << L"\n";
                    } else {
                        error = true;
                    }
                } else if (column_type == DOUBLE_TYPE) {
                    if(tbl.Get(column_name, value_double)) {
                        std::wostringstream wos;
                        wos << value_double;
                        wstring wstr = wos.str();
                        values[i]->addValue(wstr.c_str());
                        //SessionLogger::info("BRANDY") << L"getResults DOUBLE_TYPE " << i << L" " << wstr << L"\n";
                    } else {
                        error = true;
                    }
                }
                if (error)
                {
                    tbl.GetErrorErrStr(_error_str);
					SessionLogger::info("BRANDY")<<"\nDBQuery::getResults() Error: "<<_error_str<<"\n";
                    break;
                }
            }
            tbl.MoveNext();
        }

        for (size_t i = 0; i < columns.size(); i++) {
            results->addColumn(columns.at(i).c_str(), values[i]);
        }
        delete [] values;

        return !error;
    }
    catch( DatabaseException & e ){
        if (_failOnDbFailure) {
            wstring errorString = L"Query\n" + query + L"\nthrew the following exception (failing as fail-on-db-failure is set to true):\n";
            throw UnrecoverableException("DBQuery::getResults()", string(errorString.begin(), errorString.end()) + e.what() + "\n");
        } else {
			SessionLogger::warn("BRANDY") << "database failure <in getResults for SQL: '" << query << "'>\n" << e.what() << endl;
            return false;
        }
    }
}

// Things we thought of and discarded: born
std::wstring DBQuery::getTemplate18Patterns() {
    wstringstream query;
    query << L"\"aka\" OR \"changed his name\" OR \"changed her name\" OR \"changed its name\" OR \"changed their name\" OR ";
    query << L"\"known as\" OR \" nee \" OR \"referred to as\" OR \"maiden name\" OR \"goes by\" OR \"alias\" OR \"spelled as\" OR ";
    query << L"\"misspelled as\" OR \"new name\" OR \"old name\" OR \"name change\" OR \"name changed\" OR \"also called\" OR ";
    query << L"\"known by\" OR \"under the name\" OR \"short for\" OR \"nickname\" OR \"nicknamed\" OR \"stands for\" OR \"now called\" OR ";
    query << L"\"nom de guerre\" OR \"nom de plume\" OR \"acronym for\" OR \"unmarried name\"";
    return query.str();
}

void DBQuery::getDocumentResolutions( wstring brandyID, map< int, pair<Symbol, Symbol> > & names ){
	
	if( names.empty() ) return;
	
	wstringstream query;
	
	query << "select entityID, xdoc_short, xdoc_canonical from Entities e, Documents d where e.docID = d.docID and d.brandyID = '" << brandyID;
	//query << "select entityID, canonical_name as xdoc_short, canonical_name as xdoc_canonical from Entities e, Documents d where e.docID = d.docID and d.brandyID = '" << wstring(brandyID.begin(), brandyID.end());
	query << "' and entityID in (" << names.begin()->first;
	
	for( std::map< int, std::pair<Symbol, Symbol> >::iterator en_it = names.begin(); ++en_it != names.end(); )
		query << "," << en_it->first;
	query << ")";
	
	vector <string> column_names;
	vector <int> column_types;
	
	column_names.push_back("entityID"); column_names.push_back( "xdoc_short" ); column_names.push_back( "xdoc_canonical" );
	column_types.push_back( INT_TYPE ); column_types.push_back( CHAR_TYPE );    column_types.push_back( CHAR_TYPE );
	
	ResultSet res;
	getResults(query.str(), column_names, column_types, &res);
	
	for(size_t i = 0; i < res.getColumnValues(0)->getNValues(); i++ )
		names[ boost::lexical_cast<int>( res.getColumnValues(0)->getValue(i) ) ] = make_pair( 
			Symbol(res.getColumnValues(1)->getValue(i)), Symbol(res.getColumnValues(2)->getValue(i)) );
	
	return;
}

double DBQuery::getConfidenceForSegment(const std::wstring& brandyID, int official_segment_id) {
    std::wstringstream query;
    query << "select confidence from SegmentEntries where docID in (select docID from Documents where brandyID='";
    query << brandyID;
    query << "') and segmentNo=";
    query << official_segment_id;

    vector <string> column_names;
    vector <int> column_types;
    
    column_names.push_back("confidence");
    column_types.push_back(DOUBLE_TYPE);

    ResultSet results;
    getResults(query.str(), column_names, column_types, &results);
	ValueVector *valResults = results.getColumnValues(0);
	// LB: I'm not sure if this is necessary, but it was crashing when the document wasn't in the database.
	//     I don't think that should ever happen in real-time, but it scared me, so...
	if (valResults->getNValues() != 0)
		return boost::lexical_cast<double>(valResults->getValue(0));
	else return 0.0;
} 

std::wstring DBQuery::getDatelineForDocument(const std::wstring& brandyID) {
    std::wstringstream query;
    query << "select dateline from Documents where brandyID='";
    query << brandyID;
    query << "'";

    vector <string> column_names;
    vector <int> column_types;
    
    column_names.push_back("dateline");
    column_types.push_back(CHAR_TYPE);

    ResultSet results;
    getResults(query.str(), column_names, column_types, &results);
	ValueVector *valResults = results.getColumnValues(0);
	if (valResults->getNValues() != 0) {
		std::wstring return_value(valResults->getValue(0));
		boost::trim(return_value);
		return return_value;
	} else {
		return L"";
	}
} 
