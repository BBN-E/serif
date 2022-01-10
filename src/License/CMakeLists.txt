###############################################################
# Copyright 2015 by Raytheon BBN Technologies Corp            #
# All Rights Reserved                                         #
#                                                             #
# License                                                     #
# Port of Nemesis licensing from Cube2.                       #
###############################################################

ADD_SERIF_FEATURE_MODULE(License
  EXCLUDED_BY_DEFAULT
  SOURCE_FILES
    LicenseModule.cpp
    LicenseModule.h
  SUBDIRS
    nemesis
    nemesis_lic
    nemesis_log
  LINK_LIBRARIES
    ${OpenSSL_LIBRARY}
    ${Boost_LIBRARIES}
  UNIX_LINK_LIBRARIES
    -lz
  WIN_LINK_LIBRARIES
    iphlpapi.lib
)

ADD_SERIF_EXECUTABLE(create_license
  SOURCE_FILES
    create_license.cpp
)

ADD_SERIF_EXECUTABLE(print_license
  SOURCE_FILES
    print_license.cpp
)

ADD_SERIF_EXECUTABLE(verify_license
  SOURCE_FILES
    verify_license.cpp
)
