// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include <wchar.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Segment.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/version.h"
#include "Generic/common/TimexUtils.h"
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/reader/DTRADocumentReader.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Zone.h"
#include "Generic/theories/Zoning.h"
#include "Generic/theories/Sentence.h"

#include <boost/date_time/posix_time/posix_time_io.hpp>


Document::Document(Symbol name, int n_regions, LocatedString **regionStrings, LanguageAttribute language)
	: _name(name), _n_regions(n_regions), _zoning(0), _metadata(_new Metadata()),
	_sourceType(Symbol(L"UNKNOWN")), _dateTimeField(0),
	_language(language)
{

	_regions = _new Region*[n_regions];
	
	for (int i = 0; i < n_regions; i++) {
		_regions[i] = _new Region(this,Symbol(),i,regionStrings[i]);
	}

	if (downcaseAllUppercaseDocs() && getCaseFromRegions(n_regions, _regions) == UPPER)
		DOWNCASED = true;
	else DOWNCASED = false;

	if (DOWNCASED) {
		for (int i = 0; i < n_regions; i++) 
			 _regions[i]->toLowerCase();
	}
}

Document::Document(Symbol name, int n_regions, Region **regions, LanguageAttribute language)
	: _name(name), _n_regions(n_regions),_zoning(0), _metadata(_new Metadata()),
	_sourceType(Symbol(L"UNKNOWN")), _dateTimeField(0),
	_language(language)
{

	if (downcaseAllUppercaseDocs() && getCaseFromRegions(n_regions, regions) == UPPER)
		DOWNCASED = true;
	else DOWNCASED = false;

	_regions = _new Region*[n_regions];
	for (int i = 0; i < n_regions; i++) {
		_regions[i] = regions[i];
		if (DOWNCASED) _regions[i]->toLowerCase();
	}
}

Document::Document(std::vector<Document*> splitDocuments) : _metadata(_new Metadata()) {
	if (splitDocuments.size() == 0)
		throw InternalInconsistencyException("Document::Document", "No split documents given");

	// Copy stuff from first document, assuming 3-digit split index
	std::wstring docid = splitDocuments[0]->getName().to_string();
	_name = Symbol(docid.substr(0, docid.length() - 4));
	_sourceType = splitDocuments[0]->getSourceType();
	if (splitDocuments[0]->getDateTimeField() != NULL)
		_dateTimeField = _new LocatedString(*(splitDocuments[0]->getDateTimeField()));
	else
		_dateTimeField = NULL;
	_language = splitDocuments[0]->getLanguage();
	DOWNCASED = splitDocuments[0]->isDowncased();
	_originalText = splitDocuments[0]->_originalText;

	// Set up Metadata
	Symbol regionSpanSymbol = Symbol(L"REGION_SPAN");
	_metadata->addSpanCreator(regionSpanSymbol, _new RegionSpanCreator());

	// Accumulate regions across documents in order
	_n_regions = 0;
	_zoning = 0;
	BOOST_FOREACH(Document* splitDocument, splitDocuments) {
		_n_regions += splitDocument->getNRegions();
	}
	if (_n_regions > 0) {
		_regions = _new Region*[_n_regions];
		int region_offset = 0;
		BOOST_FOREACH(Document* splitDocument, splitDocuments) {
			// Check that everything is from the same original document
			docid = splitDocument->getName().to_string();
			if (Symbol(docid.substr(0, docid.length() - 4)) != _name)
				throw InternalInconsistencyException("Document::Document", "Split documents don't have same original document ID");
			if (splitDocument->getSourceType() != _sourceType)
				throw InternalInconsistencyException("Document::Document", "Split documents don't have same source type");
			if (splitDocument->getLanguage() != _language)
				throw InternalInconsistencyException("Document::Document", "Split documents don't have same language");
			if (splitDocument->isDowncased() != DOWNCASED)
				throw InternalInconsistencyException("Document::Document", "Split documents don't have same case");
			if (splitDocument->_originalText != _originalText)
				throw InternalInconsistencyException("Document::Document", "Split documents don't have same original text");
			if ((splitDocument->getDateTimeField() == NULL && _dateTimeField != NULL) || (splitDocument->getDateTimeField() != NULL && _dateTimeField == NULL) || (_dateTimeField != NULL && splitDocument->getDateTimeField()->toWString() != _dateTimeField->toWString()))
				throw InternalInconsistencyException("Document::Document", "Split documents don't have same datetime field");

			// Copy each region and accumulate metadata spans
			for (int r = 0; r < splitDocument->getNRegions(); r++) {
				Region* splitRegion = _new Region(NULL, splitDocument->getRegions()[r]->getRegionTag(), region_offset + r, const_cast<LocatedString*>(splitDocument->getRegions()[r]->getString()));
				Symbol tag = splitRegion->getRegionTag();
				_metadata->newSpan(regionSpanSymbol, splitRegion->getStartEDTOffset(), splitRegion->getEndEDTOffset(), &tag);
				_regions[region_offset + r] = splitRegion;
			}
			region_offset += splitDocument->getNRegions();
		}
	} else
		_regions = NULL;
}

