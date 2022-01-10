// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SIMPLERULENAMELINKER_H
#define SIMPLERULENAMELINKER_H

#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ProbModel.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/DebugStream.h"
#define MAXSTRINGLEN 300


class SimpleRuleNameLinker
{
private:
	struct HashKey {
        const static int N = 50;
        size_t operator()(const Symbol* s) const {
            size_t val = 0;
			for (int i = 0; i < N; i++) {
				if (s[i].is_null())
					continue;
                val = (val << 2) + s[i].hash_code();
			}
            return val;
        }
    };

    struct EqualKey {
        const static int N = 50;
        bool operator()(const Symbol* s1, const Symbol* s2) const {
            for (int i = 0; i < N; i++) {
                if (s1[i] != s2[i]) {
                    return false;
                }
            }
            return true;
        }
    };

public:
	typedef serif::hash_map<Symbol*, Symbol, HashKey, EqualKey> HashTable;

public:
	SimpleRuleNameLinker();
	~SimpleRuleNameLinker();
	void setCurrSolution(LexEntitySet * currSolution);
	bool isOKToLink(Mention * currMention, EntityType linkType, EntityGuess * entGuess);
	bool isMetonymyLinkCase(Mention * currMention, EntityType linkType, EntityGuess * entGuess);
	bool isCityAndCountry(Mention* currMention, EntityType linkType, EntityGuess *entGuess);
private:
	DebugStream _debug;
	LexEntitySet * _currSolution;
	HashTable * _mentions;
	HashTable * _normalizedMentions;
	int _nNameMentions;

	HashTable * _noise;
	HashTable * _alternateSpellings;
	HashTable * _suffixes;
	HashTable * _designators;
	HashTable * _capitals;
	bool _distillationMode;

	bool isOKToLinkPER(Mention * currMention, EntityGuess * entGuess);
	bool isOKToLinkORG(Mention * currMention, EntityGuess * entGuess);
	bool isOKToLinkGPE(Mention * currMention, EntityGuess * entGuess);
	bool isOKToLinkOTH(Mention * currMention, EntityGuess * entGuess);
	bool isWKAllowable(Mention * currMention, EntityGuess * entGuess);
	bool isPartOf(Symbol * currTokens, int nTokens);
	bool isPartOfLongestNormalized(Symbol * currTokens, int nTokens);
	bool hasTokenMatch(const wchar_t * token, bool normalized = false);
	bool hasTokenMatchInMultWords(const wchar_t * token);
	void clearMentions();
	void loadNameMentions(Entity * entity);
	void loadFileIntoTable(std::string filepath, HashTable * table);
	void loadAlternateSpellings(std::string filepath);
	void loadCapitalsAndCountries(std::string filepath);
	Symbol getValueFromTable(HashTable *table, Symbol * key);
	Symbol * getSymbolArray(const wchar_t * str);
	Symbol getLowerSymbol(const wchar_t * str);
	int getTokenCount(Symbol * tokens);
	double _edit_distance_threshold;

};

#endif
