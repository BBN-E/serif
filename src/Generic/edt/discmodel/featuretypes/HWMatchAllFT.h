// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HWMATCH_ALL_FT_H
#define HWMATCH_ALL_FT_H

#include "Generic/common/Assert.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuadgram2IntFeature.h"
#include "Generic/discTagger/DTTrigram2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"

/* This features checks for match between mention head words and entity head words
 * it reconrds the position of the match and fires for mentions and for positions
 * it may be beneficial to fire also per entity (once)
 */
class HWMatchAllFT : public DTCorefFeatureType {
public:
	HWMatchAllFT() : DTCorefFeatureType(Symbol(L"hw-match-all")) {}

	~HWMatchAllFT() {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgram2IntFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, 0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		static bool pos_found[MAX_SENTENCE_TOKENS];

		DTCorefObservation *o = static_cast<DTCorefObservation*>(
				state.getObservation(0));
		Entity *entity = o->getEntity();
		const Mention *ment = o->getMention();
		const EntitySet *eset = o->getEntitySet();
		const MentionSymArrayMap *HWMap = o->getHWMentionMapper();

		EntityType mentEntType = ment->getEntityType();
		Symbol mentEntTypeSymbol;
		if(!mentEntType.isDetermined()) // this is required
			mentEntTypeSymbol = NO_ENTITY_TYPE;
		else
			mentEntTypeSymbol = mentEntType.getName();


		int nFeatures = 0;
		bool discard = false;

		SymbolArray **mentArray = HWMap->get(ment->getUID());
		SerifAssert (mentArray != NULL);
		SerifAssert (*mentArray != NULL);
		const Symbol *mentHeadWords = (*mentArray)->getArray();
		int n_ment_words = (*mentArray)->getLength();
		SerifAssert (n_ment_words <= MAX_SENTENCE_TOKENS);

		// get the entity mention level
		Symbol entityMentionLevel = MENTION;
		for (int m = 0; m < entity->getNMentions(); m++) {
			Mention* entMent = eset->getMention(entity->getMention(m)); 
			Mention::Type mentMentionType = entMent->getMentionType();
			if (mentMentionType == Mention::NAME)
				entityMentionLevel = DTCorefObservation::NAME_LVL;
		}

		for (int i=0; i< n_ment_words; i++)
			pos_found[i] = false;


		// mapall loop
		for (int m = 0; m < entity->getNMentions(); m++) {
			if (discard) break;

			Mention* entMent = o->getEntitySet()->getMention(entity->getMention(m)); 
			SymbolArray **entityArray = HWMap->get(entity->getMention(m));
			SerifAssert (entityArray != NULL);
			SerifAssert (*entityArray != NULL);
			const Symbol *entHeadWords = (*entityArray)->getArray();
			int n_ent_words = (*entityArray)->getLength();
			for (int i=0; i< n_ment_words; i++) {
				if (discard) break;

				//const wchar_t *mentIstr = mentHeadWords[i].to_string();
				//size_t mentIstrLen = wcslen(mentIstr);
				//// get the abbrev without the '.'
				//if(mentIstrLen>0 && mentIstr[mentIstrLen-1]==L'.'){
				//	mentWordIsAbbreviated = true;
				//	wcsncpy(mentIstrAbbrev, mentIstr, mentIstrLen-1);
				//	mentIstrAbbrev[mentIstrLen] = L'\0';
				//}else
				//	mentWordIsAbbreviated = false;
				for (int j = 0; j < n_ent_words; j++) {
					if (mentHeadWords[i] == entHeadWords[j]) {
						if (nFeatures >= MAX_FEATURES_PER_EXTRACTION-2){
							discard = true;
							break;
						}
						//else
						//if (nFeatures > 50) std::cerr<<"Debug early HWMatchAll ent word "<<j<<" and nfeatures "<<nFeatures<<"\n";
						resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
						if(entMent->getMentionType() == Mention::NAME) {
							resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), NAME_MENTION, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
						}
						if(!pos_found[i]) {
							pos_found[i] = true;
//							resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), UNIQUE, SymbolConstants::nullSymbol, entityMentionLevel, i, j);
						}
 					}
					// this section currently doesn't help. It may help after we fix the annotation
					// and have more examples of abbreviations
//					else if(mentIstrLen>2) { // not exact match
//						const wchar_t *entityJstr = entHeadWords[j].to_string();
//						size_t entityJstrLen = wcslen(entityJstr);
//						if(entityJstrLen>2 && entityJstr[0]==mentIstr[0] && entityJstr[1]==mentIstr[1]) {
//							if (mentWordIsAbbreviated && wcsstr(mentIstrAbbrev, entityJstr) != NULL) {
//								resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION_x_ABBREV, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
//								if(entMent->getMentionType() == Mention::NAME) {
//									resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), NAME_MENTION_x_ABBREV, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
//								}
//								//.....
////								pos_found[i] = true;
//							}else {
//								if(entityJstrLen>0 && entityJstr[entityJstrLen-1]==L'.'){
//									entityWordIsAbbreviated = true;
//									wcsncpy(entityJstrAbbrev, entityJstr, entityJstrLen-1);
//									entityJstrAbbrev[entityJstrLen] = L'\0';
//								}else
//									entityWordIsAbbreviated = false;
//
//								if (entityWordIsAbbreviated && wcsstr(entityJstrAbbrev, mentIstr) != NULL) {
//									resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION_x_ABBREV, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
//									if(entMent->getMentionType() == Mention::NAME) {
//										resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), NAME_MENTION_x_ABBREV, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
//									}
//									//.....
////									pos_found[i] = true;
//								}else if(entityJstrLen>3 && mentIstrLen>3 &&
//									((wcsstr(entityJstr, mentIstr) != NULL) || (wcsstr(mentIstr, entityJstr) != NULL))) {
//										resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION_x_MAYBE_ABBREV, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
//										if(entMent->getMentionType() == Mention::NAME) {
//											resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), NAME_MENTION_x_MAYBE_ABBREV, mentEntTypeSymbol, entityMentionLevel, i, ((j<5) ? j : 5));
//										}
//										//...
////										pos_found[i] = true;
//								}
//							}
//						}
//					}// end-else
					if (j==n_ent_words-1 && i==n_ment_words-1 && n_ent_words>1 && n_ment_words>1) {
						if (mentHeadWords[i] == entHeadWords[j]) {
							if (nFeatures >= MAX_FEATURES_PER_EXTRACTION-2){
								discard = true;
								break;
							} 
							resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION_LAST_WORD, mentEntTypeSymbol, entityMentionLevel, i, -1);
							if(i==j)
								resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION_LAST_WORD_SAME_POS, mentEntTypeSymbol, entityMentionLevel, i, -1);
						}else {
							if (nFeatures >= MAX_FEATURES_PER_EXTRACTION){
								discard = true;
								break;
							} 
							resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION_LAST_WORD_CLSH, mentEntTypeSymbol, entityMentionLevel, i, -1);
						}
					}//if
				}//for
			}
		}// for mapall

		if (discard) {
			//std::cerr<<"Debug exit  discard mid HWMatchAll nfeatures "<<nFeatures<<"\n";
			return nFeatures;
		}

		n_positions_matched = 0;
		for (int i=0; i< n_ment_words; i++) {
			if(pos_found[i]) {
				n_positions_matched++;
			}
		}


		// normalize it
		n_ment_words = (n_ment_words<5) ? n_ment_words : 5;

