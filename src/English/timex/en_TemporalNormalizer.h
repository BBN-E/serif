// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_TEMPORAL_NORMALIZER_H
#define EN_TEMPORAL_NORMALIZER_H

#include <string>
#include "boost/regex.hpp"
#include "Generic/values/TemporalNormalizer.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/TimexUtils.h"
#include "Generic/common/hash_map.h"
#include "Generic/theories/Zone.h"

#define MAX_KEYS 500

class DocTheory;
class Parse;
class SynNode;
class TimeObj;
class TemporalString;
class TimePoint;
class LocatedString;

using namespace std;

class EnglishTemporalNormalizer : public TemporalNormalizer {
private:
	friend class EnglishTemporalNormalizerFactory;

public:
	static const int num_tuc = 12;
	static const wstring timeUnitCodes[num_tuc];
	static const int num_ptu = 12;
	static const wstring pureTimeUnits[num_ptu];
	static const int num_etu = 10;
	static const wstring exTimeUnits[num_etu];

	static const wstring _HALF_;
	static const wstring _QUARTER_;
	static const wstring _THREE_QUARTERS_;

	~EnglishTemporalNormalizer();
	void normalizeTimexValues(DocTheory *docTheory);
	void normalizeDocumentDate(const DocTheory *docTheory, wstring &docDateTime, wstring &additionalDocTime);
	void normalizeZoneDocumentDate(const wstring & originalDtString, wstring &docDateTime, wstring &additionalDocTime);


	static bool isPureTimeUnit(const wstring &str);
	static bool isExTimeUnit(const wstring &str);
	static void test();
	static wstring timeUnit2Code(const wstring &str);

	TimeObj* normalize(wstring str);
	wstring canonicalizeString(wstring str);
	TimeObj* highProbabilityMatch(TemporalString *ts);
	void matchGenericRefs(const wstring &s, TimeObj *time);
	void matchModifiers(const wstring &s, TimeObj *time);
	void combineAdjacentNumbers(TemporalString *t);
	void dealWithFractions(TemporalString *t);

	// added by JSG for normalization after the main pass
//	wstring normalizeStringToVal( wstring & timeString );
	
private:
	EnglishTemporalNormalizer();

	TimePoint *_context;
	
	static const int num_tenses = 4;
	static const wstring tenseStrings[num_tenses];

	typedef enum {UNK_TENSE = 0, PAST_TENSE, PRESENT_TENSE, FUTURE_TENSE} Tense;
	Tense _contextVerbTense;
	Tense getContextVerbTense(Parse *parse, int start_tok, int end_tok);
	void addHeadWordAndTagToChain(const SynNode *node, wstring& tag_chain, wstring& word_chain);

	TimeObj* getAndParseDateString(TemporalString *ts);
	TimeObj* getAndParseTimeString(TemporalString *ts);

	wstring getAdditionalDocTime(const wstring & originalDtString);
	
	wstring parseExplicitDateTime(const wstring & instr);

	
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

	typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> SubstitutionTable;
	SubstitutionTable* _table;
	Symbol _keys[MAX_KEYS];
	int _num_keys;
	void initializeSubstitutionTable();

	bool allDigits(const wstring &str);

};

class EnglishTemporalNormalizerFactory: public TemporalNormalizer::Factory {
public:
	virtual TemporalNormalizer *build()
	{ return _new EnglishTemporalNormalizer(); }
};


#endif
