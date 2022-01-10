// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "LanguageVariant.h"

const Symbol LanguageVariant::languageVariantAnySym(L"ANY");
std::map<Symbol,std::map<Symbol,LanguageVariant_ptr> > LanguageVariant::_instances;
LanguageVariant_ptr LanguageVariant::_anyLanguageVariant;

bool LanguageVariant::matchesConstraint(LanguageVariant const& languageVariant) const {
	if (languageVariant.getLanguage() == languageVariantAnySym) {
		return true;
	}
	if (languageVariant.getLanguage().is_null() ||
		languageVariant.getLanguage() == _language) {
			return (languageVariant.getVariant().is_null() || 
				languageVariant.getVariant() == _variant);
	}
	return false;
}

const LanguageVariant_ptr LanguageVariant::getLanguageVariantAny() { 
	if (!_anyLanguageVariant)
		_anyLanguageVariant = boost::make_shared<LanguageVariant>(languageVariantAnySym,languageVariantAnySym);
		
	return _anyLanguageVariant; 
}

const LanguageVariant_ptr LanguageVariant::getLanguageVariant(Symbol const& language, Symbol const& variant) {
	if (language.is_null() && variant.is_null()) {
		_instances[Symbol()][Symbol()] = boost::make_shared<LanguageVariant>();
	}
	if (!language.is_null() && _instances.find(language) == _instances.end()) {
		_instances[language][Symbol()] = boost::make_shared<LanguageVariant>(language);
	}
	if (!variant.is_null() && _instances[language].find(variant) == _instances[language].end()) {
		_instances[language][variant] = boost::make_shared<LanguageVariant>(language,variant);
	}
	return _instances[language][variant];
}
