// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Generic/PNPChunking/featuretypes/NPChunkWordFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPrevTagFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkReducedTagFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPrevWordFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkNextWordFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkIdFWordFeatFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkWC8FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkWC12FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkWC16FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkWC20FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPrevWC8FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPrevWC12FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPrevWC16FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPrevWC20FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkNextWC8FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkNextWC12FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkNextWC16FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkNextWC20FeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkP2WordFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkN2WordFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkPOSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkP1POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkP2POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkN1POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkN2POSFeatureType.h"

#include "Generic/PNPChunking/featuretypes/NPChunkP1W_WordFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkN1W_WordFeatureType.h"

#include "Generic/PNPChunking/featuretypes/NPChunkP2P1POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkN1N2POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkP1POS_POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkN1POS_POSFeatureType.h"

#include "Generic/PNPChunking/featuretypes/NPChunkP2P1P0POSFeatureType.h"

#include "Generic/PNPChunking/featuretypes/NPChunkN0N1N2POSFeatureType.h"
#include "Generic/PNPChunking/featuretypes/NPChunkP1P0N1POSFeatureType.h"




#include "Generic/PNPChunking/PNPChunkFeatureTypes.h"


bool PNPChunkFeatureTypes::_instantiated = false;

void PNPChunkFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new NPChunkWordFeatureType();
	_new NPChunkPrevTagFeatureType();
	_new NPChunkReducedTagFeatureType();
	_new NPChunkPrevWordFeatureType();
	_new NPChunkNextWordFeatureType();
	//_new NPChunkLCWordFeatureType();
	_new NPChunkIdFWordFeatFeatureType();
	_new NPChunkWC8FeatureType();
	_new NPChunkWC12FeatureType();
	_new NPChunkWC16FeatureType();
	_new NPChunkWC20FeatureType();
	_new NPChunkPrevWC8FeatureType();
	_new NPChunkPrevWC12FeatureType();
	_new NPChunkPrevWC16FeatureType();
	_new NPChunkPrevWC20FeatureType();
	_new NPChunkNextWC8FeatureType();
	_new NPChunkNextWC12FeatureType();
	_new NPChunkNextWC16FeatureType();
	_new NPChunkNextWC20FeatureType();
	_new NPChunkP2WordFeatureType();
	_new NPChunkN2WordFeatureType();
	_new NPChunkPOSFeatureType();
	_new NPChunkP2POSFeatureType();
	_new NPChunkP1POSFeatureType();
	_new NPChunkN2POSFeatureType();
	_new NPChunkN1POSFeatureType();
	
	_new NPChunkP1W_WordFeatureType();
	_new NPChunkN1W_WordFeatureType();
	_new NPChunkP2P1POSFeatureType();
	_new NPChunkN1N2POSFeatureType();
	_new NPChunkP1POS_POSFeatureType();
	_new NPChunkN1POS_POSFeatureType();
	_new NPChunkP2P1P0POSFeatureType();
	_new NPChunkP1P0N1POSFeatureType();
	_new NPChunkN0N1N2POSFeatureType();





}

