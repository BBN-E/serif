/**
 * Factory class for ElfRelationArg.
 *
 * @file ElfRelationArg.h
 * @author afrankel@bbn.com
 * @date 2010.08.03
 **/

#pragma once

#include "Generic/theories/DocTheory.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "ElfRelationArg.h"
#include "Generic/common/bsp_declare.h"

#include <set>

BSP_DECLARE(ReturnPatternFeature);

/**
 * Returns a vector of ElfRelationArg instances. Its length is 1
 * unless it is (a) extracting subpatterns from a regex with parenthesized
 * subexpressions or (b) dealing with an NFLPlayer.
 *
 * @author afrankel@bbn.com
 * @date 2010.08.03
 **/
class ElfRelationArgFactory {
public:
	static std::vector<ElfRelationArg_ptr> from_return_feature(const DocTheory* doc_theory, 
		ReturnPatternFeature_ptr feature);
private:
	static std::vector<ElfRelationArg_ptr> nfl_player_from_return_feature(const DocTheory* doc_theory, 
													ReturnPatternFeature_ptr feature);
	static std::vector<ElfRelationArg_ptr> from_return_feature_w_regex(const DocTheory* doc_theory, 
													ReturnPatternFeature_ptr feature);
};
