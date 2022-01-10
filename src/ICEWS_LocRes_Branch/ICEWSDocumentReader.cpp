// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// Serif HTTP Server command-line interface

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "ICEWS/ICEWSDocumentReader.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "ICEWS/SentenceSpan.h"
#include "ICEWS/ICEWSDB.h"
#include "ICEWS/Stories.h"
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <limits.h>

using namespace xercesc;
using namespace SerifXML;

namespace {
	Symbol ICEWS_SENTENCE_SYM(L"ICEWS_Sentence");
}

namespace ICEWS {

ICEWSDocumentReader::ICEWSDocumentReader(): _one_region_per_sentence(false) {
	std::vector<std::string> region_breaks = ParamReader::getStringVectorParam("doc_reader_region_breaks");
	BOOST_FOREACH(const std::string& s, region_breaks) {
		if (boost::iequals(s, "sentence"))
			_one_region_per_sentence = true;
	}
}


Document* ICEWSDocumentReader::readDocument(InputStream &strm, const wchar_t * filename) {
	xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromStream(strm);
	Document* doc = readDocumentFromDom(xmlStory, Symbol(filename), 0);
	xmlStory->release();
	return doc;
}

Document* ICEWSDocumentReader::readDocumentFromByteStream(std::istream &strm, const wchar_t * filename) {
	xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromStream(strm);
	Document* doc = readDocumentFromDom(xmlStory, Symbol(filename), 0);
	xmlStory->release();
	return doc;
}

Document* ICEWSDocumentReader::readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName) {
	return readDocumentFromDom(xmlStory, docName, 0);
}

Document* ICEWSDocumentReader::readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName, const wchar_t *publicationDate) 
{
	static const XMLCh* X_sentence = xercesc::XMLString::transcode("sentence");
	static const XMLCh* X_index =  xercesc::XMLString::transcode("index");
	size_t sentence_cutoff = static_cast<size_t>(ParamReader::getOptionalIntParamWithDefaultValue("icews_hard_sentence_cutoff", INT_MAX));

	std::wstring storyText;
	typedef std::pair<int, int> SentenceLocation;
	std::vector<SentenceLocation> sentenceLocations;
	std::vector<size_t> sentenceNumbers;
	SentenceLocation publicationDateLoc;

	DOMElement* xmlRoot = xmlStory->getDocumentElement();
	DOMNodeList* xmlSentences = xmlRoot->getElementsByTagName(X_sentence);
	int pos = 0;
	
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

	// Write the sentences, one on each line.
	for (size_t i=0; i<xmlSentences->getLength() && i<sentence_cutoff; ++i) {
		DOMElement* xmlSentence = dynamic_cast<DOMElement*>(xmlSentences->item(i));
		// Add the sentence to the storyText.
		std::wstring sentText = SerifXML::transcodeToStdWString(xmlSentence->getTextContent());
		std::string sentNoStr = SerifXML::transcodeToStdString(xmlSentence->getAttribute(X_index));
		size_t sentno = 0;
		try {
			sentno = boost::lexical_cast<int>(sentNoStr);
		} catch (boost::bad_lexical_cast &) {
			throw new UnexpectedInputException("ICEWSDocumentReader::readDocumentFromDom", 
											   "Bad <sentence index='...'> value",
											   sentNoStr.c_str());
		}
		if (sentText.size() == 0) continue; // just in case.
		storyText += sentText;
		sentenceLocations.push_back(SentenceLocation(pos, pos+sentText.size()));
		sentenceNumbers.push_back(sentno);
		pos += sentText.size();
		// Put a newline after each sentence.
		storyText.append(L"\n\n");
		pos += 2;
	}
	// Ensure that the document text isn't entirely empty:
	if (storyText.empty()) storyText = L" ";

	// If we have a publication date, then append it to the document contents.
	if (!publicationDateStr.empty()) {
		const static std::wstring pubDatePrefix(L"Publication date: ");
		storyText += pubDatePrefix;
		pos += pubDatePrefix.size();
		storyText += publicationDateStr;
		publicationDateLoc = SentenceLocation(pos, pos+publicationDateStr.size());
		pos += publicationDateStr.size();
		storyText.append(L"\n\n");
		pos += 2;
	}

	LocatedString content(storyText.c_str(), true);
	Document *doc = 0;
	if (_one_region_per_sentence) {
		// Create a document with one region for each sentence.
		std::vector<LocatedString*> regions;
		BOOST_FOREACH(SentenceLocation sentLoc, sentenceLocations) 
			regions.push_back(content.substring(sentLoc.first, sentLoc.second));
		doc = _new Document(docName, regions.size(), &(regions[0]));
		BOOST_FOREACH(LocatedString *region, regions) 
			delete region;
	} else {
		// Create a new document that contains a single region (the storyText).
		LocatedString* regionStrings[] = {&content};
		doc = _new Document(docName, 1, regionStrings);
	}
	doc->setOriginalText(&content);

	// Set the date time field to the publication date.
	if (!publicationDateStr.empty()) {
		LocatedString *dateTimeField = content.substring(publicationDateLoc.first, publicationDateLoc.second);
		doc->setDateTimeField(dateTimeField); // makes copy of LocatedString
		delete dateTimeField;
	} else {
		SessionLogger::warn("ICEWS") << "No publication date found for: " << docName;
	}

	// Use 'metadata' to record the locations of sentences.
	doc->getMetadata()->addSpanCreator(ICEWS_SENTENCE_SYM,
		_new IcewsSentenceSpanCreator());

	for (size_t i=0; i<sentenceLocations.size(); ++i) {
		size_t sent_no = sentenceNumbers[i];
		SentenceLocation sentLoc = sentenceLocations[i];
		doc->getMetadata()->newSpan(ICEWS_SENTENCE_SYM, 
			EDTOffset(sentLoc.first), EDTOffset(sentLoc.second-1), &sent_no);
	}

	return doc;
}

}
