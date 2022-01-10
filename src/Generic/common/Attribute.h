// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.
//
// Attribute.h -- type-checked wrapper classes for attribute enums

#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"

/** Wrapper classes intended to be used instead of enumerations to represent
  * fixed sets of attribute values.  The main advantage of the wrapper is
  * type safety. It disallows implicit casting of enum values to ints,
  * preventing the assignment of one type of enum to another
  * (e.g. polarity = future_tense).  In addition, the wrapper associates
  * each enum value with a corresponding string value.
  *
  * Templated based on a Type class.  The Type class represents a single
  * attribute with a fixed set of string values.  It stores a static const
  * Attribute<Type> object for each possible attribute value.  Indiviual
  * attribute values should be accessed by using the corresponding static
  * const object, for example, use Polarity::NEGATIVE to obtain the
  * Attribute<Polarity> object with the string value "Negative".
  */

template<typename Type>
class Attribute {
private:
	typedef typename Type::EnumType EnumType;
public:
	/*********************************************************************
	 * Public Interface
	 *********************************************************************/
	/** Create a new attribute with the undefined value. */
	Attribute(): _value(UNDEFINED_VALUE) {}

	/** Copy constructor */
	Attribute(const Attribute<Type> &other): _value(other._value) {}

	/** Return true if this offset's value is not the undefined_value. */
	bool is_defined() const { return _value != UNDEFINED_VALUE; }

	/** Return the attribute with the given name.  If no such attribute
	  * is found, then throw an exception. */
	static const Attribute getFromString(const wchar_t *string)
	{ return Attribute(getEnumValueFromString(string)); }

	/** Return the attribute with the given integer value.  If no such
	  * attribute is found, then throw an exception. */
	static const Attribute getFromInt(int value) 
	{ return Attribute(getEnumValueFromInt(value)); }

	/** Return an integer value corresponding to this attribute value.  
	  * This integer can be converted back to an attribute using 
	  * getFromInt(). */
	int toInt() const { return _value; }

	/** Return the name of this attribute value.  This name can be 
	  * converted back to an attribute using getFromString(). */
	const wchar_t* toString() const {
		if (!is_defined())
			throw InternalInconsistencyException("Attribute::to_string()", 
												 "Called to_string on undefined Attribute");
		return Type::STRINGS[_value];
	}

	bool operator == (const Attribute<Type> &other) const { return _value == other._value; }
	bool operator != (const Attribute<Type> &other) const { return _value != other._value; }
	bool operator < (const Attribute<Type> &other) const { return _value < other._value; } // This lets us participate in std::sets
public:
	/** This constructor can only be called by Type, because there is no
	  * public access to Type::EnumType, and so no one other than the 
	  * Type class can get a copy of an EnumType -- and without an EnumType
	  * object, no one else can call this constructor. */
	explicit Attribute(EnumType value) { _value = value; }

protected:
	/*********************************************************************
	 * Private Data
	 *********************************************************************/
	EnumType _value;
	const static EnumType UNDEFINED_VALUE = EnumType(-1);
	/*********************************************************************
	 * Private Helper Methods
	 *********************************************************************/
	static EnumType getEnumValueFromInt(int value) {
		if (value >= -1 && value < Type::N_VALUES)
			return EnumType(value);
		throw InternalInconsistencyException("Attribute::getFromInt()", 
											 "Interger value out of range.");
	}

	static const EnumType getEnumValueFromString(const wchar_t *string) {
		bool caseSensitive = Type::CASE_SENSITIVE;
		for (int val = 0; val < Type::N_VALUES; val++) {
			if ((caseSensitive && !wcscmp(string, Type::STRINGS[val])) ||
				(!caseSensitive && !_wcsicmp(string, Type::STRINGS[val])))
				return EnumType(val);
		}
		std::stringstream error;
		error << "Unknown attribute type string: " << UnicodeUtil::toUTF8StdString(string);
		throw UnexpectedInputException("Attribute::getFromString()", error.str().c_str());
	}
};

class Polarity { 
	friend class Attribute<Polarity>;
private:
	typedef enum {POSITIVE_, NEGATIVE_} EnumType; 
	static const wchar_t *STRINGS[];
	static const int N_VALUES = 2;
	static const bool CASE_SENSITIVE = true;
public:
	static const Attribute<Polarity> POSITIVE;
	static const Attribute<Polarity> NEGATIVE;
};
typedef Attribute<Polarity> PolarityAttribute;

