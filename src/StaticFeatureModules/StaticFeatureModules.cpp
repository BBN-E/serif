// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

@AUTO_GENERATED_FILE@

/************************************************************************
 * NOTE: This is a template file, that is filled in by CMake before it
 * is compiled.  The template output is written to:
 * 
 *     ${DYNAMIC_INCLUDES_DIR}/StaticFeatureModules.cpp
 * 
 * "REGISTER_STATIC_FEATURE_INCLUDES" is replaced by #include
 * statements for each statically linked feature module.
 * 
 * "REGISTER_STATIC_FEATURE_COMMANDS" is replaced by calls to
 * FeatureModule::registerModule() for each statically linked feature
 * module.
 * 
 * List of feature modules registered by this file:
 *     @STATICALLY_LINKED_FEATURE_MODULES@
 ************************************************************************/

#include "Generic/common/leak_detection.h"
#include "StaticFeatureModules/StaticFeatureModules.h"
#include "Generic/common/FeatureModule.h"

@REGISTER_STATIC_FEATURE_INCLUDES@

void registerStaticFeatureModules() {
@REGISTER_STATIC_FEATURE_COMMANDS@
}
