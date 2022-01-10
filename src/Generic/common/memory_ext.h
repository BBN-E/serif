// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef MEMORY_EXT
#define MEMORY_EXT


// Created by JSG. This header adds additional lightweight classes for memory management following the Resource Aquisition is Initialization (RAII)
// paradigm, in the spirit (and modeled off of) the auto_ptr class from the C++ std. (defined in <memory>). These classes ease exception-safe programming.
//
// 'auto_array' is equivalent to auto_ptr, except that it uses an array (rather than scalar) destructor call of the underlying memory. Example:
//
//	char * foobar() {
//      // buffer's memory will be destructed if an exception is thrown, or returned by foobar if not.
//		auto_array<wchar_t> buffer( new wchar_t[ big_value ] );
//		if( some_predicate )
//			throw std::exception();
//		buffer.release();
// }
//
//
// 'auto_handle' is useful for managing resources which must be released through an explicit function call (sockets are an ideal example).
//
//	void foobar() {
//		typedef auto_handle< SOCKET, int (__stdcall *)(SOCKET) > auto_SOCKET;
//
//		// the socket is guarenteed to have 'closesocket' called upon it when 'sock' leaves scope
//		auto_SOCKET sock( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ), INVALID_SOCKET, &closesocket );
//		if( some_predicate )
//			throw std::exception();
//
//		do_something_with_socket;
//
//		return;
//	}
//
// !! BIG FAT WARNING!!
// There's a bug in the microsoft STL implementation of auto_ptr_ref (helper to auto_ptr)-- the constructor needs to be explicit. 
// If not, than statements like:
// auto_ptr<Foo> foo_ptr = new Foo( bar );
// are legal, but do *very* strange things through the magic of implicit casts (auto_ptr interprets Foo * as Foo **)
// QUICK FIX: add the keyword 'explicit' in front of your <memory> auto_ptr_ref struct definition!

#include <memory>


// TEMPLATE CLASS auto_array -- ensures destruction of
// c-arrays when the owning auto_array leaves scope
template<class _Ty>
	class auto_array;

template<class _Ty>
	struct auto_array_ref
		{	// proxy reference for auto_array copying
	explicit auto_array_ref(void *_Right)
		: _Ref(_Right)
		{	// construct from generic pointer to auto_array ptr
		}

	void *_Ref;	// generic pointer to auto_array ptr
	};

template<class _Ty>
	class auto_array
		{	// wrap an object pointer to ensure destruction
public:
	typedef _Ty element_type;

	explicit auto_array(_Ty *_Ptr = 0) throw()
		: _Myptr(_Ptr)
		{	// construct from object pointer
		}

	auto_array(auto_array<_Ty>& _Right) throw()
		: _Myptr(_Right.release())
		{	// construct by assuming pointer from _Right auto_array
		}

	auto_array(auto_array_ref<_Ty> _Right) throw()
		{	// construct by assuming pointer from _Right auto_array_ref
		_Ty **_Pptr = (_Ty **)_Right._Ref;
		_Ty *_Ptr = *_Pptr;
		*_Pptr = 0;	// release old
		_Myptr = _Ptr;	// reset this
		}

	template<class _Other>
		operator auto_array<_Other>() throw()
		{	// convert to compatible auto_array
		return (auto_array<_Other>(*this));
		}

	template<class _Other>
		operator auto_array_ref<_Other>() throw()
		{	// convert to compatible auto_array_ref
		_Other *_Testptr = (_Ty *)_Myptr;	// test implicit conversion
		auto_array_ref<_Other> _Ans(&_Myptr);
		return (_Testptr != 0 ? _Ans : _Ans);
		}

	template<class _Other>
		auto_array<_Ty>& operator=(auto_array<_Other>& _Right) throw()
		{	// assign compatible _Right (assume pointer)
		reset(_Right.release());
		return (*this);
		}

	template<class _Other>
		auto_array(auto_array<_Other>& _Right) throw()
		: _Myptr(_Right.release())
		{	// construct by assuming pointer from _Right
		}

	auto_array<_Ty>& operator=(auto_array<_Ty>& _Right) throw()
		{	// assign compatible _Right (assume pointer)
		reset(_Right.release());
		return (*this);
		}

	auto_array<_Ty>& operator=(auto_array_ref<_Ty> _Right) throw()
		{	// assign compatible _Right._Ref (assume pointer)
		_Ty **_Pptr = (_Ty **)_Right._Ref;
		_Ty *_Ptr = *_Pptr;
		*_Pptr = 0;	// release old
		reset(_Ptr);	// set new
		return (*this);
		}

	~auto_array()
		{	// destroy the object
		delete [] (_Ty *)_Myptr;
		}

	_Ty& operator*() const throw()
		{	// return designated value

 #if _HAS_ITERATOR_DEBUGGING
		if (_Myptr == 0)
			_DEBUG_ERROR("auto_array not dereferencable");
 #endif /* _HAS_ITERATOR_DEBUGGING */

		__analysis_assume(_Myptr);

		return (*(_Ty *)_Myptr);
		}

	_Ty *operator->() const throw()
		{	// return pointer to class object
		return (&**this);
		}

	_Ty *get() const throw()
		{	// return wrapped pointer
		return ((_Ty *)_Myptr);
		}

	_Ty& operator [] (int index)
		{
		return ((_Ty *)_Myptr)[index];
		}

	_Ty *release() throw()
		{	// return wrapped pointer and give up ownership
		_Ty *_Tmp = (_Ty *)_Myptr;
		_Myptr = 0;
		return (_Tmp);
		}

	void reset(_Ty* _Ptr = 0)
		{	// destroy designated object and store new pointer
		if (_Ptr != _Myptr)
			delete [] (_Ty *)_Myptr;
		_Myptr = _Ptr;
		}

private:
	const _Ty *_Myptr;	// the wrapped object pointer
};


