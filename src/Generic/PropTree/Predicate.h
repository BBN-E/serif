// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPPREDICATES_H
#define PROPPREDICATES_H

#include <ostream>
#include <map>
#include <boost/functional/hash.hpp>

#include "Generic/common/Symbol.h"

class Mention;

class Predicate {
public:
	// Mention-derived types
	static const Symbol NONE_TYPE;
	static const Symbol NAME_TYPE;
	static const Symbol PRON_TYPE;
	static const Symbol DESC_TYPE;
	static const Symbol PART_TYPE;
	static const Symbol APPO_TYPE;
	static const Symbol LIST_TYPE;
	static const Symbol INFL_TYPE;

	// Other types
	static const Symbol VERB_TYPE;
	static const Symbol MOD_TYPE;
	static const Symbol MODAL_TYPE;

	Predicate( const Symbol& t, const Symbol& p, bool neg = false ) 
		: _type(t), _pred(p), _neg(neg) {}

	// Provides a total-ordering of Predicates
	inline bool operator < ( const Predicate & rhs ) const {		
		if( _pred != rhs._pred ) return Symbol::fast_less_than()( _pred, rhs._pred );
		if( _type != rhs._type ) return Symbol::fast_less_than()( _type, rhs._type );
		return _neg && !rhs._neg;
	}
	
	inline bool operator == ( const Predicate & rhs ) const {
		return _pred == rhs._pred && _type == rhs._type && _neg == rhs._neg;
	}
	
	inline size_t hash_code() const {
		return _pred.hash_code() ^ _type.hash_code() ^ (_neg ? 1 : 0);
	}
	
	Symbol pred() const   { return _pred; }
	Symbol type() const   { return _type; }
	bool negative() const { return _neg;  }

	// checks against a short list of common, invalid predicates (eg, "'s", etc)
	static bool validPredicate( const Symbol & p );
	
	static float getPredicateWeight(Symbol predicate);
	static bool isInvalidPredicate(Symbol predicate);
	
	// returns proper predicate type for the mention
	static Symbol mentionPredicateType( const Mention * );

protected:
	
	Symbol _pred, _type;
	bool _neg;
};

// Predicate total-ordering
template<typename PredType>
struct predicate_cmp {
	inline bool operator() ( const PredType & lhs, const PredType & rhs ) const {
		return lhs < rhs;
	}
	inline bool operator() ( const PredType & lhs, const std::pair<PredType, float>& rhs ) const {
		return lhs < rhs.first;
	}
	inline bool operator() ( const std::pair<PredType, float> & lhs, 
			const PredType & rhs ) const {
		return lhs.first < rhs;
	}
	inline bool operator() (  const std::pair<PredType, float> & lhs, 
			const std::pair<PredType, float> & rhs ) const {
		return lhs.first < rhs.first;
	}
};

// ordering over predicate symbol (eg, not a total ordering, so don't use in map/set)
//  Note that a sequence sorted under operator < is also sorted under predicate_cmp
template <typename PredType>
struct predicate_pred_cmp {
	inline bool operator() ( const PredType & lhs, const PredType & rhs ) const {
		return Symbol::fast_less_than()( lhs.pred(), rhs.pred() );
	}
	inline bool operator() ( const PredType & lhs, const std::pair<PredType, float>& rhs ) const {
		return Symbol::fast_less_than()( lhs.pred(), rhs.first.pred() );
	}
	inline bool operator() ( const std::pair<PredType, float>& lhs, const PredType & rhs ) const {
		return Symbol::fast_less_than()( lhs.first.pred(), rhs.pred() );
	}
	inline bool operator() ( const std::pair<PredType, float>& lhs, 
		const std::pair<PredType, float>& rhs ) const {
		return Symbol::fast_less_than()( lhs.first.pred(), rhs.first.pred() );
	}
	inline bool operator() ( const Symbol & lhs, const PredType & rhs ) const {
		return Symbol::fast_less_than()( lhs, rhs.pred() );
	}
	inline bool operator() ( const PredType & lhs, const Symbol & rhs ) const {
		return Symbol::fast_less_than()( lhs.pred(), rhs );
	}
	inline bool operator() ( const Symbol & lhs, const std::pair<PredType, float>& rhs ) const {
		return Symbol::fast_less_than()( lhs, rhs.first.pred() );
	}
	inline bool operator() ( const std::pair<PredType, float>& lhs, const Symbol & rhs ) const {
		return Symbol::fast_less_than()( lhs.first.pred(), rhs );
	}
};

// output of debug representations
class UTF8OutputStream;
std::wostream    & operator << (std::wostream & s,    const Predicate & p);
UTF8OutputStream & operator << (UTF8OutputStream & s, const Predicate & p);

#endif

