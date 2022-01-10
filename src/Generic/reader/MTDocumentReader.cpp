// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

// MS considers std::copy deprecated, for their strange reasons. It's safely
// used here, and there's no cross-platform alternative.
#pragma warning(disable: 4996)

#include "Generic/common/leak_detection.h"

#include "Generic/reader/MTDocumentReader.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Segment.h"
#include "Generic/common/Attribute.h"
#include "Generic/theories/Metadata.h"
#include "Generic/reader/RegionSpanCreator.h"

#include <vector>
#include <sstream>

#include "boost/algorithm/string/replace.hpp"

using namespace std;

MTDocumentReader::MTDocumentReader(LanguageAttribute lang) {
	std::string segment_input_field = ParamReader::getRequiredParam("segment_input_field");
	_segment_input_field = std::wstring(segment_input_field.begin(), segment_input_field.end());
	std::string segment_input_attribute = ParamReader::getParam("segment_input_attribute","");
	_segment_input_attribute = std::wstring(segment_input_attribute.begin(), segment_input_attribute.end());
	_languageAttribute = lang;
}
MTDocumentReader::MTDocumentReader(std::string segment_input_field, std::string segment_input_attribute, LanguageAttribute lang) :
	_segment_input_field(segment_input_field.begin(), segment_input_field.end()), 
	_segment_input_attribute(segment_input_attribute.begin(), segment_input_attribute.end()),
	_languageAttribute(lang) {
}

MTDocumentReader::MTDocumentReader(std::wstring segment_input_field, std::wstring segment_input_attribute, LanguageAttribute lang) : \
	_segment_input_field(segment_input_field), 
	_segment_input_attribute(segment_input_attribute),
	_languageAttribute(lang) {
}

MTDocumentReader::MTDocumentReader(Symbol defaultInputType) {
	throw UnexpectedInputException("MTDocumentReader::MTDocumentReader", 
		"defaultInputType not supported by this factory");
}

// Reads segments until the given input stream is exhausted
void MTDocumentReader::readSegments( InputStream & stream, vector< WSegment > & segments ){

	// Read all segments from the stream
	while( stream ){
		
		// avoid a copy, construct the segment in-place
		segments.push_back( WSegment() );
		stream >> segments.back();

		// test if the extraction failed
		if( !stream ){
			segments.pop_back();
			break;
		}
	}

	return;
}


