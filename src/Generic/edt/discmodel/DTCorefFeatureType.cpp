// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"

Symbol DTCorefFeatureType::modeltype = Symbol(L"coref");

Symbol DTCorefFeatureType::CLASH_SYM = Symbol(L"CLASH");
Symbol DTCorefFeatureType::MATCH_SYM = Symbol(L"MATCH");
Symbol DTCorefFeatureType::MATCH_UNIQU_SYM = Symbol(L"MATCH_UNQ_SYM");

Symbol DTCorefFeatureType::NO_ENTITY_TYPE = Symbol(L"NO_ENTITY_TYPE");
Symbol DTCorefFeatureType::UNIQUE = Symbol(L"UNIQ");

Symbol DTCorefFeatureType::MENTION = Symbol(L"MNT");
Symbol DTCorefFeatureType::NAME_MENTION = Symbol(L"NM_MNT");
Symbol DTCorefFeatureType::PER_MENTION_x_ABBREV = Symbol(L"Pmnt_ABBRV");
Symbol DTCorefFeatureType::NAME_MENTION_x_ABBREV = Symbol(L"NM_MNTxABBRV");
Symbol DTCorefFeatureType::PER_MENTION_x_MAYBE_ABBREV = Symbol(L"Pmnt_MAYBABBRV");
Symbol DTCorefFeatureType::NAME_MENTION_x_MAYBE_ABBREV = Symbol(L"NM_MNTxMAYBABBRV");
Symbol DTCorefFeatureType::MENTION_LAST_WORD = Symbol(L"MNT_LST_WRD");
Symbol DTCorefFeatureType::MENTION_LAST_WORD_SAME_POS = Symbol(L"MNT_LST_WRD_SMPS");
Symbol DTCorefFeatureType::MENTION_LAST_WORD_CLSH = Symbol(L"MNT_LST_WRD_CLSH");
Symbol DTCorefFeatureType::POS_MATCH = Symbol(L"POS_MTCH");
Symbol DTCorefFeatureType::NAME = Symbol(L"NAME");
Symbol DTCorefFeatureType::POS_NOT_MATCH = Symbol(L"POS_N_MTCH");

Symbol DTCorefFeatureType::NAME_MENTION_AND_ENTITY_NAME_LEVEL = Symbol(L"NM_MNTxNM_LVL");
Symbol DTCorefFeatureType::MENTION_LAST_WORD_AND_ENTITY_NAME_LEVEL = Symbol(L"MNT_LST_WRDxNM_LVL");
Symbol DTCorefFeatureType::MENTION_LAST_WORD_SAME_POS_AND_ENTITY_NAME_LEVEL = Symbol(L"MNT_LST_WRD_SMPSxNM_LVL");
Symbol DTCorefFeatureType::MENTION_LAST_WORD_CLSH_AND_ENTITY_NAME_LEVEL = Symbol(L"MNT_LST_WRD_CLSHxNM_LVL");
Symbol DTCorefFeatureType::POS_MATCH_AND_ENTITY_NAME_LEVEL = Symbol(L"POS_MTCHxNM_LVL");
Symbol DTCorefFeatureType::POS_NOT_MATCH_AND_ENTITY_NAME_LEVEL = Symbol(L"POS_N_MTCHxNM_LVL");
Symbol DTCorefFeatureType::PER_MENTION_AND_ENTITY_NAME_LEVEL = Symbol(L"Pmnt_x_NM_LVL");
Symbol DTCorefFeatureType::PER_ENTITY_AND_ENTITY_NAME_LEVEL = Symbol(L"Pent_x_NM_LVL");

