// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_ALIAS_TABLE_H
#define CH_ALIAS_TABLE_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SexpReader.h"

class AliasTable {
public:
	/** 
	  * Returns a pointer to an instance of the singleton
	  * class AliasTable. Initializes the table from file
	  * on first call.
	  *
	  * @return a pointer to an instance of AliasTable
	  */
	static AliasTable* getInstance();
	static void destroyInstance();
	int lookup(Symbol key, Symbol results[], int max_results);

protected:
	AliasTable();

private:
	static AliasTable* _instance;

	void initialize(const char* file_name);

	class SymArray {
	public:
		Symbol *array;
		int length;
		SymArray(): length(0), array(0) { }
		SymArray(Symbol array_[], int length_) {
			length = length_;
			array = _new Symbol[length];
			for(int i=0; i<length; i++)
				array[i] = array_[i];
		}
		SymArray(SymArray &other) {
			length = other.length;
			array = _new Symbol[length];
			for(int i=0; i<length; i++)
				array[i] = other.array[i];
		}

		~SymArray() {
			delete[] array;
		}
	};

	class AliasFileReader: public SexpReader {
	public:
		AliasFileReader();
		AliasFileReader(const char *file_name);
		Symbol getKeySymbol() throw(UnexpectedInputException);
		int getAliasSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException);
	};
	
//private:
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

	typedef hash_map <Symbol, SymArray*, HashKey, EqualKey> SymbolSymArrayMap;
	SymbolSymArrayMap *_table;

	bool add(Symbol key, SymArray *value);
	
	static DebugStream _debugOut;
};

#endif
