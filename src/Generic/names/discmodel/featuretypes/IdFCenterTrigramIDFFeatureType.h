// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_CENTERTRIGRAM_IDF_FEATURE_TYPE_H
#define D_T_IDF_CENTERTRIGRAM_IDF_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

/* A trigram feature of IdfIdfFeatureType
*/
class IdFCenterTrigramIDFFeatureType : public PIdFFeatureType {
public:
	IdFCenterTrigramIDFFeatureType() : PIdFFeatureType(Symbol(L"centertrigramIdF"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if((state.getIndex()-1) < 0){
			return 0;
		}
		if((state.getIndex()+1) >= state.getNObservations()){
			return 0;
		}
		TokenObservation *o_1 = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()-1));
		TokenObservation *o_2= static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		TokenObservation *o_3= static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()+1));
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), o_1->getIdFWordFeature(),
			o_2->getIdFWordFeature(), o_3->getIdFWordFeature());
		return 1;
	}


};

#endif
