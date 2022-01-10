// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/Stories.h"
#include "Generic/icews/ICEWSDocumentReader.h"
#include "Generic/icews/ICEWSDB.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Offset.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/LocatedString.h"
#include "Generic/driver/Batch.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/common/UnexpectedInputException.h"
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <xercesc/dom/DOM.hpp>
#include <fstream>

using namespace SerifXML;

Document* Stories::readDocument(StoryId storyId) {
	std::wostringstream docNameStr;
	docNameStr << "icews.stories." << storyId.getId();
	Symbol docName(docNameStr.str());
	DatabaseConnection_ptr icews_db(ICEWSDB::getStoriesDb());
	std::ostringstream query;

	query << "SELECT XMLText, ";

	std::string db_url = ParamReader::getParam("icews_stories_db");
	if (db_url.empty())
		db_url = ParamReader::getRequiredParam("icews_db");
	
	if (db_url.find("oracle") == 0) {
		query << "to_char(publicationdate, 'YYYY-MM-DD'), ";
	} else if (db_url.find("mysql") == 0) {
		query << "PublicationDate, ";
	} else {
		std::ostringstream msg;
		msg << "icews_stories_db (or icews_db if icews_stories_db unspecified) must start with 'oracle' or 'mysql' to read stories from the db: " << db_url;
		throw UnexpectedInputException("Stories::readDocument", msg.str().c_str());
	}
	query << "Headline, Publisher, SourceName"
		  <<" FROM stories where StoryID=" << storyId.getId();
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
		ICEWSDocumentReader documentReader;
		std::string xmlString = row.getCellAsString(0);
		std::wstring publicationDate(row.getCellAsWString(1));
		std::wstring headline(row.getCellAsWString(2));
		std::wstring publisher(row.getCellAsWString(3));
		std::wstring sourceName(row.getCellAsWString(4));

		if (!xmlString.empty()) {
			xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromString(xmlString.c_str());
			Document *doc = documentReader.readDocumentFromDom(
				xmlStory, docName, 
				(publicationDate.empty()?0:publicationDate.c_str()),
				(headline.empty()?0:headline.c_str()),
				(publisher.empty()?0:publisher.c_str()),
				(sourceName.empty()?0:sourceName.c_str()));
			xmlStory->release();
			return doc;
		} else {
			// Create an empty document:
			LocatedString content(L" ");
			LocatedString* regionStrings[] = {&content};
			Document *doc = _new Document(docName, 1, regionStrings);
			doc->setOriginalText(&content);
			SessionLogger::warn_user("ICEWS") << "No XMLText found for story " << static_cast<unsigned long>(storyId.getId());
			return doc;
		}
	}

	std::stringstream errMsg;
	errMsg << "Story with StoryID " << storyId.getId() << " not found";
	throw UnexpectedInputException("Stories::readDocument", errMsg.str().c_str());
}

void Stories::saveCachedSerifxmlForStory(StoryId storyId, const DocTheory* docTheory) {
	std::string cacheDir = ParamReader::getParam("icews_cache_serifxml_in_dir");

	// Save it in the database cache (if we're using it)
	if (ParamReader::isParamTrue("icews_cache_serifxml_in_database")) {
		DatabaseConnection_ptr icews_db(ICEWSDB::getStorySerifXMLDb());
		ensureStorySerifxmlTableExists(icews_db);
		std::ostringstream serifxml;
		XMLSerializedDocTheory serializedDoc(docTheory);
		serializedDoc.save(serifxml);
		std::ostringstream query;
		query << "INSERT INTO story_serifxml(StoryID,SerifXML) VALUES "
			<< "(" << storyId.getId() << ", '" 
			<< DatabaseConnection::sanitize(serifxml.str()) << "');";
		icews_db->iter(query);
	} 
	// Otherwise save it in the directory cache (if we're using it).
	else if (!cacheDir.empty()) {
		OutputUtil::makeDir(cacheDir.c_str());
		std::ostringstream filename;
		filename << cacheDir << SERIF_PATH_SEP << "story_serifxml." << storyId.getId() << ".xml";
		XMLSerializedDocTheory serializedDoc(docTheory);
		serializedDoc.save(filename.str().c_str());
	}
}

std::pair<Document*, DocTheory*> Stories::loadCachedSerifxmlForStory(StoryId storyId) {
	// Check in the database cache (if we're using it).
	if (ParamReader::isParamTrue("icews_cache_serifxml_in_database")) {
		DatabaseConnection_ptr icews_db(ICEWSDB::getStorySerifXMLDb());
		ensureStorySerifxmlTableExists(icews_db);
		std::ostringstream query;
		query << "SELECT SerifXML FROM story_serifxml WHERE StoryId=" << storyId.getId();
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			XMLSerializedDocTheory serializedDoc(XMLUtil::loadXercesDOMFromString(row.getCell(0)));
			return serializedDoc.generateDocTheory();
		}
	}

	// Check in the file cache (if we're using it).
	std::string cacheDir = ParamReader::getParam("icews_cache_serifxml_in_dir");
	if (!cacheDir.empty()) {
		std::ostringstream filename;
		filename << cacheDir << SERIF_PATH_SEP << "story_serifxml." << storyId.getId() << ".xml";
		std::ifstream stream(filename.str().c_str());
		if (stream) {
			XMLSerializedDocTheory serializedDocTheory(XMLUtil::loadXercesDOMFromStream(stream)); 
			return serializedDocTheory.generateDocTheory();
		}
	}

	// Story not found!
	return std::pair<Document*,DocTheory*>(0,0);
}