class Tense { 
	friend class Attribute<Tense>;
private:
	typedef enum {UNSPECIFIED_, PAST_, PRESENT_, FUTURE_} EnumType; 
	static const wchar_t *STRINGS[];
	static const int N_VALUES = 4;
	static const bool CASE_SENSITIVE = true;
public:
	static const Attribute<Tense> UNSPECIFIED;
	static const Attribute<Tense> PAST;
	static const Attribute<Tense> PRESENT;
	static const Attribute<Tense> FUTURE;
};
typedef Attribute<Tense> TenseAttribute;

class Modality { 
	friend class Attribute<Modality>;
private:
	typedef enum {ASSERTED_, OTHER_} EnumType; 
	static const wchar_t *STRINGS[];
	static const int N_VALUES = 2;
	static const bool CASE_SENSITIVE = true;
public:
	static const Attribute<Modality> ASSERTED;
	static const Attribute<Modality> OTHER;
};
typedef Attribute<Modality> ModalityAttribute;

class Genericity { 
	friend class Attribute<Genericity>;
private:
	typedef enum {SPECIFIC_, GENERIC_} EnumType; 
	static const wchar_t *STRINGS[];
	static const int N_VALUES = 2;
	static const bool CASE_SENSITIVE = true;
public:	
	static const Attribute<Genericity> SPECIFIC;
	static const Attribute<Genericity> GENERIC;
};
typedef Attribute<Genericity> GenericityAttribute;

class PropositionStatus {
	friend class Attribute<PropositionStatus>;
private:
	typedef enum {DEFAULT_, IF_, FUTURE_, NEGATIVE_, ALLEGED_, MODAL_, UNRELIABLE_} EnumType;
	static const wchar_t *STRINGS[];
	static const int N_VALUES = 7;
	static const bool CASE_SENSITIVE = false;
public:
	static const Attribute<PropositionStatus> DEFAULT;
	static const Attribute<PropositionStatus> IF;
	static const Attribute<PropositionStatus> FUTURE;
	static const Attribute<PropositionStatus> NEGATIVE;
	static const Attribute<PropositionStatus> ALLEGED;
	static const Attribute<PropositionStatus> MODAL;
	static const Attribute<PropositionStatus> UNRELIABLE;
};
typedef Attribute<PropositionStatus> PropositionStatusAttribute;

class MentionConfidenceStatus {
	friend class Attribute<MentionConfidenceStatus>;

private:
	typedef enum {		
		UNKNOWN_CONFIDENCE_,
		ANY_NAME_, // any unambiguous name mention
		AMBIGUOUS_NAME_, // name mention that could belong to two different entities "_Clinton_... Hillary Clinton... Bill Clinton..."
		TITLE_DESC_, // e.g. _President_ Barack Obama
		COPULA_DESC_, // e.g. Microsoft is _a big software company_
		APPOS_DESC_, // e.g. Microsoft, _a big software company_
		ONLY_ONE_CANDIDATE_DESC_, //only one candidate of this type in document before desc (possibly excluding dateline ORGs)
		PREV_SENT_DOUBLE_SUBJECT_DESC_, // adjacent sentences such as "Obama anounced .." "_The President_ was visiting..."
		OTHER_DESC_, // any other descriptor
		WHQ_LINK_PRON_, // Microsoft, _which_ is a big software company
		NAME_AND_POSS_PRON_, // Bob and _his_ dog
		DOUBLE_SUBJECT_PERSON_PRON_, // Bob said that _he_ would go shopping (two subjects, both persons, no other name preceding the pronoun)
		ONLY_ONE_CANDIDATE_PRON_, //only one candidate of this type in document before pronoun (possibly excluding dateline ORGs)
		PREV_SENT_DOUBLE_SUBJECT_PRON_, // adjacent sentences such as "Obama said..." "_He_ denied .."
		OTHER_PRON_, // any other pronoun
		NO_ENTITY_	// not co-referenct with another entity
	} EnumType;
	static const wchar_t *STRINGS[];
	static const int N_VALUES = 15;
	static const bool CASE_SENSITIVE = false;

public:
	static const Attribute<MentionConfidenceStatus> UNKNOWN_CONFIDENCE;
	static const Attribute<MentionConfidenceStatus> ANY_NAME;
	static const Attribute<MentionConfidenceStatus> AMBIGUOUS_NAME;
	static const Attribute<MentionConfidenceStatus> TITLE_DESC;
	static const Attribute<MentionConfidenceStatus> COPULA_DESC;
	static const Attribute<MentionConfidenceStatus> APPOS_DESC;
	static const Attribute<MentionConfidenceStatus> ONLY_ONE_CANDIDATE_DESC;
	static const Attribute<MentionConfidenceStatus> PREV_SENT_DOUBLE_SUBJECT_DESC;
	static const Attribute<MentionConfidenceStatus> OTHER_DESC;
	static const Attribute<MentionConfidenceStatus> WHQ_LINK_PRON;
	static const Attribute<MentionConfidenceStatus> NAME_AND_POSS_PRON;
	static const Attribute<MentionConfidenceStatus> DOUBLE_SUBJECT_PERSON_PRON;
	static const Attribute<MentionConfidenceStatus> ONLY_ONE_CANDIDATE_PRON;
	static const Attribute<MentionConfidenceStatus> PREV_SENT_DOUBLE_SUBJECT_PRON;
	static const Attribute<MentionConfidenceStatus> OTHER_PRON;
	static const Attribute<MentionConfidenceStatus> NO_ENTITY;
};

