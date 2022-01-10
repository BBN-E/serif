// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_BOOST_UTIL_H
#define SERIF_BOOST_UTIL_H

#include <boost/version.hpp>
#include <boost/make_shared.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/filesystem.hpp>

/** CONST_REFERENCE(T)
  *
  * Return a const reference version of the given type.  This is safe to use
  * even if T is already const or is already a reference.  This is used to
  * implement BOOST_MAKE_SHARED_nARG_CONSTRUCTOR (defined below).
  */
#define CONST_REFERENCE(T) boost::add_reference<boost::add_const<T>::type>::type

/** BOOST_MAKE_SHARED_nARG_CONSTRUCTOR(CLASS_NAME, ARG1_TYPE, ARG2_TYPE, ...) 
  *
  * Use this macro inside the body of a class to declare that boost::make_shared
  * should be considered a friend function when used in conjunction with the
  * constructor that takes the given argument types.  This allows the constructor 
  * to be declared private (making it impossible to accidentally create an instance 
  * of the object without immediatly storing it in a boost::shared_ptr).  
  * Example usage:
  *
  * class Foo {
  *   private:
  *     Foo(int size, const char* name);
  *     MAKE_SHARED_2ARG_CONSTRUCTOR(Foo, int, const char*);
  * };
  * 
  * boost::shared_ptr<Foo> myFoo = boost::make_shared<Foo>(3, "Bob");
  *
  * Note that you need to explicitly specify the number of arguments 
  * that the constructor takes as part of the macro name.  Also, note that 
  * macros don't mix well with templated types that contain commas -- so 
  * if you have such a type, then you should typedef it to a shorter name 
  * before using it with this macro.
  */
#define BOOST_MAKE_SHARED_0ARG_CONSTRUCTOR(CLASS_NAME) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>()
#define BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1))
#define BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2))
#define BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3))
#define BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3, ARG_TYPE4) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3), CONST_REFERENCE(ARG_TYPE4))
#define BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3, ARG_TYPE4, ARG_TYPE5) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3), CONST_REFERENCE(ARG_TYPE4), CONST_REFERENCE(ARG_TYPE5))
#define BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3, ARG_TYPE4, ARG_TYPE5, ARG_TYPE6) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3), CONST_REFERENCE(ARG_TYPE4), CONST_REFERENCE(ARG_TYPE5), CONST_REFERENCE(ARG_TYPE6))
#define BOOST_MAKE_SHARED_7ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3, ARG_TYPE4, ARG_TYPE5, ARG_TYPE6, ARG_TYPE7) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3), CONST_REFERENCE(ARG_TYPE4), CONST_REFERENCE(ARG_TYPE5), CONST_REFERENCE(ARG_TYPE6), CONST_REFERENCE(ARG_TYPE7))
#define BOOST_MAKE_SHARED_8ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3, ARG_TYPE4, ARG_TYPE5, ARG_TYPE6, ARG_TYPE7, ARG_TYPE8) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3), CONST_REFERENCE(ARG_TYPE4), CONST_REFERENCE(ARG_TYPE5), CONST_REFERENCE(ARG_TYPE6), CONST_REFERENCE(ARG_TYPE7), CONST_REFERENCE(ARG_TYPE8))
#define BOOST_MAKE_SHARED_9ARG_CONSTRUCTOR(CLASS_NAME, ARG_TYPE1, ARG_TYPE2, ARG_TYPE3, ARG_TYPE4, ARG_TYPE5, ARG_TYPE6, ARG_TYPE7, ARG_TYPE8, ARG_TYPE9) \
	friend boost::shared_ptr<CLASS_NAME> boost::make_shared<CLASS_NAME>(CONST_REFERENCE(ARG_TYPE1), CONST_REFERENCE(ARG_TYPE2), CONST_REFERENCE(ARG_TYPE3), CONST_REFERENCE(ARG_TYPE4), CONST_REFERENCE(ARG_TYPE5), CONST_REFERENCE(ARG_TYPE6), CONST_REFERENCE(ARG_TYPE7), CONST_REFERENCE(ARG_TYPE8), CONST_REFERENCE(ARG_TYPE9))

/**
  * Wrap the two versions of Boost's filesystem path interface
  * so we can work with versions of Boost ranging from 1.40 to 1.55.
  */
#if BOOST_VERSION < 104400
#define BOOST_FILESYSTEM_PATH_AS_STRING(PATH) PATH.file_string()
#define BOOST_FILESYSTEM_PATH_GET_FILENAME(PATH) PATH.filename()
#define BOOST_FILESYSTEM_PATH_GET_EXTENSION(PATH) PATH.extension()
#define BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(ITERATOR) ITERATOR->leaf()
#define BOOST_FILESYSTEM_DIR_ITERATOR_GET_PATH(ITERATOR) ITERATOR->string()
#else
#define BOOST_FILESYSTEM_PATH_AS_STRING(PATH) PATH.string()
#define BOOST_FILESYSTEM_PATH_GET_FILENAME(PATH) PATH.filename().string()
#define BOOST_FILESYSTEM_PATH_GET_EXTENSION(PATH) PATH.extension().string()
#define BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(ITERATOR) BOOST_FILESYSTEM_PATH_GET_FILENAME(ITERATOR->path())
#define BOOST_FILESYSTEM_DIR_ITERATOR_GET_PATH(ITERATOR) BOOST_FILESYSTEM_PATH_AS_STRING(ITERATOR->path())
#endif

#endif