int Document::getCaseFromRegions(int n_regions, Region **regions) {

	int upper_count = 0;
	int lower_count = 0;

	for (int j = 0; j < n_regions; j++) {
		const LocatedString *region = regions[j]->getString();
		for (int k = 0; k < region->length(); k++) {
			if (iswupper(region->charAt(k)))
				upper_count++;
			else if (iswlower(region->charAt(k)))
				lower_count++;
		}
	}

	if (upper_count == 0 && lower_count > 1) 
		return LOWER;
	if (upper_count > 0 && (float)upper_count / (upper_count + lower_count) > 0.5) {
		SessionLogger::dbg("doc_upper_0") << "Document classified as uppercase because >0.5 of alpha chars are upper.";
		return UPPER;
	}
	return MIXED;

}
/*int Document::getCaseFromZones(int n_zones, Zone **zones) {

	int upper_count = 0;
	int lower_count = 0;

	for (int j = 0; j < n_zones; j++) {
		const LocatedString *zone = zones[j]->getString();
		for (int k = 0; k < zone->length(); k++) {
			if (iswupper(zone->charAt(k)))
				upper_count++;
			else if (iswlower(zone->charAt(k)))
				lower_count++;
		}
	}

	if (upper_count == 0 && lower_count > 1) 
		return LOWER;
	if (upper_count > 0 && (float)upper_count / (upper_count + lower_count) > 0.5) {
		SessionLogger::dbg("doc_upper_0") << "Document classified as uppercase because >0.5 of alpha chars are upper.";
		return UPPER;
	}
	return MIXED;

}*/

int Document::getCaseFromRegions(std::vector<Region*> regions) {

	int upper_count = 0;
	int lower_count = 0;

	for (unsigned int j = 0; j < regions.size(); j++) {
		const LocatedString *region = regions[j]->getString();
		for (int k = 0; k < region->length(); k++) {
			if (iswupper(region->charAt(k)))
				upper_count++;
			else if (iswlower(region->charAt(k)))
				lower_count++;
		}
	}

	if (upper_count == 0 && lower_count > 1) 
		return LOWER;
	if (upper_count > 0 && (float)upper_count / (upper_count + lower_count) > 0.5) {
		SessionLogger::dbg("doc_upper_1") << "Document classified as uppercase because >0.5 of alpha chars are upper.";								
		return UPPER;
	}
	return MIXED;

}


int Document::downcaseAllUppercaseDocs() {
	return ParamReader::getRequiredTrueFalseParam("downcase_all_uppercase_docs");
}

