// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/theories/Document.h"
#include "Generic/common/limits.h"
#include "Generic/common/TimexUtils.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Zone.h"
#include "Generic/theories/Zoning.h"
#include "Generic/reader/RegionSpan.h"
#include "Generic/reader/RegionSpanCreator.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Offset.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/BoostUtil.h"
#include <wchar.h>
#include <boost/regex.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

class Metadata;

Symbol DefaultDocumentReader::AUTO_SYM = Symbol(L"auto");
Symbol DefaultDocumentReader::TEXT_SYM = Symbol(L"text");
Symbol DefaultDocumentReader::SGM_SYM = Symbol(L"sgm");
Symbol DefaultDocumentReader::OSCTEXT_SYM = Symbol(L"osctext");

Symbol DefaultDocumentReader::ERE_WKSP_AUTO_SYM = Symbol(L"events_workshop");
Symbol DefaultDocumentReader::PROXY_SYM = Symbol(L"proxy");

Symbol DefaultDocumentReader::DF_SYM = Symbol(L"discussion_forum");
class DefaultDocumentReader;

DefaultDocumentReader::DefaultDocumentReader() 
: _rawtext_doc_count(0), 
  _n_region_types(0),
  _n_region_breaks(0),
  _n_speaker_types(0),
  _n_receiver_types(0),
  _n_id_types(0),
  _n_datetime_types(0),
  _n_relevantTags(0),
  _regions(1024)
{ 
	int max_param_len = 100;

	// The valid input types, and their effects:
	//   * text: return the document as-is, with the entire contents of the document
	//     in a single <TEXT> region.
	//   * sgm: the document must contian valid SGML.  Use identifyRegions() to create 
	//     a subregion for the contents of any element whose tag is listed in _regionTypes.  
	//     Populate metadata with region spans; and look for a date-time element.
	//   * auto: if checkRelevantTagsParsable() is true, treat the document as "sgm"; 
	//     otherwise, treat it as "text".
	_inputType = ParamReader::getParam(Symbol(L"input_type"));
	if (_inputType.is_null()) _inputType = AUTO_SYM;
	if ((_inputType != TEXT_SYM) && (_inputType != SGM_SYM) && (_inputType != AUTO_SYM) &&
		(_inputType != OSCTEXT_SYM) && (_inputType != ERE_WKSP_AUTO_SYM) && (_inputType != PROXY_SYM) && (_inputType != DF_SYM))
	{
		throw UnexpectedInputException("DefaultDocumentReader::DefaultDocumentReader",
			"Parameter 'input_type' must be set to 'sgm', 'text', 'auto', 'osctext', 'events_workshop', 'proxy' or 'discussion_forum'");
	}

	_use_filename_as_docid = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_filename_as_docid", false);
	_docid_includes_directory_levels = ParamReader::getOptionalIntParamWithDefaultValue("docid_includes_directory_levels",0);

	_calculate_edt_offsets = ParamReader::getRequiredTrueFalseParam("doc_reader_calculate_edt_offsets");
	_doc_reader_replace_invalid_xml_characters = ParamReader::getOptionalTrueFalseParamWithDefaultVal("doc_reader_replace_invalid_xml_characters", false);
	_offsets_start_at_first_tag = ParamReader::getRequiredTrueFalseParam("doc_reader_offsets_start_at_first_tag");

	std::string param = ParamReader::getParam("doc_reader_regions_to_process");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		_n_region_types = convertCommaDelimitedListToSymbolArray(param.c_str(), _regionTypes, MAX_DOC_REGION_TYPES);
	} else {
		_regionTypes[_n_region_types++] = Symbol(L"TEXT");
	}

	/* In order to run Serif on discussion forum data, the following parameters need to be included:
	 *	input_type: discussion_forum
	 *	doc_reader_zones_to_process: The names of the XML tags which we want zones to be created from. E.g.,POST,QUOTE 
	 *	discussion_forum.author_attributes: The attributes' names related to author in the XML tag. E.g.,author, orig_author
	 *	discussion_forum.datetime_attributes: The attributes' names related to datetime in the XML tag. E.g.,datetime
	 * Also needs to override one parameter:
	 *  doc_reader_regions_to_process: needs to include the names from doc_reader_zones_to_process
	 */
	param = ParamReader::getParam("doc_reader_zones_to_process");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		convertCommaDelimitedListToSymbolSet(param.c_str(), _zoneTypes);
	} 


	param = ParamReader::getParam("discussion_forum.author_attributes");
	if (!param.empty()) {
		convertCommaDelimitedListToSymbolSet(param.c_str(), _authorAttributes);
	} 

	param = ParamReader::getParam("discussion_forum.datetime_attributes");
	if (!param.empty()) {
		convertCommaDelimitedListToSymbolSet(param.c_str(), _datetimeAttributes);
	} 


	param = ParamReader::getParam("doc_reader_region_breaks");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		_n_region_breaks = convertCommaDelimitedListToSymbolArray(param.c_str(), _regionBreaks, MAX_DOC_REGION_BREAKS);
	} else {
		_regionBreaks[_n_region_breaks++] = Symbol(L"TURN");
		_regionBreaks[_n_region_breaks++] = Symbol(L"P");
	}

	param = ParamReader::getParam("doc_reader_speaker_tags");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		_n_speaker_types = convertCommaDelimitedListToSymbolArray(param.c_str(), _speakerTypes, MAX_DOC_SPEAKER_TYPES);
	} else {
		_speakerTypes[_n_speaker_types++] = Symbol(L"SPEAKER");
		_speakerTypes[_n_speaker_types++] = Symbol(L"POSTER");
	}

	param = ParamReader::getParam("doc_reader_receiver_tags");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		_n_receiver_types = convertCommaDelimitedListToSymbolArray(param.c_str(), _receiverTypes, MAX_DOC_RECEIVER_TYPES);
	} else {
		_receiverTypes[_n_receiver_types++] = Symbol(L"RECEIVER");
	}

	param = ParamReader::getParam("doc_reader_id_tags");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		_n_id_types = convertCommaDelimitedListToSymbolArray(param.c_str(), _docIDTypes, MAX_DOC_ID_TYPES);
	} else {
		_docIDTypes[_n_id_types++] = Symbol(L"DOCNO");
		_docIDTypes[_n_id_types++] = Symbol(L"DOCID");
	}

	param = ParamReader::getParam("doc_reader_datetime_tags");
	if (!param.empty()) {
		boost::to_upper(param); // We uppercase since SGMLTag::findNextSGMLTag does
		_n_datetime_types = convertCommaDelimitedListToSymbolArray(param.c_str(), _docDateTimeTypes, MAX_DOC_DATETIME_TYPES);
	} else {
		_docDateTimeTypes[_n_datetime_types++] = Symbol(L"DATETIME");
	}

	// initialize full array of relevant tags
	int i;
	for (i = 0; i < _n_region_types; i++)
		_relevantTags[_n_relevantTags++] = _regionTypes[i];
	for (i = 0; i < _n_id_types; i++)
		_relevantTags[_n_relevantTags++] = _docIDTypes[i];
	for (i = 0; i < _n_datetime_types; i++)
		_relevantTags[_n_relevantTags++] = _docDateTimeTypes[i];

}

Document* DefaultDocumentReader::readDocument(InputStream &stream, const wchar_t * filename) {
	return readDocument(stream, filename, _inputType);
}

Document* DefaultDocumentReader::readDocument(const LocatedString *source, const wchar_t * filename) {
	return readDocument(source, filename, _inputType);
}

Document* DefaultDocumentReader::readDocument(InputStream &stream, const wchar_t * filename, Symbol inputType) {
	LocatedString source(stream);
	handleInvalidXmlCharacters(&source);  // Handle invalid XML characters
	return readDocument(&source, filename, inputType);
}

