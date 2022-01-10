// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/MemoryPool.h"
#include <boost/pool/pool.hpp>
#include <Generic/common/InternalInconsistencyException.h>

SimpleMemoryPool::SimpleMemoryPool(size_t item_size, size_t items_per_block): 
	_pool(new boost::pool<>(item_size, items_per_block)),
	_item_size(item_size), _items_per_block(items_per_block) {}

SimpleMemoryPool::~SimpleMemoryPool() {
	delete _pool;
}

void *SimpleMemoryPool::allocate(size_t size) {
	if ((size != 0) && (size != _item_size)) 
		throw InternalInconsistencyException("MemoryPool::allocate",
			"Memory pool does not support arbitrary-sized allocations");
	_pool->set_next_size(_items_per_block);
	return _pool->ordered_malloc();
}
void SimpleMemoryPool::deallocate(void * const chunk, size_t size) {
	if ((size != 0) && (size != _item_size)) 
		throw InternalInconsistencyException("MemoryPool::allocate",
			"Memory pool does not support arbitrary-sized allocations");
	_pool->ordered_free(chunk);
}
bool SimpleMemoryPool::purge_memory() {
	return _pool->purge_memory();
}
bool SimpleMemoryPool::release_memory() {
	return _pool->release_memory();
}

void SimpleMemoryPool::setItemsPerBlock(size_t items_per_block) {
	_items_per_block = items_per_block;
}
