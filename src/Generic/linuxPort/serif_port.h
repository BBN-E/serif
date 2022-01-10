// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/* -*- mode: c++ -*-
 * SERIF porting utilities
 *
 * $Id$
 */

#ifndef __SERIF_PORT_H__
#define __SERIF_PORT_H__

#ifdef SERIF_PATH_SEP
# undef SERIF_PATH_SEP
#endif

#if !defined(_WIN32)

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <wchar.h>

/*
 * well-known constants
 */

// path separator
#define SERIF_PATH_SEP   "/"
#define LSERIF_PATH_SEP L"/"
// default directory mask
#define SERIF_DEFAULT_UMASK 0755

# ifndef HAVE_STRNCPY_S
#  define strncpy_s strncpy
# endif

# ifndef HAVE_STRCPY_S
#  define strcpy_s strcpy
# endif

# ifndef HAVE_STRCAT_S
#  define strcat_s strcat
# endif

# ifndef HAVE_STRCAT_S
#  define strncat_s strncat
# endif

# ifndef HAVE_SPRINTF_S
#  define sprintf_s sprintf
# endif

# ifndef HAVE__GETCWD
#  define _getcwd getcwd
# endif

# ifndef HAVE__CHDIR
#  define _chdir chdir
# endif

# ifndef HAVE__STRDUP
#  define _strdup strdup
# endif

# ifndef HAVE__WCSDUP
# define _wcsdup wcsdup
# endif

# ifndef HAVE__MKDIR
#  define _mkdir(dir) mkdir(dir,SERIF_DEFAULT_UMASK)
# endif

# ifndef HAVE__RMDIR
#  define _rmdir(dir) rmdir(dir)
# endif

# ifndef HAVE__SNWPRINTF
#  define _snwprintf swprintf
# endif

# ifndef HAVE__STRICMP
#  define _stricmp strcasecmp
# endif

# ifndef HAVE__SNPRINTF
#  define _snprintf snprintf
# endif

# ifndef HAVE__WTOI
#  define _wtoi(s) wcstol(s,0,10)
# endif

# ifndef HAVE__WCSICMP
#  define _wcsicmp wcscasecmp
# endif

# ifndef HAVE_ISWASCII
#  define iswascii isascii
# endif

# ifndef HAVE_CPP_MAX
# define _cpp_max  std::max
# define _cpp_min  std::min
# endif

# ifndef HAVE_ITOA
#  define _itoa itoa
# endif

# ifndef HAVE__STRREV
#  define _strrev strrev
# endif

# ifndef HAVE__SCANF_S
#  define sscanf_s sscanf
# endif

# define _strupr_s(s,len) strupr(s)
# define _wcsupr_s(s,len) wstrupr(s)

char * strupr(char *str);
wchar_t * wstrupr(wchar_t *str);
char* itoa(int value, char*  str, int radix);
void strreverse(char* begin, char* end);

#ifndef _MAX_PATH
#  define _MAX_PATH 260
#endif

#else

# define SERIF_PATH_SEP   "\\"
# define LSERIF_PATH_SEP L"\\"

#endif /* !WIN32 */
#endif /* !__SERIF_PORT_H__ */
