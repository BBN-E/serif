# Find 7Zip include paths and libraries

FIND_PATH(7Zip_INCLUDE_DIR Archive/7z/7zAlloc.h
    "${CMAKE_CURRENT_SOURCE_DIR}/../../External/7Zip/C"
    )

FIND_LIBRARY(7Zip_LIBRARY
    NAMES
       lib7Zip.so
       7Z_C
   PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../External/7Zip/Release"
    )

if (7Zip_INCLUDE_DIR)
    MESSAGE(STATUS "Found 7Zip include directory: ${7Zip_INCLUDE_DIR}")
endif(7Zip_INCLUDE_DIR)

if (7Zip_LIBRARY)
    MESSAGE(STATUS "Found 7Zip library: ${7Zip_LIBRARY}")
endif(7Zip_LIBRARY)

if (7Zip_INCLUDE_DIR)
    if (7Zip_LIBRARY)
        SET (7Zip_FOUND "YES")
    endif(7Zip_LIBRARY)
endif(7Zip_INCLUDE_DIR)

