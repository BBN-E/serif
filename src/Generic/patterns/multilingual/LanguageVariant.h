// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LANGUAGE_VARIANT_H
#define LANGUAGE_VARIANT_H

#include <string>
#include <map>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

BSP_DECLARE(LanguageVariant);
//class LanguageVariant;
//typedef boost::shared_ptr<LanguageVariant> LanguageVariant_ptr;


class LanguageVariant : private boost::noncopyable, public boost::enable_shared_from_this<LanguageVariant> { 
private:
	const Symbol _language;
	const Symbol _variant;

	static std::map<Symbol,std::map<Symbol,LanguageVariant_ptr> > _instances;
	static LanguageVariant_ptr _anyLanguageVariant;

	LanguageVariant(Symbol const& language = Symbol(), Symbol const& variant = Symbol()) : _language(language), _variant(variant) {}
	BOOST_MAKE_SHARED_0ARG_CONSTRUCTOR(LanguageVariant);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(LanguageVariant, const Symbol&);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(LanguageVariant, const Symbol&, const Symbol&);

public:
	const static Symbol languageVariantAnySym;
	
	LanguageVariant(const LanguageVariant& languageVariant) : _language(languageVariant.getLanguage()), _variant(languageVariant.getVariant()) {}

	static const LanguageVariant_ptr getLanguageVariantAny();
	static const LanguageVariant_ptr getLanguageVariant(Symbol const& language = Symbol(), Symbol const& variant = Symbol());	

	bool matchesConstraint(LanguageVariant const& languageVariant) const;
	bool isFullySpecified() { return !_language.is_null() && !_variant.is_null(); }
	const Symbol getLanguage() const { return _language; }
	const Symbol getVariant() const { return _variant; }


	bool operator== (LanguageVariant const& languageVariant) const {
		return _language == languageVariant.getLanguage() && _variant == languageVariant.getVariant() ;
	}
    bool operator!= (LanguageVariant const& languageVariant) const {
		return _language != languageVariant.getLanguage() || _variant != languageVariant.getVariant() ;
	}

	const std::wstring getLanguageString() const { return getLanguage().is_null() ? L"" : getLanguage().to_string(); }

	const std::wstring toString() const { 
		std::wstring lang = getLanguage().is_null() ? L"" : getLanguage().to_string();
		std::wstring var =  getVariant().is_null() ? L"" : getVariant().to_string();
		return lang+L"_"+var;
	}

	size_t hash_code() const { 
		return hash_value(_language) ^ hash_value(_variant);
	}

	struct HashKey {
        size_t operator()(LanguageVariant const& lv) const {
            return lv.hash_code();
        }
    };
	struct EqualKey {
		bool operator()(LanguageVariant const& lv1, LanguageVariant const& lv2) const {
			return lv1 == lv2;
		}
	};
	struct PtrHashKey {
        size_t operator()(LanguageVariant_ptr const lv) const {
            return lv->hash_code();
        }
    };
	struct PtrEqualKey {
		bool operator()(LanguageVariant_ptr const lv1, LanguageVariant_ptr const lv2) const {
			return *lv1 == *lv2;
		}
	};
};



inline size_t hash_value( const LanguageVariant_ptr& languageVariant ){
	return languageVariant->hash_code(); 
}

inline bool equal_to( const LanguageVariant_ptr& lv1,  const LanguageVariant_ptr& lv2) {
	return *lv1 == *lv2;
}



//size_t hash_value(LanguageVariant_ptr const& languageVariant);
/*size_t hash_value(LanguageVariant_ptr const& languageVariant) {
	std::hash<Symbol> hasher;
	return hasher(languageVariant->getLanguage()) ^ 
		   hasher(languageVariant->getVariant());
}*/

#endif
