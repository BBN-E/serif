// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef kr_RULE_NAME_LINKER_H
#define kr_RULE_NAME_LINKER_H

#include <fstream>
#include "common/hash_map.h"
#include "common/ParamReader.h"
#include "edt/RuleNameLinker.h"
#include "common/DebugStream.h"
#include "edt/EntityGuess.h"


/**
 * Rule-based methods for associating name
 * mentions with entities
 */

class KoreanRuleNameLinker : public RuleNameLinker {
private:
	friend class KoreanRuleNameLinkerFactory;

	struct HashKey {
        size_t operator()(const Symbol key) const {
			return key.hash_code();
        }
    };
	struct EqualKey {
        bool operator()(const Symbol key1, const Symbol key2) const {
            return (key1 == key2);
        }
    };

public:
	typedef hash_map<Symbol, Symbol, HashKey, EqualKey> HashTable;

public:
	~KoreanRuleNameLinker();

	/**
	 * using a set of rules, either create a new entity from currMention or associate it with
	 * an entity in currSolution.
	 *
	 * @param currSolution The set of entities up to this point
	 * @param currMention The mention to be linked (or not linked)
	 * @param results The possibly many but realistically one set of entities after this linking
	 * @param max_results The maximum size of results
	 * @return the size of results
	 */
	int linkMention (LexEntitySet * currSolution, int currMentionUID,
					 EntityType linkType, LexEntitySet *results[],
					 int max_results);

	void cleanUpAfterDocument();


private:
	DebugStream _debug;

	HashTable * _per_mentions;
	HashTable * _org_mentions;
	HashTable * _gpe_mentions;
	HashTable * _oth_mentions;
	
	HashTable * _per_attributes;
	HashTable * _org_attributes;
	HashTable * _gpe_attributes;
	HashTable * _oth_attributes;
	HashTable * _unique_per_attributes;
	
	static int initialTableSize;

	static const Symbol ATTR_FULLEST;

	KoreanRuleNameLinker();
	void generatePERVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes, HashTable *uniqAttributes);
	void generateORGVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes);
	void generateGPEVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes);
	void generateOTHVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes);

	void mergeMentions(HashTable * source, HashTable * destination, Symbol entID);
	Symbol linkPERMention(LexEntitySet *currSolution, Mention *currMention);
	Symbol linkORGMention(LexEntitySet *currSolution, Mention *currMention);
	Symbol linkGPEMention(LexEntitySet *currSolution, Mention *currMention);
	Symbol linkOTHMention(LexEntitySet *currSolution, Mention *currMention);
	Symbol getValueFromTable(HashTable *table, Symbol key);
	void addAttribute(HashTable *table, Symbol key, Symbol value);
	void replaceChar(std::wstring &str, const std::wstring &from,
					 const std::wstring &to);

	Symbol getEntityIDFromTerminal(LexEntitySet *currSolution, 
							   Mention *currMention, const SynNode *terminal);
	Mention *getMentionFromTerminal(LexEntitySet *currSolution, 
							   Mention *currMention, const SynNode *terminal);


};

class KoreanRuleNameLinkerFactory: public RuleNameLinker::Factory {
	virtual RuleNameLinker *build() { return _new KoreanRuleNameLinker(); }
};

#endif