typedef Attribute<MentionConfidenceStatus> MentionConfidenceAttribute;
	

/** Specialized subclass of Attribute that keeps two names for each value: 
  * a short name and a full name.  This is used to implement the Language
  * attribute, where we want to recognize both full language names (such
  * as "English") and abbreviated names (such as "en").*/
template<typename Type>
class TwoNameAttribute: public Attribute<Type> {
private:
	typedef typename Type::EnumType EnumType;
public:
	explicit TwoNameAttribute(EnumType value): Attribute<Type>(value) {}
	TwoNameAttribute(): Attribute<Type>() {} // undefined value
	TwoNameAttribute(const TwoNameAttribute<Type> &other): Attribute<Type>(other) {} // copy constructor
	static const TwoNameAttribute getFromString(const wchar_t *string)
	{ return TwoNameAttribute(getEnumValueFromString(string)); }
	static const TwoNameAttribute getFromInt(int value) 
	{ return TwoNameAttribute(Attribute<Type>::getEnumValueFromInt(value)); }
	const wchar_t* toShortString() const {
		if (!Attribute<Type>::is_defined())
			throw InternalInconsistencyException("TwoNameAttribute::to_string()", 
												 "Called to_string on undefined Attribute");
		return Type::SHORT_STRINGS[Attribute<Type>::_value];
	}
protected:
	static const EnumType getEnumValueFromString(const wchar_t *string) {
		bool caseSensitive = Type::CASE_SENSITIVE;
		for (int val = 0; val < Type::N_VALUES; val++) {
			if ((caseSensitive && !wcscmp(string, Type::SHORT_STRINGS[val])) ||
				(!caseSensitive && !_wcsicmp(string, Type::SHORT_STRINGS[val])))
				return EnumType(val);
		}
		return Attribute<Type>::getEnumValueFromString(string);
	}
};

class Language { 
	friend class Attribute<Language>;
	friend class TwoNameAttribute<Language>;
private:
	typedef enum {
		UNSPECIFIED_,                             // Unknown/unspecified language
		ENGLISH_, ARABIC_, CHINESE_,             // Supported languages
		SPANISH_, FARSI_, HINDI_, BENGALI_, THAI_, KOREAN_, URDU_ // Partially-implemented languages
	} EnumType;
	static const int N_VALUES = 9;
	static const wchar_t *STRINGS[];
	static const wchar_t *SHORT_STRINGS[];
	static const bool CASE_SENSITIVE = false;
public:
	static const TwoNameAttribute<Language> ENGLISH;
	static const TwoNameAttribute<Language> ARABIC;
	static const TwoNameAttribute<Language> CHINESE;
	static const TwoNameAttribute<Language> FARSI;
	static const TwoNameAttribute<Language> HINDI;
	static const TwoNameAttribute<Language> BENGALI;
	static const TwoNameAttribute<Language> THAI;
	static const TwoNameAttribute<Language> KOREAN;
	static const TwoNameAttribute<Language> URDU;
	static const TwoNameAttribute<Language> SPANISH;
	static const TwoNameAttribute<Language> UNSPECIFIED;
};
typedef TwoNameAttribute<Language> LanguageAttribute;

#endif