Document::Document(Symbol name, LanguageAttribute language)
	: _name(name), DOWNCASED(false), _n_regions(0), _regions(0), _zoning(0),
	_metadata(_new Metadata()), _dateTimeField(0),
	_sourceType(Symbol(L"UNKNOWN")), _language(language)
{
}

Document::~Document() {
	freeRegions();	
	if(_zoning){
		delete _zoning;
	}
	delete _metadata;
	if (_dateTimeField) delete _dateTimeField;
}

/**
 * The current metadata is deleted. The Document object takes
 * the responsibility for deleting the given Metadata object
 * when it is no longer needed.
 *
 * @param metadata the new metadata for the document.
 */
void Document::setMetadata(Metadata *metadata) {
	delete _metadata;
	_metadata = metadata;
}
void Document::setSourceType(Symbol sourcetype){
	_sourceType = sourcetype;
}

void Document::freeRegions() {
	for (int i = 0; i < _n_regions; i++) {
		delete _regions[i];
	}
	delete[] _regions;
	_n_regions = 0;
}

/*void Document::freeZones() {
	for (int i = 0; i < _n_zones; i++) {
		
		delete _zones[i];
	}
	
		delete[] _zones;
	_n_zones = 0;
}*/

void Document::takeRegions(std::vector<Region*> regions) {
	if (downcaseAllUppercaseDocs() && getCaseFromRegions(regions) == UPPER)
		DOWNCASED = true;

	_n_regions = (unsigned int) regions.size();

	_regions = _new Region*[_n_regions];
	for (int i = 0; i < _n_regions; i++) {
		_regions[i] = regions[i];
		if (DOWNCASED) _regions[i]->toLowerCase();
	}
}



void Document::takeZoning(Zoning* zoning) {
	//if (downcaseAllUppercaseDocs() && getCaseFromRegions(regions) == UPPER)
	//	DOWNCASED = true;
		_zoning=zoning;

		if (DOWNCASED) 
		{
			_zoning->toLowerCase();
		}
	if(!_zoning->assertZonesCompatibleWithRegions(_regions,_n_regions)){
		throw InternalInconsistencyException("Document::Document", "Zones are not compatible with Regions");
	}
}



void Document::dump(std::ostream &out, int indent) {

	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Document "
		<< "(document " << _name.to_debug_string() << "):";

	for (int i = 0; i < _n_regions; i++) {
		out << newline;
		_regions[i]->dump(out, indent + 2);
	}

	delete[] newline;
}

void Document::updateObjectIDTable() const {
	throw InternalInconsistencyException("Document::updateObjectIDTable()",
										"Using unimplemented method.");
}

void Document::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Document::saveState()",
										"Using unimplemented method.");

}

void Document::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Document::resolvePointers()",
										"Using unimplemented method.");

}

void Document::copyOriginalText(Document *other)
{
	if (_originalText)
		throw InternalInconsistencyException("Document::setOriginalText", 
		"Original text of document already set");

	// Shared pointer assignment will increment the reference count for both
	_originalText = other->_originalText;
}

void Document::setOriginalText(const LocatedString *text)
{
	if (_originalText)
		throw InternalInconsistencyException("Document::setOriginalText", 
								"Original text of document already set");
	
	// Caller retains ownership of original pointer, internally we copy into a new shared pointer
	_originalText = boost::make_shared<const LocatedString>(*text);
}

void Document::setDateTimeField(const LocatedString *datetime)
{
	if (_dateTimeField)
		throw InternalInconsistencyException("Document::setDateTimeField", 
								"Date/Time field of document already set");
	
	if (datetime != 0)
		_dateTimeField = _new LocatedString(*datetime);
	else _dateTimeField = 0;
}

const wchar_t* Document::XMLIdentifierPrefix() const {
	return L"doc";
}