// Given segments and an input field name, this method returns a LocatedString constructed by combining the given field in all segments.
// If the 'regions' parameter is non-null, LocatedString regions bounding each segment are additionally returned by argument reference.
// If 'preserve_offsets' is true, rely on segments specified offsets to be region contents.
auto_ptr< LocatedString > MTDocumentReader::constructLocatedStringsFromField( const vector< WSegment > & segments,
																			  wstring field_name,
																			  vector< LocatedString * > * regions /* = NULL */,
																			  vector< EDTOffset > * region_offsets_ret /* = NULL */,
																			  bool preserve_offsets /* = false */ )
{	
	if( regions && !regions->empty() )
		throw UnexpectedInputException( "MTDocumentReader::constructLocatedStringsFromField",
										"regions argument vector, if non-null, must be empty" );

	// construct a string representation of the document, with segment region offsets
	// Region offsets are pairwise for contentful segments if offsets are being preserved
	wstringstream doc_content;
	vector <EDTOffset> region_offsets;
	if (!preserve_offsets) {
		region_offsets.push_back(EDTOffset(0));
	}
	
	// It's possible that more than one field of field_name exists. MTDocumentReader just uses the first one. 
	// A future option is to add optional attribute filters to select by (ie <mt_output theory_rank="1"> foobar...)
	unsigned int num_segments_with_field = 0;
	for (size_t i = 0; i < segments.size(); i++) {
			
		// sanity check for presence of field and entry
		WSegment::const_iterator field_it = segments[i].find(field_name);

		// Fake an entry if don't have the expected field or we don't have the expected attribute for the field
		if (field_it == segments[i].end() || 
			((_segment_input_attribute != L"") &&
			 (field_it->second.at(0).attributes.find(_segment_input_attribute) == field_it->second.at(0).attributes.end()))) {
				doc_content << L" ";
				if (!preserve_offsets) {
					region_offsets.push_back( EDTOffset(region_offsets.back().value() + 1) );
				} else {
					// We need to be able to assume one offset pair per segment later
					throw UnexpectedInputException("MTDocumentReader::constructLocatedStringsFromField",
												   "Preserving offsets but found segment without field or field without attribute");
				}
		} else {			
			num_segments_with_field++;
			if (!preserve_offsets) {
				std::wstring value;
				if (_segment_input_attribute != L"") {
					value = field_it->second.at(0).attributes.get(_segment_input_attribute);
				} else {
					value = field_it->second.at(0).value;
				}

				if( value.length() ){
					doc_content << value;
					region_offsets.push_back( EDTOffset(region_offsets.back().value() + (int) value.length()) );
				} else {
					doc_content << L" ";
					region_offsets.push_back( EDTOffset(region_offsets.back().value() + 1) );
				}
			} else {
				// Get the passed-through start and end offsets of the whole segment; they're specified
				// as inclusive, so we need to +1 to get string indices
				EDTOffset start(_wtoi(segments[i].segment_attributes().get(L"start-offset").c_str()));
				EDTOffset end(_wtoi(segments[i].segment_attributes().get(L"end-offset").c_str()) + 1);

				// Pad from the previous segment with whitespace if necessary
				EDTOffset previous(0);
				if (region_offsets.size() > 0) {
					previous = region_offsets.back();
				}
				for (EDTOffset k = previous; k < start; ++k) {
					doc_content << L" ";
				}

				// Count \r\n CRLF as one character and append
				std::wstring value;
				if (_segment_input_attribute != L"") {
					value = field_it->second.at(0).attributes.get(_segment_input_attribute);
				} else {
					value = field_it->second.at(0).value;
				}
				boost::algorithm::replace_all(value, std::wstring(L"\r\n"), std::wstring(L"\n"));
				doc_content << value;

				// Something's really wrong if the segment string is bigger than its offsets
				size_t length = value.length();
				if (length > (size_t)(end.value() - start.value())) {
					std::stringstream ss;
					ss << "Field string length exceeds segment offsets.  value.length()=" << value.length() << ", start=" << start << ", end=" << end;
					throw InternalInconsistencyException("MTDocumentReader::constructLocatedStringsFromField", ss.str().c_str());
				}

				// Pad the end of this segment with whitespace if necessary
				//   This is usually necessary because of how Serif counts offsets for SGML tags
				for (size_t pad = value.length(); pad < (size_t)(end.value() - start.value()); pad++) {
					doc_content << L" ";
				}

				// Store the pairwise offsets for this segment, so we can generate a region
				region_offsets.push_back(start);
				region_offsets.push_back(end);
			}
		}
	}

	// If we have segments but none had the expected field, throw an Exception
	if (segments.size() > 0 && num_segments_with_field == 0) {
		throw UnexpectedInputException( "MTDocumentReader::constructLocatedStringsFromField",
			("No segments found with field: " + string(field_name.begin(), field_name.end())).c_str());
	}

	// construct a document-level located string, and additional located region substrings
	auto_ptr<LocatedString> doc_lstring(_new LocatedString(doc_content.str().c_str()));
	
	// optionally construct LocatedString subregions
	if( regions ){
		if (!preserve_offsets) {
			// Regions are defined adjacently, one per segment
			for( size_t r = 0; r < segments.size(); r++ ) {
				//int start_index = doc_lstring->positionOfStartOffset(region_offsets[r]);
				//int end_index = doc_lstring->positionOfStartOffset(region_offsets[r+1]);
				//regions->push_back( doc_lstring->substring( start_index, end_index ) );

				// Inside MTDocumentReader the EDT region offsets are literally indexed into the located string, so this is safe to do
				regions->push_back( doc_lstring->substring( region_offsets[r].value(), region_offsets[r+1].value() ) );
			}
		} else {
			// Regions are defined pairwise in the offsets list when we're preserving
			for (size_t r = 0; r < region_offsets.size(); r += 2) {
				//int start_index = doc_lstring->positionOfStartOffset(region_offsets[r]);
				//int end_index = doc_lstring->positionOfStartOffset(region_offsets[r+1]);
				//regions->push_back( doc_lstring->substring( start_index, end_index ) );

				// Inside MTDocumentReader the EDT region offsets are literally indexed into the located string, so this is safe to do
				regions->push_back( doc_lstring->substring( region_offsets[r].value(), region_offsets[r+1].value() ) );
			}
		}
	}
	
	// optionally return segment boundaries within the document's LocatedString
	if( region_offsets_ret ){
		region_offsets_ret->clear();
		region_offsets_ret->swap( region_offsets );
	}
	
	return doc_lstring;
}



