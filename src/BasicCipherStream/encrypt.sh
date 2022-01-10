#!/bin/bash
#
#    encrypt.sh <src> <dst>
#
# Encrypt a given source file using the standard password file.
# The destination file must not exist (i.e., this script will refuse
# to overwrite an existing file).

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
    echo "Error: input file '$1' not found"
    exit -1
fi

if [ ! -f "$1" ]; then
    echo "Error: '$1' is not a file"
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

echo "Encrypting '$1' -> '$2'"
"$ENCRYPT" "$1" "$2"
