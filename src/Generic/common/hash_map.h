// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HASH_MAP_H
#define HASH_MAP_H

#ifdef USE_BOOST_UNORDERED_MAP_TO_IMPLEMENT_HASH_MAP
/***********************************************************************
 * Boost-based implementation of hash_map
 ***********************************************************************/

#include <boost/unordered_map.hpp>

namespace serif {

template<class Key, class Value, class Hash, class Eq>
class hash_map: public boost::unordered_map<Key, Value, Hash, Eq> {
public:
	typedef typename boost::unordered_map<Key, Value, Hash, Eq>::iterator iterator;
	typedef typename boost::unordered_map<Key, Value, Hash, Eq>::const_iterator const_iterator;

	explicit hash_map(size_t size = 0, Hash const& hasher = Hash(), Eq const& key_equal = Eq())
		: boost::unordered_map<Key, Value, Hash, Eq>(size, hasher, key_equal) {}

	hash_map(hash_map const &other): boost::unordered_map<Key, Value, Hash, Eq>(other) {}
	hash_map& operator=(hash_map const &other) { boost::unordered_map<Key, Value, Hash, Eq>::operator=(other); return this; }

	// This returns a pointer to a value if an entry with the given key
	// exists; otherwise, it returns 0. (It differs from operator[] in
	// that it does not create the entry if it does not already exist.)
	Value* get(const Key& key) const {
		const_iterator it = find(key);
		return (it==this->end()) ? 0 : const_cast<Value*>(&(it->second));
	}
};


struct IntegerHashKey {
	size_t operator()(const int a) const {
		return a;
	}
};

struct IntegerEqualKey {
   bool operator()(const int a, const int b) const {
       return a == b;
   }
};

} // namespace serif

#else
/***********************************************************************
 * Original (home-grown) implementation of hash_map
 ***********************************************************************/

#include <locale>
#include <utility>
#include <cstddef>
#include <new>
#include <string>
#include <iomanip>
#include <cmath>
#include <sstream>
// #define this to enable detailed tracing, for performance
// #define HASH_MAP_COUNT_LOOKUPS true

namespace serif {

template<class Key, class Value, class Hash, class Eq>
class hash_map {
private:

    Hash hash;
    Eq eq;
    double max_load;
    const Value default_value;
    size_t num_buckets;
	size_t num_entries;
	#ifdef HASH_MAP_COUNT_LOOKUPS
	mutable size_t num_lookups;
	mutable size_t num_lookup_eqs;
	#endif
	
	typedef std::pair<const Key, Value> KeyValuePair;

    struct Entry {
		KeyValuePair key_val_pair;
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