namespace {
	void _saveSegment(const WSegment& segment, SerifXML::XMLTheoryElement segElem) {
		using namespace SerifXML;
		// Serialize the segment attributes.
		typedef std::pair<std::wstring, std::wstring> WStringPair;
		BOOST_FOREACH(WStringPair attr, segment.segment_attributes()) {
			XMLTheoryElement attrElem = segElem.addChild(X_Attribute);
			attrElem.setAttribute(X_key, attr.first);
			attrElem.setAttribute(X_val, attr.second);
		}
		// Serialize the segment fields.
		typedef std::pair<std::wstring, WSegment::field_entries_t> FieldPair;
		BOOST_FOREACH(FieldPair field, segment) {
			XMLTheoryElement fieldElem = segElem.addChild(X_Field);
			fieldElem.setAttribute(X_name, field.first);
			BOOST_FOREACH(WSegment::field_entry_t entry, field.second) {
				XMLTheoryElement entryElem = fieldElem.addChild(X_Entry);
				BOOST_FOREACH(WStringPair attr, entry.attributes) {
					XMLTheoryElement attrElem = entryElem.addChild(X_Attribute);
					attrElem.setAttribute(X_key, attr.first);
					attrElem.setAttribute(X_val, attr.second);
				}
				entryElem.addChild(X_Contents).addText(entry.value.c_str());
			}
		}
	}

	void _loadSegment(WSegment& segment, SerifXML::XMLTheoryElement segElem) {
		using namespace SerifXML;
		// Read segment attributes.
		WSegment::attributes_t& segmentAttrs = segment.segment_attributes();
		std::vector<XMLTheoryElement> segmentAttrElems = segElem.getChildElementsByTagName(X_Attribute, false);
		BOOST_FOREACH(XMLTheoryElement segmentAttrElem, segmentAttrElems) {
			segmentAttrs[segmentAttrElem.getAttribute<std::wstring>(X_key)] = 
				segmentAttrElem.getAttribute<std::wstring>(X_val);		
		}
		// Read segment fields.
		std::vector<XMLTheoryElement> fieldElems = segElem.getChildElementsByTagName(X_Field, false);
		BOOST_FOREACH(XMLTheoryElement fieldElem, fieldElems) {
			std::wstring fieldName = fieldElem.getAttribute<std::wstring>(X_name);
			std::vector<XMLTheoryElement> entryElems = fieldElem.getChildElementsByTagName(X_Entry);
			BOOST_FOREACH(XMLTheoryElement entryElem, entryElems) {
				WSegment::attributes_t entryAttribs;
				std::vector<XMLTheoryElement> attrElems = entryElem.getChildElementsByTagName(X_Attribute, false);
				BOOST_FOREACH(XMLTheoryElement attrElem, attrElems) {
					entryAttribs[attrElem.getAttribute<std::wstring>(X_key)] =
						attrElem.getAttribute<std::wstring>(X_val);
				}
				XMLTheoryElement entryValue = entryElem.getUniqueChildElementByTagName(X_Contents);
				segment.add_field(fieldName, entryValue.getText<std::wstring>(), entryAttribs);
			}
		}
	}
}

void Document::addTimeZone(std::stringstream &ss, const boost::local_time::posix_time_zone &timeZone) const {
	ss << (timeZone.base_utc_offset().hours() > 0 ? "+" : "-")
	   << std::setfill('0') << std::setw(2) 
	   << abs(timeZone.base_utc_offset().hours())
	   << ":"
	   << std::setfill('0') << std::setw(2)
	   << timeZone.base_utc_offset().minutes();
}

