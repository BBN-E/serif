###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# ICEWS                                                       #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(ICEWS
  EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
  SOURCE_FILES
    ICEWSModule.cpp
    ICEWSModule.h

    ActorInfo.cpp
    ActorInfo.h
    ActorMention.cpp
    ActorMention.h
    ActorMentionSet.cpp
    ActorMentionSet.h
    ActorMentionFinder.cpp
    ActorMentionFinder.h
    ActorMentionPattern.h
    ActorMentionPattern.cpp
    EventMention.cpp
    EventMention.h
    EventMentionFinder.h
    EventMentionFinder.cpp
    EventMentionPattern.h
    EventMentionPattern.cpp
    EventMentionSet.cpp
    EventMentionSet.h
    EventType.h
    EventType.cpp
    Gazetteer.h
    Gazetteer.cpp
    ICEWSDB.cpp
    ICEWSDB.h
    ICEWSDocumentReader.h
    ICEWSDocumentReader.cpp
    ICEWSDriver.cpp
    ICEWSDriver.h
    ICEWSOutputWriter.cpp
    ICEWSOutputWriter.h
    Identifiers.h
    Identifiers.cpp
    JabariTokenMatcher.h
    JabariTokenMatcher.cpp
	LocationMentionResolver.cpp
	LocationMentionResolver.h
    SentenceSpan.h
    SentenceSpan.cpp
    Stories.h
    Stories.cpp
    TenseDetection.h
    TenseDetection.cpp
    ICEWSQueueFeeder.h
  SUBDIRS
  LINK_LIBRARIES
    Generic
    ${MySQLClient_LIBRARIES}
  )
