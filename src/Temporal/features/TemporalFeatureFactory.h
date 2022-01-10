#ifndef _TEMPORAL_FEATURE_FACTORY_H_
#define _TEMPORAL_FEATURE_FACTORY_H_

#include <string>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(TemporalFeature)
	
class TemporalFeatureFactory {
public:
	static TemporalFeature_ptr create(const std::wstring& metadata);
};

#endif

