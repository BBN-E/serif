// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ATTRIBUTE_VALUE_PAIR_H
#define ATTRIBUTE_VALUE_PAIR_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"


class AttributeValuePairBase;
typedef boost::shared_ptr<AttributeValuePairBase> AttributeValuePair_ptr;

/**
  *  An abstract base class for objects that represent key-value pairs of 
  *  extracted information. 
  *
  *  Each AttributeValuePair object also stores the name of the 
  *  AttributeValuePairExtractor that created it.
  *
  *  This base class allows all AttributeValuePair objects to be stored and 
  *  compared uniformly, regardless of value type.
  */
class AttributeValuePairBase : boost::noncopyable {
public:
	virtual ~AttributeValuePairBase() {};
	
	/** Return true if this pair is equivalent to other. */
	virtual bool equals(const AttributeValuePair_ptr other) const = 0;

	/** Return true if this pair's value is equivalent to other's value. */
	virtual bool valueEquals(const AttributeValuePair_ptr other) const = 0;

	/** Produce a wstring representation suitable for debug output. */
	virtual std::wstring toString() const = 0;

	/** Return the key. */
	const Symbol& getKey() const { return _key; }

	/** Return the fully qualified name - i.e. _extractorName:_key. */
	std::wstring getFullName() const;

	static std::wstring getFullName(Symbol extractorName, Symbol key);
	
protected:
	AttributeValuePairBase() {}
	AttributeValuePairBase(Symbol key, Symbol extractorName) : _extractorName(extractorName), _key(key) {}		

	// Name of the AttributeValuePairExtractor that created this pair.
	Symbol _extractorName;

	// Key part of the key-value pair. 
	Symbol _key;
};

/**
  *  Represents a single key-value item of extracted information. 
  *
  *  @tparam ValueType The type used to store the extracted value (e.g. Symbol, float).
  */
template <class ValueType>
class AttributeValuePair : public AttributeValuePairBase {
public:

	typedef boost::shared_ptr< AttributeValuePair<ValueType> > ptr_type;

	/** Construct and return a boost::shared_ptr to a new pair. */
	static ptr_type create(Symbol key, ValueType value, Symbol extractorName) {
		return ptr_type(new AttributeValuePair(key, value, extractorName));
	}

	/** Return the value. */
	const ValueType& getValue() const { return _value; }

	/** Return the name of the AttributeValuePairExtractor that created this pair. */
	Symbol getExtractorName() const { return _extractorName; }

	/** Return true if this pair is equivalent to other. */
	bool equals(const AttributeValuePair_ptr other) const;
	bool equals(const boost::shared_ptr< AttributeValuePair<ValueType> > other) const;

	/** Return true if this pair's value is equivalent to other's value. */
	bool valueEquals(const AttributeValuePair_ptr other) const;
	bool valueEquals(const boost::shared_ptr< AttributeValuePair<ValueType> > other) const;

	/** Produce a wstring representation suitable for debug output. */
	std::wstring toString() const;

private:
	/** Private constructor - use create() to construct new instances */
	AttributeValuePair(Symbol key, ValueType value, Symbol extractorName) : AttributeValuePairBase(key, extractorName), _value(value) {}

	// Value part of the key-value pair. 
	const ValueType _value;
};

template<> bool AttributeValuePair<const class Proposition*>::equals(const boost::shared_ptr< AttributeValuePair<const Proposition*> > other) const;
template<> bool AttributeValuePair<const class Mention*>::equals(const boost::shared_ptr< AttributeValuePair<const Mention*> > other) const;
template<> bool AttributeValuePair<const class SynNode*>::equals(const boost::shared_ptr< AttributeValuePair<const SynNode*> > other) const;

template<> std::wstring AttributeValuePair<float>::toString() const;
template<> std::wstring AttributeValuePair<Symbol>::toString() const;
template<> std::wstring AttributeValuePair<const class Proposition*>::toString() const;
template<> std::wstring AttributeValuePair<const class Mention*>::toString() const;
template<> std::wstring AttributeValuePair<const class SynNode*>::toString() const;

#endif
