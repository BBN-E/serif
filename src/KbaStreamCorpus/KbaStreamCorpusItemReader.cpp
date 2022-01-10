// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "KbaStreamCorpus/KbaStreamCorpusItemReader.h"
#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"

#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/state/XMLSerializedDocTheory.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

namespace {
	boost::regex WHITESPACE("^\\s*$");
}

KbaStreamCorpusItemReader::KbaStreamCorpusItemReader()
	: _skip_docs_with_empty_language_code(false),
	  _read_serifxml_from_chunk(false)
{
	_skip_docs_with_empty_language_code = ParamReader::isParamTrue("kba_skip_docs_with_empty_language_code");
	_read_serifxml_from_chunk = ParamReader::isParamTrue("kba_read_serifxml_from_chunk");

	// Initialize any document readers.
	std::string cleanVisibleReader = ParamReader::getParam("kba_clean_visible_document_reader", "sgm");
	_documentReaders[CLEAN_VISIBLE].reset(DocumentReader::build(cleanVisibleReader.c_str()));

	std::string rawReader = ParamReader::getParam("kba_raw_document_reader", "sgm");
	_documentReaders[RAW].reset(DocumentReader::build(rawReader.c_str()));

	std::string cleanHtmlReader = ParamReader::getParam("kba_clean_html_document_reader", "");
	if (!boost::iequals(cleanHtmlReader, "none")) {
		if (!cleanHtmlReader.empty()) {
			_documentReaders[CLEAN_HTML].reset(DocumentReader::build(cleanHtmlReader.c_str()));
		} else if (DocumentReader::hasFactory("html")) {
			// Default to HTML document reader, *if* it's available.
			_documentReaders[CLEAN_HTML].reset(DocumentReader::build("html"));
		}
	}

	// Get the ordered list of source fields to check.
	std::vector<std::string> sourceList = ParamReader::getStringVectorParam("kba_stream_item_sources");
	if (sourceList.empty()) {
		sourceList.push_back("clean_html");
		sourceList.push_back("clean_visible");
	}
	BOOST_FOREACH(const std::string& sourceName, sourceList) {
		Source source = NO_SOURCE;
		if (boost::iequals(sourceName, "raw"))
			source = RAW;
		else if (boost::iequals(sourceName, "clean_visible"))
			source = CLEAN_VISIBLE;
		else if (boost::iequals(sourceName, "clean_html"))
			source = CLEAN_HTML;
		else if (boost::iequals(sourceName, "none"))
			source = NO_SOURCE;
		else
			throw UnexpectedInputException("KbaStreamCorpusItemReader::KbaStreamCorpusItemReader",
										   "Bad value for kba_stream_item_sources: ", sourceName.c_str());
		if (_documentReaders.find(source) != _documentReaders.end()) {
			_sourceList.push_back(source);
		} else {
			SessionLogger::warn("kba-stream-corpus") 
				<< "Not using source type " << sourceName
				<< " because no document reader is available for it; use the "
				<< "kba_" << sourceName << "_document_reader parameter to specify one.";
		}
	}

}

std::string KbaStreamCorpusItemReader::getSourceName(const streamcorpus::StreamItem& item) const {
	return getSourceName(selectSource(item).first);
}

std::string KbaStreamCorpusItemReader::getSourceName(Source source) const {
	if (source == RAW) return "raw";
	if (source == CLEAN_VISIBLE) return "clean_visible";
	if (source == CLEAN_HTML) return "clean_html";
	if (source == NO_SOURCE) return "none";
	return "unknown";
}


