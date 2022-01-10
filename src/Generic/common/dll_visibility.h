// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

/** Define two macros (DLL_PUBLIC and DLL_LOCAL) which can be used to
 * explicitly specify the desired visibility for a symbol.  (Note that
 * the default visibility depends on the options given to the
 * compiler.) */

#ifndef DLL_VISIBILITY_H
#define DLL_VISIBILITY_H

#define BUILDING_DLL

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
#define DLL_PUBLIC __attribute__ ((dllexport))
    #else
#define DLL_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
#define DLL_PUBLIC __attribute__ ((dllimport))
    #else
#define DLL_PUBLIC __declspec(dllimport)
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

#endif
