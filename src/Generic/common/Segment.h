// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEGMENT_H
#define SEGMENT_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/UnexpectedInputException.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

/* Segments are a generalized mechanism for passing around chunked things,
 * so that programs may process and add information to chunks in-place,
 * without needing to know anything about other programs which already have or
 * may later process the chunk. The general paradigm is: a process reads fields it
 * cares about, adds some new fields based on that input, and passes all other 
 * fields through untouched to the output WSegment.
 *
 * Currently MTDocumentReader and MTResultCollector utilize WSegments, and WSegments
 * of files currently being processed are stored by MTDocumentReader in the Document object.
 *
 */

namespace serif_segment_internals {
	// these need to be declared prior to Segment's inheritance declaration
	//  they're typedef'd into Segment, thus in private namespace
	
	template< class Str_t >
	class attributes_t : public std::map< Str_t, Str_t > {
	public:
		attributes_t(){}
		attributes_t( const std::map<Str_t,Str_t> & m )
			: std::map<Str_t,Str_t>(m) {}
		
		// checked access to attribute mapping
		const Str_t & get( const Str_t & key ) const {
			typename std::map< Str_t, Str_t >::const_iterator it;
			typename std::map< Str_t, Str_t >::const_iterator end = this->end();
			if( (it = this->find( key )) == end ) {
                std::string keyErr = " attribute key not found for key ";
				char chrkey[256];
				StringTransliterator::transliterateToEnglish(chrkey, key.c_str(), 255);
				keyErr = keyErr.append(chrkey);
				throw UnrecoverableException("Segment::attributes_t::get()", keyErr);
			}
			return it->second;
		}
	};
	
	template< class Str_t >
	struct field_entry_t {
		Str_t value;
		attributes_t<Str_t> attributes;
		
		field_entry_t() {}
		field_entry_t( const field_entry_t & fe )
			: value(fe.value), attributes(fe.attributes) {}
		field_entry_t(	const Str_t & value,
						const attributes_t<Str_t> & attributes )
			: value(value), attributes(attributes) {}
		
		bool operator == ( const field_entry_t & t ) const {
			return value == t.value && attributes == t.attributes;
		}
	};
};


template< class Str_t >
class BaseSegment : public std::map< Str_t, std::vector< serif_segment_internals::field_entry_t< Str_t > > > {
public:
	
	// attribute/value pair & containing mapping
        typedef serif_segment_internals::attributes_t< Str_t > attributes_t;
	
	typedef serif_segment_internals::field_entry_t< Str_t > field_entry_t;
	
	typedef std::vector< field_entry_t > field_entries_t;
	
	typedef std::map< Str_t, field_entries_t > segment_t;
	
	BaseSegment() {}
	
	// Constructs from a serialized segment
	SERIF_EXPORTED BaseSegment( const Str_t & ) throw(UnexpectedInputException);
	
	// Constructs a segment with particular segment_attributes()
	BaseSegment( const attributes_t & a )
		: _seg_attr(a) {}
	
	// serialization
	SERIF_EXPORTED Str_t to_string() const;
	
	// Top-level segment attributes
	attributes_t & segment_attributes(){
		return _seg_attr;
	}
	const attributes_t & segment_attributes() const {
		return _seg_attr;
	}
	
	// const function to check field existence
	bool has_field( const Str_t & field ) const {
		typename segment_t::const_iterator it;
		typename segment_t::const_iterator end = this->end();
		return ( (it = find( field )) != end );
	}
	
	// checked, const field access (non-const can just use [field])
	const field_entries_t & get_field( const Str_t & field ) const {
		typename segment_t::const_iterator it;
        typename segment_t::const_iterator end = this->end();
		std::stringstream ss;
		ss << "field: " << field << " not found";
		if( (it = find( field )) == end )
			throw UnrecoverableException( "Segment::get_field",
					ss.str().c_str());
		return it->second;
	}
	
	field_entry_t & add_field(	const Str_t & name,
								const Str_t & value = Str_t(),
								const attributes_t & att = attributes_t() )
	{
		field_entries_t & fe = (*this)[name];
		return *fe.insert( fe.end(), field_entry_t(value, att) );
	}
	
protected:
	
	attributes_t _seg_attr;
};


// Explicit instantiations
typedef BaseSegment< std::wstring > WSegment;

// operators for reading / writing WSegments to streams. These deal with issues like sgml encoding/decoding.
SERIF_EXPORTED std::wistream & operator >> ( std::wistream & stream, WSegment & WSegment ) throw(UnexpectedInputException);
SERIF_EXPORTED std::wostream & operator << ( std::wostream & stream, const WSegment & WSegment );

class UTF8OutputStream;
SERIF_EXPORTED UTF8OutputStream & operator << ( UTF8OutputStream & stream, const WSegment & WSegment );

#endif // #ifndef SEGMENT_H

