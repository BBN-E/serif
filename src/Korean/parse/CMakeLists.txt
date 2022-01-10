####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/parse
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR parse)

# include all the files in the directory here:
SET (LOCAL_FILES 
  kr_LanguageSpecificFunctions.h
  kr_NodeInfo.cpp
  kr_NodeInfo.h
  kr_ParseSeeder.cpp
  kr_ParseSeeder.h
  kr_Parser.cpp
  kr_Parser.h
  kr_STags.cpp
  kr_STags.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