void Document::saveXML(SerifXML::XMLTheoryElement documentElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Document::saveXML", "Expected context to be NULL");
	// Document attributes
	documentElem.setAttribute(X_docid, getName());
	documentElem.setAttribute(X_source_type, getSourceType());
	documentElem.setAttribute(X_is_downcased, isDowncased());
	// Original text
	XMLTheoryElement originalTextElem;
	if (_url.empty() || !documentElem.getOptions().external_original_text) {
		originalTextElem = documentElem.saveChildTheory(X_OriginalText, getOriginalText());
	} else {
		originalTextElem = documentElem.addChild(X_OriginalText);
		originalTextElem.setAttribute(X_href, _url);
	}
	// Date-time field
	if (getDateTimeField())
		documentElem.saveChildTheory(X_DateTime, getDateTimeField());

	if (_documentTimePeriod) {
		std::stringstream start;
		start.imbue(std::locale(std::locale::classic(),       
			new boost::posix_time::time_facet("%Y-%m-%dT%H:%M:%S")));
		start << _documentTimePeriod->begin();

		std::stringstream end;
		end.imbue(std::locale(std::locale::classic(),       
			new boost::posix_time::time_facet("%Y-%m-%dT%H:%M:%S")));
		end << _documentTimePeriod->end();

		if (_documentTimeZone) {
			addTimeZone(start, *_documentTimeZone);
			addTimeZone(end, *_documentTimeZone);
		}

		documentElem.setAttribute(X_document_time_start, start.str());
		documentElem.setAttribute(X_document_time_end, end.str());
	}

	// Regions
	XMLTheoryElement regions = documentElem.addChild(X_Regions);
	for (int i=0; i<getNRegions(); ++i) {
		if (getRegions()[i]->getRegionNumber() != i)
			throw InternalInconsistencyException("Document::saveXML", "Inconsistent region number");
		regions.saveChildTheory(X_Region, getRegions()[i]);
	}
	// Zones
	if(_zoning){
	
		XMLTheoryElement zones = documentElem.addChild(X_Zones);
		XMLTheoryElement zoning=zones.addChild(X_Zoning);
		for (int i=0; i<_zoning->getSize(); ++i) {
			zoning.saveChildTheory(X_Zone, getZoning()->getRoots()[i]);
		}
	}
	// Segments 
	XMLTheoryElement segments = documentElem.addChild(X_Segments);
	for (int i=0; i<getNSegments(); ++i) {
		XMLTheoryElement segmentElem = segments.addChild(X_Segment);
		_saveSegment(_segments[i], segmentElem);
	}
	// Metadata
	if (getMetadata())
		documentElem.saveChildTheory(X_Metadata, getMetadata());
	// Language
	documentElem.setAttribute(X_language, getLanguage().toString());
}

