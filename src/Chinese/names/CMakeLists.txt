####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Chinese/names 
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(names
  SOURCE_FILES
    ch_IdFNameRecognizer.cpp
    ch_IdFNameRecognizer.h
    ch_IdFWordFeatures.cpp
    ch_IdFWordFeatures.h
    ch_NameRecognizer.cpp
    ch_NameRecognizer.h
    discmodel/ch_PIdFCharModel.cpp
    discmodel/ch_PIdFCharModel.h
#  discmodel/ch_PIdFCharModelStub.cpp   # not included
#  discmodel/ch_PIdFCharModelStub.h # not included
    discmodel/ch_PIdFCharSentence.cpp
    discmodel/ch_PIdFCharSentence.h
    discmodel/ch_PIdFNameRecognizer.cpp
    discmodel/ch_PIdFNameRecognizer.h
)
