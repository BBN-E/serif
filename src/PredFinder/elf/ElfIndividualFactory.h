/**
 * Factory class for ElfIndividual and its subclasses.
 *
 * @file ElfIndividualFactory.h
 * @author nward@bbn.com
 * @date 2010.06.23
 **/

#pragma once

#include "Generic/common/bsp_declare.h"
#include "ElfIndividual.h"
#include <set>

class DocTheory;
BSP_DECLARE(PatternFeatureSet);

/**
 * Builds ElfIndividual object instances (possibly subclasses)
 * from various Serif/Brandy/LearnIt classes.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
class ElfIndividualFactory {
public:
	static ElfIndividual_ptr from_feature_set(const DocTheory* doc_theory, const PatternFeatureSet_ptr feature_set);
};
