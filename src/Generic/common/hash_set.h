// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HASH_SET_H
#define HASH_SET_H

#include <utility>
#include <cstddef>
#include <algorithm>

template<class Key, class Hash, class Eq>
class hash_set {
private:
	size_t num_elements;
    size_t num_buckets;
    Hash hash;
    Eq eq;
    float max_load;

    struct Entry {
        Key key;
        Entry* next;
    };

	struct Block {
		size_t size;
		Entry *entries;
		Block *next;
	};

    Entry** hash_array;
    Entry* free_entries;
	Block* blocks;

    void allocate_entries(size_t block_size)
    {
		Block* new_block = new Block;
		new_block->next = blocks;
		blocks = new_block;

		new_block->size = block_size;
		new_block->entries = new Entry[block_size];

        for (size_t i = 0; i < (block_size - 1); i++) {
            new_block->entries[i].next = &new_block->entries[i + 1];
        }
        new_block->entries[block_size - 1].next = free_entries;
        free_entries = new_block->entries;
    }

    void grow()
    {
        size_t new_num_buckets = (num_buckets * 2) + 1;
        Entry** new_hash_array = new Entry*[new_num_buckets];
        for (size_t i = 0; i < new_num_buckets; i++) {
            new_hash_array[i] = 0;
        }
        for (size_t j = 0; j < num_buckets; j++) {
            Entry* p = hash_array[j];
            while (p) {
                Entry* q = p->next;
                size_t k = hash(p->key) % new_num_buckets;
                p->next = new_hash_array[k];
                new_hash_array[k] = p;
                p = q;
            }
        }
        delete [] hash_array;
        hash_array = new_hash_array;
        size_t block_size = 
			static_cast<size_t>((new_num_buckets - num_buckets) * max_load);
        allocate_entries(block_size);
        num_buckets = new_num_buckets;
    }

public:
    hash_set(size_t buckets)
        : num_buckets(buckets), hash(Hash()), eq(Eq()), max_load(0.75),
		free_entries(0), blocks(0), num_elements(0)
    {
        hash_array = new Entry*[num_buckets];
        for (size_t i = 0; i < num_buckets; i++) {
            hash_array[i] = 0;
        }
        size_t block_size = (std::max)(static_cast<size_t>(num_buckets * max_load),
                static_cast<size_t>(1));
        allocate_entries(block_size);
    }

	~hash_set() {
		delete [] hash_array;

		while (blocks) {
			Block *dead_block = blocks;
			blocks = blocks->next;

			delete[] dead_block->entries;
			delete dead_block;
		}
	}

    class iterator {
        //friend class hash_set;
    private:
        const hash_set* container;
        size_t bucket;
        const Entry* element;
    public:
        iterator() {}

        iterator(const iterator& iter)
        {
            container = iter.container;
            bucket = iter.bucket;
            element = iter.element;
        }

        iterator(const hash_set* _container, size_t _bucket, Entry* _element)
            : container(_container), bucket(_bucket), element(_element)
        {}

        friend bool operator==(const iterator i, const iterator j)
        {
            return (i.element == j.element);
        }

        friend bool operator!=(const iterator i, const iterator j)
        {
            return (i.element != j.element);
        }

        const Key& operator*()
        {
            return (*element).key;
        }

        iterator& operator++()
        {
            size_t num_buckets = container->num_buckets;
            Entry** hash_array = container->hash_array;

            element = element->next;
            if (element) {
                return *this;
            }
            do {
                ++bucket;
            } while ((bucket < num_buckets) && (hash_array[bucket] == 0));
            if (bucket < num_buckets) {
                element = hash_array[bucket];
            }
            return *this;
        }
    };

    friend class iterator;

    iterator begin() const
    {
        Entry* element = 0;
        size_t bucket = 0;
        while ((bucket < num_buckets) && (hash_array[bucket] == 0)) {
            ++bucket;
        }
        if (bucket != num_buckets) {
            element = hash_array[bucket];
        }
        return iterator(this, bucket, element);
    }

    iterator end() const
    {
        return iterator(this, num_buckets, 0);
    }