Document* DefaultDocumentReader::readDocument(const LocatedString *source, const wchar_t * filename, Symbol inputType) {
	// Default value:
	if (inputType.is_null())
		inputType = _inputType;

	_regions.clear();
	Document *result = 0;
	Symbol name = Symbol();
	Symbol doctype = Symbol(L"UNKNOWN");
	LocatedString *datetimeString = 0;
	SGMLTag tag;
	int i = 0;

	try {
		if (inputType == AUTO_SYM) {
			if (checkRelevantTagsParsable(*source, filename, false))
				inputType = SGM_SYM;
			else
				inputType = TEXT_SYM;
		}

		if (inputType == TEXT_SYM) {
			if (_use_filename_as_docid) {
				name = cleanFileName(filename);
				name = removeFileExtension(name.to_string());
			} else {	
				name = getNextRawTextDocName();
			}
		}
		else if (inputType == OSCTEXT_SYM){ 
			if (_use_filename_as_docid) {
				name = cleanFileName(filename);
				name = removeFileExtension(name.to_string());
			} else {	
				name = getNextRawTextDocName();
			}
			
			//get a date and 
			/*   - The information labeled as "DOCCOUNTRY" and "INFODATE" at the top
					of the documents was displayed as unmarkable meta-data for
					annotators. 	
			*/
			
			int datepos = source->indexOf(L"INFODATE:");
			if(datepos == -1){
				throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
					"INFODATE is not present in  ", name.to_debug_string());
			} 
			else if(source->indexOf(L"INFODATE", datepos+1) > -1){
				throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
					"Multipel INFODATE fields in  ", name.to_debug_string());
			}
			//get the actual date
			int soffset = datepos + 9;
			while(iswspace(source->charAt(soffset)))
				soffset++;
			datetimeString = source->substring(soffset, soffset+8);
		}
		else if (inputType == ERE_WKSP_AUTO_SYM){ 
			if (_use_filename_as_docid) {
				name = cleanFileName(filename);
				name = removeFileExtension(name.to_string());
			} else {	
				name = getNextRawTextDocName();
			}

			// put a dummy date: date is not particularly interesteing for this type of files.
			datetimeString = new LocatedString(L"20990101");
		}
		else if (inputType == PROXY_SYM){ 
			if (_use_filename_as_docid) {
				name = cleanFileName(filename);
				name = removeFileExtension(name.to_string());
			} else {	
				name = getNextRawTextDocName();
			}
			
			int datepos = source->indexOf(L"DATE:");
			if(datepos == -1){
				throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
					"DATE is not present in  ", name.to_debug_string());
			} 
			else if(source->indexOf(L"INFODATE", datepos+1) > -1){
				throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
					"Multipel DATE fields in  ", name.to_debug_string());
			}
			//get the actual date
			int soffset = datepos + 5;
			while(iswspace(source->charAt(soffset)))
				soffset++;
			datetimeString = source->substring(soffset, soffset+10);
		}

		else if (inputType == DF_SYM){ 
			if (_use_filename_as_docid) {
				name = cleanFileName(filename);
				name = removeFileExtension(name.to_string());
			} else {	
				name = getNextRawTextDocName();
			}
			//datetimeString = source->substring(soffset, soffset+10);
			//what should we set the datetimeString here? because there is no date for one document
		}
		else {
			checkRelevantTagsParsable(*source, filename, true);

			// Find the file ID.
			if (_use_filename_as_docid) {
				name = cleanFileName(filename);
				name = removeFileExtension(name.to_string());
			} else {
				for (i = 0; i < _n_id_types; i++) {
					tag = SGMLTag::findOpenTag(*source, _docIDTypes[i], 0);
					if (!tag.notFound()) {
						int end_open_tag = tag.getEnd();
						tag = SGMLTag::findCloseTag(*source, _docIDTypes[i], end_open_tag);
						if (tag.notFound()) {
							std::wstring filename_as_wstring(filename);		
							std::string filename_as_string(filename_as_wstring.begin(), filename_as_wstring.end());
							throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
								"Document id tag not terminated in file ", filename_as_string.c_str());
						}
						int start_close_tag = tag.getStart();
						LocatedString *nameString = source->substring(end_open_tag, start_close_tag);
						nameString->trim();
						name = nameString->toSymbol();
						delete nameString;
						break;	
					}
				}
			}
			if (name.is_null()) { // If we didn't find a file ID, look for the MT SGM style docid
				tag = SGMLTag::findOpenTag(*source, Symbol(L"DOC"), 0); // Look for the MT SGM style genre
				if (!tag.notFound() && !tag.getAttributeValue(L"docid").is_null()) {
					name = tag.getAttributeValue(L"docid");
				}
				else if (!tag.notFound() && !tag.getAttributeValue(L"id").is_null()) {
					name = tag.getAttributeValue(L"id");
				}
				else { // If we still didn't find a file ID, default to using the filename
					name = cleanFileName(filename);
					name = removeFileExtension(name.to_string());
				}
			}
			if (_docid_includes_directory_levels)
				name = keepDirectoryLevels(name, filename);

			// Find the source type
			tag = SGMLTag::findOpenTag(*source, Symbol(L"DOCTYPE"), 0);
			if(!tag.notFound() && !tag.getAttributeValue(L"SOURCE").is_null()){
				doctype = tag.getAttributeValue(L"SOURCE");
			} else {
				tag = SGMLTag::findOpenTag(*source, Symbol(L"DOC"), 0); // Look for the MT SGM style genre
				if(!tag.notFound() && !tag.getAttributeValue(L"genre").is_null()) {
					doctype = tag.getAttributeValue(L"genre");
				}
			} 
		}

		// create the new document object
		result = _new Document(name);
		if (!(_calculate_edt_offsets || _offsets_start_at_first_tag) || !(inputType == DF_SYM || inputType == SGM_SYM))
			// When not using EDT offsets, skipping to first tag, or for formats that don't call identifyRegions, need OriginalText first
			result->setOriginalText(source);
		if (inputType == DF_SYM) {
			result->setSourceType(DF_SYM);
		} else {			
			std::wstring default_source_type = ParamReader::getWParam("default_source_type");
			if (doctype == Symbol(L"UNKNOWN") && !default_source_type.empty()) {
				result->setSourceType(Symbol(default_source_type));
			} else {
				result->setSourceType(doctype);
			}
		}
		if (inputType == OSCTEXT_SYM){
			LocatedString* region_str = 0;
			int next_offset = getNextFBISRegion(source, 0, region_str);
			int rno = 0;
			while(region_str != 0){
				_regions.push_back(_new Region(result, _regionTypes[0], rno, region_str));
				delete region_str;
				next_offset = getNextFBISRegion(source, next_offset, region_str);
				rno++; // incremental region number
			}
			result->takeRegions(_regions);
		}
		else if (inputType == ERE_WKSP_AUTO_SYM) {
			if(!checkXmlHasText(source)) { // text
				LocatedString *region_str = _new LocatedString(*source); 
				processRawText(region_str);
				cleanRegion(region_str, inputType);
				_regions.push_back(_new Region(result, _regionTypes[0], 0, region_str));
				result->takeRegions(_regions);
				delete region_str;
			}
			else { // XML file which has a <text> region
				int startPos = source->indexOf(L"<text ");
				if(startPos < 0)
					throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
						"no <text in file");

				int endPos = source->indexOf(L"</text>");
				if(endPos < 0)
					throw UnexpectedInputException("DefaultDocumentReader::readDocument()", 
						"no </text> in file");

				LocatedString *region_str = source->substring(startPos+6, endPos);

				processRawText(region_str);
				cleanRegion(region_str, inputType);
				_regions.push_back(_new Region(result, _regionTypes[0], 0, region_str));
				result->takeRegions(_regions);
				delete region_str;
			}
		}
		else if (inputType == PROXY_SYM) {
//			printAllRegexMatches();
			LocatedString* region_str = 0;
			int next_offset = getNextProxyRegion(source, 0, region_str);
			int rno = 0;
			while(region_str != 0){
				_regions.push_back(_new Region(result, _regionTypes[0], rno, region_str));
				delete region_str;
				next_offset = getNextProxyRegion(source, next_offset, region_str);
				rno++;
			}
			result->takeRegions(_regions);
		}
		else if (inputType == DF_SYM) {
			identifyRegions(result, source, inputType);
			datetimeString = findDateTimeTag(result);
			identifyDFZones(result, result->getOriginalText(), inputType);
			
		}
		else if (inputType == TEXT_SYM) {
			LocatedString *region_str = _new LocatedString(*source); 
			processRawText(region_str);
			cleanRegion(region_str, inputType);
			Region* region = _new Region(result, _regionTypes[0], 0, region_str);
			if (isJustifiedText(region_str)) {
				if (isDoubleSpacedText(region_str)) {
					region->addContentFlag(Region::DOUBLE_SPACED);
				}
				region->addContentFlag(Region::JUSTIFIED);
			}
			_regions.push_back(region);
			result->takeRegions(_regions);
			delete region_str;
		} else if (inputType == SGM_SYM) {
			identifyRegions(result, source, inputType);
			datetimeString = findDateTimeTag(result);
		} else {
			// We shouldn't ever get here after previous checks
			throw UnexpectedInputException("DefaultDocumentReader::readDocument()", "Unknown input_type parameter");
		}

		// DocumentTime stored as posix time object
		if (datetimeString) {
			result->setDateTimeField(datetimeString);
			std::wstring timeStr(datetimeString->toString());
			boost::optional<boost::gregorian::date> date;
			boost::optional<boost::posix_time::time_duration> timeOfDay;
			boost::optional<boost::local_time::posix_time_zone> timeZone;

			TimexUtils::parseDateTime(timeStr, date, timeOfDay, timeZone);

			if (date) {
				if (timeOfDay) {
					boost::posix_time::ptime ptime(*date, *timeOfDay);
					result->setDocumentTimePeriod(ptime, ptime);
				} else {
					boost::posix_time::ptime start(*date);
					result->setDocumentTimePeriod(start, boost::posix_time::seconds(86399)); // start of day, end of day
				}
				if (timeZone) {
					result->setDocumentTimeZone(*timeZone);
				}
			}
		}
		// Could not find good date region, try the docid
		if (!result->getDocumentTimePeriod()) {
			std::string isoString = UnicodeUtil::toUTF8StdString(TimexUtils::extractDateFromDocId(result->getName())) + "T0";
			try {
				boost::posix_time::ptime ptime;
				ptime = boost::posix_time::from_iso_string(isoString);
				result->setDocumentTimePeriod(ptime, boost::posix_time::seconds(86399)); // start of day, end of day
			} catch (...)  {
				// bad date
			}
		}
	} catch(UnrecoverableException &) {
		delete datetimeString;
		throw;
	}

	delete datetimeString;

	return result;
}