void Stories::ensureStorySerifxmlTableExists(DatabaseConnection_ptr icews_db) {
	static bool completed = false;
	if (completed) return;
	std::ostringstream query;
	query << "CREATE TABLE IF NOT EXISTS `story_serifxml` ("
		<< "`SerifXMLId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
		<< "`StoryId` BIGINT UNSIGNED NOT NULL ,"
		<< "`SerifXML` LONGTEXT NOT NULL)"
		<< " ENGINE=MyISAM DEFAULT CHARSET=utf8"
		<< " COMMENT='Caches SERIF analysis for each story';";
	icews_db->iter(query);
	completed = true;
}

struct DatabaseIterator: public Stories::IteratorCore {
	DatabaseConnection::RowIterator rowIter;
	DatabaseIterator(DatabaseConnection::RowIterator rowIter): rowIter(rowIter) {}
	StoryId dereference() { return StoryId(rowIter.getCellAsInt64(0)); }
	void increment() { ++rowIter; }
	bool equal(Stories::IteratorCore_ptr const &other) { 
		if (boost::shared_ptr<DatabaseIterator> p = boost::dynamic_pointer_cast<DatabaseIterator>(other))
			return rowIter == p->rowIter;
		else
			return false;
	}
};

struct BatchIterator: public Stories::IteratorCore {
	boost::shared_ptr<Batch> batch;
	size_t n;
	BatchIterator(boost::shared_ptr<Batch> batch, size_t n=0): batch(batch), n(n) {}
	StoryId dereference() { 
		if (n>=batch->getNDocuments())
			throw InternalInconsistencyException("BatchIterator::dereference", "past end of sequence");
		return StoryId(boost::lexical_cast<SQLBigInt>(batch->getDocumentPath(n))); }
	void increment() { ++n; }
	bool equal(Stories::IteratorCore_ptr const &other) { 
		if (boost::shared_ptr<BatchIterator> p = boost::dynamic_pointer_cast<BatchIterator>(other))
			return n == p->n;
		else
			return false;
	}
};


std::string Stories::getStoryPublicationDate(Document *document) {
	boost::optional<boost::gregorian::date> documentDate = document->getDocumentDate();
	if (documentDate) {
		// Should return in extended ISO format
		std::wstring normalizedDate = document->normalizeDocumentDate();
		// Get rid of information after the date (i.e. time zone)
		if (normalizedDate.size() > 10) {
			normalizedDate = normalizedDate.substr(0,10);
		}
		return UnicodeUtil::toUTF8StdString(normalizedDate);
	}
	// First, use the document's DateTimeField
	if (document->getDateTimeField()) {
		std::string dateTime = UnicodeUtil::toUTF8StdString(document->getDateTimeField()->toWString());
		static const boost::regex VALID_DATE_TIME("[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]");
		if (boost::regex_match(dateTime, VALID_DATE_TIME))
			return dateTime;
	}
	// If that fails, try to get a publication date from the docid.
	std::string pubDate = extractPublicationDateFromDocId(document->getName());
	if (!pubDate.empty())
		return pubDate;
	// If that fails, then try to get a StoryId from the docid, & use that to look up pub date.
	StoryId extractedStoryId = extractStoryIdFromDocId(document->getName());
	if (!extractedStoryId.isNull())
		return getStoryPublicationDate(extractedStoryId);
	// Otherwise, no date is available
	return std::string();
}

std::string Stories::getStoryPublicationDate(StoryId storyId) {
	std::ostringstream query;
	query << "SELECT PublicationDate from stories WHERE storyId=" << storyId.getId();
	DatabaseConnection_ptr icews_db(ICEWSDB::getStoriesDb());
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row)
		return row.getCellAsString(0);
	return "UNKNOWN-DATE";
}

