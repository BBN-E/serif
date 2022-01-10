// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// Serif HTTP Server command-line interface

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/reader/VIKTRSDocumentReader.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/preprocessors/VIKTRSSentenceSpan.h"
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
	Symbol VIKTRS_SENTENCE_SYM(L"VIKTRS_Sentence");
}

VIKTRSDocumentReader::VIKTRSDocumentReader(): _one_region_per_sentence(false) {
	std::vector<std::string> region_breaks = ParamReader::getStringVectorParam("doc_reader_region_breaks");
	BOOST_FOREACH(const std::string& s, region_breaks) {
		if (boost::iequals(s, "sentence"))
			_one_region_per_sentence = true;
	}
}

VIKTRSDocumentReader::VIKTRSDocumentReader(Symbol defaultInputType) {
	throw UnexpectedInputException("VIKTRSDocumentReader::VIKTRSDocumentReader", 
		"defaultInputType not supported by this factory");
}

Document* VIKTRSDocumentReader::readDocument(InputStream &strm, const wchar_t * filename) {
	if (!boost::filesystem::exists(filename)) {
		std::stringstream msg;
		msg << UnicodeUtil::toUTF8StdString(filename) << " does not exist.";
		throw UnexpectedInputException("VIKTRSDocumentReader::readDocument", msg.str().c_str());
	}
	xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromStream(strm);
	DOMElement* xmlRoot = xmlStory->getDocumentElement();
	static const XMLCh* X_docid = xercesc::XMLString::transcode("docid");
	DOMNodeList* xmlDocids = xmlRoot->getElementsByTagName(X_docid);
	Document* doc;
	if (xmlDocids->getLength() > 0) {
		DOMElement* xmlDocid = dynamic_cast<DOMElement*>(xmlDocids->item(0));
		std::wstring docidText = SerifXML::transcodeToStdWString(xmlDocid->getTextContent());
		doc = readDocumentFromDom(xmlStory, Symbol(docidText));
	}
	else {
		doc = readDocumentFromDom(xmlStory, Symbol(filename));
	}
	xmlStory->release();
	return doc;
}

Document* VIKTRSDocumentReader::readDocumentFromByteStream(std::istream &strm, const wchar_t * filename) {
	if (!boost::filesystem::exists(filename)) {
		std::stringstream msg;
		msg << UnicodeUtil::toUTF8StdString(filename) << " does not exist.";
		throw UnexpectedInputException("VIKTRSDocumentReader::readDocumentFromByteStream", msg.str().c_str());
	}
	xercesc::DOMDocument* xmlStory = XMLUtil::loadXercesDOMFromStream(strm);
	DOMElement* xmlRoot = xmlStory->getDocumentElement();
	static const XMLCh* X_docid = xercesc::XMLString::transcode("docid");
	DOMNodeList* xmlDocids = xmlRoot->getElementsByTagName(X_docid);
	Document* doc;
	if (xmlDocids->getLength() > 0) {
		DOMElement* xmlDocid = dynamic_cast<DOMElement*>(xmlDocids->item(0));
		std::wstring docidText = SerifXML::transcodeToStdWString(xmlDocid->getTextContent());
		doc = readDocumentFromDom(xmlStory, Symbol(docidText));
	}
	else {
		doc = readDocumentFromDom(xmlStory, Symbol(filename));
	}
	xmlStory->release();
	return doc;
}


