####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/common
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR common)

# include all the files in the directory here:
SET (LOCAL_FILES 
  UnicodeEucKrEncoder.cpp
  UnicodeEucKrEncoder.h
  kr_StringTransliterator.cpp
  kr_StringTransliterator.h
  kr_WordConstants.cpp
  kr_WordConstants.h
  kr_version.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
