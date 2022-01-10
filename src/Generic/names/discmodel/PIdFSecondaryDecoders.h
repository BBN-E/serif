// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PIDF_SECONDARY_DECODERS_H
#define PIDF_SECONDARY_DECODERS_H
#define MAX_DECODERS 10

#include "Generic/common/Symbol.h"
//#include "Generic/discTagger/DTFeature.h"
//#include "Generic/discTagger/DTTagSet.h"
//#include "Generic/discTagger/DTFeatureTypeSet.h"
//#include "Generic/discTagger/PDecoder.h"

class UTF8InputStream;
class UTF8OutputStream;
class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class IdFSentence;
class IdFWordFeatures;
class PDecoder;
class NameTheory;
class TokenSequence;
class PIdFSentence;
class DocTheory;
class DTObservation;

class PIdFSecondaryDecoders {
public:
	PIdFSecondaryDecoders();
	~PIdFSecondaryDecoders();
	void AddDecoderResultsToObservation(std::vector<DTObservation *> & observations);
private:
	//	LexicalEntry* _entries_by_id[MAX_ENTRIES];
	Symbol _decoderNames[MAX_DECODERS];
	PDecoder* _secondaryDecoders[MAX_DECODERS];
	DTFeature::FeatureWeightMap *_secondaryWeights[MAX_DECODERS];
	DTTagSet *_secondaryTagSets[MAX_DECODERS];
	DTFeatureTypeSet *_secondaryFeatureTypes[MAX_DECODERS];
	int _n_decoders;


};
#endif
