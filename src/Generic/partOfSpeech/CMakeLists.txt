####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/partOfSpeech                                             #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(partOfSpeech
  SOURCE_FILES
    DefaultPartOfSpeechRecognizer.h
    PartOfSpeechRecognizer.cpp
    PartOfSpeechRecognizer.h
    xx_PartOfSpeechRecognizer.h
    discmodel/PPartOfSpeechFeatureType.h
    discmodel/PPartOfSpeechFeatureTypes.h
    discmodel/PPartOfSpeechModel.h
    discmodel/PPartOfSpeechRecognizer.cpp
    discmodel/PPartOfSpeechRecognizer.h
    discmodel/PPartOfSpeechSentence.h
    discmodel/featuretypes/PartOfSpeechIdFWordFeatFeatureType.h
    discmodel/featuretypes/PartOfSpeechLCWordFeatureType.h
    discmodel/featuretypes/PartOfSpeechN1W_WordFeatureType.h
    discmodel/featuretypes/PartOfSpeechN2WordFeatureType.h
    discmodel/featuretypes/PartOfSpeechNextWC12FeatureType.h
    discmodel/featuretypes/PartOfSpeechNextWC16FeatureType.h
    discmodel/featuretypes/PartOfSpeechNextWC20FeatureType.h
    discmodel/featuretypes/PartOfSpeechNextWC8FeatureType.h
    discmodel/featuretypes/PartOfSpeechNextWordFeatureType.h
    discmodel/featuretypes/PartOfSpeechP1W_WordFeatureType.h
    discmodel/featuretypes/PartOfSpeechP2WordFeatureType.h
    discmodel/featuretypes/PartOfSpeechPrevTagFeatureType.h
    discmodel/featuretypes/PartOfSpeechPrevWC12FeatureType.h
    discmodel/featuretypes/PartOfSpeechPrevWC16FeatureType.h
    discmodel/featuretypes/PartOfSpeechPrevWC20FeatureType.h
    discmodel/featuretypes/PartOfSpeechPrevWC8FeatureType.h
    discmodel/featuretypes/PartOfSpeechPrevWordFeatureType.h
    discmodel/featuretypes/PartOfSpeechWC12FeatureType.h
    discmodel/featuretypes/PartOfSpeechWC16FeatureType.h
    discmodel/featuretypes/PartOfSpeechWC20FeatureType.h
    discmodel/featuretypes/PartOfSpeechWC8FeatureType.h
    discmodel/featuretypes/PartOfSpeechWordFeatureType.h
)
