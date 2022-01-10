#ifndef _SYMBOL_SERIALIZATION_H_
#define _SYMBOL_SERIALIZATION_H_

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_free.hpp>
#include "Generic/common/Symbol.h"

namespace boost {
	namespace serialization {
		template<class Archive>
		void save(Archive & ar, const Symbol & s, const unsigned int version)
		{
			// since Symbol() != Symbol(L"") we need to record whether 
			// the symbol is empty
			bool empty = (s.is_null());
			ar << empty;

			if (!empty) {
				const wstring str(s.to_string());
				ar << str;
			}
		}

		template<class Archive>
		void load(Archive& ar, Symbol& s, const unsigned int version) {
			bool emptySymbol;

			ar >> emptySymbol;

			if (emptySymbol) {
				s = Symbol();
			} else {
				wstring str;
				ar >> str;
				return Symbol(str);
			}
		}
	}
}

BOOST_SERIALIZATION_SPLIT_FREE(Symbol);

#endif
