// Copyright 2011 BBN Technologies
// All rights reserved

#ifndef DEFAULT_IDF_WORD_FEATURES_H
#define DEFAULT_IDF_WORD_FEATURES_H

#include "Generic/names/IdFWordFeatures.h"
#include "Generic/common/Symbol.h"
#include "Generic/names/IdFListSet.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DefaultIdFWordFeatures : public IdFWordFeatures {
private:
    DefaultIdFWordFeatures(int mode = DEFAULT, IdFListSet *listSet = 0);
	friend struct IdFWordFeatures::FactoryFor<DefaultIdFWordFeatures>;
public:
	int getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const;
	Symbol features(Symbol wordSym, bool first_word, 
		bool normalized_word_in_voc) const;
	bool containsDigit(Symbol wordSym) const;
	Symbol normalizedWord(Symbol wordSym) const;


private:
	int _mode;
	IdFListSet* _listSet;
	bool _model_available;
	enum { IDF_NAME_FINDER, PIDF_NAME_FINDER, NONE} _name_finder;
	Symbol extendedFeatures(Symbol wordSym, bool first_word) const;
	Symbol getCaseFeature(std::wstring word, bool first_word, 
			bool normalized_word_is_in_vocab) const;
	Symbol getOtherValueFeature(Symbol wordSym, bool first_word, 
			bool normalized_word_is_in_vocab) const;
	Symbol getTimexFeature(Symbol wordSym, bool first_word, 
			bool normalized_word_is_in_vocab) const;

	bool containsASCII(Symbol wordSym) const;

	int getAllTimexFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const { return 0; }
	int getAllOtherValueFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const { return 0; }

};

#endif
