####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/morphology
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR morphology)

# include all the files in the directory here:
SET (LOCAL_FILES 
  kr_FeatureValueStructure.cpp
  kr_FeatureValueStructure.h
  kr_Klex.cpp
  kr_Klex.h
  kr_MorphDecoder.cpp
  kr_MorphDecoder.h
  kr_MorphModel.cpp
  kr_MorphModel.h
  kr_MorphologicalAnalyzer.cpp
  kr_MorphologicalAnalyzer.h
  kr_Retokenizer.cpp
  kr_Retokenizer.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