Document * MTDocumentReader::readDocument(InputStream &stream, const wchar_t * filename) {	
	vector< WSegment > segments;
	readSegments( stream, segments );
	
	// extract a LocatedString & corresponding regions
	vector< LocatedString * > doc_regions;
	vector< EDTOffset > region_offsets;
	bool preserve_offsets = ParamReader::isParamTrue("mt_doc_reader_preserve_offsets");
	auto_ptr< LocatedString > doc_lstring( constructLocatedStringsFromField(segments, _segment_input_field, & doc_regions, & region_offsets, preserve_offsets ) );

	// Sanity check that the number of doc regions is the same as the number of segments
	if (segments.size() != doc_regions.size()) {
		throw UnexpectedInputException("MTDocumentReader::readDocument", "segments.size() != doc_regions.size()");
										
	}

	wstring doc_name( filename );
	
	// remove leading directories
	doc_name = doc_name.substr(doc_name.rfind(LSERIF_PATH_SEP) + 1);
	// in case we are using forward slashes in Windows path, which is acceptable in Windows Serif
	doc_name = doc_name.substr(doc_name.rfind(L"/") + 1);
	
	LocatedString ** regions = _new LocatedString * [doc_regions.size()];
	std::copy( doc_regions.begin(), doc_regions.end(), regions );
	auto_ptr<Document> doc( _new Document( Symbol(doc_name.c_str()), (int) segments.size(), regions, _languageAttribute ) );
	
	// Free our LocatedStrings!  Document makes a copy of them.  Note that doc_regions and regions point at the same thing.
	for (size_t i = 0; i < doc_regions.size(); i++) {
		delete doc_regions[i];
	}
	delete[] regions;

	wstring date_time, genre;
	auto_ptr<Metadata> metadata( _new Metadata() );
	Symbol regionSpanSymbol = Symbol(L"REGION_SPAN");
	metadata->addSpanCreator( regionSpanSymbol, _new RegionSpanCreator() );
	
	// look for metadata spans we wish to recover into a Metadata()
	// also, copy segments for passing off to the Document()
	std::vector<WSegment> c_segments(segments.size());
	for( size_t i = 0; i < segments.size(); i++ ){
		c_segments[i] = segments[i];
		
		if( segments[i].segment_attributes().find(L"meta") != segments[i].segment_attributes().end() &&
			segments[i].segment_attributes()[L"meta"] != L"" )
		{	
			Symbol name = Symbol( segments[i].segment_attributes()[L"meta"].c_str() );
			// metadata spans use inclusive offsets, thus the -1
			if (!preserve_offsets) {
				metadata->newSpan(regionSpanSymbol, region_offsets[i], EDTOffset(region_offsets[i+1].value() - 1), &name );
			} else {
				// Offset-preserving segment files never have missing segments, one offset pair per segment
				metadata->newSpan(regionSpanSymbol, region_offsets[2*i], EDTOffset(region_offsets[2*i + 1].value() - 1), &name);
			}
		}
		
		if( segments[i].segment_attributes().find(L"date-time") != segments[i].segment_attributes().end() )
			date_time = segments[i].segment_attributes()[L"date-time"];
		if( segments[i].segment_attributes().find(L"genre") != segments[i].segment_attributes().end() )
			genre = segments[i].segment_attributes()[L"genre"];
	}
	
	doc->setOriginalText(doc_lstring.release());
	doc->takeSegments(c_segments);
	doc->setMetadata(metadata.release());
	
	if( date_time.length() )
		doc->setDateTimeField( _new LocatedString( date_time.c_str() ));
	if( genre.length() )
		doc->setSourceType( Symbol(genre.c_str()) );
	
	return doc.release();
}
