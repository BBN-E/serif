####################################################################
# Copyright (c) 2013 by Raytheon BBN Technologies                  #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/brandy.cmake                                               #
#                                                                  #
# Adding executables for Brandy solution                           #
####################################################################

SET(CREATE_BRANDY_PROJECTS OFF CACHE BOOL
    "Build the Brandy projects")
MARK_AS_ADVANCED (CREATE_BRANDY_PROJECTS)

IF (CREATE_BRANDY_PROJECTS)

	IF (NOT SYMBOL_THREADSAFE)
		MESSAGE( WARNING "When building Brandy, you *must* enable SYMBOL_THREADSAFE for ThreadedDocumentLoader" )
	ENDIF ()

    # Enable Brandy solution
    ADD_SUBDIRECTORY(../Brandy Brandy)

ENDIF (CREATE_BRANDY_PROJECTS)