Stories::StoryIDRange Stories::getStoryIds() {
	if (ParamReader::isParamTrue("icews_get_story_ids_from_batch_file")) {
		boost::shared_ptr<Batch> batch = boost::make_shared<Batch>();
		batch->loadFromFile(ParamReader::getParam("batch_file").c_str());
		return std::make_pair(StoryIDIterator(boost::make_shared<BatchIterator>(batch, 0)),
		                      StoryIDIterator(boost::make_shared<BatchIterator>(batch, batch->getNDocuments())));
	}

	DatabaseConnection_ptr icews_db(ICEWSDB::getStoriesDb());
	std::vector<std::string> conditions;
	if (ParamReader::hasParam("icews_min_storyid"))
		conditions.push_back(std::string("storyID >= "+
							 ParamReader::getParam("icews_min_storyid")));
	if (ParamReader::hasParam("icews_max_storyid"))
		conditions.push_back(std::string("storyID <= "+
							 ParamReader::getParam("icews_max_storyid")));
	if (ParamReader::hasParam("icews_min_story_ingest_date"))
		conditions.push_back(std::string("IngestDate >= "+
										 icews_db->toDate(ParamReader::getParam("icews_min_story_ingest_date"))));
	if (ParamReader::hasParam("icews_max_story_ingest_date"))
		conditions.push_back(std::string("IngestDate <= "+
										 icews_db->toDate(ParamReader::getParam("icews_max_story_ingest_date"))));
	if (ParamReader::hasParam("icews_min_story_publication_date"))
		conditions.push_back(std::string("PublicationDate >= "+
										 icews_db->toDate(ParamReader::getParam("icews_min_story_publication_date"))));
	if (ParamReader::hasParam("icews_max_story_publication_date"))
		conditions.push_back(std::string("PublicationDate <= "+
										 icews_db->toDate(ParamReader::getParam("icews_max_story_publication_date"))));
	if (ParamReader::hasParam("icews_story_source"))
		conditions.push_back("source = " + 
							 icews_db->quote(ParamReader::getParam("icews_story_source")));

	std::ostringstream query;
	query << "SELECT StoryID from stories";
	for (size_t i=0; i<conditions.size(); ++i) {
		if (i==0) query << " WHERE ";
		else query << " AND ";
		query << conditions[i];
	}
	query << " ORDER BY StoryID";

	StoryIDIterator start = StoryIDIterator(boost::make_shared<DatabaseIterator>(icews_db->iter_with_limit(query)));
	StoryIDIterator end = StoryIDIterator(boost::make_shared<DatabaseIterator>(icews_db->end()));

	if (start == end) {
		SessionLogger::warn_user("ICEWS") << "No stories found for processing using the following query: " << query;
	}

	return std::make_pair(start, end);
}

StoryId Stories::extractStoryIdFromDocId(Symbol docid) {
	static const boost::wregex DOCID_RE_1(L"(.*[/\\\\])?([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])\\.([0-9]+)\\.xml(\\..*)?");
	static const boost::wregex DOCID_RE_2(L"icews\\.stories\\.([0-9]+)");
	static const boost::wregex DOCID_RE_3(L"([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])\\.([0-9]+)");
	boost::wsmatch match;
	std::wstring docid_str(docid.to_string());
	if (boost::regex_match(docid_str, match, DOCID_RE_1)) {
		return StoryId(boost::lexical_cast<StoryId::id_type>(match.str(5)));
	} else if (boost::regex_match(docid_str, match, DOCID_RE_2)) {
		return StoryId(boost::lexical_cast<StoryId::id_type>(match.str(1)));
	} else if (boost::regex_match(docid_str, match, DOCID_RE_3)) {
		return StoryId(boost::lexical_cast<StoryId::id_type>(match.str(4)));
	}
	return StoryId();
}

std::string Stories::extractPublicationDateFromDocId(Symbol docid) {
	static const boost::wregex DOCID_RE_1(L"(.*[/\\\\])?([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])\\.([0-9]+)\\.xml(\\..*)?");
	static const boost::wregex DOCID_RE_3(L"([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])\\.([0-9]+)");
	static const boost::wregex DOCID_RE_WMS(L"[a-z]+-[a-z]+-[a-z]+-[a-z]+-[A-Z][A-Z][A-Z]([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])");
	static const boost::wregex DOCID_RE_FALLBACK(L"([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])\\.(.*)\\.xml");	
	boost::wsmatch match;
	std::wstring docid_str(docid.to_string());
	if (boost::regex_match(docid_str, match, DOCID_RE_1)) {
		std::wostringstream dateTime;
		dateTime << match.str(2) << "-" << match.str(3) << "-" << match.str(4);
		return UnicodeUtil::toUTF8StdString(dateTime.str());
	} else if (boost::regex_match(docid_str, match, DOCID_RE_3)) {
		std::wostringstream dateTime;
		dateTime << match.str(1) << "-" << match.str(2) << "-" << match.str(3);
		return UnicodeUtil::toUTF8StdString(dateTime.str());
	} else if (boost::regex_match(docid_str, match, DOCID_RE_WMS)) {
		std::wostringstream dateTime;
		dateTime << match.str(1) << "-" << match.str(2) << "-" << match.str(3);
		return UnicodeUtil::toUTF8StdString(dateTime.str());
	} else if (boost::regex_match(docid_str, match, DOCID_RE_FALLBACK)) {
		std::wostringstream dateTime;
		dateTime << match.str(1) << "-" << match.str(2) << "-" << match.str(3);
		return UnicodeUtil::toUTF8StdString(dateTime.str());
	}
	return std::string();
}
