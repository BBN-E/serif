// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_TYPE_H
#define VALUE_TYPE_H


#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include <vector>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

#define VALUE_TYPE_UNDET_INDEX 0

class SERIF_EXPORTED ValueType {
public:

	/** default constructor makes UNDET type */
	ValueType() : _info(getUndetType()._info) {}

	/** This constructor throws an UnexpectedInputException if it doesn't
	  * recognize the name Symbol.
	  * When you construct an value type object, and there is the
	  * the possibility that the Symbol doesn't correspond to an actual
	  * type, then you should probably trap that exception so that it
	  * it doesn't halt Serif. */
	ValueType(Symbol name);

	/// Gets type's unique number, which corresponds to getType(int i)
	int getNumber() const { return _info->index - 1; }

	/// Gets type's name (e.g., NUMERIC.PERCENT, CONTACT-INFO.URL, etc.)
	Symbol getNameSymbol() const { return _info->name; }

	/// Gets type's nickname (e.g., URL for CONTACT-INFO.URL, etc.)
	Symbol getNicknameSymbol() const { return _info->nickname; }

	/// Gets everything before the '.' (e.g., NUMERIC in NUMERIC.PERCENT)
	Symbol getBaseTypeSymbol() const;

	/// Gets everything after the '.' (e.g., PERCENT in NUMERIC.PERCENT)
	Symbol getSubtypeSymbol() const;

	/// true iff the type is anything but UNDET
	bool isDetermined() const {
		return _info->index != VALUE_TYPE_UNDET_INDEX;
	}

	bool operator==(const ValueType &other) const {
		return _info == other._info;
	}
	bool operator!=(const ValueType &other) const {
		return _info != other._info;
	}
	ValueType &operator=(const ValueType &other) {
		_info = other._info;
		return *this;
	}
	bool isForEventsOnly() const { return _info->for_events_only; }

	static ValueType getUndetType() {
		return ValueType(VALUE_TYPE_UNDET_INDEX);
	}

	/** Get the number of types (not counting UNDET) */
	static int getNTypes() {
		const ValueTypeArray &array = getValueTypeArray();
		return static_cast<int>(array.valueTypes.size()) - 1; // subtract 1 to omit UNDET
	}
	/** Get type #i */
	static ValueType getType(int i) {
		return ValueType(i + 1); // shift index to skip UNDET
	}

	static bool isValidValueType(Symbol possibleType);
	
	static bool isNullSubtype(Symbol possibleSubtype) { return possibleSubtype == NONE; }

private:
	static Symbol NONE;

	struct ValueTypeInfo {
		int index;
		Symbol name;
		Symbol nickname;
		bool for_events_only;
	};

	const ValueTypeInfo *_info;

	ValueType(int index);

	struct ValueTypeArray {
		ValueTypeArray() {}

        std::vector<ValueTypeInfo> valueTypes;
	};
	static const ValueTypeArray &getValueTypeArray();
};


#endif

