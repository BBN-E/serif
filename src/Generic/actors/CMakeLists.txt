####################################################################
# Copyright (c) 2013 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/actors                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(actors
  SOURCE_FILES
  ActorDB.h
  ActorDB.cpp
  ActorEditDistance.cpp
  ActorEditDistance.h
  ActorEntityScorer.cpp
  ActorEntityScorer.h
  ActorInfo.cpp
  ActorInfo.h
  ActorMentionFinder.cpp
  ActorMentionFinder.h
  ActorPattern.h
  ActorTokenSubsetTrees.cpp
  ActorTokenSubsetTrees.h
  AWAKEActorInfo.cpp
  AWAKEActorInfo.h
  AWAKEDB.cpp
  AWAKEDB.h
  AWAKEGazetteer.cpp
  AWAKEGazetteer.h
  Gazetteer.cpp
  Gazetteer.h
  Identifiers.cpp
  Identifiers.h
  JabariTokenMatcher.cpp
  JabariTokenMatcher.h

  LocationMentionResolver.cpp
  LocationMentionResolver.h
)
