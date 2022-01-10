// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// Serif HTTP Server command-line interface

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/icews/ICEWSDocumentReader.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/common/TimexUtils.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/icews/ICEWSDB.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/Stories.h"
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <limits.h>

using namespace xercesc;
using namespace SerifXML;

namespace {
	Symbol ICEWS_SENTENCE_SYM(L"ICEWS_Sentence");
}


ICEWSDocumentReader::ICEWSDocumentReader(): _one_region_per_sentence(false), _include_headline(false),
											_sentence_cutoff(INT_MAX){
	std::vector<std::string> region_breaks = ParamReader::getStringVectorParam("doc_reader_region_breaks");
	BOOST_FOREACH(const std::string& s, region_breaks) {
		if (boost::iequals(s, "sentence"))
			_one_region_per_sentence = true;
	}
	_include_headline = ParamReader::isParamTrue("icews_include_headline");
	_include_publisher = ParamReader::isParamTrue("icews_include_publisher");
	_include_source_name = ParamReader::isParamTrue("icews_include_source_name");
	_sentence_cutoff = static_cast<size_t>(ParamReader::getOptionalIntParamWithDefaultValue("icews_hard_sentence_cutoff", INT_MAX));
}

ICEWSDocumentReader::ICEWSDocumentReader(Symbol defaultInputType) {
	throw UnexpectedInputException("ICEWSDocumentReader::ICEWSDocumentReader", 
		"defaultInputType not supported by this factory");
}

Document* ICEWSDocumentReader::readDocument(InputStream &strm, const wchar_t * filename) {
	if (!boost::filesystem::exists(filename)) {
		std::stringstream msg;
		msg << "Document: " << UnicodeUtil::toUTF8StdString(filename) << " does not exist.";
		throw UnexpectedInputException("ICEWSDocumentReader::readDocument", msg.str().c_str());
	}
	xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromStream(strm);
	Document* doc = readDocumentFromDom(xmlStory, Symbol(filename), 0);
	xmlStory->release();
	return doc;
}

Document* ICEWSDocumentReader::readDocumentFromByteStream(std::istream &strm, const wchar_t * filename) {
	if (!boost::filesystem::exists(filename)) {
		std::stringstream msg;
		msg << "Document: " << UnicodeUtil::toUTF8StdString(filename) << " does not exist.";
		throw UnexpectedInputException("ICEWSDocumentReader::readDocumentFromByteStream", msg.str().c_str());
	}
	xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromStream(strm);
	
	boost::filesystem::wpath docPathBoost(filename);
	std::wstring base_doc_name = UnicodeUtil::toUTF16StdString(BOOST_FILESYSTEM_PATH_GET_FILENAME(docPathBoost));

	Document* doc = readDocumentFromDom(xmlStory, Symbol(base_doc_name), 0);
	xmlStory->release();
	return doc;
}

Document* ICEWSDocumentReader::readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName) {
	return readDocumentFromDom(xmlStory, docName, 0);
}

/** Modifies storyText! */
ICEWSDocumentReader::RegionSpec ICEWSDocumentReader::addRegion(const wchar_t* text, Symbol tag, std::wstring& storyText) {
	storyText.append(tag.to_string());
	storyText.append(L": ");
	int start = static_cast<int>(storyText.size());
	storyText.append(text);
	int end = static_cast<int>(storyText.size());
	// Add a blank line after the headline.
	storyText.append(L"\n\n");
	return RegionSpec(start, end, tag);
}

	/** Modifies storyText! */
std::vector<ICEWSDocumentReader::RegionSpec> ICEWSDocumentReader::addSentences(xercesc::DOMDocument* xmlStory, std::wstring &storyText) {
	static const XMLCh* X_sentence = xercesc::XMLString::transcode("sentence");
	static const XMLCh* X_index =  xercesc::XMLString::transcode("index");
	std::vector<RegionSpec> sentRegionSpecs; // return value
	DOMElement* xmlRoot = xmlStory->getDocumentElement();
	DOMNodeList* xmlSentences = xmlRoot->getElementsByTagName(X_sentence);
	for (size_t i=0; i<xmlSentences->getLength() && i<_sentence_cutoff; ++i) {
		static const Symbol SENTENCE_TAG(L"SENTENCE");
		DOMElement* xmlSentence = dynamic_cast<DOMElement*>(xmlSentences->item(i));
		// Add the sentence to the storyText.
		std::wstring sentText = SerifXML::transcodeToStdWString(xmlSentence->getTextContent());
		std::string sentNoStr = SerifXML::transcodeToStdString(xmlSentence->getAttribute(X_index));
		size_t sentno = 0;
		try {
			sentno = boost::lexical_cast<int>(sentNoStr);
		} catch (boost::bad_lexical_cast &) {
			throw UnexpectedInputException("ICEWSDocumentReader::readDocumentFromDom", 
											   "Bad <sentence index='...'> value in ICEWS Document",
											   sentNoStr.c_str());
		}
		if (sentText.size() == 0) continue; // skip empty sentences.
		int sent_start = static_cast<int>(storyText.size());
		storyText += sentText;
		int sent_end = static_cast<int>(storyText.size());
		sentRegionSpecs.push_back(RegionSpec(sent_start, sent_end, SENTENCE_TAG, sentno));
		// Put a blank line after each sentence.
		storyText.append(L"\n\n");
	}
	return sentRegionSpecs;
}

