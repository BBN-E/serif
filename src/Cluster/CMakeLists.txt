###############################################################
# Copyright (c) 2007 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Cluster                                                     #
#                                                             #
###############################################################

ADD_SERIF_LIBRARY(ClusterLib
  SUBDIRS
	Cluster
	ExtractBigrams
	ReplaceLowFreq
  LINK_LIBRARIES
	Generic)

ADD_SERIF_EXECUTABLE(Cluster
  SOURCE_FILES 
	ClusterMain.cpp
  LINK_LIBRARIES
	ClusterLib
)
