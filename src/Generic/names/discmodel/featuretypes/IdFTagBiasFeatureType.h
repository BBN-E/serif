// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_TAG_BIAS_FEATURE_TYPE_H
#define D_T_TAG_BIAS_FEATURE_TYPE_H


class IdFTagBiasFeatureType  : public PIdFFeatureType {

public:

	IdFTagBiasFeatureType() : PIdFFeatureType(Symbol(L"tag-bias"), InfoSource::OBSERVATION) {}

	~IdFTagBiasFeatureType(void){}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
	DTFeature **resultArray) const {
		TokenObservation* o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		resultArray[0] = _new DTMonogramFeature(this, state.getTag());
		return 1;
	}

};
#endif
