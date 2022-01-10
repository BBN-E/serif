####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/reader                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(reader
  SOURCE_FILES
    DefaultDocumentReader.cpp
    DefaultDocumentReader.h
    DefaultDocumentSplitter.cpp
    DefaultDocumentSplitter.h
    DocumentReader.cpp
    DocumentReader.h
    DocumentSplitter.cpp
    DocumentSplitter.h
    DocumentZoner.cpp
    DocumentZoner.h
	DTRADocumentReader.cpp
	DTRADocumentReader.h
    INQADocumentReader.cpp
    INQADocumentReader.h
    MTDocumentReader.cpp
    MTDocumentReader.h
    ProseDocumentZoner.cpp
    ProseDocumentZoner.h
    RegionSpan.h
    RegionSpan.cpp
    RegionSpanCreator.h
    RegionDocumentSplitter.cpp
    RegionDocumentSplitter.h
    SectionHeadersDocumentZoner.cpp
    SectionHeadersDocumentZoner.h
    SGMLTag.cpp
    SGMLTag.h
    VIKTRSDocumentReader.cpp
    VIKTRSDocumentReader.h
    xx_DocumentReader.h
)
