// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef STD_HASH_H
#define STD_HASH_H

#if defined(_WIN32)
#include <hash_map>
#include <hash_set>
#elif defined(__APPLE_CC__)
#include <unordered_map>
#include <unordered_set>
#else
#include <ext/hash_map>
#include <ext/hash_set>
#endif

namespace stdhash {

#if defined(_WIN32)
	
	template<class _Kty,
	class _Ty,
	class _Tr = stdext::hash_compare< _Kty, std::less<_Kty> >,
	class _Alloc = std::allocator< std::pair<const _Kty, _Ty> > >
	class hash_map		: public stdext::hash_map<_Kty, _Ty, _Tr, _Alloc> { };
	
	template<class _Kty,
	class _Tr = stdext::hash_compare< _Kty, std::less<_Kty> >,
	class _Alloc = std::allocator<_Kty> >
	class hash_set		: public stdext::hash_set<_Kty, _Tr, _Alloc> { };

#elif defined(__APPLE_CC__)
    
	template<class _Key,
	class _Tp,
	class _HashFn   = std::hash<_Key>,
	class _EqualKey = std::equal_to<_Key>,
	class _Alloc    = std::allocator< std::pair<const _Key, _Tp> > >
	class hash_map		: public std::unordered_map< _Key, _Tp, _HashFn, _EqualKey, _Alloc > { };
	
	template<class _Value,
	class _HashFcn  = std::hash<_Value>,
	class _EqualKey = std::equal_to<_Value>,
	class _Alloc    = std::allocator<_Value> >
	class hash_set		: public std::unordered_set< _Value, _HashFcn, _EqualKey, _Alloc > { };
	
#else
	
	template<class _Key,
	class _Tp,
	class _HashFn   = __gnu_cxx::hash<_Key>,
	class _EqualKey = __gnu_cxx::equal_to<_Key>,
	class _Alloc    = __gnu_cxx::allocator<_Tp> >
	class hash_map		: public __gnu_cxx::hash_map< _Key, _Tp, _HashFn, _EqualKey, _Alloc > { };
	
	template<class _Value,
	class _HashFcn  = __gnu_cxx::hash<_Value>,
	class _EqualKey = __gnu_cxx::equal_to<_Value>,
	class _Alloc    = __gnu_cxx::allocator<_Value> >
	class hash_set		: public __gnu_cxx::hash_set< _Value, _HashFcn, _EqualKey, _Alloc > { };
	
#endif

}

#endif
