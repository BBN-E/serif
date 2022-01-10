
// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "ICEWS/ICEWSDB.h"

// Helpers

namespace {
	DatabaseConnection_ptr makeDBConnection(const std::string& url) {
		DatabaseConnection_ptr icews_db = DatabaseConnection::connect(url);
		if (ParamReader::isParamTrue("icews_profile_sql"))
		  icews_db->enableProfiling();
		return icews_db;
	}
}


namespace ICEWS {
DatabaseConnection_ptr getSingletonIcewsDb() {
  static DatabaseConnection_ptr icews_db;
  if (!icews_db)
	  icews_db = makeDBConnection(ParamReader::getRequiredParam("icews_db"));
  return icews_db;
}

DatabaseConnection_ptr getSingletonIcewsStoriesDb() {
  static DatabaseConnection_ptr icews_stories_db;
  if (!icews_stories_db) {
	  std::string db_url = ParamReader::getParam("icews_stories_db");
	  if (db_url.empty() || (db_url == ParamReader::getParam("icews_db"))) {
		  icews_stories_db = getSingletonIcewsDb();
	  } else {
		  icews_stories_db = makeDBConnection(db_url);
	  }
  }
  return icews_stories_db;
}

DatabaseConnection_ptr getSingletonIcewsStorySerifXMLDb() {
  static DatabaseConnection_ptr icews_story_serifxml_db;
  if (!icews_story_serifxml_db) {
	  std::string db_url = ParamReader::getParam("icews_story_serifxml_db");
	  if (db_url.empty() || (db_url == ParamReader::getParam("icews_db"))) {
		  icews_story_serifxml_db = getSingletonIcewsDb();
	  } else if (db_url == ParamReader::getParam("icews_stories_db")) {
		  icews_story_serifxml_db = getSingletonIcewsStoriesDb();
	  } else {
		  icews_story_serifxml_db = makeDBConnection(db_url);
	  }
  }
  return icews_story_serifxml_db;
}

DatabaseConnection_ptr getSingletonIcewsOutputDb() {
  static DatabaseConnection_ptr icews_output_db;
  if (!icews_output_db) {
	  std::string db_url = ParamReader::getParam("icews_output_db");
	  if (db_url.empty() || (db_url == ParamReader::getParam("icews_db"))) {
		  icews_output_db = getSingletonIcewsDb();
	  } else {
		  icews_output_db = makeDBConnection(db_url);
	  }
  }
  return icews_output_db;
}

DatabaseConnection_ptr getSingletonIcewsGeonamesDb() {
  static DatabaseConnection_ptr icews_geonames_db;
  if (!icews_geonames_db) {
	  std::string db_url = ParamReader::getParam("icews_geonames_db");
	  if (db_url.empty() || (db_url == ParamReader::getParam("icews_db"))) {
		  icews_geonames_db = getSingletonIcewsDb();
	  } else {
		  icews_geonames_db = makeDBConnection(db_url);
	  }
  }
  return icews_geonames_db;
}


}

