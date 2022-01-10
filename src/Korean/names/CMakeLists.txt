####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/names 
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR names)

# include all the files in the directory here:
SET (LOCAL_FILES 
  kr_IdFWordFeatures.cpp
  kr_IdFWordFeatures.h
  kr_NameRecognizer.cpp
  kr_NameRecognizer.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
