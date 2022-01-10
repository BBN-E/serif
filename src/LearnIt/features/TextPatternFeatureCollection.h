#pragma once
#include <vector>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/features/CanopyUtils.h"
#include "LearnIt/features/FeatureCollectionIterator.h"

BSP_DECLARE(TextPatternFeatureCollection)
BSP_DECLARE(BrandyPatternFeature)
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(Target)

class TextPatternFeatureCollection {
public:
	static TextPatternFeatureCollection_ptr createFromTextPatterns(Target_ptr target, 
		FeatureAlphabet_ptr alphabet, double threshold, bool include_negatives);
	static TextPatternFeatureCollection_ptr createFromKeywordInSentencePatterns(
		Target_ptr target, FeatureAlphabet_ptr alphabet, double threshold,
		bool include_negatives);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TextPatternFeatureCollection,
		Target_ptr, const std::vector<BrandyPatternFeature_ptr>&,
		const std::wstring&);
	FeatureCollectionIterator<BrandyPatternFeature> applicableFeatures(
		const SlotFillersVector& slotFillersVector, const DocTheory* docInfo, 
		int sent_no);
private:
	static TextPatternFeatureCollection_ptr create(const std::wstring& featureClass,
		Target_ptr target, FeatureAlphabet_ptr alphabet, double threshold, 
		bool include_negatives, const std::wstring& search_pattern);
	TextPatternFeatureCollection(Target_ptr target,
		const std::vector<BrandyPatternFeature_ptr>& features,
		const std::wstring& regex);
	
	typedef CanopyUtils<Symbol> SymbolCanopy;
	typedef std::map<int, int> FeatureVotes;
	
	std::vector<BrandyPatternFeature_ptr> _features;
	std::set<int> _applicableFeatures;
	std::vector<int> _votes_required;
	std::vector<int> _alwaysUse;
	SymbolCanopy::KeyToFeatures _wordsToPatterns;
	FeatureVotes _votes;
};
