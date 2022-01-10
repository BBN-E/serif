##
## SERIF Linux Build Script
##
## This script runs cmake to build a Makefile for a specified version
## of SERIF, and then builds the specified target(s) using make.
##

# Set appropriate environment variables.
export PATH=+CMAKE_PATH+:/usr/bin:/bin
export CC=+GCC_PATH+/gcc
export CXX=+GCC_PATH+/g++
export ARCH_SUFFIX=+ARCH_SUFFIX+
export BOOST_ROOT=+BOOST_ROOT+
export GPERFTOOLS_ROOT=+GPERFTOOLS_ROOT+
export YAMCHA_ROOT=+YAMCHA_ROOT+

# Create the build directory (erase it if it already exists).
if [ -e "+BUILD_DIR+" ]; then
    echo "Warning: build_dir (+BUILD_DIR+) already exists; erasing it."
    rm -rf "+BUILD_DIR+"
fi
mkdir -p "+BUILD_DIR+"

# Build the requested serif binaries.
cd "+BUILD_DIR+"
cmake +SRC_DIR+ -DCMAKE_BUILD_TYPE=+BUILD_TYPE+ +CMAKE_OPTIONS+ \
    || exit $status

# Hack to modify all of the "link.txt" files, to move -lrt to the end of the libraries list.
# For some reason, that is the fix to the problem of not finding the clock_gettime function when
# doing linking with the -static option.
# There should be some way in CMake to enforce this, but I couldn't dig deep enough to find out.
for linker_file in */CMakeFiles/*/link.txt; do
    echo "Moving -lrt library to end of list in file $linker_file"
    mv $linker_file $linker_file.orig
    cat $linker_file.orig | sed -e 's/^\(.*\) -lrt\(.*\)$/\1\2 -lrt/' > $linker_file
    rm $linker_file.orig
done

make +MAKE_TARGETS+ \
    || exit $status
