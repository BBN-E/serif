###############################################################
# Copyright (c) 2012 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# BasicCipherStream                                           #
# Provides support for reading encrypted streams              #
###############################################################
ADD_SERIF_FEATURE_MODULE(BasicCipherStream
  EXCLUDED_BY_DEFAULT
SOURCE_FILES
  UTF8BasicCipherInputStream.h
  UTF8BasicCipherInputStream.cpp
  BasicCipherStreamModule.h
  BasicCipherStreamModule.cpp
)

ADD_SERIF_EXECUTABLE(BasicCipherStreamEncryptFile
  SOURCE_FILES BasicCipherStreamEncryptFile.cpp)

# Read the encyption key from a file, and generate a header file that
# contains it.
IF (FEATUREMODULE_BasicCipherStream_INCLUDED)
    IF (WIN)
        # Windows
        SET(BASIC_CIPHER_STREAM_PASSWORD_DIR "//raid68/u17/serif/encryption_key")
    ELSE(WIN)
      IF(OSX)
        # OS X
        SET(BASIC_CIPHER_STREAM_PASSWORD_DIR "/Volumes/serif/encryption_key")
      ELSE(OSX)
        # Unix
        SET(BASIC_CIPHER_STREAM_PASSWORD_DIR "/d4m/serif/encryption_key")
      ENDIF(OSX)
    ENDIF(WIN)
    SET (BASIC_CIPHER_STREAM_PASSWORD_FILE 
      "${BASIC_CIPHER_STREAM_PASSWORD_DIR}/serif_xor_password_v2.txt" 
      CACHE FILENAME
      "The file containing the encyption password that should be used to read Basic-encrypted files")
    FILE(STRINGS ${BASIC_CIPHER_STREAM_PASSWORD_FILE} BASIC_CIPHER_STREAM_PASSWORD)
    SET (PASSWORD_H_SRC ${CMAKE_CURRENT_SOURCE_DIR}/BasicCipherStreamEncryptionKey.h.in)
    SET (PASSWORD_H_DST ${DYNAMIC_INCLUDES_DIR}/BasicCipherStreamEncryptionKey.h)
    CONFIGURE_FILE(${PASSWORD_H_SRC} ${PASSWORD_H_DST})
    APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${PASSWORD_H_DST})
    MESSAGE(STATUS "Created ${PASSWORD_H_DST}")
ENDIF()
