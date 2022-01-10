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

PASSWORD_FILE=serif_openssl_password_v1.txt
CIPHER=rc4

###########################################################################
# ARGUMENT PROCESSING
###########################################################################

if [ $# != 2 ]; then
    echo "Usage: %0 <src> <dst>"
    exit -1
fi

PASSWORD_DIR=/d4m/serif/encryption_key
if [ ! -e $PASSWORD_DIR ]; then
    PASSWORD_DIR=//raid68/u17/serif/encryption_key
    if [ ! -e $PASSWORD_DIR ]; then
	echo "Error: password directory not found!"
	exit -1
    fi
fi

if [ ! -e "$PASSWORD_DIR/$PASSWORD_FILE" ]; then
    echo "Error: password file '$PASSWORD_FILE'"\
         "not found in '$PASSWORD_DIR'"
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

###########################################################################
# SCRIPT
###########################################################################

for FILENAME in `find $1 -type 'f' -printf '%P\n' |grep -v '\(^\|/\).svn/'`; do
    SRC=$1/$FILENAME
    DST=$2/$FILENAME
    echo "Encrypting '$SRC' ->"
    echo "           '$DST'"
    mkdir -p `dirname $DST`
    openssl enc -"$CIPHER" -in "$SRC" -out "$DST" \
	-pass "file:$PASSWORD_DIR/$PASSWORD_FILE"
done
