// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/clutter/EntityTypeClutterFilter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventSet.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"

#include "Arabic/clutter/ar_ClutterFilter.h"

#include <map>
#include <string>


ArabicClutterFilter::ArabicClutterFilter () {
	_filterOn = ParamReader::getOptionalTrueFalseParamWithDefaultVal("filter_clutter", false);
	if (_filterOn) {
		bool _use_ACE2008EvalFilter = ParamReader::getOptionalTrueFalseParamWithDefaultVal("filter_clutter_use_ace2008eval_filter", false);
		if (_use_ACE2008EvalFilter) {
			addFilter(_new ACE2008EvalClutterFilter());
		}
		bool _filterWeapons = ParamReader::getOptionalTrueFalseParamWithDefaultVal("filter_clutter_filter_WEA", false);
		if (_filterWeapons) {
			addFilter(_new EntityTypeClutterFilter(L"WEA"));
		}
		bool _filterVEH = ParamReader::getOptionalTrueFalseParamWithDefaultVal("filter_clutter_filter_VEH", false);
		if (_filterWeapons) {
			addFilter(_new EntityTypeClutterFilter(L"VEH"));
		}
	}
}

ArabicClutterFilter::~ArabicClutterFilter () {
}

void ArabicClutterFilter::filterClutter (DocTheory *docTheory) {
	if (_filterOn)
		filterClutterRaw(docTheory);
}

