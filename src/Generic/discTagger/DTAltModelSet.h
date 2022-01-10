// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_ALTMODEL_SET_H
#define DT_ALTMODEL_SET_H

#include "Generic/common/Symbol.h"
class DTFeatureTypeSet;
class DTTagSet;
class DTObservation;
class P1Decoder;
class MaxEntModel;
class DTFeature;

#ifndef MAX_ALT_DT_DECODERS
#define MAX_ALT_DT_DECODERS 5
#endif

class DTAltModelSet{
public:
	DTAltModelSet();
	~DTAltModelSet();

	void initialize(Symbol modeltype, const char* paramName);

	int getNAltDecoders();
	Symbol getDecoderName(int i);
	Symbol getDecoderPrediction(int i, DTObservation* obs);
	Symbol getDecoderPrediction(Symbol name, DTObservation* obs);
	int addDecoderPredictionsToObservation(DTObservation* obs);

private:
	typedef struct{
		Symbol _name;
		int _type;
		P1Decoder* _p1Decoder;
		MaxEntModel *_maxentDecoder;
		DTTagSet* _tagSet;
		DTFeatureTypeSet* _features;
		DTFeature::FeatureWeightMap* _weights;
	} AltDecoder;
	AltDecoder _altDecoders[MAX_ALT_DT_DECODERS];
	int _nDecoders;

	enum {P1, MAXENT};

	bool _is_initialized;
};





#endif