// TEMPLATE CLASS auto_handle -- ensures function application
// on a handle when the owning auto_handle goes out of scope
template<class T, class F>
class auto_handle;

template<class T, class F>
struct auto_handle_ref {

	// proxy reference for auto_handle copying
	explicit auto_handle_ref( T & right, const T & null_handle, const F & function )
		: _handle( right ),_null_handle( null_handle ), _function( function ) {}

	T & _handle;
	const T & _null_handle;
	const F & _function;
};

template<class T, class F>
class auto_handle {
public:
	
	typedef T handle_type;
	typedef F function_type;
	
	// represents an invalid handle value
	const T null_handle;
	const F function;

	// explicit constructors must specify the invalid handle value,
	// and the function to be applied to a handle on destruction
	explicit auto_handle( const T & null_handle, const F & function ) throw()
		: null_handle( null_handle ), function( function ) {}

	explicit auto_handle( const T & handle, const T & null_handle, const F & function ) throw()
		: _handle( handle ), null_handle( null_handle ), function( function ) {}

	// construct by assuming from right auto_handle
	auto_handle( auto_handle & right ) throw()
		: _handle( right.release() ), null_handle( right.null_handle ), function( right.function ) {}

	// construct by assuming from right auto_handle_ref
	auto_handle( auto_handle_ref<T,F> right ) throw()
		: null_handle( right._null_handle ), function( right._function )
	{
		// assign this
		_handle = right._handle;
		// release old
		right._handle = right._null_handle;
	}

	~auto_handle() {
		function( _handle );
	}

	template<class G>
	auto_handle<T,F> & operator= ( auto_handle<T,G> & right ) throw()
	{
		// if right == right.null_handle, reset to *our* null_handle value
		reset( right.get() == right.null_handle ? null_handle : right.release() );
		return (*this);
	}

	template<class G>
	auto_handle<T,F> & operator= ( auto_handle_ref<T,G> right ) throw()
	{
		// if right == right.null_handle, reset to *our* null_handle value
		reset( right._handle == right._null_handle ? null_handle : right._handle );
		// release old by assigning *their* null_handle value
		right._handle = right._null_handle;
		return (*this);
	}

	operator auto_handle_ref<T,F> () throw(){
		return auto_handle_ref<T,F>( _handle, null_handle, function );
	}

	T get() const throw() {
		return _handle;
	}

	T release() throw() {
		// return handle, giving up ownership
		T tmp = _handle;
		_handle = null_handle;
		return tmp;
	}

	void reset( const T & handle ){
		// apply function on the current object, and store a new one
		if( _handle != null_handle )
			function( _handle );
		_handle = handle;
	}

private:
	T _handle;
};


// helper methods allowing implicit template parameterization

template<class T, class F>
auto_handle<T,F> make_auto_handle( const T & null_handle, const F & function )
{
	return auto_handle<T,F>( null_handle, function );
}

template<class T, class F>
auto_handle<T,F> make_auto_handle( const T & handle, const T & null_handle, const F & function )
{
	return auto_handle<T,F>( handle, null_handle, function );
}

template<class T>
auto_array<T> make_auto_array( T * ptr )
{
	return auto_array<T>(ptr);
}

template<class T>
std::auto_ptr<T> make_auto_ptr( T * ptr )
{
	return std::auto_ptr<T>(ptr);
}

template<class A, class T>
A & auto_setpass( A & an_auto, T & an_inst )
{
	an_auto.reset( an_inst );
	return an_auto;
}


#endif // MEMORY_EXT
