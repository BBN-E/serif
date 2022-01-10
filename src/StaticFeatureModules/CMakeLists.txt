########################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                             #
# All Rights Reserved.                                                 #
#                                                                      #
# StaticFeatureModules                                                 #
#                                                                      #
# This library is mainly intended for use by binaries that are built   #
# outside of cmake (eg brandy).  For projects that are built using     #
# cmake, the ADD_SERIF_EXECUTABLE macro will automatically generate an #
# executable-specific StaticFeatureModules.cpp file, and link it in to #
# the executable.                                                      #
#                                                                      #
########################################################################

# Fill in the contents of the StaticFeatureModules.cpp file.
SET(StaticFeatureModules_dst 
    "${DYNAMIC_STATIC_FEATURE_MODULES_DIR}/StaticFeatureModules.cpp")
GENERATE_STATIC_FEATURE_MODULES_CPP(
    "${StaticFeatureModules_dst}" ${STATICALLY_LINKED_FEATURE_MODULES})

# Build the StaticFeatureModules project.
ADD_SERIF_LIBRARY(StaticFeatureModules
  SOURCE_FILES
    ${StaticFeatureModules_dst}
    StaticFeatureModules.h
  LINK_LIBRARIES
    ${STATICALLY_LINKED_FEATURE_MODULES}
)

