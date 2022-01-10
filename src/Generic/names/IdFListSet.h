// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_LIST_SET_H
#define IDF_LIST_SET_H

class IdFSentenceTokens;
class PIdFSentence;

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"


#define MAX_IDF_LIST_NAME_LENGTH 100

class ListNode {
public:
	ListNode(Symbol sym) : children(0), next(0), word(sym) {}
	ListNode() : children(0), next(0), word(Symbol()) {}

	Symbol word;
	ListNode *children;
	ListNode *next;
};

/**
 * Manages the lists for IdF in both decode and training.
 * 
 * Basic algorithm for handling lists, in both decode and training:
 * When we see a set of symbols that is equivalent to a name on a list, we
 * push those symbols together into one symbol (by means of underscores, currently).
 * So, "United States" becomes "United_States". We then train or decode on this
 * revised (or "translated") set of symbols. The translation takes place in IdFSentence 
 * for training, and in decodeSentence(NBest) in decode. These new symbols are 
 * treated exactly as regular words, except that when they are being treated 
 * as unknown, they are changed into a feature :listFeature#, where # corresponds 
 * to the particular list which they were found. 
 *
 * ******THE SAME LISTS MUST BE USED IN DECODE AND TRAINING*******
 *
 * The constructor takes a file that is a list of filenames, each one itself a list of names. 
 * The lists must be pre-tokenized. Because the UTF8Token currently is only
 * really designed to work with sexps, the lists come in sexp format. The lists may
 * have names that designate their type, but IdentiFinder doesn't actually use this 
 * information (it just understands that all names in one list should be treated
 * similarly). 
 * So, the internal format of the list should be:
 *    (first name)
 *    (second name)
 *    ...
 *    (last name)
 *
 * Technical details:
 *
 * 1) When translating the original set of symbols, we obviously want to take the longest
 *    name possible. So, "San Francisco residents" --> "San_Francisco residents", but
 *    "San Francisco Giants" --> "San_Francisco_Giants". 
 * 2) For this translation process, we hash on the first Symbol of every name and store
 *    the rest of the information in a basic graph structure (in ListNodes). 
 * 3) For finding feature values associated with unknown list words, we use a separate
 *    hash table that just hashes on the compound names (e.g. United_States), to provide
 *    a one-step lookup for the feature value.
  */
class IdFListSet {
public:
	IdFListSet(const char *file_name);
	int isListMember(IdFSentenceTokens *tokens, int start_index) const;
	int isListMember(PIdFSentence *tokens, int start_index) const;
	Symbol getFeature(Symbol sym) const;
	Symbol barSymbols(IdFSentenceTokens *tokens, int start_index, int length) const;

private:
	Symbol barSymbols(Symbol firstWord, Symbol* ngram, int extra_words);

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

	HashKey hasher;
    EqualKey eqTester;
	typedef serif::hash_map<Symbol, ListNode *, HashKey, EqualKey> GraphHash;
	GraphHash* _graphHash;

	typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> FeatureTable;
	FeatureTable* _featureTable;

	int matchNode(ListNode *node, IdFSentenceTokens *tokens, int index) const;
	int matchNode(ListNode *node, PIdFSentence *tokens, int index) const;

	void readList(const wchar_t *list_file_name, int list_num);
	void addToTable(Symbol firstWord, int list_num, Symbol* ngram, int extra_words);
	Symbol *listFeatures;
	int numLists;

};

#endif