// Identify the internal regions of the document
void DefaultDocumentReader::identifyRegions(Document *document, const LocatedString *source, Symbol inputType) {
	Metadata *metadata = document->getMetadata();
	Symbol regionSpanSymbol = Symbol(L"REGION_SPAN");
	metadata->addSpanCreator(regionSpanSymbol, _new RegionSpanCreator());

	checkRelevantTagsParsable(*source, document->getName().to_string(), true);

	std::vector<RegionInfo> regionBlocks;
	std::vector<RegionInfo> spanBlocks;
	std::list<LocatedString::OffsetEntry> offsets;
	SGMLTag tag;
	int start = 0;
	int n_blocks = 0;
	bool seenUnskippedRegion = false;
	if (_offsets_start_at_first_tag) {
		tag = SGMLTag::findNextSGMLTag(*source, start);
		if (!tag.notFound() && tag.getStart() > start) {
			// accumulate byte offset up to first tag
			int byteOffset;
			int c;
			for (byteOffset = 0, c = 0; c < tag.getStart(); c++) {
				byteOffset += UnicodeUtil::countUTF8Bytes(source->charAt(c));
			}

			// need to insert an entry for the skipped content before first tag
			offsets.push_back(LocatedString::OffsetEntry());
			LocatedString::OffsetEntry &firstEntry = offsets.back();
			firstEntry.endPos = tag.getStart();
			firstEntry.startOffset = OffsetGroup(ByteOffset(0), CharOffset(0), EDTOffset(0), ASRTime());
			firstEntry.endOffset = OffsetGroup(ByteOffset(byteOffset - 1), CharOffset(firstEntry.endPos - 1), EDTOffset(0), ASRTime());
			firstEntry.is_edt_skip_region = true;

			// update the tag search start so the normal region finding starts with the first tag
			start = tag.getStart();
		}
	}
	do {
		tag = SGMLTag::findNextSGMLTag(*source, start);
		if (_calculate_edt_offsets && !tag.notFound()) {
			// accumulate byte offset up to current position
			int byteOffset;
			int c;
			for (byteOffset = 0, c = 0; c < tag.getStart(); c++) {
				byteOffset += UnicodeUtil::countUTF8Bytes(source->charAt(c));
			}

			// determine EDT offset up to current position
			EDTOffset startEDTOffset;
			if (tag.getStart() == 0)
				// tag at beginning of document, so inclusive EDT offset is 0
				startEDTOffset = EDTOffset(0);
			else if (start == 0) {
				// need to insert an entry for the unskipped content before first tag
				offsets.push_back(LocatedString::OffsetEntry());
				LocatedString::OffsetEntry &firstEntry = offsets.back();
				firstEntry.endPos = tag.getStart();
				firstEntry.startOffset = OffsetGroup(ByteOffset(0), CharOffset(0), EDTOffset(0), ASRTime());
				firstEntry.endOffset = OffsetGroup(ByteOffset(byteOffset - 1), CharOffset(firstEntry.endPos - 1), EDTOffset(firstEntry.endPos - 1), ASRTime());
				firstEntry.is_edt_skip_region = false;
				startEDTOffset = EDTOffset(tag.getStart());
			} else if (tag.getStart() == start) {
				// tag is immediately adjacent to previous tag, no gap in EDT offsets
				LocatedString::OffsetEntry &prevEntry = offsets.back();
				startEDTOffset = prevEntry.startOffset.edtOffset;
			} else {
				// need to insert unskipped offsets between tags
				LocatedString::OffsetEntry prevEntry = offsets.back();
				offsets.push_back(LocatedString::OffsetEntry());
				LocatedString::OffsetEntry &unskippedEntry = offsets.back();
				unskippedEntry.startPos = start;
				unskippedEntry.endPos = tag.getStart();
				unskippedEntry.is_edt_skip_region = false;
				startEDTOffset = prevEntry.endOffset.edtOffset;
				if (seenUnskippedRegion)
					// first skip region(s) gets offset 0, others get end offset of preceding unskipped region
					++startEDTOffset;
				unskippedEntry.startOffset = OffsetGroup(ByteOffset(prevEntry.endOffset.byteOffset.value() + 1), CharOffset(unskippedEntry.startPos), startEDTOffset, ASRTime());
				startEDTOffset = EDTOffset(startEDTOffset.value() + unskippedEntry.endPos - unskippedEntry.startPos - 1);
				unskippedEntry.endOffset = OffsetGroup(ByteOffset(byteOffset - 1), CharOffset(unskippedEntry.endPos - 1), startEDTOffset, ASRTime());
				seenUnskippedRegion = true;
			}

			// create the entry for this skipped tag
			offsets.push_back(LocatedString::OffsetEntry());
			LocatedString::OffsetEntry &tagEntry = offsets.back();
			tagEntry.startPos = tag.getStart();
			tagEntry.endPos = tag.getEnd();
			tagEntry.is_edt_skip_region = true;
			tagEntry.startOffset = OffsetGroup(ByteOffset(byteOffset), CharOffset(tagEntry.startPos), startEDTOffset, ASRTime());
			for (; c < tag.getEnd(); c++) {
				byteOffset += UnicodeUtil::countUTF8Bytes(source->charAt(c));
			}
			tagEntry.endOffset = OffsetGroup(ByteOffset(byteOffset - 1), CharOffset(tagEntry.endPos - 1), startEDTOffset, ASRTime());
		}
		start = tag.getEnd();
		if (tag.isOpenTag()) {
			// look for enclosed region <x>...</x>
			int i = 0;
			while (i < _n_region_types && tag.getName() != _regionTypes[i]) {
				i++;
			}
			if (i < _n_region_types) {
				// make sure we have a matching close tag
				SGMLTag closeTag = SGMLTag::findCloseTag(*source, tag.getName(), start);
				if (closeTag.notFound())
					continue;

				// figure out where new block should be inserted, start at back of list (n_blocks).
				// shift each block back one slot, until we reach the insert location.
				int insert = n_blocks;  
				while (insert > 0 && regionBlocks[insert-1].start > start) {
					if ((int)regionBlocks.size() == insert) {
						regionBlocks.push_back(RegionInfo());
					}
					regionBlocks[insert].open_tag_length = regionBlocks[insert-1].open_tag_length;
					regionBlocks[insert].close_tag_length = regionBlocks[insert-1].close_tag_length;
					regionBlocks[insert].start = regionBlocks[insert-1].start;
					regionBlocks[insert].end = regionBlocks[insert-1].end;
					regionBlocks[insert].tag = regionBlocks[insert-1].tag;
					insert--;
				}
				// insert the new block
				if ((int)regionBlocks.size() == insert) {
					regionBlocks.push_back(RegionInfo());
				}
				regionBlocks[insert].start = start;
				regionBlocks[insert].tag = tag.getName();
				regionBlocks[insert].open_tag_length = tag.getEnd() - tag.getStart();
				tag = SGMLTag::findCloseTag(*source, _regionTypes[i], tag.getEnd());
				regionBlocks[insert].close_tag_length = tag.getEnd() - tag.getStart();
				int end = tag.getStart();
				regionBlocks[insert].end = end;
				n_blocks++;

				// Handle case where tag is nested inside the existing block (insert-1)
				// Result should be:
				// <old> ... <new> ... </new> ... </old> => <1> ... </1><2> ... </2><3> ... </3>
				// <1> ... </1> stored at index 'insert-1'
				// <2> ... </2> at index 'insert'
				// <3> ... </3> at index 'insert+1'
				if (insert > 0 && regionBlocks[insert-1].end > regionBlocks[insert].start) {
					// check for invalid nesting, e.g. <old> ... <new> ... </old> ... </new> 
					if (regionBlocks[insert-1].end < regionBlocks[insert].end) {
						throw UnexpectedInputException("DefaultDocumentReader::identifyRegions()", 
											"Found overlapping, non-nested text regions in file ",
											document->getName().to_debug_string());
					}
					// Open up an empty slot for the region between </new> and </old> (<3>...</3>)
					for (int m = n_blocks; m > insert+1; m--) {
						if ((int)regionBlocks.size() == m) {
							regionBlocks.push_back(RegionInfo());
						}
						regionBlocks[m].open_tag_length = regionBlocks[m-1].open_tag_length;
						regionBlocks[m].close_tag_length = regionBlocks[m-1].close_tag_length;
						regionBlocks[m].start = regionBlocks[m-1].start;
						regionBlocks[m].end = regionBlocks[m-1].end;
						regionBlocks[m].tag = regionBlocks[m-1].tag;
					}
					// Insert the new region (<3>...</3>)
					if ((int)regionBlocks.size() == (insert+1)) {
						regionBlocks.push_back(RegionInfo());
					}
					regionBlocks[insert+1].open_tag_length = regionBlocks[insert].close_tag_length;
					regionBlocks[insert+1].close_tag_length = regionBlocks[insert-1].close_tag_length;
					regionBlocks[insert+1].start = regionBlocks[insert].end + regionBlocks[insert].close_tag_length;
					regionBlocks[insert+1].end = regionBlocks[insert-1].end;
					regionBlocks[insert+1].tag = regionBlocks[insert-1].tag;

					// Fix the end info for old region (<1>...</1>) 
					regionBlocks[insert-1].end = regionBlocks[insert].start - regionBlocks[insert].open_tag_length;
					regionBlocks[insert-1].close_tag_length = regionBlocks[insert].open_tag_length;
					n_blocks++;
					
				}
				
				// add a metadata span for this block
				spanBlocks.push_back(RegionInfo());
				RegionInfo &spanBlock = spanBlocks.back();
				spanBlock.tag = tag.getName();
				spanBlock.start = start;
				spanBlock.end = end;
			}
			// look for single region break tag i.e. <turn>
			else if (_n_region_breaks > 0) {
				int j = 0;
				while (j < _n_region_breaks && tag.getName() != _regionBreaks[j]) {
					j++;
				}
				if (j < _n_region_breaks) {
					if (n_blocks > 0) {
						// look for a region that encloses this tag, otherwise no need to break
						// e.g.  <x> ... <y> ... </x>
						int n = n_blocks - 1;
						while (n >= 0 && !(regionBlocks[n].start < start && regionBlocks[n].end > start)) {
							n--;
						}
						// Found an enclosing block, so we need to fix it.
						// Result should look like:
						// <x> ... </x><x> ... </x>
						if (n >= 0) {
							// figure out where new block should be inserted, start at back of list (n_blocks).
							// shift each block back one slot, until we reach the insert location.
							int insert = n_blocks;
							while (regionBlocks[insert-1].start > start) {
								if ((int)regionBlocks.size() == insert) {
									regionBlocks.push_back(RegionInfo());
								}
								regionBlocks[insert].open_tag_length = regionBlocks[insert-1].open_tag_length;
								regionBlocks[insert].close_tag_length = regionBlocks[insert-1].close_tag_length;
								regionBlocks[insert].start = regionBlocks[insert-1].start;
								regionBlocks[insert].end = regionBlocks[insert-1].end;
								regionBlocks[insert].tag = regionBlocks[insert-1].tag;
								insert--;
							}
							// insert the new block
							int start_tag = tag.getStart();
							int end_tag = tag.getEnd();
							if ((int)regionBlocks.size() == insert) {
								regionBlocks.push_back(RegionInfo());
							}
							regionBlocks[insert].open_tag_length = end_tag - start_tag;
							regionBlocks[insert].close_tag_length = regionBlocks[n].close_tag_length;
							regionBlocks[insert].start = end_tag;
							regionBlocks[insert].end = regionBlocks[n].end;
							regionBlocks[insert].tag = regionBlocks[n].tag;
							
							// update the enclosing block
							if ((int)regionBlocks.size() == n) {
								regionBlocks.push_back(RegionInfo());
							}
							regionBlocks[n].end = start_tag;
							regionBlocks[n].close_tag_length = regionBlocks[insert].open_tag_length;
							n_blocks++;
						}
					}
				}
			}
		}		
	} while (!tag.notFound());

	if (offsets.size() > 0) {
		// calculate the last entry of the string if necessary
		LocatedString::OffsetEntry prevEntry = offsets.back();
		if (prevEntry.endPos != source->length()) {
			// need to insert an entry for the unskipped content after last tag
			offsets.push_back(LocatedString::OffsetEntry());
			LocatedString::OffsetEntry &lastEntry = offsets.back();
			lastEntry.startPos = prevEntry.endPos;
			lastEntry.endPos = source->length();
			EDTOffset startEDTOffset = prevEntry.endOffset.edtOffset;
			if (seenUnskippedRegion)
				// first skip region gets offset 0, others get end offset of preceding unskipped region
				++startEDTOffset;
			lastEntry.startOffset = OffsetGroup(ByteOffset(prevEntry.endOffset.byteOffset.value() + 1), CharOffset(lastEntry.startPos), startEDTOffset, ASRTime());
			EDTOffset endEDTOffset = EDTOffset(startEDTOffset.value() + lastEntry.endPos - lastEntry.startPos - 1);
			int byteOffset = 0;
			for (int c = 0; c < source->length(); c++) {
				byteOffset += UnicodeUtil::countUTF8Bytes(source->charAt(c));
			}
			lastEntry.endOffset = OffsetGroup(ByteOffset(byteOffset - 1), CharOffset(source->length() - 1), endEDTOffset, ASRTime());
			lastEntry.is_edt_skip_region = false;
		}
	}

	if (_calculate_edt_offsets) {
		// skip carriage returns if necessary
		std::list<LocatedString::OffsetEntry>::iterator offset_i = offsets.begin();
		int byteOffset = 0;
		for (int c = 0; c < source->length(); c++) {
			if (source->charAt(c) == L'\r') {
				// get iterator to the offset entry containing this carriage return
				while (offset_i->startPos < c && offset_i->endPos <= c)
					++offset_i;

				// determine if we need to split this entry or just add an adjacent one
				std::list<LocatedString::OffsetEntry>::iterator prev_offset_i;
				std::list<LocatedString::OffsetEntry>::iterator return_offset_i;
				EDTOffset return_edt_offset(0);
				if (c == offset_i->startPos) {
					// need to break the \r off into a new entry before this one
					if (offset_i != offsets.begin()) {
						// if this wasn't the first entry, \r gets previous entry's EDTOffset, otherwise 0
						prev_offset_i = offset_i;
						--prev_offset_i;
						return_edt_offset = prev_offset_i->endOffset.edtOffset;
					}
					return_offset_i = offsets.insert(offset_i, LocatedString::OffsetEntry());

					// this entry that started with \r has its start incremented by one character
					offset_i->startPos++;
					++(offset_i->startOffset.byteOffset);
					++(offset_i->startOffset.charOffset);
					++(offset_i->startOffset.edtOffset);
				} else if (c == offset_i->endPos) {
					// need to break the \r off into a new entry after this one
					//   this case is unlikely because \r should be followed by \n unless we have a pre-Mac OS X file
					prev_offset_i = offset_i;
					++offset_i;
					return_offset_i = offsets.insert(offset_i, LocatedString::OffsetEntry());

					// this entry that ended with \r has its end decremented by one character
					prev_offset_i->endPos--;
					--(prev_offset_i->endOffset.byteOffset);
					--(prev_offset_i->endOffset.charOffset);
					--(prev_offset_i->endOffset.edtOffset);
					return_edt_offset = prev_offset_i->endOffset.edtOffset;
				} else {
					// need to insert two new entries, one for the \r and one for the rest of this entry
					prev_offset_i = offset_i;
					++offset_i;
					return_offset_i = offsets.insert(offset_i, LocatedString::OffsetEntry());
					std::list<LocatedString::OffsetEntry>::iterator next_offset_i = offsets.insert(offset_i, LocatedString::OffsetEntry());

					// the \r will get the EDT offset of the previous character
					return_edt_offset = EDTOffset(prev_offset_i->startOffset.edtOffset.value() + c - prev_offset_i->startPos - 1);

					// the new end entry that comes after the \r will inherit the old entry's end offsets
					//   the start EDT offset starts 1 higher because of decrement pass later in this loop
					next_offset_i->startPos = c + 1;
					next_offset_i->endPos = prev_offset_i->endPos;
					next_offset_i->startOffset = OffsetGroup(ByteOffset(byteOffset + 1), CharOffset(c + 1), EDTOffset(return_edt_offset.value() + 2), ASRTime());
					next_offset_i->endOffset = prev_offset_i->endOffset;
					next_offset_i->is_edt_skip_region = prev_offset_i->is_edt_skip_region;

					// this entry that contained an \r has its end set to precede the \r
					prev_offset_i->endPos = c;
					prev_offset_i->endOffset = OffsetGroup(ByteOffset(byteOffset - 1), CharOffset(c - 1), return_edt_offset, ASRTime());

					// next iteration will start with the new remainder of this entry
					offset_i = next_offset_i;
				}

				// set the offsets for the carriage return
				OffsetGroup return_offset(ByteOffset(byteOffset), CharOffset(c), return_edt_offset, ASRTime());
				return_offset_i->startPos = c;
				return_offset_i->endPos = c + 1;
				return_offset_i->startOffset = return_offset;
				return_offset_i->endOffset = return_offset;
				return_offset_i->is_edt_skip_region = true;

				// decrement the EDT offsets for all of the following entries
				for (std::list<LocatedString::OffsetEntry>::iterator update_offset_i = offset_i; update_offset_i != offsets.end(); ++update_offset_i) {
					--(update_offset_i->startOffset.edtOffset);
					--(update_offset_i->endOffset.edtOffset);
				}
			}

			// accumulate bytes-per-character as we search for \r
			byteOffset += UnicodeUtil::countUTF8Bytes(source->charAt(c));
		}
	}

	if (offsets.size() > 0) {
		// store the updated locatedstring (this can only happen once)
		document->setOriginalText(_new LocatedString(*source, offsets));
	} else if (document->getOriginalText() == NULL) {
		// If we get here without an original text, we didn't have to modify offsets, so just use the specified source.
		document->setOriginalText(source);
	}

	// create the region Spans
	BOOST_FOREACH(RegionInfo spanBlock, spanBlocks) {
		EDTOffset spanStart = document->getOriginalText()->start<EDTOffset>(spanBlock.start);
		EDTOffset spanEnd = document->getOriginalText()->end<EDTOffset>(spanBlock.end);
		metadata->newSpan(regionSpanSymbol, spanStart, spanEnd, &spanBlock.tag);
	}

	// create the final Region objects
	if (n_blocks > 0) {
		int num_regions = 0;
		int j = 0;
		do {
			if (j == n_blocks - 1 || regionBlocks[j].end < regionBlocks[j+1].start) {
				int start = regionBlocks[j].start;
				int end = regionBlocks[j].end;

				// Make sure this region isn't empty.
				if (start != end) {
					// Make sure this region contains at least one valid EDT character. 
					// This may not be the case when only SGML tags (which don't have 
					// EDT offsets) are present.
					if (start >= end) {
						LocatedString *region_str = document->getOriginalText()->substring(regionBlocks[j].start, regionBlocks[j].end);
						SessionLogger::warn("ignore_region")
							<< "Ignoring region without any valid EDT characters: \n"
							<< region_str->toString() << "\n";
					} 
					else {
						LocatedString *region_str = document->getOriginalText()->substring(start, end);
						cleanRegion(region_str, inputType);
						// Make sure again that the region isn't empty after the clean up.
						if (region_str->length() > 0) {
							_regions.push_back(_new Region(document, regionBlocks[j].tag, num_regions, region_str));
							if (isSpeakerTag(_regions[num_regions]->getRegionTag())) {
								_regions[num_regions]->setSpeakerRegion(true);
							}
							if (isReceiverTag(_regions[num_regions]->getRegionTag())) {
								_regions[num_regions]->setReceiverRegion(true);
							}
							if (isJustifiedText(region_str)) {
								if (isDoubleSpacedText(region_str)) {
									_regions[num_regions]->addContentFlag(Region::DOUBLE_SPACED);
								}
								_regions[num_regions]->addContentFlag(Region::JUSTIFIED);
							}
							num_regions++;
						} 
						delete region_str;
					}
				}
			}
			else {
				throw UnexpectedInputException("DefaultDocumentReader::identifyRegions()", 
												"Found overlapping or improperly-nested text regions in file ",
												document->getName().to_debug_string());
			}
			j++;
		} while (j < n_blocks);
	}

	// Give ownership of the resulting array of Regions to the Document object
	document->takeRegions(_regions);
}