Symbol DTCorefFeatureType::ONE_WORD_NAME_MATCH = Symbol(L"MTCH_1W");
Symbol DTCorefFeatureType::PER_MENTION = Symbol(L"Pmnt");
Symbol DTCorefFeatureType::PER_DESC_MENTION = Symbol(L"Pdesc");
Symbol DTCorefFeatureType::PER_ENTITY = Symbol(L"Pent");
Symbol DTCorefFeatureType::ACCUMULATOR = Symbol(L"ACC");
Symbol DTCorefFeatureType::UNMTCH = Symbol(L"UNMTCH");
Symbol DTCorefFeatureType::FIRST_NAME_MATCH = Symbol(L"FST_MTCH");
Symbol DTCorefFeatureType::FIRST_MATCH_HAS_MID(L"FST_MTCH&HAS_MID");
//Symbol DTCorefFeatureType::MID_NAME_CLASH(L"MID_CLSH");
Symbol DTCorefFeatureType::LAST_NAME_MATCH = Symbol(L"LST_MTCH");
Symbol DTCorefFeatureType::FIRST_NAME_CLASH = Symbol(L"FST_CLSH");
Symbol DTCorefFeatureType::LAST_NAME_CLASH = Symbol(L"LST_CLSH");
Symbol DTCorefFeatureType::IN_MENTION = Symbol(L"MNT");
Symbol DTCorefFeatureType::IN_ENTITY = Symbol(L"ENT");
Symbol DTCorefFeatureType::HAS_MIDDLE_NAME = Symbol(L"HS_MID_NM");
Symbol DTCorefFeatureType::BOTH_HAVE_MIDDLE_NAME = Symbol(L"2MID_NM");
Symbol DTCorefFeatureType::MENT_HAS_MIDDLE_NAME_ONLY = Symbol(L"ONL_NMT_HS_MD");
Symbol DTCorefFeatureType::FIRST_AND_LAST_NAME_MATCH = Symbol(L"1ST&2ND_MTCH");
Symbol DTCorefFeatureType::FIRST_AND_LAST_NAME_CLASH = Symbol(L"1ST&2ND_CLSH");
Symbol DTCorefFeatureType::MIDDLE_NAME_CLASH = Symbol(L"MID_CLSH");
Symbol DTCorefFeatureType::MIDDLE_NAME_MATCH = Symbol(L"MID_MTCH");
Symbol DTCorefFeatureType::MIDDLE_NAME_ED03 = Symbol(L"MID_ED03");
Symbol DTCorefFeatureType::FIRST_NAME_ED03 = Symbol(L"FNM_ED03");
Symbol DTCorefFeatureType::ONE_WORD_NAME_CLASH = Symbol(L"1CLSH");
Symbol DTCorefFeatureType::HONORARY_NO_MATCH = Symbol(L"HONOR_N_MTCH");
Symbol DTCorefFeatureType::HONORARY_MATCH = Symbol(L"HONOR_MTCH");
Symbol DTCorefFeatureType::MENT_HAS_LARGE_LAST_NAME = Symbol(L"MNT_LRG_LST");
Symbol DTCorefFeatureType::LAST_2NAMES_MATCH = Symbol(L"LST_2_MTCH");
Symbol DTCorefFeatureType::LAST_2NAMES_CLASH = Symbol(L"LST_2_CLSH");
Symbol DTCorefFeatureType::TWO_TO_ONE_CLASH = Symbol(L"2to1_CLSH");
Symbol DTCorefFeatureType::FIRST_TO_LAST_MATCH = Symbol(L"FST2LST_MTCH");



Symbol DTCorefFeatureType::HW2NONHW = Symbol(L"HW2NONHW");
Symbol DTCorefFeatureType::HW2HW = Symbol(L"HW2HW");
Symbol DTCorefFeatureType::NONHW2NONHW = Symbol(L"NONHW2NONHW");
Symbol DTCorefFeatureType::NONHW2HW = Symbol(L"NONHW2HW");

Symbol DTCorefFeatureType::SUFFIX_MATCH = Symbol(L"SFX_MTCH");
Symbol DTCorefFeatureType::SUFFIX_CLASH = Symbol(L"SFX_CLSH");
Symbol DTCorefFeatureType::SUFFIX_MISSING = Symbol(L"SFX_MISS");

Symbol DTCorefFeatureType::ZERO_DOT_2 = Symbol(L"0.2");
Symbol DTCorefFeatureType::ZERO_DOT_3 = Symbol(L"0.3");
Symbol DTCorefFeatureType::ZERO_DOT_1 = Symbol(L"0.1");
Symbol DTCorefFeatureType::SHORT_WORD = Symbol(L"SHRTW");

	// For (English) Gender/Number feature
Symbol DTCorefFeatureType::HAS_FEMININE = Symbol(L"HS_FMN");
Symbol DTCorefFeatureType::HAS_MASCULINE = Symbol(L"HS_MSC");
Symbol DTCorefFeatureType::GENDER_EQUAL = Symbol(L"GNDR_EQL");
Symbol DTCorefFeatureType::MOSTLY_NEUTRAL = Symbol(L"MST_NEUTRAL");
Symbol DTCorefFeatureType::MOSTLY_FEMININE = Symbol(L"MST_FMN");
Symbol DTCorefFeatureType::MOSTLY_MASCULINE = Symbol(L"MST_MSC");
Symbol DTCorefFeatureType::MORE_FEMININE = Symbol(L"MR_FMN");
Symbol DTCorefFeatureType::MORE_MASCULINE = Symbol(L"MR_MSC");
Symbol DTCorefFeatureType::HAS_SINGULAR = Symbol(L"HS_SNG");
Symbol DTCorefFeatureType::HAS_PLURAL = Symbol(L"HS_PL");
Symbol DTCorefFeatureType::MORE_SINGULAR = Symbol(L"MR_SGL");
Symbol DTCorefFeatureType::MORE_PLURAL = Symbol(L"MR_PL");
Symbol DTCorefFeatureType::NUMBER_EQUAL = Symbol(L"NUM_NEUTRAL");

Symbol DTCorefFeatureType::CITI_TO_COUNTRY = Symbol(L"CTY2CTRY");
Symbol DTCorefFeatureType::COUNTRY_TO_CITI = Symbol(L"CTRY2CTY");
