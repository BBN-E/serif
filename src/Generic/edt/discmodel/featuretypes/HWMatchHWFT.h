// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HWMATCH_HW_FT_H
#define HWMATCH_HW_FT_H

#include "Generic/common/Assert.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"

/* this is a more limited version of HWMatchAllFT
 * it matches mention head words to entity head words.
 */
class HWMatchHWFT : public DTCorefFeatureType {
public:
	HWMatchHWFT() : DTCorefFeatureType(Symbol(L"hw-match-hw")) {}

	~HWMatchHWFT() {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{

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
		SymbolArray **mentArray = HWMap->get(ment->getUID());
		SerifAssert (mentArray != NULL);
		SerifAssert (*mentArray != NULL);
		const Symbol *mentHeadWords = (*mentArray)->getArray();
		int n_ment_words = (*mentArray)->getLength();
		SerifAssert (n_ment_words <= MAX_SENTENCE_TOKENS);
		
		Symbol entityMentionLevel = o->getEntityMentionLevel();

		for (int m = 0; m < entity->getNMentions(); m++) {
			Mention* entMent = o->getEntitySet()->getMention(entity->getMention(m)); 
			SymbolArray **entityArray = HWMap->get(entity->getMention(m));
			SerifAssert (entityArray != NULL);
			SerifAssert (*entityArray != NULL);
			const Symbol *entHeadWords = (*entityArray)->getArray();
			int n_ent_words = (*entityArray)->getLength();
			for (int i=0; i< n_ment_words; i++) {
				for (int j = 0; j < n_ent_words; j++) {
					if (mentHeadWords[i] == entHeadWords[j]) {
						if (nFeatures >= MAX_FEATURES_PER_EXTRACTION-3)
							return nFeatures;
						//else
						resultArray[nFeatures++] = _new DTQuintgramFeature(this, state.getTag(), PER_MENTION, mentEntTypeSymbol, entityMentionLevel, mentHeadWords[i]);
						if(entMent->getMentionType() == Mention::NAME) {
							resultArray[nFeatures++] = _new DTQuintgramFeature(this, state.getTag(), NAME_MENTION, mentEntTypeSymbol, entityMentionLevel, mentHeadWords[i]);
						}
 					}
					if (j==n_ent_words-1 && i==n_ment_words-1 && n_ent_words>1 && n_ment_words>1) {
						if (mentHeadWords[i] == entHeadWords[j]) {
							resultArray[nFeatures++] = _new DTQuintgramFeature(this, state.getTag(), MENTION_LAST_WORD, mentEntTypeSymbol, entityMentionLevel, mentHeadWords[i]);
							if(i==j)
								resultArray[nFeatures++] = _new DTQuintgramFeature(this, state.getTag(), MENTION_LAST_WORD_SAME_POS, mentEntTypeSymbol, entityMentionLevel, mentHeadWords[i]);
						}else {
							resultArray[nFeatures++] = _new DTQuintgramFeature(this, state.getTag(), MENTION_LAST_WORD_CLSH, mentEntTypeSymbol, entityMentionLevel, mentHeadWords[i]);
						}
					}
				}//for
			}
		}
		
		return nFeatures;
	}

};

#endif
