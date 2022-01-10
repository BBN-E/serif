#!/bin/bash
# 
# encrypt_dir.sh <src_dir> <dst_dir>
#
# Recursively encrypt all files in a given source directory, and
# writes the results to a new parallel directory tree.  In particular,
# for each file in <src> (including subdirectories), generate a
# corresponding encrypted file in <dst> with the same relative path.
# Subdirectories are created as needed.  The destination directory
# must not exist (i.e., this script will refuse to overwrite an
# existing directory).
#

###########################################################################
# CONFIGURATION
###########################################################################

BINPATH="\
    .\
    .. \
    build/BasicCipherStream/Release \
    build/BasicCipherStream/Debug \
    ../build/BasicCipherStream/Release \
    ../build/BasicCipherStream/Debug"

###########################################################################
# ARGUMENT PROCESSING
###########################################################################

if [ $# != 2 ]; then
    echo "Usage: %0 <src> <dst>"
    exit -1
fi

if [ ! -e "$1" ]; then
    echo "Error: input directory '$1' not found"
    exit -1
fi

if [ ! -d "$1" ]; then
    echo "Error: '$1' is not a directory"
    exit -1
fi

if [ -e "$2" ]; then
    echo "Error: output file '$2' already exists"
    exit -1
fi

for BINDIR in $BINPATH; do 
    if [ -x "$BINDIR/BasicCipherStreamEncryptFile" ]; then
	ENCRYPT="$BINDIR/BasicCipherStreamEncryptFile"
	break
    fi
done
if [ ! -x "$ENCRYPT" ]; then
    echo "Error: could not find BasicCipherStreamEncryptFile binary"
    exit -1
fi
echo "Found BasicCipherStreamEncryptFile at: $ENCRYPT"

###########################################################################
# SCRIPT
###########################################################################

for FILENAME in `find $1 -type 'f' -printf '%P\n'`; do
    SRC=$1/$FILENAME
    DST=$2/$FILENAME
    echo "Encrypting '$SRC' ->"
    echo "           '$DST'"
    mkdir -p `dirname $DST`
    "$ENCRYPT" "$SRC" "$DST"
done
