#ifndef _RELATION_FEATURE_COLLECTION_H_
#define _RELATION_FEATURE_COLLECTION_H_

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/features/CanopyUtils.h"
#include "LearnIt/features/FeatureCollectionIterator.h"

BSP_DECLARE(RelationFeatureCollection)
BSP_DECLARE(BrandyPatternFeature)
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(Target)

class RelationFeatureCollection {
public:
	static PropPatternFeatureCollection_ptr create(Target_ptr target,
		const FeatureAlphabet& alphabet, double threshold, bool include_negatives);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(PropPatternFeatureCollection,
		Target_ptr, const std::vector<BrandyPatternFeature_ptr>&);
	FeatureCollectionIterator<BrandyPatternFeature> applicableFeatures(
		const SlotFillersVector& slotFillersVector, const DocTheory* doc, 
		int sent_no, const MentionToEntityMap& mentionToEntityMap);
private:
	typedef CanopyUtils<Symbol> SymbolCanopy;
	PropPatternFeatureCollection(Target_ptr target,
		const std::vector<BrandyPatternFeature_ptr>& features);
	std::vector<BrandyPatternFeature_ptr> _features;
	std::set<int> _applicableFeatures;
	SymbolCanopy::KeyToFeatures _predicatesToPatterns;
};
#endif