LocatedString* DefaultDocumentReader::findDateTimeTag(Document* document) const {
	for (int i = 0; i < _n_datetime_types; i++) {
		SGMLTag tag = SGMLTag::findOpenTag(*(document->getOriginalText()), _docDateTimeTypes[i], 0);
		if (!tag.notFound()) {
			int end_open_tag = tag.getEnd();
			tag = SGMLTag::findCloseTag(*(document->getOriginalText()), _docDateTimeTypes[i], end_open_tag);
			if (tag.notFound()) {
				throw UnexpectedInputException("DefaultDocumentReader::findDateTimeTag()", 
					"Date/time tag not terminated in document", document->getName().to_debug_string());
			}
			int start_close_tag = tag.getStart();
			LocatedString* datetimeString = document->getOriginalText()->substring(end_open_tag, start_close_tag);
			datetimeString->trim();
			return datetimeString;	
		}
	}
	return NULL;
}

bool DefaultDocumentReader::checkRelevantTagsParsable(const LocatedString& source, const wchar_t * filename,
													  bool throwExceptionIfNotParsable)
{
	int parsableDepth;
	try {
		parsableDepth = SGMLTag::isParsableSGML(source, _relevantTags, _n_relevantTags);
	} catch (UnexpectedInputException &e) {
		if (throwExceptionIfNotParsable) {
			throw e;
		}
		return false;
	}

	if (parsableDepth > 0) {
		return true;
	} else if (parsableDepth == 0) {
		if (throwExceptionIfNotParsable) {
			std::stringstream errmsg;
			errmsg << "This document contains no SGML tags that represent parsable content. Valid tags are: ";
			for (int i = 0; i < _n_region_types; i++) {
				if (i != 0) {
					errmsg << ", ";
					if (i == _n_region_types - 1) {
						errmsg << "and ";
					}
				}
				errmsg << _regionTypes[i];
			}
			errmsg << ". ";
			errmsg << "Please check the contents of the document, add a new tag to be processed via the doc_reader_regions_to_process parameter, or (if your document has no SGML tags) switch your input_type parameter to 'text'.";
			throw UnexpectedInputException("DefaultDocumentReader::checkRelevantTagsParsable()", errmsg.str().c_str());
		}
		return false;
	}

	// Should never happen; this will get returned as an exception instead, which is caught above.
	return false;
}

