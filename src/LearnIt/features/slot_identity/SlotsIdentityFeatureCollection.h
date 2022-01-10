#pragma once
#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/FeatureCollectionIterator.h"
#include "LearnIt/features/slot_identity/SlotsCanopy.h"

BSP_DECLARE(SlotsIdentityFeatureCollection)
BSP_DECLARE(SlotsIdentityFeature)
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(SlotsCanopy)
BSP_DECLARE(Target)

class SlotsIdentityFeatureCollection {
public:
	static SlotsIdentityFeatureCollection_ptr create(Target_ptr target, 
		FeatureAlphabet_ptr alphabet, double threshold,
		bool include_negatives);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(SlotsIdentityFeatureCollection,
		Target_ptr, const std::vector<SlotsIdentityFeature_ptr>&);
	FeatureCollectionIterator<SlotsIdentityFeature> applicableFeatures(
		const SlotFillersVector& slotFillersVector, const DocTheory* doc, 
		int sent_no);
private:
	SlotsIdentityFeatureCollection(Target_ptr target,
		const std::vector<SlotsIdentityFeature_ptr>& features);
	std::vector<SlotsIdentityFeature_ptr> _features;
	std::set<int> _applicableFeatures;
	SlotsCanopy _canopy;
};
