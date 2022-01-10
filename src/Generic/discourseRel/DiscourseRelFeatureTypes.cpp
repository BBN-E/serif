// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

/* need to modify later */

// This is where the _new instances of the feature types live.

#include "Generic/discourseRel/DiscourseRelFeatureTypes.h"
#include "Generic/discourseRel/xx_DiscourseRelFeatureTypes.h"

//list .h files defining the features
#include "Generic/discourseRel/featuretypes/DRIsFirstWordFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRIsLastWordFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRParentPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRRightSiblingPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRLeftSiblingPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRP0RightSiblingPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRRightNeighborPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRLeftNeighborPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRP0LeftNeighborPOSCPPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRP0RightNeighborPOSCPPOSFeatureType.h"
#include "Generic/discourseRel/featuretypes/DRLeadingConn.h"
#include "Generic/discourseRel/featuretypes/DRShareEntities.h"
#include "Generic/discourseRel/featuretypes/DRShareNPArgEntities.h"
#include "Generic/discourseRel/featuretypes/DRShare2NPArgEntities.h"

bool DiscourseRelFeatureTypes::_instantiated = false;

void DiscourseRelFeatureTypes::ensureBaseFeatureTypesInstantiated(){
	if (_instantiated)
		return;
	_instantiated = true;
	
	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType
	_new DRIsFirstWordFeatureType();
	_new DRIsLastWordFeatureType();
	_new DRPOSFeatureType();
	_new DRParentPOSFeatureType();
	_new DRLeftSiblingPOSFeatureType();
	_new DRRightSiblingPOSFeatureType();
	_new DRP0RightSiblingPOSFeatureType();
	_new DRLeftNeighborPOSFeatureType();
	_new DRRightNeighborPOSFeatureType();
	_new DRP0LeftNeighborPOSCPPOSFeatureType();
	_new DRP0RightNeighborPOSCPPOSFeatureType();
	_new DRLeadingConn();
	_new DRShareEntities();
	_new DRShareNPArgEntities();
	_new DRShare2NPArgEntities();

}




boost::shared_ptr<DiscourseRelFeatureTypes::Factory> &DiscourseRelFeatureTypes::_factory() {
	static boost::shared_ptr<DiscourseRelFeatureTypes::Factory> factory(new GenericDiscourseRelFeatureTypesFactory());
	return factory;
}

