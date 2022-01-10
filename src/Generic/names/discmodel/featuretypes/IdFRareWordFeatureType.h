// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_RARE_WORD_FEATURE_TYPE_H
#define D_T_RARE_WORD_FEATURE_TYPE_H


class IdFRareWordFeatureType  : public PIdFFeatureType {
private:
	static const int rare_threshold = 5;

public:

	IdFRareWordFeatureType() : PIdFFeatureType(Symbol(L"rare-word"), InfoSource::OBSERVATION) {}

	~IdFRareWordFeatureType(void){}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
		DTFeature **resultArray) const {
			TokenObservation* o = static_cast<TokenObservation*>(
				state.getObservation(state.getIndex()));
			if(o->getWordCount()<rare_threshold){
//				cerr<<o->getSymbol()<<"    "<<o->getWordCount();
//				cerr<<"    rare passed "<<o->getWordCount()<<endl;
				resultArray[0] = _new DTMonogramFeature(this, state.getTag());
				return 1;
			}
			/*else{
				cerr<<o->getSymbol()<<"    "<<o->getWordCount();
				cerr<<"    isn't rare passed "<<o->getWordCount()<<endl;
			}*/
		return 0;
	}

};
#endif
