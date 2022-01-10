###############################################################
# Copyright (c) 2012 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# OpenSSLCipherStream                                         #
# Provides support for reading encrypted streams              #
###############################################################
IF (OpenSSL_FOUND)
  ADD_SERIF_FEATURE_MODULE(OpenSSLCipherStream
    EXCLUDED_BY_DEFAULT
  SOURCE_FILES
    UTF8OpenSSLCipherInputStream.h
    UTF8OpenSSLCipherInputStream.cpp
    OpenSSLCipherStreamModule.h
    OpenSSLCipherStreamModule.cpp
  LINK_LIBRARIES
    ${OpenSSL_LIBRARY}
  UNIX_LINK_LIBRARIES
    -lz
  )
  
  ADD_SERIF_EXECUTABLE(OpenSSLCipherStreamTest
    SOURCE_FILES OpenSSLCipherStreamTest.cpp)
  
  # Read the encyption key from a file, and generate a header file that
  # contains it.
  IF (FEATUREMODULE_OpenSSLCipherStream_INCLUDED)
      IF (WIN)
          # Windows
          SET(OpenSSL_PASSWORD_DIR "//raid68/u17/serif/encryption_key")
      ELSE(WIN)
        IF(OSX)
          # OS X
          SET(OpenSSL_PASSWORD_DIR "/Volumes/serif/encryption_key")
        ELSE(OSX)
          # Unix
          SET(OpenSSL_PASSWORD_DIR "/d4m/serif/encryption_key")
        ENDIF(OSX)
      ENDIF(WIN)
      SET (OpenSSL_PASSWORD_FILE 
        "${OpenSSL_PASSWORD_DIR}/serif_openssl_password_v1.txt" 
        CACHE FILENAME
        "The file containing the encyption password that should be used to read OpenSSL-encrypted files")
      FILE(STRINGS ${OpenSSL_PASSWORD_FILE} OPENSSL_PASSWORD)
      SET (PASSWORD_H_SRC ${CMAKE_CURRENT_SOURCE_DIR}/OpenSSLEncryptionKey.h.in)
      SET (PASSWORD_H_DST ${DYNAMIC_INCLUDES_DIR}/OpenSSLEncryptionKey.h)
      CONFIGURE_FILE(${PASSWORD_H_SRC} ${PASSWORD_H_DST})
      APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${PASSWORD_H_DST})
      MESSAGE(STATUS "Created ${PASSWORD_H_DST}")
  ENDIF()

ELSE()
  MESSAGE(STATUS "OpenSSL not found -- not including OpenSSLCipherStream")
ENDIF()
