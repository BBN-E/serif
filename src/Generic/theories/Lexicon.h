// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXICON_H
#define LEXICON_H
#define MAX_ENTRIES_PER_SYMBOL 500
#define MAX_ENTRIES 5000000 //500000 //5000000 //300000
#define MAX_SYMBOLS 100000 //500000 //100000 //500000
#define MAX_DYNAMIC_ENTRIES 500

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/LexicalEntry.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnrecoverableException.h"
#include <vector>


class Lexicon : public Theory {
private: 

	typedef Symbol::HashMap<std::vector<LexicalEntry*> > SymbolHashMap;
	SymbolHashMap* _entries_by_symbol_static; 
	SymbolHashMap* _entries_by_symbol_dynamic; 
	std::vector<LexicalEntry*> _entries_by_id;
	size_t _start_size;		// this is the number of static entries


	void addEntry(LexicalEntry* le, bool isStaticEntry) throw (UnrecoverableException);
	void addEntryDBG(LexicalEntry* le, bool isStaticEntry) throw (UnrecoverableException);
	int addEntriesByKey(Symbol key, LexicalEntry** results, 
						int maxResults, int firstIndex, 
						bool useStaticTable);

	LexicalEntry* findEntryInTable(LexicalEntry* entry, SymbolHashMap* table);

public:
	/** Return a pointer to the singleton session lexicon.  If the
	 * session lexicon does not exist (i.e., if the first time this
	 * method is called, or the first time it is called after a call
	 * to destroySessionLexicon(), then a new lexicon is created using
	 * the lexicon factory. */
	static Lexicon *getSessionLexicon();

	/** Destroy the session lexicon. */
	static void destroySessionLexicon();

	/** Hook for registering new Lexicon factories */
	struct Factory { virtual Lexicon *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }


	Lexicon(size_t n_entries);
	~Lexicon();
	//int new_stem_count;
	bool lexiconFull(){	
		if(_entries_by_id.size() < (MAX_ENTRIES - 1000)) return false;
		else return true;
	};
	//void setClearLexicon(bool cl){ _clear_lexicon = cl;};
	int getEntriesByKey(Symbol key, LexicalEntry** result, int size);
	int getNEntriesByKey(Symbol key);
	size_t getStartSize() {return _start_size;};
	LexicalEntry* getEntryByID(size_t id);
	size_t getNextID();

	// If the lexicon already contains a lexical entry whose value is equal to the given
	// entry, then return the entry that is in the lexicon.  Otherwise, return NULL.
	// Both static and dynamic entries are checked.
	LexicalEntry* findEntry(LexicalEntry* entry);

	void addStaticEntry(LexicalEntry* le) throw (UnrecoverableException);
	void addDynamicEntry(LexicalEntry* le) throw (UnrecoverableException);
	void clearDynamicEntries();

	void addStaticEntryDBG(LexicalEntry* le);
	void addDynamicEntryDBG(LexicalEntry* le);

	bool hasID(size_t id);
	bool isDynamicEntry(size_t id);
	bool isStaticEntry(size_t id);
	bool hasKey(Symbol key);
	size_t getNEntries();
	void dump(UTF8OutputStream &uos);
	void dumpDynamicVocab(UTF8OutputStream &uos);

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Lexicon(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

	//size_t getEntriesByKey(Symbol key, LexicalEntry** result);
	//void removeAddedEntries();
private:
	static boost::shared_ptr<Factory> &_factory(); 	
	static Lexicon *_sessionLexicon;
};

struct DefaultLexiconFactory: public Lexicon::Factory {
	Lexicon* build() { 
		return _new Lexicon(static_cast<size_t>(0));
	}
};

#endif
