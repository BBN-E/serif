// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACRONYMMATCH_FT_H
#define ACRONYMMATCH_FT_H

#include "Generic/common/Assert.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/AcronymMaker.h"

#include "Generic/theories/Mention.h"

#define MAX_ACRONYMS	16

// This feature creates a mention acronyms and check whether they appear in the entity.
// This is only one way check. The other way check of entity acronym against a mention
// is done thorugh the AbbrevTable feature type.
class AcronymMatchFT : public DTCorefFeatureType {
public:
	AcronymMatchFT() : DTCorefFeatureType(Symbol(L"acronym-match")) {
		acronyms = _new Symbol[MAX_ACRONYMS];
	}
	
	~AcronymMatchFT(){ delete [] acronyms; }

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		const MentionSymArrayMap *HWMap = o->getHWMentionMapper();
		const Mention *ment = o->getMention();
		Entity *entity = o->getEntity();
		EntityType mentType = ment->getEntityType();
		EntityType entType = entity->getType();
		Symbol mentTypeSym = mentType.isDetermined() ? mentType.getName() : SymbolConstants::nullSymbol;
		Symbol entTypeSym = entType.isDetermined() ? entType.getName() : SymbolConstants::nullSymbol;

		SymbolArray **mentArray = HWMap->get(ment->getUID());
		SerifAssert (mentArray != NULL);
		SerifAssert (*mentArray != NULL);
		const Symbol *mentHeadWords = (*mentArray)->getArray();
		int n_ment_words = (*mentArray)->getLength();
		SerifAssert (n_ment_words <= MAX_SENTENCE_TOKENS);
		int nAcronyms = AcronymMaker::getSingleton().generateAcronyms(mentHeadWords, n_ment_words, acronyms, MAX_ACRONYMS);

		bool acronym_found = false;
		bool discard = false;
		int n_feat=0;
		for (int i=0; i< nAcronyms; i++) {
			for (int m=0; m<entity->getNMentions(); m++) {
				SymbolArray **entityArray = HWMap->get(entity->getMention(m));
				SerifAssert (entityArray != NULL);
				SerifAssert (*entityArray != NULL);
				int n_ent_words = (*entityArray)->getLength();
				if (n_ent_words!=1)
					continue;
				Symbol entHeadWord = ((*entityArray)->getArray())[0];
				if (acronyms[i] == entHeadWord) {
					acronym_found = true;

					if( n_feat >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-1)){
						discard = true;
						break;
					}else{
						resultArray[n_feat++] = _new DTTrigramFeature(this, state.getTag(), mentTypeSym, MATCH_SYM);
					}
				}else { // try edit distance with acronyms here
				}
			}//for
		}//for
		if (acronym_found){
			if( n_feat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
				discard = true;
			}else{
				resultArray[n_feat++] = _new DTTrigramFeature(this, state.getTag(), mentTypeSym, MATCH_UNIQU_SYM);
			}
		}
		if (discard) {
			SessionLogger::warn("DT_feature_limit") 
				<<"AcronymMatchFT discarding features beyond MAX_FEATURES_PER_EXTRACTION";
		}
		return n_feat;
	}

	Symbol *acronyms;

};
#endif
