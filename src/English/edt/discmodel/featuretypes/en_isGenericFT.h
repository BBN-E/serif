// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_GENERIC_FT_H
#define en_GENERIC_FT_H

#include "Generic/common/Symbol.h"
//#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "English/edt/en_DescLinkFeatureFunctions.h"
#include "Generic/theories/Mention.h"


class EnglishIsGenericFT : public DTCorefFeatureType {
public:

	EnglishIsGenericFT() : DTCorefFeatureType(Symbol(L"is-ment-generic")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}


	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
//		std::cerr<<"b4 EnglishIsGenericFT->extractFeatures()"<<std::endl;
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
//		std::cerr<<"af EnglishIsGenericFT->extractFeatures()"<<std::endl;

		
		if(EnglishDescLinkFeatureFunctions::_isGenericMention(o->getMention(), o->getEntitySet())){
//		if(EnglishDescLinkFeatureFunctions::_isDefinite(o->getMention()->getNode())){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}//else
		return 0;
	}


};
#endif
