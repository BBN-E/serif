######################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                           #
# All Rights Reserved.                                               #
#                                                                    #
# Cmake/definitions.cmake                                            #
#                                                                    #
# This file sets up the compiler flags that should be used when      #
# compiling and linking source files.                                #
#                                                                    #
######################################################################

######################################################################
# Compiler flags: All Architectures
######################################################################

# Add a preprocessor define to indicate whether MT_DECODER is available.
IF (MTDecoder_FOUND)
  ADD_DEFINITIONS( -DMT_DECODER )
ENDIF (MTDecoder_FOUND)

######################################################################
# Compiler flags: Windows-only
######################################################################
IF (WIN32 OR WIN64)
  SET (EXEC_RESERVED_STACK_SIZE 10000000 CACHE INTEGER 
       "Set the executable reserved stack size (default 10MB)")

  ADD_DEFINITIONS( -DPC )
  ADD_DEFINITIONS( -W3 -WX -Ot )

  # Optionally allow multi-processor compilation
  SET(SERIF_FAST_COMPILE OFF CACHE BOOL "Use multiple cores to compile in Visual Studio")
  IF(SERIF_FAST_COMPILE)
    ADD_DEFINITIONS("/MP")
  ENDIF(SERIF_FAST_COMPILE)

  # Exclude rarely-used services from Windows headers.  For more 
  # info, see: <http://support.microsoft.com/kb/166474>
  ADD_DEFINITIONS( -DWIN32_LEAN_AND_MEAN )
  ADD_DEFINITIONS( -DVC_EXTRALEAN )

  # Don't use the macro-style min/max in windows.h
  ADD_DEFINITIONS( -DNOMINMAX )

  # Declare that we expect to be running Windows XP or higher.
  ADD_DEFINITIONS( -D_WIN32_WINNT=0x0501 )

  # Increase the reserved stack size.
  SET(CMAKE_EXE_LINKER_FLAGS 
      "${CMAKE_EXE_LINKER_FLAGS} -STACK:${EXEC_RESERVED_STACK_SIZE}")

  # Supress the following warnings
  ADD_DEFINITIONS( -D_CRT_SECURE_NO_DEPRECATE 
                   -D_CRT_NON_CONFORMING_SWPRINTFS 
                   -D_SCL_SECURE_NO_WARNINGS)

  # Promote select W4 warnings to W3 so they trigger a compile error
  # Other than 4239, these were all taken from http://blogs.msdn.com/b/vcblog/archive/2010/12/14/off-by-default-compiler-warnings-in-visual-c.aspx
  # and https://svn.boost.org/trac/boost/wiki/Guidelines/WarningsGuidelines
  #ADD_DEFINITIONS("/w34191") # => unsafe conversion from 'type of expression' to 'type required' - FeatureModule.cpp violates this.
  ADD_DEFINITIONS("/w34238") # => don't take address of temporaries 
  ADD_DEFINITIONS("/w34239") # => non-standard extension used
  #ADD_DEFINITIONS("/w34242") # => conversion from 'type1' to 'type2', possible loss of data - std::transform violates this.  Can fix eventually by using something else.
  ADD_DEFINITIONS("/w34244") # => int conversion from 'type1' to 'type2', possible loss of data - more specific than above
  ADD_DEFINITIONS("/w34263") # => member function does not override any base class virtual member function 
  ADD_DEFINITIONS("/w34264") # => no override available for virtual member function from base 'class'; function is hidden 
  ADD_DEFINITIONS("/w34018") # => signed/unsigned comparison
  #ADD_DEFINITIONS("/w34265") # => class has virtual functions, but destructor is not virtual - Boost violates this.
  ADD_DEFINITIONS("/w34266") # => no override available for virtual member function from base 'type'; function is hidden 
  ADD_DEFINITIONS("/w34288") # => For-loop scoping (this is the default) 
  ADD_DEFINITIONS("/w34302") # => truncation from 'type 1' to 'type 2' 
  ADD_DEFINITIONS("/w34346") # => require "typename" where the standard requires it.
  ADD_DEFINITIONS("/w34826") # => conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior 
  ADD_DEFINITIONS("/w34905") # => wide string literal cast to 'LPSTR' 
  ADD_DEFINITIONS("/w34906") # => string literal cast to 'LPWSTR' 
  ADD_DEFINITIONS("/w34928") # => illegal copy-initialization; more than one user-defined conversion has been implicitly applied 

  # Supress errors we get when we statically link with the xercesc
  # library.  Basically, xercesc declares some functions as 'dllimport'
  # (using __declspec) when we import them; but since we're using
  # static linking, they're not actually imported from a DLL.
  ADD_DEFINITIONS("/wd4049") # => locally defined symbol imported.
  ADD_DEFINITIONS("/wd4217") # => locally defined symbol imported in function
  # Supress warnings caused by using throw declarations:
  ADD_DEFINITIONS("/wd4290") # => non-empty "throw" declarations
  ADD_DEFINITIONS("/wd4503") # recommended by boost

######################################################################
# Compiler flags: UNIX-only
######################################################################
ELSEIF(UNIX)
  SET (TREAT_WARNINGS_AS_ERRORS ON CACHE BOOL 
       "Treat warnings as errors during compilation")

  # Preprocessor flag to indicate we're compling on unix.
  ADD_DEFINITIONS( -DUNIX )

  # Enable warnings.
  #ADD_DEFINITIONS( -Woverloaded-virtual )
  #ADD_DEFINITIONS( -Wnon-virtual-dtor )
  ADD_DEFINITIONS( -Wreturn-type)
  ADD_DEFINITIONS( -Wsign-compare)
  ADD_DEFINITIONS( -Wno-deprecated )
  IF (TREAT_WARNINGS_AS_ERRORS)
    ADD_DEFINITIONS( -Werror )
  ENDIF (TREAT_WARNINGS_AS_ERRORS)

  # Make sure we're using the right C++ library for codecvt support
  IF(OSX)
    ADD_DEFINITIONS( -stdlib=libc++ )
  ENDIF(OSX)

ENDIF (WIN32 OR WIN64)

