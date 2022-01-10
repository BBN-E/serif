// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORDNET
#define WORDNET

#include "Generic/wordnet/xx_wordnet_externc.h" 
#include "Generic/common/Symbol.h" 
#include "Generic/parse/ChartEntry.h"
#include "Generic/common/std_hash.h"

// for speeding up

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"

#include "dynamic_includes/common/ProfilingDefinition.h"
#ifdef DO_WORDNET_PROFILING
#include "Generic/common/GenericTimer.h"
#endif

#define Max_Hypernym_Cached 100
#define Max_Hypernym_perPOS_Cached 50
#define Total_WordNet_Words 122000


// this line is added for debug purpose
//#include "Generic/common/UTF8OutputStream.h"
#include <list>

struct synset_cache_key_type {
	Symbol word; int pos; int wnptr;
	synset_cache_key_type( const Symbol & word, int pos, int wnptr ) : word(word), pos(pos), wnptr(wnptr) {}
	bool operator < ( const synset_cache_key_type & rhs ) const {
		if( word != rhs.word ) return word < rhs.word;
		if( pos != rhs.pos ) return pos < rhs.pos;
		return wnptr < rhs.wnptr;
	}
	operator size_t () const { return word.hash_code() ^ pos ^ (wnptr << 8); }
}; 

#if !defined(_WIN32)

#if defined(__APPLE_CC__)
      namespace std {
#else
      namespace __gnu_cxx {
#endif
            template<> struct hash<synset_cache_key_type> {
                  size_t operator()( const synset_cache_key_type & s ) const { return (size_t)(s); };
            };
      }

#endif


#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED WordNet {

// this is a singleton class
public:
	static WordNet* getInstance();
	static void deleteInstance();
	void cleanup();
protected:
	WordNet();
	~WordNet();
private:
	void init();
	static WordNet* _instance;

	char* dictionary_path;
	char* bin_path;

	int _nlookup;
	int _nfirst;
	int _nmult;

	int matches_wordlist (SynsetPtr synptr, const char *word, int level);
	bool matches_wordlist_base (SynsetPtr synptr, const char *word);
	Symbol lowercase_symbol(Symbol s) const;
	void copywchar_tochar(const wchar_t* wcstr, char* cstr, int max_len);
	void copychar_towchar(const  char* cstr, wchar_t* wcstr, int max_len);

	Symbol NN;
	Symbol NNS;
	Symbol NNP;

	char *pos;

	// new structure for speeding up
	struct SymbolArrayInt {
		int num;
		Symbol words[Max_Hypernym_Cached];
	};

	struct IntArrayInt {
		int num;
		int offsets[Max_Hypernym_perPOS_Cached];
	};
	// end of new structure for speeding up

	typedef stdhash::hash_map< std::pair<Symbol, int>, int> SymbolIntToIntHash;
	SymbolIntToIntHash * nthNounHypernymClassHash;
	SymbolIntToIntHash * nthVerbHypernymClassHash;
	SymbolIntToIntHash * nthOtherHypernymClassHash;

	typedef stdhash::hash_map<Symbol, bool> SymbolBoolHash;
	SymbolBoolHash * personWords_firstSense;
	SymbolBoolHash * personWords_allSenses;
	SymbolBoolHash * locationWords;
	SymbolBoolHash * partitiveWords;

	typedef stdhash::hash_map<Symbol, Symbol> SymbolSymbolHash;
	SymbolSymbolHash * nounStems;
	SymbolSymbolHash * verbStems;

	// new hash tables for speeding up WordNet access
	SymbolBoolHash * wordnetWords;
	
	typedef stdhash::hash_map<Symbol, SymbolArrayInt> SymbolToSymbolArrayIntHash;
	SymbolToSymbolArrayIntHash * hypernymClassHash_firstSense;
	SymbolToSymbolArrayIntHash * hypernymClassHash_allSenses;

	typedef stdhash::hash_map< std::pair<Symbol, Symbol>, bool> SymbolSymbolToBoolHash;
	SymbolSymbolToBoolHash * hyponymClassHash_firstSense;
	SymbolSymbolToBoolHash * hyponymClassHash_allSenses;

	typedef stdhash::hash_map<Symbol, IntArrayInt> SymbolToIntArrayIntHash;
	SymbolToIntArrayIntHash * nounHypernymOffsetHash;
	SymbolToIntArrayIntHash * verbHypernymOffsetHash;
	// end of adding new hash tables
	
	// Wordnet lookup caching, used by getSynonyms()
	static const int WORDNET_CACHE_SIZE = 10000;

	typedef std::list< std::pair< synset_cache_key_type, SynsetPtr > > synset_lru_list_type;

	typedef stdhash::hash_map< synset_cache_key_type, synset_lru_list_type::iterator > synset_cache_type;
	
	synset_lru_list_type synset_lru_list;
	synset_cache_type synset_cache;

public:
	// The returned SynsetPtr should be freed by free_syns after used.
	SynsetPtr getSynSet(Symbol word, int POS, int whichsense);
	SynsetPtr getFirstSynSet(Symbol word, int POS) { return getSynSet(word, POS, 1); }
	SynsetPtr getAllSynSets(Symbol word, int POS) { return getSynSet(word, POS, 0); }
	
	typedef stdhash::hash_map<Symbol, double> CollectedSynonyms;

	bool isHyponymOf(Symbol word, const char* proposed_hypernym, bool use_all_senses = false);
	bool isPerson(Symbol word, bool use_all_senses = false);
	bool isLocation(Symbol word);
	bool isPartitive(ChartEntry *entry);
	bool isNumberWord(Symbol word);
	bool isInWordnet(Symbol word);
	bool isNounInWordnet(Symbol word); // not cached
	bool isVerbInWordnet(Symbol word); // not cached

	Symbol stem(Symbol word, int POS) const;
	Symbol stem(Symbol word, Symbol POS) const;
	Symbol stem_noun(Symbol word) const;
	Symbol stem_verb(Symbol word) const;

	int getSenseCount(Symbol word, int POS);

	int getNthHypernymClass(Symbol word, int n);	
	int getNthHypernymClass(Symbol word, Symbol POS, int n);
	int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS);
	//added 04.04.06 by M. Levit:
	int getSynonyms(const Symbol&, CollectedSynonyms&, int, int, int, int=ALLSENSES);
	int collectNextLevel(SynsetPtr, int, CollectedSynonyms&);

	// new functions for speeding up WordNet access
	int getHypernymOffsets(Symbol word, Symbol pos, int *results, int MAX_RESULTS);

	// for debug
	//UTF8OutputStream temp_out1;

	// for timing scheme
	#ifdef DO_WORDNET_PROFILING
	mutable GenericTimer stemTimer;
	mutable GenericTimer morphTimer;
	mutable GenericTimer steminNounStemTimer;
	mutable GenericTimer hashCheckinginNounStemTimer;
	mutable GenericTimer hashInsertioninNounStemTimer;
	mutable GenericTimer getHypernymTimer;
	mutable GenericTimer getNthHypernymTimer;
	mutable GenericTimer getSynsetTimer;
	void printTrace();
	#endif
};

#endif
