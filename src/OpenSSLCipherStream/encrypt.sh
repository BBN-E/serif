#!/bin/bash
#
#    encrypt.sh <src> <dst>
#
# Encrypt a given source file using the standard password file and a
# default cipher (rc4).  The destination file must not exist (i.e.,
# this script will refuse to overwrite an existing file).

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

###########################################################################
# SCRIPT
###########################################################################

echo "Encrypting '$1' -> '$2'"
openssl enc -"$CIPHER" -in "$1" -out "$2" \
    -pass "file:$PASSWORD_DIR/$PASSWORD_FILE"
