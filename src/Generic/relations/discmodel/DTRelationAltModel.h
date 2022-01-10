// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_RELATION_ALTMODEL_H
#define DT_RELATION_ALTMODEL_H

#include "Generic/common/Symbol.h"
class DTFeatureTypeSet;
class DTTagSet;
class DTRelationSet;
class RelationObservation;
class P1Decoder;
class MaxEntModel;
class DTFeature;

#ifndef MAX_ALT_DT_DECODERS
#define MAX_ALT_DT_DECODERS 5
#endif 

class DTRelationAltModel{
public:
	DTRelationAltModel();
	~DTRelationAltModel();

	void initialize();

	int getNAltDecoders();
	Symbol getDecoderName(int i);
	Symbol getDecoderPrediction(int i, RelationObservation* obs);
	Symbol getDecoderPrediction(Symbol name, RelationObservation* obs);
	int addDecoderPredictionsToObservation(RelationObservation* obs);

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
