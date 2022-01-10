// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WITHINBRACKETSHWMATCH_FT_H
#define WITHINBRACKETSHWMATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTSeptgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/wordClustering/WordClusterClass.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"

/* this feature tries to capture linkage between [Boston] and [the city of Boston]
 * where 'Boston' is a post-modifier of the nominal mention.
 */
class WithinBracketsHWMatchFT : public DTCorefFeatureType {
public:
	WithinBracketsHWMatchFT() : DTCorefFeatureType(Symbol(L"within-brackets-hw-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTSeptgramIntFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		o = static_cast<DTCorefObservation*>(state.getObservation(0));
		eset = o->getEntitySet();
		HWMap = o->getHWMentionMapper();
		wordsWithinBracketsMap = o->getWordsWithinBracketsMapper();

		entity = o->getEntity();
		ment = o->getMention();

		// get the mention EntityType
		EntityType mentEntType = ment->getEntityType();
		mentEntTypeSymbol = mentEntType.isDetermined() ? mentEntType.getName(): NO_ENTITY_TYPE;

		Symbol entityMentionLevel = o->getEntityMentionLevel();

		n_feat = 0;

		// get mention not-in-Head Words
		mentNotInHWArray = wordsWithinBracketsMap->get(ment->getUID());
		if (mentNotInHWArray != NULL) {
			mentNotInHeadWords = (*mentNotInHWArray)->getArray();
			n_ment_not_hw_words = (*mentNotInHWArray)->getLength();
			SerifAssert (n_ment_not_hw_words <= MAX_SENTENCE_TOKENS);
		}

		// get mention Head Words
		mentHWArray = HWMap->get(ment->getUID());
		SerifAssert (mentHWArray != NULL);
		SerifAssert (*mentHWArray != NULL);
		mentHeadWords = (*mentHWArray)->getArray();
		n_ment_hw_words = (*mentHWArray)->getLength();
		SerifAssert (n_ment_hw_words <= MAX_SENTENCE_TOKENS && n_ment_hw_words >0);

		// compute the mention word[0] cluster
		wchar_t c[200];
		WordClusterClass mentCluster(mentHeadWords[0], true);
		swprintf(c, 200, L"c4:%d", mentCluster.c4());
		Symbol mentWord0Cluster4 = Symbol(c);
		swprintf(c, 200, L"c6:%d", mentCluster.c6());
		Symbol mentWord0Cluster6 = Symbol(c);
		swprintf(c, 200, L"c8:%d", mentCluster.c8());
		Symbol mentWord0Cluster8 = Symbol(c);
		swprintf(c, 200, L"c12:%d", mentCluster.c12());
		Symbol mentWord0Cluster12 = Symbol(c);



		for (int m = 0; m < entity->getNMentions(); m++) {
			if(n_feat>MAX_FEATURES_PER_EXTRACTION-10)
				return n_feat;

			entMent = eset->getMention(entity->getMention(m)); 

			entityNotInHWArray = wordsWithinBracketsMap->get(entity->getMention(m));
			if (entityNotInHWArray != NULL) {
				SerifAssert (*entityNotInHWArray != NULL);
				entNotInHeadWords = (*entityNotInHWArray)->getArray();
				n_ent_not_hw_words = (*entityNotInHWArray)->getLength();
				SerifAssert (n_ent_not_hw_words <= MAX_SENTENCE_TOKENS);
			}
			norm_n_ent_not_hw_words = n_ent_not_hw_words<5 ? n_ent_not_hw_words : 5;

			entityHWArray = HWMap->get(entity->getMention(m));
			SerifAssert (entityHWArray != NULL);
			SerifAssert (*entityHWArray != NULL);
			entHeadWords = (*entityHWArray)->getArray();
			n_ent_hw_words = (*entityHWArray)->getLength();
			SerifAssert (n_ent_hw_words <= MAX_SENTENCE_TOKENS && n_ent_hw_words > 0);

			// compute the entity word[0] cluster
			WordClusterClass entityCluster(entHeadWords[0], true);
			swprintf(c, 200, L"c4:%d", entityCluster.c4());
			Symbol entityWord0Cluster4 = Symbol(c);
			swprintf(c, 200, L"c6:%d", entityCluster.c6());
			Symbol entityWord0Cluster6 = Symbol(c);
			swprintf(c, 200, L"c8:%d", entityCluster.c8());
			Symbol entityWord0Cluster8 = Symbol(c);
			swprintf(c, 200, L"c12:%d", entityCluster.c12());
			Symbol entityWord0Cluster12 = Symbol(c);
			
			
			for (int i=0; i< n_ment_hw_words; i++) {
				mentHW = mentHeadWords[i];

				// test for mention hw with entity non-hw
				if (entityNotInHWArray != NULL) {
					for (int j = 0; j < n_ent_not_hw_words; j++) {
						if(mentHW == entNotInHeadWords[j]) {
							if(n_feat>MAX_FEATURES_PER_EXTRACTION-20) return n_feat;
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, SymbolConstants::nullSymbol, PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, entHeadWords[0], 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, entityWord0Cluster4, 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, entityWord0Cluster6, 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, entityWord0Cluster8, 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
								, entityWord0Cluster12, 0);
							if(entMent->getMentionType() == Mention::NAME) {
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, SymbolConstants::nullSymbol, PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, entHeadWords[0], 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, entityWord0Cluster4, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, entityWord0Cluster6, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, entityWord0Cluster8, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, HW2NONHW, entityMentionLevel
									, entityWord0Cluster12, 0);
							}
						}
					}//for
				}

				// test for mention hw with entity hw
				//for (int j = 0; j < n_ent_hw_words; j++) {
				//	if(mentHW == entHeadWords[j]) {
				//		resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, mentEntTypeSymbol, HW2HW, entityMentionLevel);
				//		if(entMent->getMentionType() == Mention::NAME) {
				//			resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, HW2HW, entityMentionLevel);
				//		}
				//	}
				//}//for
			}//for

			if (mentNotInHWArray != NULL) {
				for (int i=0; i< n_ment_not_hw_words; i++) {
					mentHW = mentNotInHeadWords[i];

					// test for mention non-hw with entity non-hw
					if (entityNotInHWArray != NULL) {
						for (int j = 0; j < n_ent_not_hw_words; j++) {
							if(n_feat>MAX_FEATURES_PER_EXTRACTION-15) return n_feat;
							if(mentHW == entNotInHeadWords[j]) {
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
									, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
									, entHeadWords[0], 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
									, entityWord0Cluster4, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
									, entityWord0Cluster6, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
									, entityWord0Cluster8, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
									, entityWord0Cluster12, 0);
								if(entMent->getMentionType() == Mention::NAME) {
									resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
										, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
									resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
										, entHeadWords[0], 0);
									resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
										, entityWord0Cluster4, 0);
									resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
										, entityWord0Cluster6, 0);
									resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
										, entityWord0Cluster8, 0);
									resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2NONHW, entityMentionLevel
										, entityWord0Cluster12, 0);
								}
							}
						}//for
					}

					// test for mention non-hw with entity hw
					for (int j = 0; j < n_ent_hw_words; j++) {
						if(n_feat>MAX_FEATURES_PER_EXTRACTION-15) return n_feat;
						if(mentHW == entHeadWords[j]) {
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
								, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
								, mentHeadWords[0], 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
								, mentWord0Cluster4, 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
								, mentWord0Cluster6, 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
								, mentWord0Cluster8, 0);
							resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, SymbolConstants::nullSymbol, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
								, mentWord0Cluster12, 0);
							if(entMent->getMentionType() == Mention::NAME) {
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
									, SymbolConstants::nullSymbol, norm_n_ent_not_hw_words);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
									, mentHeadWords[0], 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
									, mentWord0Cluster4, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
									, mentWord0Cluster6, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
									, mentWord0Cluster8, 0);
								resultArray[n_feat++] = _new DTSeptgramIntFeature(this, state.getTag(), PER_MENTION, NAME, mentEntTypeSymbol, NONHW2HW, entityMentionLevel
									, mentWord0Cluster12, 0);								
							}
						}
					}//for
				}//for
			}//	if (mentNotInHWArray != NULL)

		}//for mention

		return n_feat;
	}




mutable DTCorefObservation *o;
mutable const MentionSymArrayMap *HWMap, *wordsWithinBracketsMap;
mutable SymbolArray **entityHWArray, **entityNotInHWArray;
mutable SymbolArray **mentHWArray, **mentNotInHWArray;
mutable const EntitySet *eset;
mutable const Mention *ment;
mutable const Mention* entMent;
mutable Entity *entity;

mutable Symbol mentHW;
mutable int n_ment_hw_words, n_ment_not_hw_words, norm_n_ent_not_hw_words;
mutable int n_ent_hw_words, n_ent_not_hw_words;
mutable Symbol mentEntTypeSymbol;
mutable int n_feat;
mutable const Symbol *mentNotInHeadWords, *mentHeadWords, *entNotInHeadWords, *entHeadWords;

};
#endif