	std::pair<iterator, bool> insert(const Key& key)
    {
        size_t i = hash(key) % num_buckets;
        for (Entry* p = hash_array[i]; p; p = p->next) {
            if (eq(key, p->key)) {
                return std::pair<iterator,bool>(iterator(this, i, p), false);
            }
        }
        if (free_entries == 0) {
            grow();
            return insert(key);
        }
        Entry* entry = free_entries;
        free_entries = entry->next;
        entry->key = key;
        entry->next = hash_array[i];
        hash_array[i] = entry;

		num_elements++;

        return std::pair<iterator,bool>(iterator(this, i, entry), true);
    }

    iterator find(const Key& key) const
    {
        size_t bucket = hash(key) % num_buckets;
        for (Entry* p = hash_array[bucket]; p; p = p->next) {
            if (eq(key, p->key)) {
                return iterator(this, bucket, p);
            }
        }
        return end();
    }

	size_t size() const
	{
		return num_elements;
	}

	// This is approximate because there's some memory overhead in
	// doing allocation, ensuring alignment, etc.
	size_t approximateSizeInBytes() {
		size_t total_size = sizeof(hash_set);

		// Add in the size of all entries.
		for (size_t i = 0; i < num_buckets; i++) {
			for (Entry *e = hash_array[i]; e != 0; e=e->next) {
				total_size += sizeof(Entry);
			}
		}
		// Add in the size of the blocks that hold the entries.
		for (Block *b = blocks; b != 0; b=b->next) {
			total_size += sizeof(Block);
		}
		return total_size;
	}

	// This clears all entries, but does not deallocate any memory
	void remove_all() {
		for (size_t i = 0; i < num_buckets; i++) {
			Entry *entry = hash_array[i];
			while (entry != 0) {
				Entry *next_entry = entry->next;

				// add to free-list
				entry->next = free_entries;
				free_entries = entry;

				entry = next_entry;
			}
			hash_array[i] = 0;
		}
		num_elements = 0;
	}
	size_t erase(const Key& key) {
        size_t bucket = hash(key) % num_buckets;
		for (Entry **p = &(hash_array[bucket]); *p; p=&((*p)->next)) {
			if (eq(key, (*p)->key)) {
				Entry *next_entry = (*p)->next;
				// add to free-list
				(*p)->next = free_entries;
				free_entries = *p;
				// Delete from hash table
				*p = next_entry;
				--num_elements;
				return 1;
			}
		}
		return 0;
	}

	// JSG: for compatability with STL types
	void clear() {
		remove_all();
	}



/* The next 5 functions are in order to increase efficiency of the Symbol class
 * There is probably a better way to derive a template inside the Symbol class than this.
 */
	inline bool exists(const Key& key) const
	{
		size_t bucket = hash(key) % num_buckets;
		for (Entry* p = hash_array[bucket]; p; p = p->next) {
			if (eq(key, p->key)) {
				return true;
			}
		}
		return false;
	}


	inline const Key find_singleton(const Key& key) const {
		size_t bucket = hash(key) % num_buckets;
		for (Entry* p = hash_array[bucket]; p; p = p->next) {
			if (eq(key, p->key)) {
				return p->key;
			}
		}
		return NULL; // this only works for pointers
	}
	inline const Key find_singleton(const Key& key, size_t hash_val) const {
		size_t bucket = hash_val % num_buckets;
		for (Entry* p = hash_array[bucket]; p; p = p->next) {
			if (eq(key, p->key)) {
				return p->key;
			}
		}
		return NULL; // this only works for pointers
	}
	std::pair<iterator, bool> insert(size_t hash_val, const Key& key)
	{
		size_t i = hash_val % num_buckets;
		for (Entry* p = hash_array[i]; p; p = p->next) {
			if (eq(key, p->key)) {
				return std::pair<iterator,bool>(iterator(this, i, p), false);
			}
		}
		if (free_entries == 0) {
			grow();
			return insert(hash_val, key);
		}
		Entry* entry = free_entries;
		free_entries = entry->next;
		entry->key = key;
		entry->next = hash_array[i];
		hash_array[i] = entry;

		num_elements++;

		return std::pair<iterator,bool>(iterator(this, i, entry), true);
	}

	inline void insertWithoutChecking(const Key& key)
    {
        size_t i = hash(key) % num_buckets;
        if (free_entries == 0) {
            grow();
            return insertWithoutChecking(key);
        }
        Entry* entry = free_entries;
        free_entries = entry->next;
        entry->key = key;
        entry->next = hash_array[i];
        hash_array[i] = entry;

		num_elements++;
    }
// end of Symbol class specific functions

};

#endif

