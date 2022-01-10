###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Generic/ASR/sentBreaker                                     #
#                                                             #
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(sentBreaker
  SOURCE_FILES
    ASRSentBreaker.cpp
    ASRSentBreaker.h
    ASRSentBreakerCustomModel.cpp
    ASRSentBreakerCustomModel.h
    ASRSentBreakerFVecModel.h
    ASRSentBreakerModel.h
    ASRSentBreakerTrainer.cpp
    ASRSentBreakerTrainer.h
    ASRSentModelFVecFilter.h
    ASRSentModelFilter.h
    ASRSentModelInstance.h
)