// check if it is a XML file and if it contains a <text></text> region
bool DefaultDocumentReader::checkXmlHasText(const LocatedString* source)
{
	int datepos = source->indexOf(L"<?xml version=");
	if(datepos!=0)
		return false;

	datepos = source->indexOf(L"<text");
	if(datepos < 0)
		return false;

	return true;
}

void DefaultDocumentReader::cleanRegion(LocatedString *region, Symbol inputType) {
	SGMLTag openTag;
	Symbol W_SYM = Symbol(L"W");
	Symbol ASR_SYM = Symbol(L"asr");

	// Remove any annotations.
	do {
		openTag = SGMLTag::findOpenTag(*region, Symbol(L"ANNOTATION"), 0);
		if (!openTag.notFound()) {
			SGMLTag closeTag = SGMLTag::findCloseTag(*region, Symbol(L"ANNOTATION"), openTag.getEnd());
			if (closeTag.notFound()) {
				region->remove(openTag.getStart(), region->length());
				return;
			}
			// Remove the annotation.
			region->remove(openTag.getStart(), closeTag.getEnd());
		}
	} while (!openTag.notFound());

	//remove everything in ignore tags 
	do {
		openTag = SGMLTag::findOpenTag(*region, Symbol(L"IGNORE"), 0);
		if (!openTag.notFound()) {
			SGMLTag closeTag = SGMLTag::findCloseTag(*region, Symbol(L"IGNORE"), openTag.getEnd());
			if (closeTag.notFound()) {
				region->remove(openTag.getStart(), region->length());
				return;
			}
			// Remove ignores.
			region->remove(openTag.getStart(), closeTag.getEnd());
		}
	} while (!openTag.notFound());

	// Remove any other tags whatsoever.
	SGMLTag next;
	do {
		next = SGMLTag::findNextSGMLTag(*region, 0);
		if (inputType == ASR_SYM && next.getName() == W_SYM) {
			Symbol startSymbol = next.getAttributeValue(L"Bsec");
			Symbol durSymbol = next.getAttributeValue(L"Dur");

			if (startSymbol.is_null() || durSymbol.is_null()) {
				throw UnexpectedInputException("DefaultDocumentReader::cleanRegion",
					"Could not find Bsec and Dur attribute in sgml tag");
			}
			
			ASRTime start((float)atof(startSymbol.to_debug_string()));
			float dur = (float)atof(durSymbol.to_debug_string());
			ASRTime end(start.value() + dur);

			for (int i = next.getEnd() + 1; i < region->length(); i++) {
				region->setAsrStartTime(i, start);
				region->setAsrEndTime(i, end);
			}
		}
	

		region->remove(next.getStart(), next.getEnd());
	} while (!next.notFound());

	
	// Replace recognized SGML entities.
	region->replace(L"&LR;", L"");
	region->replace(L"&UR;", L"");
	region->replace(L"&MD;", L"--");
	region->replace(L"&AMP;", L"&");
}