Document* ICEWSDocumentReader::readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName, const wchar_t *publicationDate,
												   const wchar_t *headline, const wchar_t *publisher, const wchar_t *sourceName) 
{
	std::wstring storyText(L"\n"); // start with a blank line to ensure story text is never empty.
	std::vector<RegionSpec> regionSpecs;

	// Add the headline, if specified (in its own region).
	if (headline && _include_headline) {
		static const Symbol HEADLINE_TAG(L"HEADLINE");
		regionSpecs.push_back(addRegion(headline, HEADLINE_TAG, storyText));
	}

	// Add the sentences, separated by blank lines.  Keep track of their locations
	// and sentence numbers (in sentRegionSpecs) so we can add metadata later.
	int sent_group_start = static_cast<int>(storyText.size());
	std::vector<RegionSpec> sentRegionSpecs = addSentences(xmlStory, storyText);

	// Ensure that the sentences section isn't entirely empty.
	storyText.append(L"\n");
	// Add region spec(s) for the sentences.
	if (_one_region_per_sentence) {
		regionSpecs.insert(regionSpecs.end(), sentRegionSpecs.begin(), sentRegionSpecs.end());
	} else {
		static const Symbol BODY_TAG(L"BODY");
		regionSpecs.push_back(RegionSpec(sent_group_start, static_cast<int>(storyText.size()), BODY_TAG));
	}

	// Add the publisher, if specified (in its own region).
	if (publisher && _include_publisher) {
		static const Symbol PUBLISHER_TAG(L"PUBLISHER");
		regionSpecs.push_back(addRegion(publisher, PUBLISHER_TAG, storyText));
	}

	// Add the source_name, if specified (in its own region).
	if (sourceName && _include_source_name) {
		static const Symbol SOURCE_NAME_TAG(L"SOURCE_NAME");
		regionSpecs.push_back(addRegion(sourceName, SOURCE_NAME_TAG, storyText));
	}


	// Get the publication date.   If no explicit publication
	// date was given, then try to extract one from the filename.
	std::wstring publicationDateStr;
	if (publicationDate) {
		publicationDateStr = publicationDate;
	} else {
		std::string extractedPubDate = Stories::extractPublicationDateFromDocId(docName);
		if (!extractedPubDate.empty()) {
			publicationDateStr = UnicodeUtil::toUTF16StdString(extractedPubDate).c_str();
		} else {
			StoryId extractedStoryId = Stories::extractStoryIdFromDocId(docName);
			if (!extractedStoryId.isNull()) {
				std::string pubDate = Stories::getStoryPublicationDate(extractedStoryId);
				publicationDateStr = UnicodeUtil::toUTF16StdString(pubDate).c_str();
			}
		}
	}
	// If we have a publication date, then append it to the document contents.
	int pub_date_start = -1;
	int pub_date_end = -1;
	if (!publicationDateStr.empty()) {
		const static std::wstring pubDatePrefix(L"Publication date: ");
		storyText += pubDatePrefix;
		pub_date_start = static_cast<int>(storyText.size());
		storyText += publicationDateStr;
		pub_date_end = static_cast<int>(storyText.size());
		// Note: we do *not* include the publication date as a region;
		// instead, we just use it to set the document's date-time
		// field.
		/*static const Symbol PUB_DATE_TAG(L"PUBLICATION_DATE");
		  regionSpecs.push_back(RegionSpec(pub_date_start, pub_date_end, PUB_DATE_TAG));*/
		storyText.append(L"\n\n");
	}


	// Convert the story text string to a LocatedString.
	LocatedString content(storyText.c_str());

	// Create the new document.
	Document *doc = _new Document(docName);
	doc->setOriginalText(&content);
	//result->setSourceType(doctype);

	// Pulication date stored on Document
	if (!publicationDateStr.empty()) {
		boost::optional<boost::gregorian::date> date;
		boost::optional<boost::posix_time::time_duration> timeOfDay;
		boost::optional<boost::local_time::posix_time_zone> timeZone;
		TimexUtils::parseDateTime(publicationDateStr, date, timeOfDay, timeZone);
		if (date) {
			boost::posix_time::ptime start(*date);
			doc->setDocumentTimePeriod(start, boost::posix_time::seconds(86399)); // start of day, end of day
		}
	}

	// Set the date time field to the publication date.
	if (pub_date_start >= 0) {
		boost::scoped_ptr<LocatedString> dateTimeField(content.substring(pub_date_start, pub_date_end));
		doc->setDateTimeField(dateTimeField.get()); // makes copy of LocatedString
	} else {
		// SessionLogger TODO
		SessionLogger::warn("ICEWS") << "No publication date found for: " << docName;
	}

	// Create the regions, and give ownership of them to the document.
	std::vector<Region*> regions;
	int region_no = 0;
	BOOST_FOREACH(RegionSpec regionSpec, regionSpecs) {
		boost::scoped_ptr<LocatedString> regionContent(content.substring(regionSpec.start, regionSpec.end));
		regions.push_back(_new Region(doc, regionSpec.tag, region_no++, regionContent.get()));
	}
	doc->takeRegions(regions);

	// Use 'metadata' to record the locations of sentences.
	doc->getMetadata()->addSpanCreator(ICEWS_SENTENCE_SYM,
		_new IcewsSentenceSpanCreator());

	BOOST_FOREACH(RegionSpec sentSpec, sentRegionSpecs) {
		doc->getMetadata()->newSpan(ICEWS_SENTENCE_SYM, 
			EDTOffset(sentSpec.start), EDTOffset(sentSpec.end-1), &sentSpec.sentno);
	}

	return doc;
}	
