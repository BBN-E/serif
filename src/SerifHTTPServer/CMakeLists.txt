####################################################################
# Copyright (c) 2010 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# SerifHTTPServer                                                  #
#                                                                  #
####################################################################

ADD_SERIF_FEATURE_MODULE(SerifHTTPServer
  SOURCE_FILES
    Doc2OutTask.cpp
    Doc2OutTask.h
    HTTPConnection.cpp
    HTTPConnection.h
    IncomingHTTPConnection.cpp
    IncomingHTTPConnection.h
    IOConnection.h
    IOConnection.cpp
    OutgoingHTTPConnection.cpp
    OutgoingHTTPConnection.h
    PatternMatchDocumentTask.cpp
    PatternMatchDocumentTask.h
    ProcessDocumentTask.cpp
    ProcessDocumentTask.h
    SerifHTTPServer.cpp
    SerifHTTPServer.h
    SerifWorkQueue.cpp
    SerifWorkQueue.h
    SerifHTTPServerModule.cpp
    SerifHTTPServerModule.h
)
