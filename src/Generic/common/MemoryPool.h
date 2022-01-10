// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stddef.h>

// Forward declaration for boost::pool.
namespace boost {
	struct default_user_allocator_new_delete;
	template <typename UserAllocator> class pool;
}

class SimpleMemoryPool {
private:
	boost::pool<boost::default_user_allocator_new_delete> *_pool;
	size_t _items_per_block;
	size_t _item_size;
public:
	SimpleMemoryPool(size_t item_size, size_t items_per_block);
	virtual ~SimpleMemoryPool();
	void *allocate(size_t size=0);
	void deallocate(void * const chunk, size_t size=0);
	bool purge_memory();
	bool release_memory();
	void setItemsPerBlock(size_t items_per_block);
};

template<class ObjClass>
class MemoryPool: public SimpleMemoryPool {
public:
	MemoryPool(size_t items_per_block): SimpleMemoryPool(sizeof(ObjClass), items_per_block) {}
};

#endif