	void init()
	{
        hash_array = new Entry*[num_buckets];
        for (size_t i = 0; i < num_buckets; i++) {
            hash_array[i] = 0;
        }

		size_t block_size = static_cast<size_t>(num_buckets * max_load);
		if (block_size == 0)
			block_size = 1;
        allocate_entries(block_size);
		#ifdef HASH_MAP_COUNT_LOOKUPS
			num_lookups = 0;
			num_lookup_eqs = 0;
		#endif
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
                size_t k = hash(p->key_val_pair.first) % new_num_buckets;
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
    hash_map()
        : num_buckets(1), hash(Hash()), eq(Eq()), max_load(0.75),
          default_value(Value()), free_entries(0), blocks(0), num_entries(0)
    {
		init();
    }

    hash_map(size_t buckets)
        : num_buckets(buckets), hash(Hash()), eq(Eq()), max_load(0.75),
          default_value(Value()), free_entries(0), blocks(0), num_entries(0)
    {
		init();
    }

    hash_map(size_t buckets, double max_load_arg)
        : num_buckets(buckets), hash(Hash()), eq(Eq()), max_load(max_load_arg),
          default_value(Value()), free_entries(0), blocks(0), num_entries(0)
    {
		init();
    }

    hash_map(size_t buckets, Hash& hash_arg, Eq& eq_arg)
        : num_buckets(buckets), hash(hash_arg), eq(eq_arg), max_load(0.75),
          default_value(Value()), free_entries(0), blocks(0), num_entries(0)
    {
		init();
    }

	// written quickly, not efficient
	hash_map(const hash_map<Key,Value,Hash,Eq>& map)
        : num_buckets(map.num_buckets), hash(map.hash), eq(map.eq),
		  max_load(map.max_load), default_value(map.default_value),
		  free_entries(0), blocks(0), num_entries(0)
	{
		init();
		for (iterator i=map.begin(), end=map.end(); i!=end; ++i)
			(*this)[(*i).first] = (*i).second;
	}

	// written quickly, not eficient
    hash_map<Key,Value,Hash,Eq>& operator=(const hash_map<Key,Value,Hash,Eq>& map)
    {
		remove_all();

		// (just leave num_buckets, esp. since shrinking isn't implemented.)
		hash = map.hash;
		eq = map.eq;
		max_load = map.max_load;
		// (Value should be the same, so default_value should be the same.)
		// (just leave free_entries, esp. since shrinking isn't implemented.)
		// (just leave blocks, esp. since shrinking isn't implemented.)

		for (iterator i=map.begin(), end=map.end(); i!=end; ++i)
			(*this)[(*i).first] = (*i).second;

		return *this;
    }

	~hash_map() {
		delete [] hash_array;

		while (blocks) {
			Block *dead_block = blocks;
			blocks = blocks->next;

			delete[] dead_block->entries;
			delete dead_block;
		}
	}

	size_t size() const
	{
		return num_entries;
	}

	// written quickly, not efficient
	void reserve(size_t buckets)
	{
		while (num_buckets * max_load < buckets)
			grow();
	}

    Value& operator[](const Key& key)
	{
        size_t i = hash(key) % num_buckets;
        for (Entry* p = hash_array[i]; p; p = p->next) {
            if (eq(key, p->key_val_pair.first)) {
                return p->key_val_pair.second;
            }
        }
        if (free_entries == 0) {
            grow();
            return operator[](key);
        }
        Entry* entry = free_entries;
        free_entries = entry->next;
		// first allow the entry to free any previously allocated memory
		(entry->key_val_pair).~KeyValuePair();
		// placement new to initialize
        new (&(entry->key_val_pair)) KeyValuePair(key, default_value);
        entry->next = hash_array[i];

        hash_array[i] = entry;
 		++num_entries;
        return entry->key_val_pair.second;
    }

	// This returns a pointer to a value if an entry with the given key
	// exists; otherwise, it returns 0. (It differs from operator[] in
	// that it does not create the entry if it does not already exist.)
	Value* get(const Key& key) const
	{
		#ifdef HASH_MAP_COUNT_LOOKUPS
			++num_lookups;
		#endif
		size_t i = hash(key) % num_buckets;
		for (Entry* p = hash_array[i]; p; p = p->next) {
			#ifdef HASH_MAP_COUNT_LOOKUPS
				++num_lookup_eqs;
			#endif
			if (eq(key, p->key_val_pair.first))
				return &p->key_val_pair.second;
		}
		return 0;
	}

	// CAUTION/BUG: 'key' must already be in the map
	Value *remove(const Key& key) {
		Entry *entry = 0;
		Entry **p_entry;
		size_t i = hash(key) % num_buckets;
		for (p_entry = &hash_array[i]; p_entry; p_entry = &(*p_entry)->next) {
			if (eq(key, (*p_entry)->key_val_pair.first)) {
				entry = *p_entry;
				break;
			}
		}
		if (entry == 0)
			return 0;

		// splice it out of the linked list
		*p_entry = entry->next;

		// insert it into the free-list
		entry->next = free_entries;
		free_entries = entry;

		--num_entries;

		return &entry->key_val_pair.second;
	}

	// STL compliant form
	size_t erase(const Key& key){
		return (remove( key ) == 0 ? 0 : 1);
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
		num_entries = 0;
	}

	// Clear all entries, and deallocate memory.
	void reset() {
		// Erase all memory.
		this->~hash_map();
		// Re-initialize from scratch.
		free_entries = 0;
		blocks = 0;
		num_entries = 0;
		num_buckets = 1;
		init();
	}

	// JSG: for compatability with STL types
	void clear() {
		remove_all();
	}

	#ifdef HASH_MAP_COUNT_LOOKUPS
	size_t get_num_lookups() {return num_lookups;}
	size_t get_num_lookup_eqs() {return num_lookup_eqs;}
	#endif

	size_t get_num_nonempty_buckets() const
	{
		size_t num_nonempty_buckets = 0;
		for (size_t i = 0; i < num_buckets; ++i)
			if (hash_array[i] != NULL)
				++num_nonempty_buckets;
		return num_nonempty_buckets;
	}

	// "path length" is number of equalities you'd compute if you looked up
	// each element once.  It's a metric for the lumpiness of the hash
	// function.  It equals sum((bucketSize*bucketSize+1)/2).
	size_t get_path_length() const
	{
		size_t path_length = 0;
		for (size_t i = 0; i < num_buckets; ++i) {
			size_t bucket_size = 0;
			for (Entry* entry = hash_array[i]; entry != NULL; entry = entry->next) {
				++bucket_size;
				path_length += bucket_size;
			}
		}
		return path_length;
	}

	// Requires operator<<(std::ostream) to be defined for Key and Value.
	void dump_internals(std::ostream& out, bool dump_contents = false)
		const
	{
		out << "hash_map " << this << " internals dump:" << "\n";
		out << "hash, eq       " << &hash << " " << &eq << "\n";
		out << "default_value  " << default_value << "\n";
		out << "max_load       " << max_load << "\n";
		out << "num_ne_buckets " << (unsigned long)get_num_nonempty_buckets() << "\n";
		out << "num_entries    " << (unsigned long)num_entries << "\n";
		out << "num_buckets    " << (unsigned long)num_buckets << "\n";
		#ifdef HASH_MAP_COUNT_LOOKUPS
		out << "num_lookups    " << (unsigned long)num_lookups << "\n";
		out << "num_lookup_eqs " << (unsigned long)num_lookup_eqs << "\n";
		#endif
		out << "path_length    " << (unsigned long)get_path_length() << "\n";
		if (dump_contents) {
			std::string index_indent;
			unsigned int index_width = (unsigned int)ceil(log10((float)num_buckets));
			for (unsigned short i = 1; i <= index_width + 1; ++i)
				index_indent += " ";
			for (size_t i = 0; i < num_buckets; ++ i) {
				if (hash_array[i] != NULL) {
					out << std::setw(index_width) << (unsigned int)i << " ";
					unsigned long bucket_size = 0;
					for (Entry* entry=hash_array[i]; entry!=NULL; entry=entry->next)
						++bucket_size;
					out << std::setw(index_width /*semi-arbitrary*/) << bucket_size << " ";
					for (Entry* entry=hash_array[i]; entry!=NULL; entry=entry->next) {
						if (entry != hash_array[i])
							out << index_indent << index_indent;
						out << entry->key_val_pair.first << " -> "
							<< entry->key_val_pair.second << "\n";
					}
				}
			}
		}
		out.flush();
	}

	void dump_buckets(std::ostream &out) const {
		for (size_t i = 0; i < num_buckets; i++) {
			if (hash_array[i] != 0) {
				size_t bucket_size = 0;
				for (Entry *entry = hash_array[i]; entry != 0;
					 entry = entry->next)
				{
					bucket_size++;
				}

				out << (int) i << ": " << (int) bucket_size << "\n";
			}
		}
	}


	class iterator {
        //friend class hash_map;
    private:
        const hash_map * container;
        size_t bucket;
        Entry* element;
    public:
        iterator() {}

        iterator(const iterator& iter)
        {
            container = iter.container;
            bucket = iter.bucket;
            element = iter.element;
        }

        iterator(const hash_map * _container, size_t _bucket, Entry* _element)
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

		KeyValuePair & operator*() const
        {
            return (*element).key_val_pair;
        }

		KeyValuePair * operator->() const
		{
			return &((*element).key_val_pair);
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

	// Pretend that we have a const_iterator type; but note that this
	// const_iterator *does* allow modifications.  This is just included
	// to allow code to be written that's compatible with both this
	// implementation and the one based on boost::unordered_map.
	typedef iterator const_iterator;

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

	iterator find(const Key& key) const
	{
		#ifdef HASH_MAP_COUNT_LOOKUPS
			++num_lookups;
		#endif
		size_t bucket = hash(key) % num_buckets;
		for (Entry* p = hash_array[bucket]; p; p = p->next) {
			#ifdef HASH_MAP_COUNT_LOOKUPS
				++num_lookup_eqs;
			#endif
			if (eq(key, p->key_val_pair.first))
				return iterator(this, bucket, p);
		}
		return end();
	}

	

	std::pair<iterator, bool> insert( const KeyValuePair & x ) {
	
		std::pair<iterator,bool> res;
		
        size_t i = hash(x.first) % num_buckets;
        for (Entry* p = hash_array[i]; p; p = p->next) {
            if (eq(x.first, p->key_val_pair.first)) {

				// already present, just return (don't insert)
				res.first = iterator(this,i,p);
				res.second = false;
				return res;
            }
        }
        if (free_entries == 0) {
            grow();
            return insert( x );
        }

        Entry* entry = free_entries;
        free_entries = entry->next;
		// first allow the entry to free any previously allocated memory
		(entry->key_val_pair).~KeyValuePair();
		// placement new to initialize
        new (&(entry->key_val_pair)) KeyValuePair(x);
        entry->next = hash_array[i];

        hash_array[i] = entry;
 		++num_entries;

		res.first = iterator(this,i,entry);
		res.second = true;
		return res;
	}

	void insert(iterator first, iterator last) {
		while( first != last ){
			insert( *first );
			++first;
		}

		return;
	}


};

// test function
void hash_map_Test();


struct IntegerHashKey {
	size_t operator()(const int a) const {
		return a;
	}
};

struct IntegerEqualKey {
   bool operator()(const int a, const int b) const {
       return a == b;
   }
};

} // namespace serif

#endif
#endif
