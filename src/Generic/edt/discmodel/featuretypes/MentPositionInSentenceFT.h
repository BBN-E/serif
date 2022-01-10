// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_POS_IN_SENT_FT_H
#define MENT_POS_IN_SENT_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"

#include "Generic/theories/Mention.h"


class MentPositionInSentenceFT : public DTCorefFeatureType {
public:
	MentPositionInSentenceFT() : DTCorefFeatureType(Symbol(L"ment-pos-in-sent")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		
		int posInSent = o->getMention()->getNode()->getStartToken();
		if(posInSent>10)
			if(posInSent>15)
				resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"15-20"));
			else
				resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"10-15"));
		else{
			wchar_t number[10];
			swprintf(number, 10, L"N%d", posInSent);
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(number));
		}
		return 1;
	}

};
#endif
