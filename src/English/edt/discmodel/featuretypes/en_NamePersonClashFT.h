// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NAME_PER_CLASH_FT_H
#define EN_NAME_PER_CLASH_FT_H

#include "Generic/common/Assert.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/EntitySet.h"
#include "English/common/en_WordConstants.h"

#include <wchar.h>

#define MAXSTRINGLEN 300
#define MAX_FEATURES 100

class EnglishNamePersonClashFT : public DTCorefFeatureType {
public:
	EnglishNamePersonClashFT() : DTCorefFeatureType(Symbol(L"en-name-per-clash")) {
	}

	enum {BOTH_ONE_WORD=0, FIRST_MATCH_LAST_MATCH, FIRST_MATCH_HAS_MID_LAST_NAME_MATCH
		, ENM_BOTH_HAVE_MIDDLE_NAME_LAST_NAME_MATCH, MIDDLE_NAME_ALSO_EQL, MIDDLE_NAME_NOT_EQL
		, FIRST_NAME_CLASH_LAST_NAME_MATCH, HAS_MIDDLE_NAME_LAST_NAME_MATCH
		, MIDDLE_NAME_EQL_LAST_NAME_MATCH, MIDDLE_NAME_NOT_EQL_LAST_NAME_MATCH
		, SUFFIX_MATCH_LAST_NAME_MATCH, ENM_BOTH_HAVE_MIDDLE_NAME_FIRST_AND_LAST_NAME_MATCH
		, SUFFIX_CLASH_LAST_NAME_MATCH, SUFFIX_MISSING_IN_ENTITY
		, SUFFIX_MISSING_IN_MENTION, FIRST_NAME_MATCH_ONE_WORD
		, FIRST_NAME_CLASH_LAST_NAME_CLASH, FIRST_NAME_MATCH_LAST_NAME_CLASH
		, ENM_ONE_WORD_NAME_CLASH, ENM_LAST_NAME_MATCH, MIDDLE_NAME_ALSO_EQL_ED03
		, ENM_HONORARY_MATCH, ENM_HONORARY_NO_MATCH ,ENM_LAST_NAME_CLASH, FIRST_NAME_ED03_LAST_NAME_MATCH
		, ENM_MENT_HAS_MID_NAME_ONLY_LAST_NAME_MATCH, MIDDLE_NAME_ALSO_EQL_ED03_LAST_NAME_MATCH
		, HAS_MIDDLE_NAME_FIRST_AND_LAST_NAME_CLASH, MIDDLE_NAME_EQL_FIRST_AND_LAST_NAME_CLASH
		, MIDDLE_NAME_ALSO_EQL_ED03_FIRST_AND_LAST_NAME_CLASH, MIDDLE_NAME_NOT_EQL_FIRST_AND_LAST_NAME_CLASH
		, ENM_BOTH_HAVE_MIDDLE_NAME_FIRST_AND_LAST_NAME_CLASH, ENM_MENT_HAS_MID_NAME_ONLY_FIRST_AND_LAST_NAME_CLASH
		, ENM_MENT_HAS_LARGE_LAST_2NAMES_LAST_NAME_MATCH, ENM_MENT_HAS_LARGE_LAST_NAME_LAST_NAME_MATCH
		, ENM_MENT_HAS_LARGE_LAST_2NAMES_LAST_NAME_CLASH, ENM_FIRST_TO_LAST_MATCH, ENM_TWO_TO_ONE_CLASH
	} FeatureId;

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		o = static_cast<DTCorefObservation*>(
				state.getObservation(0));
		entitySet = o->getEntitySet();
		entity = o->getEntity();
		ment = o->getMention();
		SerifAssert ( ment->getMentionType() == Mention::NAME || ment->getMentionType() == Mention::NEST ); // this is a Name linking feature only
		if (ment->getEntityType() != EntityType::getPERType())
			return 0; // this is a PER specific feature
		HWMap = o->getHWMentionMapper();

		for (size_t i=0 ;i<MAX_FEATURES;i++) {
			per_entity_not_fire[i] = true;
		}

