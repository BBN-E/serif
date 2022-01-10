// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

/**
 * Allows patterns to define either an atom as a return label
 * or a series of key-value pairs accessible as a dictionary.
 *
 * @file PatternReturn.h
 * @author nward@bbn.com
 * @date 2010.06.25
 **/

#ifndef PATTERN_RETURN
#define PATTERN_RETURN

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/foreach.hpp>
#include <map>
#include <vector>
#include <list>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/state/XMLElement.h"
#include "Generic/state/XMLIdMap.h"
class Sexp;
BSP_DECLARE(PatternReturn);

/**
 * Represents a constraint-type expression usable in most patterns
 * to indicate that the pattern returns something when it matches.
 *
 * In the Brandy world, this return is a simple label, a single atom
 * in the (return LABEL) expression of a pattern. We maintain backwards
 * compatibility with that functionality in this more advanced return type.
 *
 * In the PredFinder world, and possibly other places where the Brandy
 * pattern language is used, we allow a sequence of two-atom child
 * S-expressions containing key-value pairs. The key is always an atom;
 * the value can be an atom or a string but is always interpreted as a string.
 * It is up to the pattern match consumer to interpret the key-value pairs
 * into their appropriate types and check for completeness.
 *
 * Create PatternReturn objects with one of:
 *
 *    boost::make_shared<PatternReturn>(Sexp*)
 *    boost::make_shared<PatternReturn>(std::vector<std::pair<std::wstring, std::wstring> >)
 *    boost::make_shared<PatternReturn>(PatternReturn_ptr)  // (copy constructor)
 *
 * @author nward@bbn.com
 * @date 2010.06.25
 **/
class PatternReturn : boost::noncopyable {
private:
	PatternReturn(Sexp *sexp);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternReturn, Sexp*);

	typedef std::pair<std::wstring, std::wstring> KeyValPair;
	typedef std::vector<KeyValPair> KeyValPairVec;
	PatternReturn(const KeyValPairVec & key_val_pairs);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternReturn, const KeyValPairVec &);

	PatternReturn(boost::shared_ptr<PatternReturn> other);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternReturn, boost::shared_ptr<PatternReturn>);

public:
	/**
	 * Inlined accessor to get this return constraint's label.
	 *
	 * @return The label, if one has been set. Will be empty (i.e., a Symbol constructed
	 * via the default constructor) if (a) map-style pattern returns are being used or 
	 * (b) label-style pattern returns are being used, but none
	 * has been set yet.
	 **/
	Symbol getLabel() const { return _return_label; }
	bool hasEmptyLabel() const;
	std::map<std::wstring, std::wstring> getCopyOfReturnValuesMap() { return _return_values; }
	size_t getNValuePairs() { return _return_values.size(); }

	std::wstring getValue(const std::wstring & key) const;
	void setValue(const std::wstring & key, const std::wstring & value);

	bool hasValue(const std::wstring & key) const;
	void dump(std::ostream &out, int indent) const;

	bool equals(PatternReturn_ptr other) {
		if (getLabel() == other->getLabel()) {
			BOOST_FOREACH(KeyValPair p, _return_values) {
				if (other->_return_values.find(p.first) == other->_return_values.end() ||
					other->_return_values[p.first] != p.second)
					return false;
			}
			return true;
		}
		return false;
	}

	std::map<std::wstring, std::wstring>::const_iterator begin() const { return _return_values.begin(); };
	std::map<std::wstring, std::wstring>::const_iterator end() const { return _return_values.end(); };

	virtual void saveXML(SerifXML::XMLElement elem);
	PatternReturn(SerifXML::XMLElement elem);

private:

	/**
	 * Inline constructor for an old-style return label
	 * (for compatibility with PseudoPattern).
	 *
	 * @param label The old-style return label Symbol.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.28
	 **/
	PatternReturn(Symbol label) : _return_label(label) { }

	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PatternReturn, Symbol);

private:
	/**
	 * Old-style single return label used for backwards
	 * compatibility with Brandy.
	 **/
	Symbol _return_label;

	/**
	 * Arbitrary string key-value dictionary, with definition
	 * of keys determined by the client.
	 **/
	std::map<std::wstring, std::wstring> _return_values;

	static std::wstring atomToWString(Symbol atom);

	// Helper method for reporting errors.
	static void throwError(Sexp* sexp, const char* reason);
};

/** 
 * A PatternReturnVec is used to represent the information required
 * by PredFinder to construct a set of args within a simple relation.
 *
 * @see http://wiki.d4m.bbn.com/wiki/PatternReturn_Hierarchy for a description of the
 * PatternReturn hierarchy.
 * 
 **/
typedef std::vector< PatternReturn_ptr > PatternReturnVec;

void dumpPatternReturnVec(const PatternReturnVec & vec, std::ostream &out, int indent);

/** 
 *  A PatternReturnVecSeq is used to represent the information required
 *  by PredFinder to construct (1) all arg bundles for a set of top-level patterns (possibly 
 *  from distinct predicate patterns), (2) all possible arg bundles for a top-level pattern 
 *  that contains an any-of PatternCombination, or (3) a combination of (1) and (2).
 **/
typedef std::list< PatternReturnVec > PatternReturnVecSeq;

void dumpPatternReturnVecSeq(const PatternReturnVecSeq & seq, std::ostream &out, int indent);
typedef std::map< Symbol, PatternReturnVecSeq > IDToPatternReturnVecSeqMap;

#endif