VIKTRSDocumentReader::RegionSpec VIKTRSDocumentReader::addRegion(const wchar_t* text, Symbol tag, std::wstring& storyText) {
	storyText.append(tag.to_string());
	storyText.append(L": ");
	int start = static_cast<int>(storyText.size());
	storyText.append(text);
	int end = static_cast<int>(storyText.size());
	// Add a blank line after the headline.
	storyText.append(L"\n\n");
	return RegionSpec(start, end, tag);
}

	
std::vector<VIKTRSDocumentReader::RegionSpec> VIKTRSDocumentReader::addSentences(xercesc::DOMDocument* xmlStory, std::wstring &storyText) {
	static const XMLCh* X_sentence = xercesc::XMLString::transcode("sentence");
	static const XMLCh* X_index =  xercesc::XMLString::transcode("pID");
	static const XMLCh* X_step_pos_id = xercesc::XMLString::transcode("sID");
	std::vector<RegionSpec> sentRegionSpecs; // return value
	DOMElement* xmlRoot = xmlStory->getDocumentElement();
	DOMNodeList* xmlSentences = xmlRoot->getElementsByTagName(X_sentence);
	for (size_t i=0; i<xmlSentences->getLength(); ++i) {
		static const Symbol SENTENCE_TAG(L"SENTENCE");
		DOMElement* xmlSentence = dynamic_cast<DOMElement*>(xmlSentences->item(i));
		// Add the sentence to the storyText.
		std::wstring sentText = SerifXML::transcodeToStdWString(xmlSentence->getTextContent());
		std::string sentNoStr = SerifXML::transcodeToStdString(xmlSentence->getAttribute(X_index));
		std::string stepNoStr = SerifXML::transcodeToStdString(xmlSentence->getAttribute(X_step_pos_id));
		size_t sentno = 0;
		try {
			sentno = boost::lexical_cast<int>(sentNoStr);
		} catch (boost::bad_lexical_cast &) {
			throw UnexpectedInputException("VIKTRSDocumentReader::readDocumentFromDom", 
											   "Bad <sentence pID='...' value",
											   sentNoStr.c_str());
		}
		size_t stepno = 0;
		try {
			stepno = boost::lexical_cast<int>(stepNoStr);
		} catch (boost::bad_lexical_cast &) {
			throw UnexpectedInputException("VIKTRSDocumentReader::readDocumentFromDom", 
											   "Bad <sentence sID='...' value",
											   stepNoStr.c_str());
		}
		if (sentText.size() == 0) continue; // skip empty sentences.
		int sent_start = static_cast<int>(storyText.size());
		storyText += sentText;
		int sent_end = static_cast<int>(storyText.size());
		sentRegionSpecs.push_back(RegionSpec(sent_start, sent_end, SENTENCE_TAG, sentno, stepno));
		// Put a blank line after each sentence.
		storyText.append(L"\n\n");
	}
	return sentRegionSpecs;
}

Document* VIKTRSDocumentReader::readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName) 
{
	std::wstring storyText(L"\n"); // start with a blank line to ensure story text is never empty.
	std::vector<RegionSpec> regionSpecs;

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
		static const Symbol TEXT_TAG(L"TEXT");
		regionSpecs.push_back(RegionSpec(sent_group_start, static_cast<int>(storyText.size()), TEXT_TAG));
	}

	// Convert the story text string to a LocatedString.
	LocatedString content(storyText.c_str());

	// Create the new document.
	Document *doc = _new Document(docName);
	doc->setOriginalText(&content);

	// Create the regions, and give ownership of them to the document.
	std::vector<Region*> regions;
	int region_no = 0;
	BOOST_FOREACH(RegionSpec regionSpec, regionSpecs) {
		boost::scoped_ptr<LocatedString> regionContent(content.substring(regionSpec.start, regionSpec.end));
		regions.push_back(_new Region(doc, regionSpec.tag, region_no++, regionContent.get()));
	}
	doc->takeRegions(regions);

	// Use 'metadata' to record the locations of sentences.
	doc->getMetadata()->addSpanCreator(VIKTRS_SENTENCE_SYM,
		_new VIKTRSSentenceSpanCreator());

	BOOST_FOREACH(RegionSpec sentSpec, sentRegionSpecs) {
		doc->getMetadata()->newSpan(VIKTRS_SENTENCE_SYM, 
			EDTOffset(sentSpec.start), EDTOffset(sentSpec.end-1), &sentSpec);
	}

	return doc;
}	