		// break the mention words. Save time if already broken this mention into words.
		// if the mentUID hasn't changed we can reuse the data from last time.
		if (mentUID != ment->getUID()) {
			mentUID = ment->getUID();
			breakPERName(mentUID, mentHeadWords, nMentHeadWords
								, suffix, suffixed
								, last_name, last_name_indx, has_last_name
								, mid_name, mid_name_indx, has_middle_name
								, first_name, first_name_indx, has_first_name
								, honorary, honorary_index, has_honorary);
		}
		int nFeatures = 0;
		for (int m = 0; m < entity->getNMentions(); m++) {
			if( entitySet->getMention(entity->getMention(m))->getMentionType() != Mention::NAME )
				continue;
			breakPERName(entity->getMention(m), entityHeadWords, nEntityHeadWords
								,entity_suffix, entity_suffixed
								,entity_last_name, entity_last_name_indx, entity_has_last_name
								,entity_mid_name, entity_mid_name_indx, entity_has_middle_name
								,entity_first_name, entity_first_name_indx, entity_has_first_name
								,entity_honorary, entity_honorary_index, entity_has_honorary);
			last_name_match = (has_last_name && entity_has_last_name && last_name == entity_last_name);
			last_name_clash = (has_last_name && entity_has_last_name && last_name != entity_last_name && has_first_name && entity_has_first_name);
			both_one_word = (first_name_indx == last_name_indx && entity_first_name_indx == entity_last_name_indx);
			honorary_one_word_match = (!has_last_name &&! entity_has_last_name && has_honorary && entity_has_honorary && entity_honorary == honorary);
			both_have_first_names = has_first_name && entity_has_first_name;
			first_name_match = (both_have_first_names && !both_one_word && first_name == entity_first_name);
			if (nFeatures >= MAX_FEATURES_PER_EXTRACTION-20)
					return nFeatures;


			if (last_name_match || honorary_one_word_match) {
/*
wcerr<<"headwords: [";
for (int i=0; i<nMentHeadWords-1; i++) {
	wcerr<<L"("<<mentHeadWords[i]<<L") ";
}
if(nMentHeadWords>=1)
	wcerr<<L"("<<mentHeadWords[nMentHeadWords-1].to_string()<<")]           ";
	
wcerr<<"ENT headwords: [";
for (int i=0; i<nEntityHeadWords-1; i++) {
	wcerr<<L"("<<entityHeadWords[i]<<L") ";
}
if(nEntityHeadWords>=1)
	wcerr<<L"("<<entityHeadWords[nEntityHeadWords-1].to_string()<<")]";
wcerr<<L"\n";
*/
				mentFirstChars = first_name.to_string();
				entFirstChars = entity_first_name.to_string();
				mentFirstLen = wcslen(mentFirstChars);
				entFirstLen = wcslen(entFirstChars);
				if (both_one_word) {
					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, ONE_WORD_NAME_MATCH, PER_MENTION);
					if (per_entity_not_fire[BOTH_ONE_WORD]) {
						per_entity_not_fire[BOTH_ONE_WORD] = false;
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, ONE_WORD_NAME_MATCH, PER_ENTITY);
					}
				} else if (first_name_match
						|| (mentFirstChars[0] == entFirstChars[0] && ((mentFirstLen==2 && mentFirstChars[1]==L'.') || (entFirstLen==2 && entFirstChars[1]==L'.')))
						|| (wcsstr(mentFirstChars, entFirstChars) != NULL) // check for substrings
						|| (wcsstr(entFirstChars, mentFirstChars) != NULL)
				) {
					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_MATCH, LAST_NAME_MATCH, PER_MENTION);
					if (per_entity_not_fire[FIRST_MATCH_LAST_MATCH]) {
						per_entity_not_fire[FIRST_MATCH_LAST_MATCH] = false;
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_MATCH, LAST_NAME_MATCH, PER_ENTITY);
					}
					if (has_middle_name) {
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_MATCH_HAS_MID, LAST_NAME_MATCH, PER_MENTION);
						if (per_entity_not_fire[FIRST_MATCH_HAS_MID_LAST_NAME_MATCH]) {
							per_entity_not_fire[FIRST_MATCH_HAS_MID_LAST_NAME_MATCH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_MATCH_HAS_MID, LAST_NAME_MATCH, PER_ENTITY);
						}
						if (entity_has_middle_name){
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), BOTH_HAVE_MIDDLE_NAME, FIRST_AND_LAST_NAME_MATCH, PER_MENTION);
							if (per_entity_not_fire[ENM_BOTH_HAVE_MIDDLE_NAME_FIRST_AND_LAST_NAME_MATCH]) {
								per_entity_not_fire[ENM_BOTH_HAVE_MIDDLE_NAME_FIRST_AND_LAST_NAME_MATCH] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), BOTH_HAVE_MIDDLE_NAME, FIRST_AND_LAST_NAME_MATCH, PER_ENTITY);
							}
							mentMid = mid_name.to_string();
							entMid = entity_mid_name.to_string();
							mentMidLen = wcslen(mentMid);
							entMidLen = wcslen(entMid);
							if (mid_name == entity_mid_name	|| 
									(mentMid[0] == entMid[0] && 
									((mentMidLen==2 && mentMid[1]==L'.') || (entMidLen==2 && entMid[1]==L'.')))) {
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_MATCH, FIRST_AND_LAST_NAME_MATCH, PER_MENTION);
								if (per_entity_not_fire[MIDDLE_NAME_ALSO_EQL]) {
									per_entity_not_fire[MIDDLE_NAME_ALSO_EQL] = false;
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_MATCH, FIRST_AND_LAST_NAME_MATCH, PER_ENTITY);
								}
							}else { // mid names not equal
								edit_distance = editDistance(mentMid,mentMidLen,entMid,entMidLen);
								ave_dist = (float)edit_distance/min(mentMidLen,entMidLen); 
								if(ave_dist<0.3) {
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_ED03, FIRST_AND_LAST_NAME_MATCH, PER_MENTION);
									if (per_entity_not_fire[MIDDLE_NAME_ALSO_EQL_ED03]) {
										per_entity_not_fire[MIDDLE_NAME_ALSO_EQL_ED03] = false;
										resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_ED03, FIRST_AND_LAST_NAME_MATCH, PER_ENTITY);
									}
								}else {
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_CLASH, FIRST_AND_LAST_NAME_MATCH, PER_MENTION);
									if (per_entity_not_fire[MIDDLE_NAME_NOT_EQL]) {
										per_entity_not_fire[MIDDLE_NAME_NOT_EQL] = false;
										resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_CLASH, FIRST_AND_LAST_NAME_MATCH, PER_ENTITY);
									}
								}
							}
						}
					}
				}
				// !both_one_word && !first_name_match
				else if (both_have_first_names
						&& !(first_name_indx < last_name_indx-1 && mentHeadWords[last_name_indx-1] == entity_first_name)
						&& !(entity_first_name_indx < entity_last_name_indx-1 && entityHeadWords[entity_last_name_indx-1] == first_name)) {
					// compute the edit distance on the first word
					edit_distance = editDistance(mentFirstChars,mentFirstLen,entFirstChars,entFirstLen);
					ave_dist = (float)edit_distance/min(mentFirstLen,entFirstLen); 
					if(ave_dist<0.3) {
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_ED03, LAST_NAME_MATCH, PER_MENTION);
						if (per_entity_not_fire[FIRST_NAME_ED03_LAST_NAME_MATCH]) {
							per_entity_not_fire[FIRST_NAME_ED03_LAST_NAME_MATCH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_ED03, LAST_NAME_MATCH, PER_ENTITY);
						}
					}else {
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_CLASH, LAST_NAME_MATCH, PER_MENTION);
						if (per_entity_not_fire[FIRST_NAME_CLASH_LAST_NAME_MATCH]) {
							per_entity_not_fire[FIRST_NAME_CLASH_LAST_NAME_MATCH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_CLASH, LAST_NAME_MATCH, PER_ENTITY);
						}
					}
				} else { // one of them is one word the other is not
					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, LAST_NAME_MATCH, PER_MENTION);
					if (per_entity_not_fire[ENM_LAST_NAME_MATCH]) {
						per_entity_not_fire[ENM_LAST_NAME_MATCH] = false;
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, LAST_NAME_MATCH, PER_ENTITY);
					}
				}

				// deal with middle names
				if (has_middle_name) {
					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HAS_MIDDLE_NAME, LAST_NAME_MATCH, PER_MENTION);
					if (per_entity_not_fire[HAS_MIDDLE_NAME_LAST_NAME_MATCH]) {
						per_entity_not_fire[HAS_MIDDLE_NAME_LAST_NAME_MATCH] = false;
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HAS_MIDDLE_NAME, LAST_NAME_MATCH, PER_ENTITY);
					}
					if (entity_has_middle_name){
						mentMid = mid_name.to_string();
						entMid = entity_mid_name.to_string();
						mentMidLen = wcslen(mentMid);
						entMidLen = wcslen(entMid);
						if (mid_name == entity_mid_name	|| 
								(mentMid[0] == entMid[0] && 
								((mentMidLen==2 && mentMid[1]==L'.') || (entMidLen==2 && entMid[1]==L'.')))) {
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_MATCH, LAST_NAME_MATCH, PER_MENTION);
							if (per_entity_not_fire[MIDDLE_NAME_EQL_LAST_NAME_MATCH]) {
								per_entity_not_fire[MIDDLE_NAME_EQL_LAST_NAME_MATCH] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_MATCH, LAST_NAME_MATCH, PER_ENTITY);
							}
						}else { // mid names not equal
							int mid_edit_distance = editDistance(mentMid,mentMidLen,entMid,entMidLen);
							float ave_dist = (float)mid_edit_distance/min(mentMidLen,entMidLen); 
							if(ave_dist<0.3) {
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_ED03, LAST_NAME_MATCH, PER_MENTION);
								if (per_entity_not_fire[MIDDLE_NAME_ALSO_EQL_ED03_LAST_NAME_MATCH]) {
									per_entity_not_fire[MIDDLE_NAME_ALSO_EQL_ED03_LAST_NAME_MATCH] = false;
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_ED03, LAST_NAME_MATCH, PER_ENTITY);
								}
							}else {
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_CLASH, LAST_NAME_MATCH, PER_MENTION);
								if (per_entity_not_fire[MIDDLE_NAME_NOT_EQL_LAST_NAME_MATCH]) {
									per_entity_not_fire[MIDDLE_NAME_NOT_EQL_LAST_NAME_MATCH] = false;
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_CLASH, LAST_NAME_MATCH, PER_ENTITY);
								}
							}
						}
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), BOTH_HAVE_MIDDLE_NAME, LAST_NAME_MATCH, PER_MENTION);
						if (per_entity_not_fire[ENM_BOTH_HAVE_MIDDLE_NAME_LAST_NAME_MATCH]) {
							per_entity_not_fire[ENM_BOTH_HAVE_MIDDLE_NAME_LAST_NAME_MATCH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), BOTH_HAVE_MIDDLE_NAME, LAST_NAME_MATCH, PER_ENTITY);
						}
					}else { // ! entity_has_middle_name
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_MIDDLE_NAME_ONLY, LAST_NAME_MATCH, PER_MENTION);
						if (per_entity_not_fire[ENM_MENT_HAS_MID_NAME_ONLY_LAST_NAME_MATCH]) {
							per_entity_not_fire[ENM_MENT_HAS_MID_NAME_ONLY_LAST_NAME_MATCH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_MIDDLE_NAME_ONLY, LAST_NAME_MATCH, PER_ENTITY);
						}
					}
				}// end has_middle_name
				if (last_name_indx-first_name_indx>2) { // larger_than_one_last_name
					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_LARGE_LAST_NAME, LAST_NAME_MATCH, PER_MENTION);
					if (per_entity_not_fire[ENM_MENT_HAS_LARGE_LAST_NAME_LAST_NAME_MATCH]) {
						per_entity_not_fire[ENM_MENT_HAS_LARGE_LAST_NAME_LAST_NAME_MATCH] = false;
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_LARGE_LAST_NAME, LAST_NAME_MATCH, PER_ENTITY);
					}
					if(entity_last_name_indx-entity_first_name_indx>=2 && mentHeadWords[last_name_indx-1]==entityHeadWords[entity_last_name_indx-1]) { // we also allow 3 length heads here
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_LARGE_LAST_NAME, LAST_2NAMES_MATCH, PER_MENTION);
						if (per_entity_not_fire[ENM_MENT_HAS_LARGE_LAST_2NAMES_LAST_NAME_MATCH]) {
							per_entity_not_fire[ENM_MENT_HAS_LARGE_LAST_2NAMES_LAST_NAME_MATCH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_LARGE_LAST_NAME, LAST_2NAMES_MATCH, PER_ENTITY);
						}
					} else if(entity_last_name_indx-entity_first_name_indx>2) {
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_LARGE_LAST_NAME, LAST_2NAMES_CLASH, PER_MENTION);
						if (per_entity_not_fire[ENM_MENT_HAS_LARGE_LAST_2NAMES_LAST_NAME_CLASH]) {
							per_entity_not_fire[ENM_MENT_HAS_LARGE_LAST_2NAMES_LAST_NAME_CLASH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_LARGE_LAST_NAME, LAST_2NAMES_CLASH, PER_ENTITY);
						}
					}
				}// end larger_than_one_last_name

				// deal with modifiers Jr.s Sr.
				if (suffixed) {
					if (entity_suffixed) {
						if(suffix == entity_suffix) {
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_MATCH, LAST_NAME_MATCH, PER_MENTION);
							if (per_entity_not_fire[SUFFIX_MATCH_LAST_NAME_MATCH]) {
								per_entity_not_fire[SUFFIX_MATCH_LAST_NAME_MATCH] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_MATCH, LAST_NAME_MATCH, PER_ENTITY);
							}
						}else {
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_CLASH, LAST_NAME_MATCH, PER_MENTION);
							if (per_entity_not_fire[SUFFIX_CLASH_LAST_NAME_MATCH]) {
								per_entity_not_fire[SUFFIX_CLASH_LAST_NAME_MATCH] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_CLASH, LAST_NAME_MATCH, PER_ENTITY);
							}
						}
					}else {
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_MISSING, IN_ENTITY, PER_MENTION);
						if (per_entity_not_fire[SUFFIX_MISSING_IN_ENTITY]) {
							per_entity_not_fire[SUFFIX_MISSING_IN_ENTITY] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_MISSING, IN_ENTITY, PER_ENTITY);
						}
					}
				}else if (entity_suffixed) {
					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_MISSING, IN_MENTION, PER_MENTION);
					if (per_entity_not_fire[SUFFIX_MISSING_IN_MENTION]) {
						per_entity_not_fire[SUFFIX_MISSING_IN_MENTION] = false;
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SUFFIX_MISSING, IN_MENTION, PER_ENTITY);
					}
				}
			}else { // !last_name_match || one doesn't have a last name
				if (nFeatures >= MAX_FEATURES_PER_EXTRACTION-3)
					return nFeatures;
				if(has_first_name){
					if (first_name_match) {
						if (last_name_clash) {
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_MATCH, LAST_NAME_CLASH, PER_MENTION);
							if (per_entity_not_fire[FIRST_NAME_MATCH_LAST_NAME_CLASH]) {
								per_entity_not_fire[FIRST_NAME_MATCH_LAST_NAME_CLASH] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_MATCH, LAST_NAME_CLASH, PER_ENTITY);
						}// one of the names is one word length
						}else {
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_MATCH, SymbolConstants::nullSymbol, PER_MENTION);
							if (per_entity_not_fire[FIRST_NAME_MATCH_ONE_WORD]) {
								per_entity_not_fire[FIRST_NAME_MATCH_ONE_WORD] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_MATCH, SymbolConstants::nullSymbol, PER_ENTITY);
							}
						}
					}else {// !first_name_match
						if(both_one_word) {
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), ONE_WORD_NAME_CLASH, SymbolConstants::nullSymbol, PER_MENTION);
							if (per_entity_not_fire[ENM_ONE_WORD_NAME_CLASH]) {
								per_entity_not_fire[ENM_ONE_WORD_NAME_CLASH] = false;
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), ONE_WORD_NAME_CLASH, SymbolConstants::nullSymbol, PER_ENTITY);
							}
						} else {
							// !first_name_match && !last_name_match - deal also with middle names
							//if (has_middle_name) {
							//	//resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HAS_MIDDLE_NAME, FIRST_AND_LAST_NAME_CLASH, PER_MENTION);
							//	//if (per_entity_not_fire[HAS_MIDDLE_NAME_FIRST_AND_LAST_NAME_CLASH]) {
							//	//	per_entity_not_fire[HAS_MIDDLE_NAME_FIRST_AND_LAST_NAME_CLASH] = false;
							//	//	resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HAS_MIDDLE_NAME, FIRST_AND_LAST_NAME_CLASH, PER_ENTITY);
							//	//}
							//	if (entity_has_middle_name){
							//		mentMid = mid_name.to_string();
							//		entMid = entity_mid_name.to_string();
							//		mentMidLen = wcslen(mentMid);
							//		entMidLen = wcslen(entMid);
							//		if (mid_name == entity_mid_name	|| 
							//				(mentMid[0] == entMid[0] && 
							//				((mentMidLen==2 && mentMid[1]==L'.') || (entMidLen==2 && entMid[1]==L'.')))) {
							//			//resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_MATCH, FIRST_AND_LAST_NAME_CLASH, PER_MENTION);
							//			//if (per_entity_not_fire[MIDDLE_NAME_EQL_FIRST_AND_LAST_NAME_CLASH]) {
							//			//	per_entity_not_fire[MIDDLE_NAME_EQL_FIRST_AND_LAST_NAME_CLASH] = false;
							//			//	resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_MATCH, FIRST_AND_LAST_NAME_CLASH, PER_ENTITY);
							//			//}
							//		}else { // mid names not equal
							//			int mid_edit_distance = editDistance(mentMid,mentMidLen,entMid,entMidLen);
							//			float ave_dist = (float)mid_edit_distance/min(mentMidLen,entMidLen); 
							//			if(ave_dist<0.3) {
							//				//resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_ED03, FIRST_AND_LAST_NAME_CLASH, PER_MENTION);
							//				//if (per_entity_not_fire[MIDDLE_NAME_ALSO_EQL_ED03_FIRST_AND_LAST_NAME_CLASH]) {
							//				//	per_entity_not_fire[MIDDLE_NAME_ALSO_EQL_ED03_FIRST_AND_LAST_NAME_CLASH] = false;
							//				//	resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_ED03, FIRST_AND_LAST_NAME_CLASH, PER_ENTITY);
							//				//}
							//			}else {
							//				resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_CLASH, FIRST_AND_LAST_NAME_CLASH, PER_MENTION);
							//				if (per_entity_not_fire[MIDDLE_NAME_NOT_EQL_FIRST_AND_LAST_NAME_CLASH]) {
							//					per_entity_not_fire[MIDDLE_NAME_NOT_EQL_FIRST_AND_LAST_NAME_CLASH] = false;
							//					resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MIDDLE_NAME_CLASH, FIRST_AND_LAST_NAME_CLASH, PER_ENTITY);
							//				}
							//			}
							//		}
							//		//resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), BOTH_HAVE_MIDDLE_NAME, FIRST_AND_LAST_NAME_CLASH, PER_MENTION);
							//		//if (per_entity_not_fire[ENM_BOTH_HAVE_MIDDLE_NAME_FIRST_AND_LAST_NAME_CLASH]) {
							//		//	per_entity_not_fire[ENM_BOTH_HAVE_MIDDLE_NAME_FIRST_AND_LAST_NAME_CLASH] = false;
							//		//	resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), BOTH_HAVE_MIDDLE_NAME, FIRST_AND_LAST_NAME_CLASH, PER_ENTITY);
							//		//}
							//	}
							//	//else { // ! entity_has_middle_name
							//	//	resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_MIDDLE_NAME_ONLY, FIRST_AND_LAST_NAME_CLASH, PER_MENTION);
							//	//	if (per_entity_not_fire[ENM_MENT_HAS_MID_NAME_ONLY_FIRST_AND_LAST_NAME_CLASH]) {
							//	//		per_entity_not_fire[ENM_MENT_HAS_MID_NAME_ONLY_FIRST_AND_LAST_NAME_CLASH] = false;
							//	//		resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), MENT_HAS_MIDDLE_NAME_ONLY, FIRST_AND_LAST_NAME_CLASH, PER_ENTITY);
							//	//	}
							//	//}
							//}// end dealing with middle names
							if (both_have_first_names) {
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_CLASH, LAST_NAME_CLASH, PER_MENTION);
								if (per_entity_not_fire[FIRST_NAME_CLASH_LAST_NAME_CLASH]) {
									per_entity_not_fire[FIRST_NAME_CLASH_LAST_NAME_CLASH] = false;
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_NAME_CLASH, LAST_NAME_CLASH, PER_ENTITY);
								}
							}else { // only ment has first name
								if(first_name == entity_last_name) { // compare with 1 word entity mention
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_TO_LAST_MATCH, SymbolConstants::nullSymbol, PER_MENTION);
									if (per_entity_not_fire[ENM_FIRST_TO_LAST_MATCH]) {
										per_entity_not_fire[ENM_FIRST_TO_LAST_MATCH] = false;
										resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), FIRST_TO_LAST_MATCH, SymbolConstants::nullSymbol, PER_ENTITY);
									}
								}else {
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), TWO_TO_ONE_CLASH, LAST_NAME_CLASH, PER_MENTION);
									if (per_entity_not_fire[ENM_TWO_TO_ONE_CLASH]) {
										per_entity_not_fire[ENM_TWO_TO_ONE_CLASH] = false;
										resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), TWO_TO_ONE_CLASH, LAST_NAME_CLASH, PER_ENTITY);
									}
								}
							}
						}// end !both one word
					}
				}else {//!has_first_name (it is honorary e.g. 'mr.', 'dr.')
					if (has_honorary) {
						if(entity_has_honorary){
							if(honorary == entity_honorary){
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HONORARY_MATCH, SymbolConstants::nullSymbol, PER_MENTION);
								if (per_entity_not_fire[ENM_HONORARY_MATCH]) {
									per_entity_not_fire[ENM_HONORARY_MATCH] = false;
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HONORARY_MATCH, SymbolConstants::nullSymbol, PER_ENTITY);
								}
							}else {
								resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HONORARY_NO_MATCH, SymbolConstants::nullSymbol, PER_MENTION);
								if (per_entity_not_fire[ENM_HONORARY_NO_MATCH]) {
									per_entity_not_fire[ENM_HONORARY_NO_MATCH] = false;
									resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), HONORARY_NO_MATCH, SymbolConstants::nullSymbol, PER_ENTITY);
								}
							}
						}else{
							//...
						}
					}else { // just last name clash
						resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, LAST_NAME_CLASH, PER_MENTION);
						if (per_entity_not_fire[ENM_LAST_NAME_CLASH]) {
							per_entity_not_fire[ENM_LAST_NAME_CLASH] = false;
							resultArray[nFeatures++] = _new DTQuadgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, LAST_NAME_CLASH, PER_ENTITY);
						}
					}
				}
			}
		}// for
		return nFeatures;
	}



	void breakPERName(MentionUID mentUID, const Symbol *&headWords, int &nHeadWords
								,Symbol &suffixname, bool &has_suffix
								,Symbol &lname, int &lname_indx, bool &has_lname
								,Symbol &mname, int &mname_indx, bool &has_mname
								,Symbol &fname, int &fname_indx, bool &has_fname
								,Symbol &hname, int &hname_indx, bool &has_hname) const
	{
		array = HWMap->get(mentUID);
		SerifAssert (array != NULL);
		SerifAssert (*array != NULL);
		headWords = (*array)->getArray();
		nHeadWords = (*array)->getLength();
		SerifAssert (nHeadWords <= MAX_SENTENCE_TOKENS);
		has_suffix = false;

		// find last name
		lname_indx = nHeadWords-1;
		lname = headWords[lname_indx];
		if (nHeadWords>1 && WordConstants::isNameSuffix(lname)){
			has_suffix = true;
			suffixname = lname;
			lname_indx = nHeadWords-2;
		}

		// find first name & skip honorary words
		fname_indx = 0;
		while (fname_indx<nHeadWords && WordConstants::isHonorificWord(headWords[fname_indx])) {
			fname_indx++;
		}
		fname = (fname_indx<nHeadWords) ? headWords[fname_indx] : SymbolConstants::nullSymbol;
		has_fname = fname_indx<lname_indx;

		// find the middle name
		has_mname = lname_indx-fname_indx==2;
		mname_indx = lname_indx-1;
		mname = has_mname ? headWords[mname_indx] : SymbolConstants::nullSymbol;

		// find an honorary(title) word
		hname_indx = -1;
		if (fname_indx>0) {
			has_hname = true;
			hname_indx = fname_indx-1;
			hname = headWords[hname_indx];
		}else {
			has_hname = false;
		}

		// check if last name exists
		if (lname_indx>hname_indx) {
			lname = headWords[lname_indx];
			has_lname = true;
		}else {
			has_lname = false;
		}
	}