void DefaultDocumentReader::handleInvalidXmlCharacters(LocatedString *locatedString) {
	for (int index = 0; index < locatedString->length(); index++) {
		wchar_t wch = locatedString->charAt(index);
		// https://www.w3.org/TR/2006/REC-xml-20060816/#charsets
		// Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF] /* any Unicode character, excluding the surrogate blocks, FFFE, and FFFF. */ 
		if (wch == 0x0009 ||
			wch == 0x000a ||
			wch == 0x000d ||
			(0x0020 <= wch && wch <= 0xd7ff) ||
			(0xe000 <= wch && wch <= 0xfffd) ||
			(0x10000 <= wch && wch <= 0x10ffff)) {
				;  // Character is fine
		} else  {
			if (_doc_reader_replace_invalid_xml_characters) {
				locatedString->replace(index, 1, L"\xfffd");
			} else {
				std::wstringstream wss;
				//<< std::hex << std::setfill('0') << std::setw(4) 
				wss << "Region has invalid XML character: 0x" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<int>(wch);
				throw UnexpectedInputException("DefaultDocumentReader::handleInvalidXmlCharacters", wss);
			}
		}
	}
}

Symbol DefaultDocumentReader::keepDirectoryLevels(Symbol name, const wchar_t* file) const {
	std::wstring result;
	result.assign(name.to_string());
	
	std::wstring file_string(file);
	int i=0;
	size_t n=0;

	size_t end_index = file_string.find_last_of(L"/\\");
	end_index--;

	while (i < _docid_includes_directory_levels) {
		size_t start_index = file_string.find_last_of(L"/\\",end_index);
		if (start_index == std::string::npos) {
			start_index = 0;
			n = end_index+1;
			i = _docid_includes_directory_levels;
		}
		else {
			++start_index;
			n = (end_index-start_index)+2;
		}
		result.insert(0,file_string.substr(start_index, n));
		end_index = start_index-2;
		i++;
	}
	return Symbol(result);
}

Symbol DefaultDocumentReader::cleanFileName(const wchar_t* file) const {
	std::wstring file_string(file);

	size_t start_index = file_string.find_last_of(L"/\\");
	if (start_index == std::string::npos)
			start_index = 0;
		else
			++start_index;

	return Symbol(file_string.substr(start_index));
}

Symbol DefaultDocumentReader::removeFileExtension(const wchar_t* file) const {
	std::wstring file_string(file);

	size_t dot_index = file_string.find_last_of(L".");

	return Symbol(file_string.substr(0, dot_index));
}

int DefaultDocumentReader::convertCommaDelimitedListToSymbolArray(const char *str, Symbol *result, int max_results) {
	int n_results = 0;
	std::wstring wide_str = L"";
	
	const char *n = str;
	while (*n != '\0') {
		wide_str.push_back((wchar_t)(*n)); 
		n++;
	}

	wchar_t *list_str = _new wchar_t[wide_str.length()+1];
	wcsncpy(list_str, wide_str.c_str(), wide_str.length()+1);
	wchar_t *p = list_str;
	while (*p != '\0' && n_results < max_results) {
		wchar_t *q = p;
		while (*q != '\0' && *q != ',') q++;
		if (*q != '\0') {
			*q = '\0';
			q++;
		}
		result[n_results++] = Symbol(p);
		p = q;
	}
	if (*p != '\0') { 
		std::string err_msg(str);
		err_msg.append(": number of Symbols exceeds max array size");
		throw UnexpectedInputException("DefaultDocumentReader::convertCommaDelimitedListToSymbolArray()", 
									   err_msg.c_str());
	}
	delete [] list_str;
	return n_results;

}