Document::Document(SerifXML::XMLTheoryElement documentElem):
_name(), _n_regions(0), _regions(0), _zoning(0), _metadata(0), _sourceType(L"UNKNOWN"), DOWNCASED(false), _dateTimeField(0),
_language(SerifVersion::getSerifLanguage()) {
	using namespace SerifXML;
	documentElem.loadId(this);

	// Check the language
	_language = LanguageAttribute::getFromString(documentElem.getAttribute<std::wstring>(X_language).c_str());
	if (_language != SerifVersion::getSerifLanguage() && SerifVersion::getSerifLanguage() != Language::UNSPECIFIED)
		documentElem.reportLoadError((std::wstring(L"This server only supports language=\"") +  
		    SerifVersion::getSerifLanguage().toString()+L"\"").c_str());

	// Name & original text are required.
	_name = documentElem.getAttribute<Symbol>(X_docid);
	_originalText = LocatedString_ptr(documentElem.loadChildTheory<LocatedString>(X_OriginalText));
	documentElem.getXMLSerializedDocTheory()->setOriginalText(_originalText.get());

	// Get the URL of the original text (if present)
	_url = documentElem.getUniqueChildElementByTagName(X_OriginalText).getAttribute<std::wstring>(X_href, L"");

	// If there are no regions, then assume we haven't done zoning yet; so do it now.
	XMLTheoryElement regionsElem = documentElem.getOptionalUniqueChildElementByTagName(X_Regions);
	if (!regionsElem) {

		// Check if we have an input type that requires a specific DocumentReader.
		Document* zonedDocument;
		bool isDTRA = (documentElem.getOptions().input_type == Symbol(L"dtra"));
		if (isDTRA) {
			// For DTRA we want to handle the raw text directly, so we rebuild from the underlying string;
			// calling toString() is safe here because a copy of the buffer is made internally
			DTRADocumentReader dtraDocReader;
			zonedDocument = dtraDocReader.readDocument(_originalText->toString(), _name.to_string());
		} else {
			DefaultDocumentReader docReader;
			zonedDocument = docReader.readDocument(_originalText.get(), _name.to_string(), documentElem.getOptions().input_type);
		}
		if (zonedDocument->_name != _name) {
			std::stringstream ss;
			ss << "Name given for document (" << _name << ") does not match name extracted from document contents (" << zonedDocument->_name << ")";
			documentElem.reportLoadWarning(ss.str());
		}

		// Steal all of zonedDocument's data; and then delete it.
		if (isDTRA)
			_originalText = zonedDocument->_originalText;
		DOWNCASED = zonedDocument->DOWNCASED;
		_metadata = zonedDocument->_metadata; zonedDocument->_metadata = 0;
		_sourceType = zonedDocument->_sourceType; zonedDocument->_sourceType = Symbol();
		_dateTimeField = zonedDocument->_dateTimeField; zonedDocument->_dateTimeField = 0;
		_segments = zonedDocument->_segments; zonedDocument->_segments.clear();
		_n_regions = zonedDocument->_n_regions; zonedDocument->_n_regions = 0;
		_regions = zonedDocument->_regions; zonedDocument->_regions = 0;
		delete zonedDocument;
	}

	// The remaining attributes are all optional.  The default values are the 
	// values they currently have (either from the initializer list, or from 
	// the zoning.  Explicit values (eg for date-time field or for the list of
	// regions) override any value we got from zoning.
	_sourceType = documentElem.getAttribute<Symbol>(X_source_type, _sourceType);
	DOWNCASED = documentElem.getAttribute<bool>(X_is_downcased, DOWNCASED);
	// Original text
	// Date-time field
	if (LocatedString *dateTimeField = documentElem.loadOptionalChildTheory<LocatedString>(X_DateTime)) {
		delete _dateTimeField; // override value from zoning.
		_dateTimeField = dateTimeField;
	}

	if (documentElem.hasAttribute(X_document_time_start) && documentElem.hasAttribute(X_document_time_end)) {
		boost::optional<boost::gregorian::date> startDate;
		boost::optional<boost::posix_time::time_duration> startTimeOfDay;
		boost::optional<boost::local_time::posix_time_zone> startTimeZone;

		TimexUtils::parseDateTime(
			documentElem.getAttribute<std::wstring>(X_document_time_start), 
			startDate, startTimeOfDay, startTimeZone);
		
		if (!startDate || !startTimeOfDay) {
			throw UnexpectedInputException("Document::Document", 
				"Could not parse document_time_start. Must be in non delimited (YYYYMMDDTHHMMSS) or extended ISO string format (YYYY-MM-DDTHH:MM:SS)");
		}

		boost::optional<boost::gregorian::date> endDate;
		boost::optional<boost::posix_time::time_duration> endTimeOfDay;
		boost::optional<boost::local_time::posix_time_zone> endTimeZone;

		TimexUtils::parseDateTime(
			documentElem.getAttribute<std::wstring>(X_document_time_end), 
			endDate, endTimeOfDay, endTimeZone);

		if (!endDate || !endTimeOfDay) {
			throw UnexpectedInputException("Document::Document", 
				"Could not parse document_time_end. Must be in non delimited (YYYYMMDDTHHMMSS) or extended ISO string format (YYYY-MM-DDTHH:MM:SS)");
		}

		boost::posix_time::ptime start(*startDate, *startTimeOfDay);
		boost::posix_time::ptime end(*endDate, *endTimeOfDay);

		_documentTimePeriod = boost::posix_time::time_period(start, end);
		if (startTimeZone)
			_documentTimeZone = startTimeZone;
	}

	// Regions
	if (regionsElem) {
		delete _regions; // override value from zoning.
		std::vector<XMLTheoryElement> regionElems = regionsElem.getChildElementsByTagName(X_Region);
		_n_regions = static_cast<int>(regionElems.size());
		_regions = _new Region*[_n_regions];
		for (int i=0; i<_n_regions; ++i)
			_regions[i] = _new Region(regionElems[i], i);
	}

	//Zones
		XMLTheoryElement zonesElem = documentElem.getOptionalUniqueChildElementByTagName(X_Zones);
		if(zonesElem){
			XMLTheoryElement zoningElem = zonesElem.getOptionalUniqueChildElementByTagName(X_Zoning);
			if(zoningElem){
				_zoning=_new Zoning(zoningElem);		
			}
		}
	
	// Segments
	if (XMLTheoryElement segmentsElem = documentElem.getOptionalUniqueChildElementByTagName(X_Segments)) {
		std::vector<XMLTheoryElement> segmentElems = segmentsElem.getChildElementsByTagName(X_Segment);
		int n_segments = static_cast<int>(segmentElems.size());
		_segments = std::vector<WSegment>(n_segments);
		for (int i=0; i<n_segments; ++i) {
			_loadSegment(_segments[i], segmentElems[i]);
		}
	}
	// Metadata
	if (Metadata *metadata = documentElem.loadOptionalChildTheory<Metadata>(X_Metadata)) {
		delete _metadata; // override value from zoning.
		_metadata = metadata;
	} else if (_metadata == NULL) {
		_metadata = _new Metadata();
	}


	if (DOWNCASED) {
		for (int i = 0; i < _n_regions; i++) 
			_regions[i]->toLowerCase();
	}
}

