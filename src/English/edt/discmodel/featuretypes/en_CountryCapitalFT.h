// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_COUNTRYCAPITAL_FT_H
#define en_COUNTRYCAPITAL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

#define MAX_NUM_WORDS_IN_MENTION_HEAD 10

class EnglishEn_CountryCapitalFT : public DTCorefFeatureType {
public:
	EnglishEn_CountryCapitalFT() : DTCorefFeatureType(Symbol(L"en-country-capital")) {}

	void validateRequiredParameters() { initializeTable();	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}


	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const;
private:
	SymbolArraySymbolArrayMap *_table[2];
	enum {country2capital, capital2country} HashTables;
	DebugStream _debugOut;

	class FileReader {
	public:
		FileReader(UTF8InputStream & file): _file(file), _nTokenCache(0) { }
		void getLeftParen() throw(UnexpectedInputException) { getToken(LPAREN); }
		void getRightParen() throw(UnexpectedInputException) { getToken(RPAREN); }
		UTF8Token getNonEOF() throw(UnexpectedInputException) { return getToken(LPAREN | RPAREN | WORD);}
		UTF8Token getToken(int specifier);
		bool hasMoreTokens();
		int getSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException);
		int getOptionalSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException);
		void pushBack(UTF8Token token);

	private:
		UTF8InputStream & _file;
		enum TokenType {
			LPAREN = 1 << 0,
			RPAREN = 1 << 1,
			EOFTOKEN = 1 << 2,
			WORD   = 1 << 3};
		int _nTokenCache;
		static const int MAX_CACHE = 5;
		UTF8Token _tokenCache[5];
	};

	void initializeTable();
	void add(Symbol *key, size_t nKey, Symbol *value, size_t nValue, size_t table_index);
	SymbolArray *lookupPhrase(SymbolArray *key, size_t index) const;
};
#endif
