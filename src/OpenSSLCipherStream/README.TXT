###############################################################
# Copyright (c) 2012 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# OpenSSLCipherStream Module                                  #
# Provides support for reading encrypted streams              #
###############################################################

This module provides support for reading selected model files from
encrypted streams, using OpenSSL symmetric encryption algorithms such
as rc4 or blowfish.  Only files that are opened with a special
parameter (described below) are eligible for decryption.  For files
that are opened with that special parameter, the OpenSSLCipherStream
module will autodetect whether the file is encrypted based on the
file's contents.

READING ENCRYPTED FILES
~~~~~~~~~~~~~~~~~~~~~~~
For a file to be eligible for decryption, it must be opened using the
following factory method:

  UTF8InputStream::build(filename, true)

This returns a new stream that automatically detects whether the
specified file is encrypted, and if so, decrypts it as it is read.
The encryption autodetection is based on the standard OpenSSL "magic
prefix" of "Salted__".  In particular, any file that begins with
exactly those eight characters will be considered encrypted; and any
other file will be considered non-encrypted and read as-is.

WRITING ENCRYPTED FILES
~~~~~~~~~~~~~~~~~~~~~~~
To encrypt files, simply use the "openssl" command-line tool.  For
example, the following command will read the file "hello.txt", encrypt
it using the "rc4" cipher and the standard password file, and write
the encrypted results to "hello.rc4":

  openssl enc -rc4 -in hello.txt -out hello.rc4 \
    -pass file:/d4m/serif/encryption_key/serif_openssl_password_v1.txt

Two shell scripts are provided to make encryption easier:

  encrypt.sh <src> <dst>
    Encrypts a given source file using the standard password file and
    a default cipher (rc4).  The destination file must not exist (i.e.,
    this script will refuse to overwrite an existing file).

  encrypt_dir.sh <src> <dst>
    Recursively encrypts all files in a given source directory, and
    writes the results to a new parallel directory tree.  In
    particular, for each file in <src> (including subdirectories), the
    script generates a corresponding encrypted file in <dst> with the
    same relative path.  Subdirectories are created as needed.  The
    destination directory must not exist (i.e., this script will
    refuse to overwrite an existing directory).

These shell scripts are located in:

  text/Core/SERIF/OpenSSLCipherStream/

PASSWORD MANAGEMENT
~~~~~~~~~~~~~~~~~~~
The symmetric password for encryption and decryption is stored in a
file specified by the CMake variable "OpenSSL_PASSWORD_FILE".  The
default value for this variable is:

  Linux:
    /d4m/serif/encryption_key/serif_openssl_password_v1.txt

  Windows:
    //raid68/u17/serif/encryption_key/serif_openssl_password_v1.txt

The contents of this file should be considered proprietary to BBN, and
not shared with anyone outside BBN.  Note that the encryption password
is *not* stored anywhere in the serif source tree.  (Thus, any source
deliveries will not include this private key.)

When CMake is run, it reads the password from this location, and
embeds it in a dynamic header file under the build tree named:

  dynamic_includes/OpenSSLEncryptionKey.h

PARAMETERS
~~~~~~~~~~
This module can be configured using two parameters:

  openssl_cipher_type [default="auto"]

    The cipher type that should be used to read all OpenSSL-encrypted
    files.  Example values are "rc4" and "bf" (blowfish).  See the
    OpenSSL documentation for EVP_get_cipherbyname() for a complete
    list.  If this parameter is set to "auto", then the
    OpenSSLCipherStream module will choose a cipher type based on the
    filename's extension.

  openssl_default_cipher_type [default="rc4"]

    The cipher type that should be used if openssl_cipher_type is set
    to "auto" and the filename's extension does not match any known
    cipher type.

