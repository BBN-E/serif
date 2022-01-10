// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/discourseRel/en_DiscourseRelFeatureTypes.h"

#include "English/discourseRel/featuretypes/en_lcW0LeftSiblingHead.h"
#include "English/discourseRel/featuretypes/en_lcW0LeftSiblingPOS.h"
#include "English/discourseRel/featuretypes/en_lcW0RightSiblingHead.h"
#include "English/discourseRel/featuretypes/en_lcW0RightSiblingPOS.h"
#include "English/discourseRel/featuretypes/en_lcWord.h"
#include "English/discourseRel/featuretypes/en_lcW0ParentPOS.h"
#include "English/discourseRel/featuretypes/en_leftSiblingHead.h"
#include "English/discourseRel/featuretypes/en_rightSiblingHead.h"
#include "English/discourseRel/featuretypes/en_leftNeighborHead.h"
#include "English/discourseRel/featuretypes/en_rightNeighborHead.h"
#include "English/discourseRel/featuretypes/en_lcW0LeftNeighborHead.h"
#include "English/discourseRel/featuretypes/en_lcW0LeftNeighborPOS.h"
#include "English/discourseRel/featuretypes/en_lcW0RightNeighborHead.h"
#include "English/discourseRel/featuretypes/en_lcW0RightNeighborPOS.h"
#include "English/discourseRel/featuretypes/en_lcW0LeftNeighborPOSCPPOS.h"
#include "English/discourseRel/featuretypes/en_lcW0RightNeighborPOSCPPOS.h"
#include "English/discourseRel/featuretypes/en_wordPair.h"
#include "English/discourseRel/featuretypes/en_wordNetPair.h"

bool EnglishDiscourseRelFeatureTypes::_instantiated = false;


void EnglishDiscourseRelFeatureTypes::ensureFeatureTypesInstantiated() {

	// get the language independent featureTypes
	if (EnglishDiscourseRelFeatureTypes::_instantiated)
		return;
	DiscourseRelFeatureTypes::ensureBaseFeatureTypesInstantiated();


	_new EnglishLcW0LeftSiblingHead();
	_new EnglishLcW0LeftSiblingPOS(); 
	_new EnglishLcW0RightSiblingHead();
	_new EnglishLcW0RightSiblingPOS();
	_new EnglishLcWord();
	_new EnglishLcW0ParentPOS();
	_new EnglishLeftSiblingHead();
	_new EnglishRightSiblingHead();
	_new EnglishLeftNeighborHead();
	_new EnglishRightNeighborHead();
	_new EnglishLcW0LeftNeighborHead();
	_new EnglishLcW0LeftNeighborPOS(); 
	_new EnglishLcW0RightNeighborHead();
	_new EnglishLcW0RightNeighborPOS();
	_new EnglishLcW0LeftNeighborPOSCPPOS();
	_new EnglishLcW0RightNeighborPOSCPPOS();
	_new EnglishWordPair();
	_new EnglishWordNetPair();


}
