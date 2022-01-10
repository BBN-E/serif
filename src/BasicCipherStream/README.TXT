###############################################################
# Copyright (c) 2012 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# BasicCipherStream Module                                    #
# Provides support for reading xor-encrypted streams          #
###############################################################

This module provides support for reading selected model files from
encrypted streams, by xor-ing them with a simple one-time pad.  Only
files that are opened with a special parameter (described below) are
eligible for decryption.  For files that are opened with that special
parameter, the BasicCipherStream module will autodetect whether the file
is encrypted based on the file's contents.

It should be noted that this encryption scheme is *VERY INSECURE*, and
should not be used for any sensitive data.  It is intended only for
use in making proprietary model files opaque, and prevent casual
misuse of BBN-proprietary data.  A determined adversary could use a
brute-force attack to fairly easily decrypt anything encrypted with
this module.

READING ENCRYPTED FILES
~~~~~~~~~~~~~~~~~~~~~~~
For a file to be eligible for decryption, it must be opened using the
following factory method:

  UTF8InputStream::build(filename, true)

This returns a new stream that automatically detects whether the
specified file is encrypted, and if so, decrypts it as it is read.
The encryption autodetection is based on a "magic prefix" of
"SerifEncrypted_BBN_Proprietary_IP\n\n".  In particular, any file that
begins with exactly those characters will be considered encrypted; and
any other file will be considered non-encrypted and read as-is.

WRITING ENCRYPTED FILES
~~~~~~~~~~~~~~~~~~~~~~~
To encrypt files, use the binary "BasicCipherStreamEncryptFile," whose source code
is located in the BasicCipherStream directory.  For example, the
following command will read the file "hello.txt", and write an
encrypted copy to "hello.enc":

  BasicCipherStreamEncryptFile hello.txt hello.enc

Two shell scripts are provided to make encryption easier:

  encrypt.sh <src> <dst>
    Encrypts a given source file using the standard password file.
    The destination file must not exist (i.e., this script will refuse
    to overwrite an existing file).

  encrypt_dir.sh <src> <dst>
    Recursively encrypts all files in a given source directory, and
    writes the results to a new parallel directory tree.  In
    particular, for each file in <src> (including subdirectories), the
    script generates a corresponding encrypted file in <dst> with the
    same relative path.  Subdirectories are created as needed.  The
    destination directory must not exist (i.e., this script will
    refuse to overwrite an existing directory).

These shell scripts are located in:

  text/Core/SERIF/BasicCipherStream/

PASSWORD MANAGEMENT
~~~~~~~~~~~~~~~~~~~
The symmetric password for encryption and decryption is stored in a
file specified by the CMake variable "XOR_PASSWORD_FILE".  The
default value for this variable is:

  Linux:
    /d4m/serif/encryption_key/serif_xor_password_v1.txt

  Windows:
    //raid68/u17/serif/encryption_key/serif_xor_password_v1.txt

The contents of this file should be considered proprietary to BBN, and
not shared with anyone outside BBN.  Note that the encryption password
is *not* stored anywhere in the serif source tree.  (Thus, any source
deliveries will not include this private key.)

When CMake is run, it reads the password from this location, and
embeds it in a dynamic header file under the build tree named:

  dynamic_includes/BasicCipherStreamEncryptionKey.h