void DefaultDocumentReader::convertCommaDelimitedListToSymbolSet(const char *str, std::set<Symbol>& result) {
	int n_results = 0;
	std::wstring wide_str = L"";
	
	const char *n = str;
	while (*n != '\0') {
		wide_str.push_back((wchar_t)(*n)); 
		n++;
	}

	wchar_t *list_str = _new wchar_t[wide_str.length()+1];
	wcsncpy(list_str, wide_str.c_str(), wide_str.length()+1);
	wchar_t *p = list_str;
	while (*p != '\0' ) {
		wchar_t *q = p;
		while (*q != '\0' && *q != ',') q++;
		if (*q != '\0') {
			*q = '\0';
			q++;
		}
		result.insert(Symbol(p));
		p = q;
	}
	if (*p != '\0') { 
		std::string err_msg(str);
		err_msg.append(": number of Symbols exceeds max array size");
		throw UnexpectedInputException("DefaultDocumentReader::convertCommaDelimitedListToSymbolArray()", 
									   err_msg.c_str());
	}
	delete [] list_str;

}


void DefaultDocumentReader::processRawText(LocatedString *document) {
	document->replace(L">", L"&gt;");
	document->replace(L"<", L"&lt;");
}

Symbol DefaultDocumentReader::getNextRawTextDocName() {
	// get the timestamp
    time_t tim=time(NULL);
	tm *now=localtime(&tim);
	wchar_t timestamp[20];
	swprintf(timestamp, 20, L"%d%02d%02d%02d%02d%02d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		
	// get next doc count
	wchar_t num[10];
#if defined(_WIN32)
	_itow(_rawtext_doc_count++, num, 10);
#else
	swprintf (num, sizeof(num)/sizeof(num[0]), L"%d", _rawtext_doc_count++);
#endif
	
	wchar_t buffer[100] = L"";
	wcscat(buffer, timestamp);
	wcscat(buffer, L"-");
	wcscat(buffer, num);

	return Symbol(buffer);
}

bool DefaultDocumentReader::isSpeakerTag(Symbol tag) {
	for (int i = 0; i < _n_speaker_types; i++) {
		if (_speakerTypes[i] == tag)
			return true;
	}
	return false;
}

bool DefaultDocumentReader::isReceiverTag(Symbol tag) {
	for (int i = 0; i < _n_receiver_types; i++) {
		if (_receiverTypes[i] == tag)
			return true;
	}
	return false;
}

bool DefaultDocumentReader::isJustifiedText(const LocatedString* text) {
	if (text == NULL)
		return false;
	
	// Bucket line lengths, ignoring empty lines
	boost::unordered_map<int, int> tabStopCounts;
	int end = 0;
	for (int i = 0; i < text->length(); i++) {
		if (text->charAt(i) == L'\n') {
			int lineLength = i - end;
			int tabStop = (int)floor(((float) lineLength)/8.0);
			if (tabStop > 0) {
				std::pair<boost::unordered_map<int, int>::iterator, bool> tabStop_i = tabStopCounts.insert(std::make_pair(tabStop, 0));
				tabStop_i.first->second++;
			}
			end = i;
		}
	}

	// Check if the longest line length bucket is also the most common
	int maxBucketCount = 0;
	int maxBucketCountKey = 0;
	for (boost::unordered_map<int, int>::const_iterator tabStop_i = tabStopCounts.begin(); tabStop_i != tabStopCounts.end(); ++tabStop_i) {
		if (tabStop_i->second > maxBucketCount) {
			maxBucketCountKey = tabStop_i->first;
			maxBucketCount = tabStop_i->second;
		}
	}
	return (maxBucketCountKey > 6);
}

bool DefaultDocumentReader::isDoubleSpacedText(const LocatedString* text) {
	if (text == NULL)
		return false;
	
	// Look for double spacing
	int doubleSpaces = 0;
	int lines = 0;
	int end = 0;
	bool twoPrevLine = false;
	bool onePrevLine = false;
	for (int i = 0; i < text->length(); i++) {
		if (text->charAt(i) == L'\n') {
			// Check if this is a longer line
			bool line = false;
			if (i - end > 40) {
				lines++;
				line = true;
			}

			// Count double spaced (non-empty, empty, non-empty)
			if (twoPrevLine && !onePrevLine && line)
				doubleSpaces++;

			// Update iterators
			twoPrevLine = onePrevLine;
			onePrevLine = line;
			end = i;
		}
	}

	// Check if the number of double-spaced lines is a non-trivial fraction of the total lines
	return (((float)doubleSpaces)/lines > 0.6);
}

int DefaultDocumentReader::getNextFBISRegion(const LocatedString* source, int startoffset, LocatedString*& regionSubstring){
		/* Find regions, per LDC
		  - In the annotation area, only the text appearing between "TEXT:"
				and "(MORE)" and/or "TEXT:" and "(ENDALL)" was displayed.
				Therefore, if "TEXT:" was followed eventually by another "TEXT:"
				without an intervening "(MORE)" or "(ENDALL)", it was not
				displayed.
		*/
		//there can be multiple text regions, take the next one....
		int textpos = source->indexOf(L"TEXT:", startoffset);
		if(textpos < 0){
			regionSubstring = 0;
			return -1;
		}		
		int morepos = source->indexOf(L"(MORE)", textpos);
		int endallpos = source->indexOf(L"(ENDALL)", textpos);
		int endpos = -1;
		if(endallpos < 0)  endpos = morepos;
		else if(morepos < 0) endpos = endallpos;
		else endpos = std::min(morepos, endallpos);
		int next = textpos;
		while( (next > -1) && (next < endpos) ){
			next = source->indexOf(L"TEXT:", textpos+1);
			if( (next > -1) && (next < endpos) ){
				textpos = next;
			}
		}
		
		regionSubstring = source->substring(textpos+5, endpos);
		

		//This is evil, the files seem to use single quotation marks as some type of line-begin metadata.  LDC does not document this, but it would mess up SERIF. 
		//Remove single quotation marks, replace double quoation marks with single
		int quoteoffset = 0;
		int searchstart = 0;
		while(quoteoffset > -1 ){
			quoteoffset = regionSubstring->indexOf(L"\"", searchstart);
			int nextchar = quoteoffset+1;
			if( (quoteoffset < 0) || ((nextchar < regionSubstring->length()) && (regionSubstring->charAt(nextchar) == L'\"'))){
				searchstart = nextchar+1; //good quote, we can search from this point
			}
			else{
				//std::wcout<<"Remove: "<<quoteoffset<<"   length: "<<regionSubstring->length()<<std::endl;
				regionSubstring->remove(quoteoffset, quoteoffset+1);
			}
		}
		regionSubstring->replace(L"\"\"", L"\"");
		
		/*//now remove things between square brackets
			- Within the displayed text sections, all square brackets and the
				text they contain were hidden.
		*/
		//bracketed text was not displayed to annotators, so remove it.  Brackets can be nested. 
		std::vector<int> open_bracket_pos;
		std::vector<int> close_bracket_pos;
		int bstart = regionSubstring->indexOf(L"[");
		
		int bend = regionSubstring->indexOf(L"]", bstart);
		boost::match_results<std::wstring::const_iterator> matches;
		boost::match_flag_type flags = boost::match_default;

		boost::wregex bracketre(L"(\\[[^[]*?\\])"); 

		
		while(boost::regex_search(regionSubstring->toWString(), matches, bracketre)){
			
			int s = static_cast<int>(matches.position());
			int e = static_cast<int>(matches.position() + matches.length());
			regionSubstring->remove(s, e);
		}

		return endpos;		
}

int DefaultDocumentReader::getNextProxyRegion(const LocatedString* source, int startoffset, LocatedString*& regionSubstring){
		/* Find regions, per LDC (LDC2013E19 docs)
		- take SUMMARY as a region
		- take each numbered (paragraph) and lettered (sub-paragraph) paragraph from BODY, and add each of them as a regionth
		*/

		//there can be multiple text regions, take the next one....

		int textpos = source->indexOf(L"SUMMARY:", startoffset);
		if(textpos >= 0){ // found SUMMARY
			int endpos = source->indexOf(L"BODY:", textpos);
			if(endpos < 0) {
				regionSubstring = source->substring(textpos+8);
//				std::cout << regionSubstring->toWString() << std::endl;
				return source->length();
			}
			else {
				regionSubstring = source->substring(textpos+8, endpos);
//				std::cout << regionSubstring->toWString() << std::endl;
				return endpos;
			}
		}

		// get numbered/lettered regions
		boost::wregex paragraphStarter(L"(\\r\\n|\\s)[A-Z0-9]\\.[\\s]+");
		boost::match_results<std::wstring::const_iterator> matches_current;

		std::wstring::const_iterator start = source->toWString().begin() + startoffset;
		std::wstring::const_iterator end = source->toWString().end();

		int startPos = -1;
		if(boost::regex_search(start, end, matches_current, paragraphStarter)){
			int posBegin = static_cast<int>(matches_current[0].first - source->toWString().begin());
			int posEnd = static_cast<int>(matches_current[0].second - source->toWString().begin());

//			std::cout<< "current match -> " << source->substring(posBegin, posEnd)->toWString() << std::endl;

			startPos = static_cast<int>(matches_current[0].first - source->toWString().begin() + matches_current[0].length());
//			std::cout<< source->substring(startPos-matches_current[0].length(), startPos)->toWString() << std::endl;

			boost::match_results<std::wstring::const_iterator> matches_next;
			if(boost::regex_search(matches_current[0].second, end, matches_next, paragraphStarter)) {
//				std::cout << startPos << "\t" << matches_next.position() << std::endl;
				regionSubstring = source->substring(startPos, static_cast<int>(matches_next[0].first-source->toWString().begin()));
//				std::cout << regionSubstring->toWString() << std::endl;

//				std::cout<< "next match -> " << source->substring(matches_next[0].first - source->toWString().begin(), matches_next[0].second - source->toWString().begin())->toWString() << std::endl;

				return static_cast<int>(matches_next[0].first-source->toWString().begin());
			}
			else {
				regionSubstring = source->substring(startPos);
//				std::cout << regionSubstring->toWString() << std::endl;
				return source->length();
			}
		}
		else {
			regionSubstring = 0;
			return -1;
		}
}


void DefaultDocumentReader::identifyDFZones(Document *document, const LocatedString *source, Symbol inputType) {
		/* Find zones
		- take post  as a region with attributes: author, datetime and id if there are any
		- take quote as a region with an attribute: orig_author if there is one
		*/

	checkRelevantTagsParsable(*source, document->getName().to_string(), true);

	std::vector<Zone*> topLevelZones;
	std::vector< boost::shared_ptr<ZoneInfo> > stack;

	int start = 0;
	SGMLTag tag;
	do {
		tag= SGMLTag::findNextSGMLTag(*source, start);
		start = tag.getEnd();
		int i = 0;
		if(_zoneTypes.find(tag.getName())!=_zoneTypes.end() ){

			boost::shared_ptr<ZoneInfo> zoneBlock(new ZoneInfo);
			zoneBlock.get()->start = start;
			zoneBlock.get()->end = tag.getStart();

			if (tag.isOpenTag()) {	
				zoneBlock.get()->tag = tag.getName();
				zoneBlock.get()->realTag=tag;
				stack.push_back(zoneBlock);
				
			}else if(tag.isCloseTag()){
				if(stack.empty()){
					throw UnexpectedInputException("DefaultDocumentReader::identifyDFZones()","While reading document, encounteed a closing tag with no corresponding opening tag. The closing tag is:", tag.getName().to_debug_string() );
			
				}
				if(stack.back()->tag!=tag.getName()){
					throw UnexpectedInputException("DefaultDocumentReader::identifyDFZones()","While reading document, encounteed that a closing tag didn't match the corresponding opening tag");
			
				}

				boost::shared_ptr<ZoneInfo> openZoneBlock=stack.back();
				openZoneBlock.get()->end=zoneBlock->end;
					
				//clean up the post/quote
				LocatedString* content=source->substring(openZoneBlock.get()->start, openZoneBlock.get()->end);
				cleanDFPostQuote(content);
				removeOtherTags(content);
				openZoneBlock.get()->pure_content=content;
				//create Zone
				Symbol zoneType=openZoneBlock.get()->tag;
				std::vector<Zone*> children=openZoneBlock.get()->children;
				LocatedString* pure_content=openZoneBlock.get()->pure_content;

				std::map<std::wstring, LSPtr> attributes=openZoneBlock.get()->realTag.getTagAttributes();

				LSPtr author;
				LSPtr datetime;
		
				std::map<std::wstring,LSPtr>::iterator it;
				for (it = attributes.begin(); it != attributes.end(); ++it){
			
					//set author field
					std::set<Symbol>::iterator author_it;
					int number_of_authors=0;
					for (author_it = _authorAttributes.begin(); author_it != _authorAttributes.end(); ++author_it){
						Symbol authorSymbol=*author_it;
						if(boost::iequals(std::wstring(authorSymbol.to_string()),it->first)){
							if(++number_of_authors>1){
								throw UnexpectedInputException("DefaultDocumentReader::identifyDFZones()","Each post/quote can only have one author");
						}
							author=it->second;
						}
					}
					//set datetime field
					std::set<Symbol>::iterator datetime_it;
					int number_of_datetimes=0;
					for (datetime_it = _datetimeAttributes.begin(); datetime_it != _datetimeAttributes.end(); ++datetime_it){	
						Symbol datetimeSymbol=*datetime_it;
						if(boost::iequals(std::wstring(datetimeSymbol.to_string()),it->first)){
							if(++number_of_datetimes>1){
								throw UnexpectedInputException("DefaultDocumentReader::identifyDFZones()","Each post/quote can only have one datetime");
							}
							datetime=it->second;
						}
					}

				}

				Zone* zone=_new Zone(zoneType,attributes,author,datetime,children,pure_content);
					
				stack.pop_back();
				if(stack.empty()){
					topLevelZones.push_back(zone);
				}else{
					boost::shared_ptr<ZoneInfo> parent=stack.back();
					parent.get()->children.push_back(zone);
				}
					
				
			}

		}
	}while(!tag.notFound());


	document->takeZoning(new Zoning(topLevelZones));

	
	
}


void DefaultDocumentReader::removeOtherTags(LocatedString *region) {
	SGMLTag openTag;



	// Remove any other tags whatsoever. Basically <a></a> and <img/>
	// We will remove the <a></a> and the content in between completely, but will only remove <img> and </img> while keep the content 
	SGMLTag next;
	do {
		next = SGMLTag::findNextSGMLTag(*region, 0);
		//std::cout << next.getName()<<"\n";

		if(next.isOpenTag() && next.getName()!=Symbol(L"IMG")){
			SGMLTag tag = SGMLTag::findCloseTag(*region, next.getName(), next.getEnd());
			region->remove(next.getStart(), tag.getEnd());
		}
		else if( next.getName()==Symbol(L"IMG")){ //only remove the tag it self
			
			region->remove(next.getStart(), next.getEnd());
			
		}
	} while (!next.notFound());

	
}

void DefaultDocumentReader::cleanDFPostQuote(LocatedString *region){

	SGMLTag next;
	int start=0;
	do {
		next = SGMLTag::findNextSGMLTag(*region, start);
		//std::cout << next.getName()<<"\n";
		if (next.isOpenTag()) {
			if(_zoneTypes.find(next.getName())!=_zoneTypes.end() ){

				SGMLTag tag = SGMLTag::findCloseTag(*region, next.getName(), next.getEnd());
				region->remove(next.getStart(), tag.getEnd());
				start=0;
			}
			else{
				start=next.getEnd();
			}

		}
		else{
		start=next.getEnd();
		}
	} while (!next.notFound());

}





