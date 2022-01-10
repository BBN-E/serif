// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

/**
 * Allows patterns to define either an atom as a return label
 * or a series of key-value pairs accessible as a dictionary.
 *
 * @file PatternReturn.cpp
 * @author nward@bbn.com
 * @date 2010.06.25
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/Pattern.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/state/XMLElement.h"

/**
 * Constructs a PatternReturn from a (return ...) S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.06.28
 **/
PatternReturn::PatternReturn(Sexp* sexp) {
	// Default to empty
	_return_label = Symbol();

	// Check the format of this pattern return
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in PatternReturn");
	Symbol constraintTypeSym = sexp->getFirstChild()->getValue();
	if (constraintTypeSym != Pattern::returnSym && constraintTypeSym != Pattern::topLevelReturnSym)
		throwError(sexp, "pattern constraint must be 'return' in PatternReturn");

	// Check if we're running in backwards-compatible label mode or using ICEWS patterns
	if (nkids == 2 && sexp->getSecondChild()->isAtom()) {
		// Just treat as an old-style return label; PatternEventFinder will recognize ICEWS-style returns later
		_return_label = sexp->getSecondChild()->getValue();
	} else {
		// Read the key-value pairs
		for (int i = 1; i < nkids; i++) {
			// Get this key-value pair and load it into the map
			Sexp* childSexp = sexp->getNthChild(i);
			if (!childSexp->isList() || childSexp->getNumChildren() != 2)
				throwError(childSexp, "key-value pairs in PatternReturn must be two-child lists");

			// Get the key (which must be an atom)
			Sexp* keySexp = childSexp->getFirstChild();
			if (!keySexp->isAtom())
				throwError(keySexp, "keys in PatternReturn pairs must be atoms");
			std::wstring key = atomToWString(keySexp->getValue());

			// Get the value (which must be an atom)
			Sexp* valueSexp = childSexp->getSecondChild();
			if (!valueSexp->isAtom())
				throwError(valueSexp, "values in PatternReturn pairs must be atoms");
			std::wstring value = atomToWString(valueSexp->getValue());

			// Store the pair in the map
			_return_values.insert(std::pair<std::wstring, std::wstring>(key, value));
		}
	}
}

/**
 * Constructs a PatternReturn from a vector key/value pairs.
 * Skips over duplicate keys.
 *
 * @author eboschee@bbn.com
 * @date 2010.11.24
 **/
PatternReturn::PatternReturn(const KeyValPairVec & key_val_pairs) {
	_return_label = Symbol();
	BOOST_FOREACH(KeyValPair pair, key_val_pairs) {
		_return_values.insert(pair);
	}
}

/**
 * Copy constructor.
 *
 * @author eboschee@bbn.com
 * @date 2010.11.24
 **/
PatternReturn::PatternReturn(PatternReturn_ptr other) {
	_return_label = other->getLabel();
	_return_values = other->getCopyOfReturnValuesMap();
}

bool PatternReturn::hasEmptyLabel() const {
	return (_return_label.is_null());
}

/**
 * Returns the value associated with a particular key.
 *
 * @param key The string key to check for.
 * @return The value for that key, otherwise the empty string.
 *
 * @author nward@bbn.com
 * @date 2010.06.28
 **/
std::wstring PatternReturn::getValue(const std::wstring & key) const {
	std::map<std::wstring, std::wstring>::const_iterator value_i = _return_values.find(key);
	if (value_i == _return_values.end())
		return L"";
	else
		return value_i->second;
}

/**
 * Sets the return value for a particular key. Overwrites any existing value.
 *
 * @param key The string key.
 * @param value The value for that key.
 *
 * @author eboschee@bbn.com
 * @date 2010.12.11
 **/
void PatternReturn::setValue(const std::wstring & key, const std::wstring & value) {
	_return_values[key] = value;
}

/**
 * Checks if the return value map has a particular key.
 *
 * @param key The string key to check for.
 * @return True if the key exists.
 *
 * @author nward@bbn.com
 * @date 2010.06.28
 **/
bool PatternReturn::hasValue(const std::wstring & key) const {
	return _return_values.find(key) != _return_values.end();
}

/**
 * Converts an atom from an S-expression into a wide
 * string, stripping quotes if necessary.
 *
 * @param atom The raw Symbol from the Sexp
 * @return The equivalent string value
 *
 * @author nward@bbn.com
 * @date 2010.06.28
 **/
std::wstring PatternReturn::atomToWString(Symbol atom) {
	// Strip quoted strings
	std::wstring value = std::wstring(atom.to_string());
	boost::algorithm::trim_if(value, boost::algorithm::is_any_of("\""));
	return value;
}

/**
 * Serializes PatternReturn object
 * 
 * @param elem The XMLElement to be filled
 * 
 * @author azamania@bbn.com
 * @date 2012.02.06
 **/
void PatternReturn::saveXML(SerifXML::XMLElement elem) {
	using namespace SerifXML;

	if (!_return_label.is_null())
		elem.setAttribute(X_return_label, _return_label);

	if (_return_values.size() > 0)
		elem.saveMap(X_return_values_map, _return_values);
}

/**
 * Deserializes PatternReturn object
 * 
 * @param elem The XMLElement to read
 * 
 * @author azamania@bbn.com
 * @date 2012.02.13
 **/
PatternReturn::PatternReturn(SerifXML::XMLElement elem) {
	using namespace SerifXML;

	if (elem.hasAttribute(X_return_label))
		_return_label = elem.getAttribute<Symbol>(X_return_label);
	else 
		_return_label = Symbol();

	if (elem.hasAttribute(X_return_values_map))
		_return_values = elem.loadMap<std::wstring, std::wstring>(X_return_values_map);
}

/**
 * Convenience method that formats an Sexp for throwing
 * an UnexpectedInputException. Copied from
 * Pattern::throwError.
 *
 * @param sexp The S-expression that caused the error
 * @param reason Text error message
 *
 * @author nward@bbn.com
 * @date 2010.06.28
 **/
void PatternReturn::throwError(Sexp* sexp, const char* reason) {
	std::stringstream error;
	error << reason << ": " << sexp->to_debug_string();
	throw UnexpectedInputException("PatternReturn::throwError", error.str().c_str());
}

/**
 * Convenience method that outputs the content of a PatternReturn. Based on
 * ArgumentPattern::dump().
 *
 * @param out The ostream to which output is to be written
 * @param indent The level to which the output should be indented
 *
 * @author afrankel@bbn.com
 * @date 2010.10.18
 **/

void PatternReturn::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "PatternReturn: ";

	if (!hasEmptyLabel()) {
		//for (int i = 0; i < indent; i++) out << " ";
		out << getLabel().to_debug_string();
	}
	else {
		BOOST_FOREACH(KeyValPair return_value, _return_values) {
			//for (int i = 0; i < indent; i++) out << " ";
			out << "(" << return_value.first << " " << return_value.second << ") ";
		}
		out << "\n";
	}
}

void dumpPatternReturnVec(const PatternReturnVec & vec, std::ostream &out, int indent) {
	BOOST_FOREACH(PatternReturn_ptr ptr, vec) {
		ptr->dump(out, indent+2);
	}
}

void dumpPatternReturnVecSeq(const PatternReturnVecSeq & seq, std::ostream &out, int indent) {
	BOOST_FOREACH(PatternReturnVec vec, seq) {
		dumpPatternReturnVec(vec, out, indent+2);
		out << "\n" << "------" << "\n";
	}
	out << "\n" << "----------------------------------------" << "\n";
}