void Document::setDocumentTimePeriod(boost::posix_time::ptime documentTimeStart, boost::posix_time::ptime documentTimeEnd) {
	_documentTimePeriod = boost::posix_time::time_period(documentTimeStart, documentTimeEnd);
}

void Document::setDocumentTimePeriod(boost::posix_time::ptime documentTimeStart, boost::posix_time::time_duration duration) {
	_documentTimePeriod = boost::posix_time::time_period(documentTimeStart, duration);
}

boost::optional<boost::posix_time::time_period> Document::getDocumentTimePeriod() const {
	return _documentTimePeriod;
}

boost::optional<boost::gregorian::date> Document::getDocumentDate() const {
	if (!_documentTimePeriod)
		return boost::none;

	if (_documentTimePeriod->begin().date() == _documentTimePeriod->end().date())
		return _documentTimePeriod->begin().date();

	return boost::none;
}

void Document::setDocumentTimeZone(boost::local_time::posix_time_zone timeZone) {
	_documentTimeZone = timeZone;
}

boost::optional<boost::local_time::posix_time_zone> Document::getDocumentTimeZone() const {
	return _documentTimeZone;
}

// Turn document date into YYYY-MM-DDTHH:MM:SS or YYYY-MM-DD if 
// possible, or XXXX-XX-XX if not. Used in temporal normalizer and 
// APF output.
std::wstring Document::normalizeDocumentDate() const {
	if (_documentTimePeriod) {
		boost::posix_time::ptime start = _documentTimePeriod->begin();
		boost::posix_time::ptime end = _documentTimePeriod->end();

		if (start == end) { // exact time, use full form YYYY-MM-DDTHH:MM:SS
			std::wstringstream wss;
			wss.imbue(std::locale(std::locale::classic(),       
				new boost::posix_time::wtime_facet(L"%Y-%m-%dT%H:%M:%S")));
			wss << start;
			return wss.str();
		}

		if (getDocumentDate()) { // single day, use just date
			return UnicodeUtil::toUTF16StdString(boost::gregorian::to_iso_extended_string(*getDocumentDate()));
		}
	}

	return L"XXXX-XX-XX";
}