private:
	int editDistance(const wchar_t *s1, size_t n1, const wchar_t *s2, size_t n2) const {
		int cost_del = 1;
		int cost_ins = 1;
		int cost_sub = 1;
		static const int cost_switch = 1; // switching 2 characters (e.g. ba insteadof ab)
		size_t i,j;

		dist[0] = 0;

		for (i = 1; i <= n1; i++) 
			dist[i*(n2+1)] = dist[(i-1)*(n2+1)] + cost_del;

		for (j = 1; j <= n2; j++) 
			dist[j] = dist[j-1] + cost_ins;

		//i=1;
		for (j = 1; j <= n2; j++) {
			int dist_del = dist[j] + cost_del;
			int dist_ins = dist[n2+1+j-1] + cost_ins;
			int dist_sub = dist[j-1] + 
							(s1[0] == s2[j-1] ? 0 : cost_sub);
			dist[n2+1+j] = min(min(dist_del, dist_ins), dist_sub);
		}

		//j=1;
		for (i = 1; i <= n1; i++) {
			int dist_del = dist[(i-1)*(n2+1)+1] + cost_del;
			int dist_ins = dist[i*(n2+1)] + cost_ins;
			int dist_sub = dist[(i-1)*(n2+1)] + 
							(s1[i-1] == s2[0] ? 0 : cost_sub);
			dist[i*(n2+1)+1] = min(min(dist_del, dist_ins), dist_sub);
		}

		for (i = 2; i <= n1; i++) {
			for (j = 2; j <= n2; j++) {
				int dist_del = dist[(i-1)*(n2+1)+j] + cost_del;
				int dist_ins = dist[i*(n2+1)+(j-1)] + cost_ins;
				int dist_sub = dist[(i-1)*(n2+1)+(j-1)] + 
								(s1[i-1] == s2[j-1] ? 0 : cost_sub);
				int dist_switch = dist[(i-2)*(n2+1)+(j-2)] + cost_switch;
				dist[i*(n2+1)+j] = min(min(dist_del, dist_ins), dist_sub);
				if(s1[i-1] == s2[j-2] && s1[i-2] == s2[j-1])
					dist[i*(n2+1)+j] = min(dist[i*(n2+1)+j], dist_switch);
			}
		}

		return dist[n1*(n2+1)+n2];
	}
	mutable int dist[(MAXSTRINGLEN+1)*(MAXSTRINGLEN+1)];

	size_t min(size_t a, size_t b) const {
		return ((a < b) ? a : b);
	}

	int min(int a, int b) const {
		return ((a < b) ? a : b);
	}


	mutable DTCorefObservation *o;
	mutable const EntitySet *entitySet;
	mutable Entity *entity;
	mutable const Mention *ment;
	mutable const MentionSymArrayMap *HWMap;
	mutable const Symbol *mentHeadWords, *entityHeadWords;
	mutable int nFeatures;
	mutable bool per_entity_not_fire[MAX_FEATURES];

	// used in the subroutine breakPERName()
	mutable SymbolArray **array;

	mutable bool last_name_match, last_name_clash, first_name_match, both_one_word
		, honorary_one_word_match, both_have_first_names;
	mutable const wchar_t *mentMid, *entMid, *mentFirstChars, *entFirstChars;
	mutable size_t mentMidLen, entMidLen, mentFirstLen, entFirstLen;

	// used for the predicted mention
	mutable MentionUID mentUID;
	mutable int nMentHeadWords;
	mutable Symbol first_name, mid_name, last_name, suffix, honorary;
	mutable int first_name_indx, mid_name_indx, last_name_indx, honorary_index;
	mutable bool suffixed, has_last_name, has_middle_name, has_first_name, has_honorary;

	// used for the entity
	mutable int nEntityHeadWords;
	mutable Symbol entity_first_name, entity_mid_name, entity_last_name, entity_suffix, entity_honorary;
	mutable int entity_first_name_indx, entity_mid_name_indx, entity_last_name_indx, entity_honorary_index;
	mutable bool entity_suffixed, entity_has_last_name, entity_has_middle_name, entity_has_first_name, entity_has_honorary;

	mutable int edit_distance;
	mutable float ave_dist;
};

#endif