std::pair<KbaStreamCorpusItemReader::Source, const std::string*> KbaStreamCorpusItemReader::selectSource(const streamcorpus::StreamItem& item) const {
	// Check that we have some text to process.
	BOOST_FOREACH(Source source, _sourceList) {
		const std::string *s = 0;
		if (source==RAW) 
			s = &item.body.raw;
		else if (source==CLEAN_VISIBLE) 
			s = &item.body.clean_visible;
		else if (source==CLEAN_HTML) 
			s = &item.body.clean_html;
		else
			s = 0;
		if (s && !boost::regex_match(*s, WHITESPACE)) {
			SessionLogger::info("kba-stream-corpus") 
				<< "Reading content of " << item.doc_id
				<< " from source field " << getSourceName(source);
			return std::make_pair(source, s);
		}
	}
	return std::make_pair(NO_SOURCE, static_cast<const std::string*>(0));
}


bool KbaStreamCorpusItemReader::skipItem(streamcorpus::StreamItem& item) {
	// Ignore documents with no text; and ignore
	// documents with the wrong language.
	if (selectSource(item).first == NO_SOURCE) {
		SessionLogger::info("kba-stream-corpus") << "Skipping item; no text to process.";
		return true;
	}

	if (item.body.language.code.empty()) {
		if (_skip_docs_with_empty_language_code) {
			SessionLogger::info("kba-stream-corpus") << "Skipping item; empty language code.";
			return true;
		}
	} else {
		if (UnicodeUtil::toUTF16StdString(item.body.language.code) !=
			SerifVersion::getSerifLanguage().toShortString()) {
			SessionLogger::info("kba-stream-corpus") << "Skipping item; language code (" << item.body.language.code
				<< ") doesn't match Serif language (" << SerifVersion::getSerifLanguage().toShortString() << ")";
			return true;
		}
	}
	return false;
}

KbaStreamCorpusItemReader::DocPair KbaStreamCorpusItemReader::itemToDocPair(streamcorpus::StreamItem& item) {
	if (skipItem(item))
		throw InternalInconsistencyException("KbaStreamCorpusItemReader::readItem",
											 "called on an item that should be skipped");

	// If the kba_read_serifxml_from_chunk parameter is used, then
	// parse the serifxml that is in the following field:
	//     StreamItem.body.taggings['serif'].raw_tagging
	if (_read_serifxml_from_chunk) {
		typedef std::map<streamcorpus::TaggerID, streamcorpus::Tagging> TaggingMap;
		TaggingMap::iterator tag_iter = item.body.taggings.find("serif");
		if (tag_iter == item.body.taggings.end()) {
			throw UnexpectedInputException("KbaStreamCorpusItemReader::readItem",
										   "StreamItem.body.taggings['serif'] not found");
		}
		const std::string &rawTagging = (*tag_iter).second.raw_tagging;
		SessionLogger::info("kba-stream-corpus") 
			<< "Reading serifxml for " << item.doc_id
			<< " (" << rawTagging.size() << " bytes)";
		if (rawTagging.empty()) {
			throw UnexpectedInputException("KbaStreamCorpusItemReader::readItem",
										   "StreamItem.body.taggings['serif'].rawTagging is empty");
		}
		xercesc::DOMDocument *domDocument = XMLUtil::loadXercesDOMFromString(rawTagging.c_str(),
																			 rawTagging.size());
		return SerifXML::XMLSerializedDocTheory(domDocument).generateDocTheory();
	}

	// Get the doc_id.
	std::string name = item.doc_id;
	const Symbol doc_id(UnicodeUtil::toUTF16StdString(name));

	// Get the document's content.
	std::pair<Source, const std::string*> sourcePair = selectSource(item);
	DocumentReader *docReader = _documentReaders[sourcePair.first].get();
	const std::string *contentField = sourcePair.second;

	// Convert to unicode.  Add some whitespace at the end of the
	// document, just in case we have any fencepost errors.
	std::wstring textStr = UnicodeUtil::toUTF16StdString(*contentField);
	textStr.append(L"\n\n");
		
	// Create the document.
	std::wistringstream textStream(textStr);
	Document *doc = docReader->readDocument(textStream, doc_id.to_string());

	// Pair it with an initial (empty) DocTheory.
	return DocPair(doc, _new DocTheory(doc));
}
