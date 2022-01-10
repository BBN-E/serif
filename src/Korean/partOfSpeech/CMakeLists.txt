####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Korean/partOfSpeech
#                                                                  #
####################################################################

# set the current directory
SET (CURRENT_DIR partOfSpeech)

# include all the files in the directory here:
SET (LOCAL_FILES 
    kr_PartOfSpeechRecognizer.cpp
    kr_PartOfSpeechRecognizer.h
)

# include all the subdirectories here:
SET (LOCAL_SUBDIRS
)

ADD_CMAKELIST_AND_REL_PATH( ${RELATIVE_PATH} "${LOCAL_FILES}")
