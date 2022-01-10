// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_RULE_NAME_LINKER_H
#define es_RULE_NAME_LINKER_H

#include <fstream>
#include "Generic/common/hash_map.h"
#include "Generic/common/ParamReader.h"
#include "Generic/edt/RuleNameLinker.h"
#include "Generic/common/DebugStream.h"
#include "Generic/edt/SimpleRuleNameLinker.h"
#include "Generic/edt/EntityGuess.h"


/**
 * Rule-based methods for associating name
 * mentions with entities
 */
class SpanishRuleNameLinker : public RuleNameLinker {
private:
	friend class SpanishRuleNameLinkerFactory;

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
	typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> HashTable;

public:
	~SpanishRuleNameLinker();

	int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID,
					 EntityType linkType, LexEntitySet *results[],
					 int max_results);

	void cleanUpAfterDocument();


private:
	DebugStream _debug;
	HashTable * _noise;
	HashTable * _alternateSpellings;
	HashTable * _designators;
	HashTable * _suffixes;
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
	static const Symbol ATTR_FIRST_MI_LAST;
	static const Symbol ATTR_FIRST_LAST;
	static const Symbol ATTR_LAST;
	static const Symbol ATTR_MI;
	static const Symbol ATTR_MIDDLE;
	static const Symbol ATTR_SUFFIX;
	static const Symbol ATTR_NO_DESIG;
	static const Symbol ATTR_SINGLE_WORD;
	static const Symbol ATTR_ABBREV;
	static const Symbol ATTR_MILITARY;

	static const int RANK_LEVEL_0;
	static const int RANK_LEVEL_10;
	static const int RANK_LEVEL_20;
	static const int RANK_LEVEL_30;
	static const int RANK_LEVEL_40;
	static const int RANK_LEVEL_50;
	static const int RANK_LEVEL_60;
	static const int RANK_LEVEL_70;
	static const int RANK_LEVEL_80;
	static const int RANK_LEVEL_90;
	static const int RANK_LEVEL_100;

	SpanishRuleNameLinker();

	void loadFileIntoTable(Symbol filepath, HashTable * table);
	void loadAlternateSpellings(Symbol filepath);
	void generatePERVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes, HashTable *uniqAttributes);
	void generateORGVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes);
	void generateGPEVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes);
	void generateOTHVariations(int wordCount, const wchar_t **wordArray, HashTable *attributes);
	void addAcronymAttributes(int wordCount, Symbol *wordArray, HashTable *attributes);
	
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


	Symbol checkForContextLinks(LexEntitySet *currSolution, Mention *currMention);
	Symbol checkForAKA(LexEntitySet *currSolution, Mention *currMention);
	Symbol checkForParenDef(LexEntitySet *currSolution, Mention *currMention);

	bool isReversedCommaName(const wchar_t **wordArray, int word_count);

	bool _use_rules;
	SimpleRuleNameLinker *_simpleRuleNameLinker;

	bool ITEA_LINKING;
};

class SpanishRuleNameLinkerFactory: public RuleNameLinker::Factory {
	virtual RuleNameLinker *build() { return _new SpanishRuleNameLinker(); }
};

#endif

