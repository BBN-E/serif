####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/descriptors
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR descriptors)

# include all the files in the directory here:
SET (LOCAL_FILES 
    kr_CompoundMentionFinder.cpp
    kr_CompoundMentionFinder.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
