####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# English/edt/MentionGroups/extractors                             #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(extractors
  SOURCE_FILES
    en_AliasExtractor.cpp
    en_AliasExtractor.h
    en_GPENameVariationsExtractor.cpp
    en_GPENameVariationsExtractor.h
    en_OperatorExtractor.cpp
    en_OperatorExtractor.h
    en_OrgNameVariationsExtractor.cpp
    en_OrgNameVariationsExtractor.h
    en_ParentheticalExtractor.cpp
    en_ParentheticalExtractor.h
    en_ParentheticalPairExtractor.cpp
    en_ParentheticalPairExtractor.h
    en_PersonNamePairExtractor.cpp
    en_PersonNamePairExtractor.h
    #en_PersonNameVariationsExtractor.cpp -- moved to EnglishPersonNameVariationsExtractor.cpp, under Generic
    #en_PersonNameVariationsExtractor.h -- moved to EnglishPersonNameVariationsExtractor.cpp, under Generic
    en_TitleExtractor.cpp
    en_TitleExtractor.h
    en_WHQLinkExtractor.cpp
    en_WHQLinkExtractor.h
)
