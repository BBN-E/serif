##
## SERIF Linux Build Script
##
## This script runs cmake to build a Makefile for a specified version
## of SERIF, and then builds the specified target(s) using make.
##

# Set appropriate environment variables.
setenv PATH +CMAKE_PATH+:/usr/bin:/bin
setenv CC +GCC_PATH+/gcc
setenv CXX +GCC_PATH+/g++
setenv ARCH_SUFFIX +ARCH_SUFFIX+
setenv BOOST_ROOT +BOOST_ROOT+
setenv GPERFTOOLS_ROOT +GPERFTOOLS_ROOT+
setenv YAMCHA_ROOT +YAMCHA_ROOT+

# Create the build directory (erase it if it already exists).
if ( -e "+BUILD_DIR+" ) then
    echo "Warning: build_dir (+BUILD_DIR+) already exists; erasing it."
    rm -rf "+BUILD_DIR+"
endif
mkdir -p "+BUILD_DIR+"

# Build the requested serif binaries.
cd "+BUILD_DIR+"
cmake +SRC_DIR+ -DCMAKE_BUILD_TYPE=+BUILD_TYPE+ +CMAKE_OPTIONS+ \
    || exit $status
make +MAKE_TARGETS+ \
    || exit $status
