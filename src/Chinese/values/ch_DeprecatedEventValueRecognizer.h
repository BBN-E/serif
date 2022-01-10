#ifndef CH_DEPRECATED_EVENT_VALUE_RECOGNIZER_H
#define CH_DEPRECATED_EVENT_VALUE_RECOGNIZER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/values/DeprecatedEventValueRecognizer.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"
#include "Generic/common/hash_set.h"
#include "Generic/common/UTF8InputStream.h"

class EventMention;
class ValueMention;
class Mention;
class SynNode;
class Proposition;
class SentenceTheory;
class EventPatternMatcher;
class SymbolHash;

class ChineseDeprecatedEventValueRecognizer: public DeprecatedEventValueRecognizer {
private:
	friend class ChineseDeprecatedEventValueRecognizerFactory;

	void addPosition(DocTheory* docTheory, EventMention *vment);
	void addCrime(DocTheory* docTheory, EventMention *vment);
	void addSentence(DocTheory* docTheory, EventMention *vment);

	bool isJobTitle(Symbol word);
	bool isCrime(Symbol word);
	bool isSentence(Symbol word);

	Symbol getBaseType(Symbol fullType);

	void addValueMention(DocTheory *docTheory, EventMention *vment, 
						const Mention *ment, Symbol valueType, Symbol argType, float score);
	void addValueMentionForDesc(DocTheory *docTheory, EventMention *vment, 
						const Mention *ment, Symbol valueType, Symbol argType, float score);
	void addValueMention(DocTheory *docTheory, EventMention *vment, 
						const SynNode *node, Symbol valueType, Symbol argType, float score);
	void addValueMention(DocTheory *docTheory, EventMention *vment, 
						int start_token, int end_token, Symbol valueType, Symbol argType, float score);
	const Mention *getNthMentionFromProp(SentenceTheory *sTheory, const Proposition *prop, int n);
	const SynNode *getNthPredHeadFromProp(const Proposition *prop, int n);

	ValueMention *_valueMentions[MAX_DOCUMENT_VALUES];
	int _n_value_mentions;

	bool _add_values_as_args;

	EventPatternMatcher *_matcher;
	Symbol TOPLEVEL_PATTERNS;
	Symbol CERTAIN_SENTENCES;

	struct HashKey {
        size_t operator()(const Symbol s) const {
			return s.hash_code();
        }
    };

    struct EqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
			return s1 == s2;
		}
    };

	typedef hash_set <Symbol, HashKey, EqualKey> SymbolSet;
	SymbolSet *_crimeList, *_sentenceList;

	SymbolHash *_jobTitleList;

	void loadList(SymbolSet *list, UTF8InputStream &file);

	ChineseDeprecatedEventValueRecognizer();

public:
	~ChineseDeprecatedEventValueRecognizer();
	void createEventValues(DocTheory* docTheory);
	

};



class ChineseDeprecatedEventValueRecognizerFactory: public DeprecatedEventValueRecognizer::Factory {
	virtual DeprecatedEventValueRecognizer *build() { return _new ChineseDeprecatedEventValueRecognizer(); } 
};
 
#endif
