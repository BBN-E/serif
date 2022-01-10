// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
//
// Offset.h -- type-checked wrapper classes for offset values.

#ifndef OFFSET_H
#define OFFSET_H

#include <boost/static_assert.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

/** Simple wrapper classes used to store offset values.  The main purpose of
  * this class is to allow us to use the type system to ensure that we keep
  * track of which offsets are byte offsets, character offsets, EDT offsets,
  * or ADT offsets; and to ensure that we don't accidentally mix offset types.
  *
  * Each offset wraps a single value, which is typically either an int or a 
  * float.  A special value (typically -1 or -1.0) is used to represent the
  * 'undefined' value for offsets (e.g., if offset information is unavailable,
  * or is not well-defined).
  * 
  * The Offset class is templated based on a Tag class.  This Tag class is
  * used to uniquely differentiate different offset types.  Additionally, it
  * defines the underlying value's type (using typename Tag::ValueType).
  * Specific offset types are defined using typedefs, below.  E.g., 
  * CharOffset is defined to be Offset<CharOffset_tag>, where CharOffset_tag
  * is an appropriate tag class.
  *
  * The OffsetGroup class (defined below) can be used to store one value for
  * each offset type.
  */
template<typename Tag>
class Offset { 
private:
	typename Tag::ValueType _value;

public:
	/** The type of value that is wrapped by this offset type. */
	typedef typename Tag::ValueType ValueType;

	/** Return the undefined value for this offset type. */
	static typename Tag::ValueType undefined_value() { return -1; } // should this be a static const in Tag instead?

	/** Create a new offset that wraps the given value. */
	explicit Offset(typename Tag::ValueType value): _value(value) {}

	/*
	// For now, allow seamless casting to/from the underlying value type.
	operator typename Tag::ValueType() const { return _value; }
	Offset(typename Tag::ValueType value): _value(value) {}
	*/

	/** Create a new offset with an undefined value. */
	explicit Offset(): _value(undefined_value()) {}

	/** Copy constructor */
	Offset(const Offset<Tag> &other): _value(other._value) {}

	/** Return the value wrapped by this offset object. */
	typename Tag::ValueType value() const { return _value; }

	/** Return true if this offset's value is not undefined_value. */
	bool is_defined() const { return _value != undefined_value(); }

	// Comparison operators: delegate to the wrapped value.  Only offsets with the
	// same type (i.e. same tag) may be compared.
	bool operator == (const Offset<Tag> &other) const { return _value == other._value; }
	bool operator != (const Offset<Tag> &other) const { return _value != other._value; }
	bool operator <  (const Offset<Tag> &other) const { return _value <  other._value; }
	bool operator <= (const Offset<Tag> &other) const { return _value <= other._value; }
	bool operator >  (const Offset<Tag> &other) const { return _value >  other._value; }
	bool operator >= (const Offset<Tag> &other) const { return _value >= other._value; }

	// Assignment operator
	Offset &operator=(const Offset<Tag> &other) { _value=other._value; return *this; }

	/** Increment this offset value by one. */
	Offset& operator ++ () { ++_value; return *this; }

	/** Decrement this offset value by one. */
	Offset& operator -- () { --_value; return *this; }

	// [XX] Should this be an explicit constructor instead??
	static Offset parse(std::wstring s) {
		return Offset(boost::lexical_cast<typename Tag::ValueType>(s));
	}
};

//===========================================================================

/** Tag for byte offsets. */
struct ByteOffset_tag { typedef int ValueType; };
typedef Offset<ByteOffset_tag> ByteOffset;

/** Tag for character offsets.  In the context of SERIF, character offsets
  * actually means unicode code point offsets. */
struct CharOffset_tag { typedef int ValueType; };
typedef Offset<CharOffset_tag> CharOffset;

/** Tag for EDT (Entity Detection and Tracking) offsets.  EDT offsets are 
  * similar to character offsets, except that the character "\r" and any 
  * string starting with "<" and ending with the matching ">" are skipped 
  * when counting offsets. */
struct EDTOffset_tag { typedef int ValueType; };
typedef Offset<EDTOffset_tag> EDTOffset;

/** Tag for time offsets, when the source data is speech.  (ASR stands for 
  * automatic speech recognition.) */
struct ASRTime_tag { typedef float ValueType; };
typedef Offset<ASRTime_tag> ASRTime;

//===========================================================================

/** A colleciton of offset values, one for each type of offset.  
  */
struct OffsetGroup {
	ByteOffset byteOffset;
	CharOffset charOffset;
	EDTOffset edtOffset;
	ASRTime asrTime;
	OffsetGroup() {}
	OffsetGroup(ByteOffset byteOffset, CharOffset charOffset, EDTOffset edtOffset, ASRTime asrTime)
		: byteOffset(byteOffset), charOffset(charOffset), edtOffset(edtOffset), asrTime(asrTime) {}
	OffsetGroup(CharOffset charOffset, EDTOffset edtOffset)
		: charOffset(charOffset), edtOffset(edtOffset) {}
	// Default copy constructor & assignment operator.

	template<typename OffsetType> OffsetType value() const;

	void setOffset(const ByteOffset& value) { byteOffset = value; }
	void setOffset(const CharOffset& value) { charOffset = value; }
	void setOffset(const EDTOffset& value) { edtOffset = value; }
	void setOffset(const ASRTime& value) { asrTime = value; }
};

template<> inline ByteOffset OffsetGroup::value<ByteOffset>() const { return byteOffset; }
template<> inline CharOffset OffsetGroup::value<CharOffset>() const { return charOffset; }
template<> inline EDTOffset OffsetGroup::value<EDTOffset>() const { return edtOffset; }
template<> inline ASRTime OffsetGroup::value<ASRTime>() const { return asrTime; }

// What offset type should be used for correct answers in CASerif?
typedef EDTOffset CAOffset;

//===========================================================================
// Streaming output for offsets.

template<typename Tag, typename Stream>
Stream &operator <<(Stream &out, const Offset<Tag> &tag) {
	out << tag.value(); return out; }

//===========================================================================
// You should never compare an offset with a bare value.  Instead, wrap the
// bare value in an appropriate Offset object first.
// 
// (We explicitly disallow these comparisons, in order to generate more 
// useful error messages if someone tries to use them.)
/*
#ifdef _WIN32
template<typename Tag>
bool operator == (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator != (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator <  (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator <= (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator >  (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator >= (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
typename Tag::ValueType operator - (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
typename Tag::ValueType operator + (const typename Tag::ValueType &v1, const Offset<Tag> &v2) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator == (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator != (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator <  (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator <= (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator >  (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
bool operator >= (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
typename Tag::ValueType operator - (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
template<typename Tag>
typename Tag::ValueType operator + (const Offset<Tag> &v2, const typename Tag::ValueType &v1) { BOOST_STATIC_ASSERT(false); }
#endif
*/

#endif
