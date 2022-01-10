####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# MaxEntSimulatedActiveLearning                                    #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(MaxEntSimulatedActiveLearning
 SOURCE_FILES
   Definitions.h
#   InstanceSet.cpp	# not included
#   InstanceSet.h	# not included
   InstanceSetTester.h
   MaxEntSALRelationMain.cpp
   MaxEntSALRelationTrainer.cpp
   MaxEntSALRelationTrainer.h
   ParameterStruct.cpp
   ParameterStruct.h
#   RelationInstance.cpp	# not included
#   RelationInstance.h		# not included
   StatTracker.h
   Utilities.cpp
   Utilities.h
   maxentSAL-devtest.par
   maxentSAL.par
)
