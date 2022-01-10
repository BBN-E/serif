// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_COREF_FEATURE_TYPE_H
#define DT_COREF_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class DTCorefFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	DTCorefFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
protected:
	static Symbol NO_ENTITY_TYPE;
	static Symbol UNIQUE;
	static Symbol MENTION;
	static Symbol NAME_MENTION;
	static Symbol NAME_MENTION_x_ABBREV;
	static Symbol PER_MENTION_x_ABBREV;
	static Symbol PER_MENTION_x_MAYBE_ABBREV;
	static Symbol NAME_MENTION_x_MAYBE_ABBREV;
	static Symbol MENTION_LAST_WORD;
	static Symbol MENTION_LAST_WORD_SAME_POS;
	static Symbol MENTION_LAST_WORD_CLSH;
	static Symbol POS_MATCH;
	static Symbol NAME;
	static Symbol POS_NOT_MATCH;
	static Symbol CLASH_SYM;
	static Symbol MATCH_SYM;
	static Symbol MATCH_UNIQU_SYM;
	static Symbol ONE_WORD_NAME_MATCH;
	static Symbol PER_MENTION;
	static Symbol PER_DESC_MENTION;
	static Symbol PER_ENTITY;
	static Symbol ACCUMULATOR;
	static Symbol UNMTCH;
	static Symbol NAME_MENTION_AND_ENTITY_NAME_LEVEL;
	static Symbol MENTION_LAST_WORD_AND_ENTITY_NAME_LEVEL;
	static Symbol MENTION_LAST_WORD_SAME_POS_AND_ENTITY_NAME_LEVEL;
	static Symbol MENTION_LAST_WORD_CLSH_AND_ENTITY_NAME_LEVEL;
	static Symbol POS_MATCH_AND_ENTITY_NAME_LEVEL;
	static Symbol POS_NOT_MATCH_AND_ENTITY_NAME_LEVEL;
	static Symbol PER_MENTION_AND_ENTITY_NAME_LEVEL;
	static Symbol PER_ENTITY_AND_ENTITY_NAME_LEVEL;
	static Symbol FIRST_NAME_MATCH;
	static Symbol FIRST_MATCH_HAS_MID;
//	static Symbol MID_NAME_CLASH;
	static Symbol LAST_NAME_MATCH;
	static Symbol FIRST_NAME_CLASH;
	static Symbol LAST_NAME_CLASH;
	static Symbol IN_MENTION;
	static Symbol IN_ENTITY;
	static Symbol HAS_MIDDLE_NAME;
	static Symbol BOTH_HAVE_MIDDLE_NAME;
	static Symbol MENT_HAS_MIDDLE_NAME_ONLY;
	static Symbol FIRST_AND_LAST_NAME_MATCH;
	static Symbol FIRST_AND_LAST_NAME_CLASH;
	static Symbol MIDDLE_NAME_MATCH;
	static Symbol MIDDLE_NAME_CLASH;
	static Symbol MIDDLE_NAME_ED03;
	static Symbol FIRST_NAME_ED03;
	static Symbol ONE_WORD_NAME_CLASH;
	static Symbol HONORARY_NO_MATCH;
	static Symbol HONORARY_MATCH;
	static Symbol MENT_HAS_LARGE_LAST_NAME;
	static Symbol LAST_2NAMES_MATCH;
	static Symbol LAST_2NAMES_CLASH;
	static Symbol TWO_TO_ONE_CLASH;
	static Symbol FIRST_TO_LAST_MATCH;
	static Symbol ZERO_DOT_1;
	static Symbol ZERO_DOT_2;
	static Symbol ZERO_DOT_3;
	static Symbol SHORT_WORD;
	static Symbol HW2HW;
	static Symbol HW2NONHW;
	static Symbol NONHW2HW;
	static Symbol NONHW2NONHW;

	// For English PER specific words - suffixes
	static Symbol SUFFIX_MATCH;
	static Symbol SUFFIX_CLASH;
	static Symbol SUFFIX_MISSING;

	// For (English) Gender/Number feature
	static Symbol HAS_FEMININE;
	static Symbol HAS_MASCULINE;
	static Symbol GENDER_EQUAL;
	static Symbol MOSTLY_NEUTRAL;
	static Symbol MOSTLY_FEMININE;
	static Symbol MOSTLY_MASCULINE;
	static Symbol MORE_FEMININE;
	static Symbol MORE_MASCULINE;
	static Symbol HAS_SINGULAR;
	static Symbol HAS_PLURAL;
	static Symbol MORE_SINGULAR;
	static Symbol MORE_PLURAL;
	static Symbol NUMBER_EQUAL;

	static Symbol CITI_TO_COUNTRY;
	static Symbol COUNTRY_TO_CITI;
};
#endif
