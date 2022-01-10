// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_NAME_RECOGNIZER_H
#define en_NAME_RECOGNIZER_H

#include "Generic/names/NameRecognizer.h"
#include <string>
#include <vector>
#include "boost/regex.hpp"

class NameTheory;
class DocTheory;
class EnglishIdFNameRecognizer;
class PIdFModel;
class SymbolHash;
class PIdFSentence;
class DTTagSet;
class Sentence;
class IdFListSet;
class PatternNameFinder;
class Zone;
class Zoning;
class EnglishNameRecognizer : public NameRecognizer {
private:
	friend class EnglishNameRecognizerFactory;

public:
	~EnglishNameRecognizer();
	const static int MAX_NAMES = 100;
	const static int MAX_DOC_NAMES = 2000;
	const static int MAX_SENTENCE_LENGTH = 2000;

	virtual void resetForNewSentence(const Sentence *sentence);

	virtual void cleanUpAfterDocument();
	virtual void resetForNewDocument(DocTheory *docTheory = 0);

	// This puts an array of pointers to NameTheorys
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the NameTheorys.
	virtual int getNameTheories(NameTheory **results, int max_theories,
						        TokenSequence *tokenSequence);
	
private:
	EnglishNameRecognizer();

	std::set<std::wstring> _authorNameList;
	bool _skip_all_namefinding;

	typedef std::vector<Symbol> SymbolVector;

	static const boost::wregex email_address_re;

	void addUnfoundNationalities(PIdFSentence &sentence);
	NameTheory *makeNameTheory(PIdFSentence &sentence);

	bool _debug_flag;
	int _sent_no;
	const TokenSequence* _tokenSequence;

	// The following few things (fixName() and some supporting stuff) are
	// copied from the old name recognizer.
	// The purpose is to enforce intra-doc name consistency.
	// I do not like this code, as it leaks memory without a reasonable
	// bound, by making symbols out of all names (which span an arbitrary
	// number of tokens). If it is determined that Serif's memory footprint
	// increases too much on long runs, this is a likely culprit.
	// -- SRS

	// names, but pre-coagulation
	struct NameSet {
		int num_words;
		Symbol* words;
	};

	// for storing every name seen so far in the document
	std::wstring _names[MAX_DOC_NAMES];
	EntityType _types[MAX_DOC_NAMES];
	NameSet _nameWords[MAX_DOC_NAMES];
	int _num_names;
	int _num_constant_names;

	void fixName(NameSpan *span, PIdFSentence *sent);
	bool isEmailAddress(Symbol sym);
	bool hasPotentiallyMisleadingCasing(const Sentence *sentence);
	bool hasAbruptCaseChange(const Sentence *sentence);
	
	void addAuthorName(std::wstring name){
		_authorNameList.insert(name);
	}
	void addAuthorName(Zone* zone);

	void setAuthorNameList();

	typedef std::map<std::wstring, EntityType> _force_type_map_t;
	std::map<std::wstring, EntityType> _forcedTypeNames;

	// utility methods that build a symbol from a bunch of symbols
	// copied from old name recognizer -- SRS
	std::wstring getNameString(int start, int end, PIdFSentence* sent);
	std::wstring getNameString(int start, int end, NameSet set);

	static Symbol _NONE_ST;
	static Symbol _NONE_CO;
	static Symbol _PER_ST;
	static Symbol _PER_CO;
	// which implementation to use
	enum { IDF_NAME_FINDER, PIDF_NAME_FINDER } _name_finder;

	EnglishIdFNameRecognizer *_idfNameRecognizer;
	PIdFModel *_pidfDecoder;
	DTTagSet *_tagSet;
	DocTheory *_docTheory;

	SymbolHash *_nationsTable;

	bool _print_sentence_selection_info;

	bool _select_decoder_case_by_sentence;
	bool _force_lowercase_sentence;

	double _misleading_capitalization_threshold;
	bool _check_misleading_capitalization;

	bool _check_initial_sentence_capitalization;

	bool _disallow_emails;

	bool _use_name_constraints_from_document;
	bool _use_names_from_document;

	void reduceSpecialNames(PIdFSentence &sentence);
	bool _reduce_special_names;
	IdFListSet *_reductionTable;
	
	PatternNameFinder *_patternNameFinder;

    // ATEA fixes
	std::vector<SymbolVector> _perSubsumptionWords;
	void loadSubsumptionWords(std::string &wordsFile, std::vector<SymbolVector> &subsumptionWords);
	bool _use_atea_name_fixes;
	void fixReversedNames(PIdFSentence &sentence);
	bool inProbableList(PIdFSentence &sentence, int name_start);
	void forceDoubleParenNames(PIdFSentence &sentence);
	bool isForceableToPerson(PIdFSentence &sentence, int start, int end);
	void joinDoubleParenNames(PIdFSentence &sentence);
	void subsumeTokensInNames(PIdFSentence &sentence, std::vector<SymbolVector> &subsumptionWords, EntityType entType);
	int countSubsumptionWordsReverse(PIdFSentence &sentence, std::vector<SymbolVector> &subsumptionWords, int index);
	int countSubsumptionWordsForward(PIdFSentence &sentence, std::vector<SymbolVector> &subsumptionWords, int index);
	std::vector<std::wstring> getAllAuthorNames(Zoning* zoning);
	void getAllAuthorNames(Zone* zone);
};

class EnglishNameRecognizerFactory: public NameRecognizer::Factory {
	virtual NameRecognizer *build() { return _new EnglishNameRecognizer(); }
};


#endif
