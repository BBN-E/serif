// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <sstream>

#include "common/Segment.h"
#include "common/SGML.h"
#include "common/UTF8OutputStream.h"


using namespace std;

// Serialization Input/Output for wstring specialization
template< >
BaseSegment<std::wstring>::BaseSegment( const wstring & s ) throw(UnexpectedInputException) {
	wistringstream ss(s);
	ss >> (*this);
	return;
}
template< >
wstring BaseSegment<std::wstring>::to_string() const {
	wostringstream ss;
	ss << (*this);
	return ss.str();
}


bool parse_attributes( wstringstream & sin, WSegment::attributes_t & attributes, vector<wchar_t> & buffer ){
	
	while( sin ){
		
		while( iswspace( sin.peek() ) )
			sin.get();
		
		// read the attribute name
		wstring att_name; getline( sin, att_name, L'=' );
		while( att_name.size() && iswspace( att_name[att_name.size()-1] ) )
			att_name.resize( att_name.size()-1 );
		
		if( ! att_name.size() )
			throw UnexpectedInputException( "WSegment::parse_attributes",
				"empty attribute name while parsing attributes" );
		
		// reached the end of the open tag
		if( att_name == L">" ) return false;

		// reached the end of the tag, and it's an empty element
		// shouldn't be any whitespace before or after due to iswspace() stuff above
		if( att_name.length() > 1 && att_name.at(0) == L'/' && att_name.at(att_name.length()-1) == L'>' )
			return true;
		
		wstring to_quote;  getline( sin, to_quote, L'\"' );
		wstring att_value; getline( sin, att_value, L'\"' );
		
		// decode the attribute value
		buffer.resize( max(att_value.size() + 1, buffer.size()) );
		attributes[ att_name ] = wstring( &(buffer[0]),
			SGML::decode(att_value.c_str(), &(buffer[0]), buffer.size()) );
	}
	
	throw UnexpectedInputException( "WSegment::parse_attributes",
		"Didn't encounter closing char > or /> while parsing attributes" );
}


template< class stream_T >
stream_T & input_segment( stream_T & stream, WSegment & seg ) throw(UnexpectedInputException) {
	
	vector< wchar_t > buffer(4096);
	
	seg.clear();
	wstring line;
	
	bool all_white_space = true;
	while( stream && (line.size() == 0 || all_white_space) ) {
		getline( stream, line );
		for (size_t c = 0; c < line.size(); c++) {
            wchar_t wc = line.at(c);
			// 0xFFFF must be explicitly skipped under Linux; adding the test doesn't hurt under Windows
			if (!iswspace(wc) && wc != 0xFFFF) { 
				all_white_space = false;
				break;
			}
		}
	}
    if( !stream ) return stream;
	
	const wchar_t * seg_start = L"<segment";
	const wchar_t * seg_end   = L"/segment";
	// backwards compatible w/ old SEGMENT files
	const wchar_t * seg_start_bc = L"<SEGMENT";
	const wchar_t * seg_end_bc   = L"/SEGMENT";
	
    if( line.substr(0, wcslen(seg_start)) != seg_start && line.substr(0, wcslen(seg_start_bc)) != seg_start_bc ) {
        fwprintf(stderr, L"Bad line:\n%ls\n", line.c_str());
        throw UnexpectedInputException( "WSegment::input_segment", "did not find expected segment start tag" );
    }
	wstringstream wstr(line.substr(wcslen(seg_start)));
	bool is_empty_tag = parse_attributes( wstr, seg.segment_attributes(), buffer );
	if (is_empty_tag)
		throw UnexpectedInputException( "WSegment::input_segment", "segment tag cannot be an empty element" );
	
	while( stream ){
		
		// skip to the next tag opening
		while( stream.get() != L'<' );
		
		// read entire field open tag
		wstring field_open; getline( stream, field_open, L'>' );
		wstringstream field_open_in; field_open_in << field_open << L" >";
		
		// check for the end-of-segment tag
		if( field_open == seg_end || field_open == seg_end_bc ) break;
		
		// pull out field name, and create a new entry
		wstring field_name; field_open_in >> field_name;
		WSegment::field_entries_t & entries( seg[field_name] );
		entries.resize( entries.size() + 1 );
		
		// parse attributes & value
		bool is_empty_tag = parse_attributes( field_open_in, entries.back().attributes, buffer );

		if (!is_empty_tag) {
			getline( stream, entries.back().value, L'<' );

			// SGML-decode in place
			buffer.resize( max(entries.back().value.size() + 1, buffer.size()) );
			entries.back().value = wstring( &(buffer[0]),
				SGML::decode(entries.back().value.c_str(), &(buffer[0]), buffer.size()) );

			// verify end-of-tag
			wstring tag_end; getline( stream, tag_end, L'>' );
			if( tag_end != L"/" + field_name ) {
				wstringstream wss;
				wss << "Expected closing tag </" << field_name;
				wss << ">, but found </" << tag_end << "> instead";

				wstring ws = wss.str();
				string s(ws.begin(), ws.end());
				throw UnexpectedInputException("Segment::input_segment", s.c_str());
			}
		}
	}

	return stream;
}


template< class stream_T >
void write_attributes( stream_T & stream, const WSegment::attributes_t & attributes, vector< wchar_t > & buffer ){
	
	for( map< wstring, wstring >::const_iterator att_it = attributes.begin();
			att_it != attributes.end(); att_it++ )
	{	
		// resize to largest possible encoded string
		buffer.resize( max( buffer.size(), att_it->second.size() * 6 + 1 ));
		SGML::encode( att_it->second.c_str(), &(buffer[0]), buffer.size() );
		
		stream << L" " << att_it->first << L"=\"" << &(buffer[0]) << L"\"";
	}
	return;
}


template< class stream_T >
stream_T & output_segment( stream_T & stream, const WSegment & seg ){
	
	vector< wchar_t > buffer(4096);
	
	// write segment open & attributes
	stream << L"<segment";
	write_attributes( stream, seg.segment_attributes(), buffer );
	stream << L">\n";
	
	// iterate over all fields
	for( WSegment::const_iterator field_it = seg.begin();
				field_it != seg.end(); field_it++ )
	{	
		for( WSegment::field_entries_t::const_iterator entry_it = field_it->second.begin();
				entry_it != field_it->second.end(); entry_it++ )
		{	
			stream << L"\t<" << field_it->first;
			write_attributes( stream, entry_it->attributes, buffer );
			stream << L">";
			
			buffer.resize( max( buffer.size(), entry_it->value.size() * 6 + 1 ));
			SGML::encode( entry_it->value.c_str(), &(buffer[0]), buffer.size() );
			
			stream << &(buffer[0]);
			stream << L"</" << field_it->first << L">\n";
		}
	}
	
	// write segment close
	stream << L"</segment>\n";
	return stream;
}


// We restrict to particular types of streams, but use template
// argument deduction to use only a single input or output method

// operators for reading / writing segments to streams
std::wistream & operator >> ( std::wistream & stream, WSegment & segment ) throw(UnexpectedInputException) {
	return input_segment( stream, segment );
}

std::wostream & operator << ( std::wostream & stream, const WSegment & segment ) {
	return output_segment( stream, segment );
}

// for backwards compatibility
UTF8OutputStream & operator << ( UTF8OutputStream & stream, const WSegment & segment ) {
	return output_segment( stream, segment );
}
