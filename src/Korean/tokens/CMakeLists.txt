####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/tokens
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR tokens)

# include all the files in the directory here:
SET (LOCAL_FILES 
  kr_Tokenizer.cpp
  kr_Tokenizer.h
  kr_Untokenizer.cpp
  kr_Untokenizer.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
