// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AUTO_GROW_VECTOR_H
#define AUTO_GROW_VECTOR_H

#include <vector>

/**
 * A drop-in replacement for std::vector, whose operator[] methods
 * automatically resize the vector when an out-of-bounds element is
 * accessed.  Otherwise, this class acts identically to std::vector.
 *
 * This class was created to ease the transition away from
 * fixed-length arrays.  In particular, a fixed-length array can be
 * replaced with an AutoGrowVector *if* the code that uses the array
 * does not assume that pointers or references into the array are
 * stable across array modifications.  When used to replace a
 * fixed-length array, the AutoGrowVector will start out empty, and
 * will grow (through use) to whatever length is required.  Where
 * possible, code that 'zeros out' the array should be replaced with a
 * call to the clear() method.
 *
 * Implementation note: this class is implemented as a wrapper for a
 * vector, rather than a subclass of vector, because std::vector is
 * not intended to be subclassed (e.g., its destructor is not virtual).
 */
template < typename T, typename Allocator = std::allocator<T> > 
class AutoGrowVector
{
private:
	// The wrapped vector:
	std::vector<T,Allocator> _elements;
public:
	// Copy typedefs from the wrapped std::vector.
	typedef typename std::vector<T,Allocator>::iterator iterator;
	typedef typename std::vector<T,Allocator>::reference reference;
	typedef typename std::vector<T,Allocator>::const_reference const_reference;
	typedef typename std::vector<T,Allocator>::const_iterator const_iterator;
	typedef typename std::vector<T,Allocator>::size_type size_type;
	typedef typename std::vector<T,Allocator>::difference_type difference_type;
	typedef typename std::vector<T,Allocator>::value_type value_type;
	typedef typename std::vector<T,Allocator>::allocator_type allocator_type;
	typedef typename std::vector<T,Allocator>::pointer pointer;
	typedef typename std::vector<T,Allocator>::const_pointer const_pointer;
	typedef typename std::vector<T,Allocator>::reverse_iterator reverse_iterator;
	typedef typename std::vector<T,Allocator>::const_reverse_iterator const_reverse_iterator;

	/** Return a reference to the n-th element.  If n is greater than
	 * equal to the size of this vector, then the vector is expanded
	 * to size=n+1 first. */
	reference operator[](size_type n) {
		if (n >= size()) resize(n+1);
		return _elements[n];
	}

	/** Return a const reference to the n-th element.  If n is greater
	 * than equal to the size of this vector, then the vector is
	 * expanded to size=n+1 first. */
	const_reference operator[](size_type n) const {
		if (n >= size()) resize(n+1);
		return _elements[n];
	}

	// All remaining methods just delegate to _elements.
	explicit AutoGrowVector ( const Allocator& alloc= Allocator() )
		: _elements(alloc) {}
	explicit AutoGrowVector ( size_type n, const T& value= T(), const Allocator& alloc= Allocator() )
		: _elements(n, value, alloc) {}
	template <class InputIterator>
	AutoGrowVector ( InputIterator first, InputIterator last, const Allocator& alloc= Allocator() )
		: _elements(first, last, alloc) {}
	AutoGrowVector ( const AutoGrowVector<T,Allocator>& x )
		: _elements(x._elements) {} // copy constructor
	template <class InputIterator>
	void assign ( InputIterator first, InputIterator last ) { 
		_elements.assign(first, last); }
	void assign ( size_type n, const T& u ) {
		_elements.assign(n, u); }
	const_reference at ( size_type n ) const {
		return _elements.at(n); }
	reference at ( size_type n ) {
		return _elements.at(n); }
	reference back ( ) {
		return _elements.back(); }
	const_reference back ( ) const {
		return _elements.back(); }
	iterator begin () {
		return _elements.begin(); }
	const_iterator begin () const {
		return _elements.begin(); }
	size_type capacity () const {
		return _elements.capacity(); }
	void clear() {
		_elements.clear(); }
	bool empty () const {
		return _elements.empty(); }
	iterator end () {
		return _elements.end(); }
	const_iterator end () const {
		return _elements.end(); }
	iterator erase ( iterator position ) {
		return _elements.erase(position); }
	iterator erase ( iterator first, iterator last ) {
		return _elements.erase(first, last); }
	reference front ( ) {
		return _elements.front(); }
	const_reference front ( ) const {
		return _elements.front(); }
	allocator_type get_allocator() const {
		return _elements.allocato(); }
	iterator insert ( iterator position, const T& x ) {
		return _elements.insert(position, x); }
    void insert ( iterator position, size_type n, const T& x ) {
		_elements.insert(position, n, x); }
	template <class InputIterator>
    void insert ( iterator position, InputIterator first, InputIterator last ) {
		_elements.insert(position, first, last); }
	size_type max_size () const {
		return _elements.max_size(); }
	AutoGrowVector<T,Allocator>& operator= (const AutoGrowVector<T,Allocator>& x) {
		_elements = x._elements; return *this; }
	void pop_back ( ) {
		_elements.pop_back(); }
	void push_back ( const T& x ) {
		_elements.push_back(x); }
	reverse_iterator rbegin() {
		return _elements.rbegin(); }
	const_reverse_iterator rbegin() const {
		return _elements.rbegin(); }
    reverse_iterator rend() {
		return _elements.rend(); }
	const_reverse_iterator rend() const {
		return _elements.rend(); }
	void reserve ( size_type n ) {
		return _elements.reserve(n); }
	void resize ( size_type sz, T c = T() ) {
		_elements.resize(sz, c); }
	size_type size() const {
		return _elements.size(); }
	void swap ( AutoGrowVector<T,Allocator>& vec ) {
		_elements.swap(vec._elements); }
};

#endif
