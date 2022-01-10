// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKEN_TAG_TABLE_H
#define TOKEN_TAG_TABLE_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"

/** Maintains a hash table to associate key and value Symbols.
*  Loading by reading a UTF8 stream sets the private record of the 
*   widest key in the table, so that strings longer than that can
*   be rejected as not present without even building the symbol fo
*   the key.
*  Each line (after the max count line up front) has an
*   open paren and some blank-delimited sequences followed by a
*   a close paren.  In the usual case there are just two such 
*   sequences, and the first becomes the key, the second the value.
*   If there are more than two, all but the last are appended
*   with blanks between to form a complex key; in any case the 
*   last sequence designates the value, and is used as the value.
*  A value of ":NULL" is commonly used to mean "no value".
*
*/
class TokenTagTable {
private:
    static const float targetLoadingFactor;
    struct HashKey {
        size_t operator()(const Symbol& s) const {
            return s.hash_code();
        }
    };
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };

    typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> Table;
    Table* table;
	int widestKey;
public:
	TokenTagTable(){
		table = _new Table(5);
	}
    TokenTagTable(UTF8InputStream& in);
	~TokenTagTable();
    const Symbol lookup(Symbol word) const;
	size_t size() const {return table->size();};
	int maxKeySize() {return widestKey;};
};

#endif