//		resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), ACCUMULATOR, mentEntTypeSymbol, entityMentionLevel, n_positions_matched, 0);

		if (nFeatures >= MAX_FEATURES_PER_EXTRACTION){
			//std::cerr<<"Debug exit mid HWMatchAll nfeatures "<<nFeatures<<"\n";
			return nFeatures;
		}
		resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), ACCUMULATOR, mentEntTypeSymbol, entityMentionLevel, n_positions_matched, n_ment_words);

//		unmatched_words = n_ment_words-n_positions_matched;
//		resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), UNMTCH, mentEntTypeSymbol, entityMentionLevel, unmatched_words, 0);
//		resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), UNMTCH, mentEntTypeSymbol, entityMentionLevel, unmatched_words, n_ment_words);
		
		for (int i=0; i< n_ment_words; i++) {
			if (nFeatures >= MAX_FEATURES_PER_EXTRACTION-2)
				return nFeatures;
			if(pos_found[i]) {
				resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_MATCH, SymbolConstants::nullSymbol, entityMentionLevel, i, n_ment_words);
				resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_MATCH, mentEntTypeSymbol, entityMentionLevel, i, n_ment_words);
			}else {
				resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_NOT_MATCH, SymbolConstants::nullSymbol, entityMentionLevel, i, n_ment_words);
				resultArray[nFeatures++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_NOT_MATCH, mentEntTypeSymbol, entityMentionLevel, i, n_ment_words);
			}
		}

		return nFeatures;
	}

private:
	mutable wchar_t mentIstrAbbrev[MAX_TOKEN_SIZE];
	mutable wchar_t entityJstrAbbrev[MAX_TOKEN_SIZE];
	mutable bool mentWordIsAbbreviated, entityWordIsAbbreviated;
//	mutable int unmatched_words;
	mutable int n_positions_matched;
};

#endif
