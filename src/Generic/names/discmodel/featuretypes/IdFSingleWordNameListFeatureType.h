// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_SINGLEWORD_NAMELIST_FEATURE_TYPE_H
#define D_T_SINGLEWORD_NAMELIST_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/ParamReader.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

#include <stdio.h>
#include <boost/scoped_ptr.hpp>

class IdFSingleWordNameListFeatureType : public PIdFFeatureType {
private:
	static bool _initialized;
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
	/// List of nationality names to help identify person descriptors
	//static hash_set<Symbol, HashKey, EqualKey> _natNames;
	typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> Table;
	static Table* _nameList;
	HashKey _hasher;
	EqualKey _eqTester;
	static Table* _knownVocabulary;
	const static int _threshold = 3;


void initializeLists(){
	if (!ParamReader::isInitialized() || !ParamReader::hasParam("pidf_name_list_file"))
	{
		_nameList = _new Table(1, _hasher, _eqTester);
		return;
	}

	std::string buffer = ParamReader::getRequiredParam("pidf_name_list_file");	
	boost::scoped_ptr<UTF8InputStream> countStream_scoped_ptr(UTF8InputStream::build(buffer.c_str()));
	UTF8InputStream& countStream(*countStream_scoped_ptr);
	UTF8Token token;
	int numLists = 0;
	int numListItems = 0;
	// count lists (for feature value array),
	//   and count number of items in lists (for approximate hash size)
	while (!countStream.eof()) {
		countStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		if (token.chars()[0] == '#')
			continue;
		numLists++;
		boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build(token.chars()));
		UTF8InputStream& listStream(*listStream_scoped_ptr);
		UTF8Token listToken;
		while (!listStream.eof()) {
			listStream >> listToken;
			if (listToken.symValue() == SymbolConstants::leftParen)
				numListItems++;
		}
		listStream.close();
	}
	countStream.close();
	_nameList = _new Table(static_cast<int>(numListItems/.7), _hasher, _eqTester);

	boost::scoped_ptr<UTF8InputStream> fileStream_scoped_ptr(UTF8InputStream::build(buffer.c_str()));
	UTF8InputStream& fileStream(*fileStream_scoped_ptr);

	// read in actual lists
	int count = 0;
	wchar_t stem_buffer[500];
	int listnum =0;
	while (!fileStream.eof()) {
		listnum++;
		fileStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		if (token.chars()[0] == '#')
			continue;
		const wchar_t* fn = token.chars();
		int fnlen = static_cast<int>(wcslen(fn));
		int fplace = fnlen-1;
		while((fplace > 0) && !(( fn[fplace] == L'/') || (fn[fplace] == L'\\'))){
			fplace--;
		}
		fplace++;
		int n =0;
		for(; fplace < fnlen; fplace++){
			stem_buffer[n] = fn[fplace];
			n++;
		}
		stem_buffer[n] = L'\0';

		readList(token.chars(), Symbol(stem_buffer), listnum);
		count++;
	}
	//read in known vocab
	buffer = ParamReader::getParam("pidf_name_list_vocab_file");
	if (buffer.empty()) {
		_knownVocabulary = _new Table(1, _hasher, _eqTester);
	}
	else{
		readVocab(buffer.c_str());
	}

}
void readList(const wchar_t *list_file_name, Symbol filename, int i) {

	boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build(list_file_name));
	UTF8InputStream& listStream(*listStream_scoped_ptr);
	UTF8Token token;
	wchar_t buffer[300];
	swprintf(buffer,300,L":listFeature-%ls-%d", filename.to_string(),i);
	Symbol feature(buffer);
	int count = 0;
	while (!listStream.eof()) {
		listStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
          char c[1000];
          sprintf( c, "ERROR: ill-formed list entry in %s: %d\n",
			  Symbol(list_file_name).to_debug_string(), count);
		  throw UnexpectedInputException("IdFSingleWordNameListFeatureType::readList", c);
        }

		listStream >> token;
		Symbol firstWord = token.symValue();

		listStream >> token;
		Symbol nextWord = token.symValue();
		if( nextWord != SymbolConstants::rightParen){
			//skip multi word name lists!
			while (nextWord != SymbolConstants::rightParen) {
				listStream >> token;
				nextWord = token.symValue();
				if (listStream.eof() ||	nextWord == SymbolConstants::leftParen) {
					char c[1000];
					sprintf( c, "ERROR: list entry without close paren in %s at line %d.  nextWord is '%s'\n",
						Symbol(list_file_name).to_debug_string(), count, nextWord.to_debug_string());
					throw UnexpectedInputException("IdFSingleWordNameListFeatureType::readList", c);
				}
			}
			continue;
		}
		else{
			//always write over what was in the list!
			//std::cout<<"Add to List: "
			//	<<feature.to_debug_string()<<" "
			//	<<firstWord.to_debug_string()<<std::endl;
			(*_nameList)[firstWord] = feature;
		}
	}

}
void readVocab(const char *vocab_file_name){

	boost::scoped_ptr<UTF8InputStream> vocabStream_scoped_ptr(UTF8InputStream::build(vocab_file_name));
	UTF8InputStream& vocabStream(*vocabStream_scoped_ptr);
	UTF8Token token;
	wchar_t buffer[300];
	Symbol feature(buffer);
	int count = 0;
	int nwords =0;
	vocabStream >> token;
	nwords =  _wtoi(token.chars());
	if(nwords < 100)
		nwords = 100;
	_knownVocabulary = _new Table(static_cast<int>(nwords/.7), _hasher, _eqTester);
	while (!vocabStream.eof()) {
		vocabStream >> token;
		Symbol word = token.symValue();
		vocabStream >>token;
		int wordcount = _wtoi(token.chars());
		if(wordcount > _threshold){
			(*_knownVocabulary)[word] = token.symValue();
		}
	}
}




public:
	IdFSingleWordNameListFeatureType() : PIdFFeatureType(Symbol(L"SingleWordNameList"), InfoSource::OBSERVATION) {
		if(!_initialized){
			initializeLists();
			_initialized = true;
		}
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));

		Symbol word = o->getSymbol();
		Table::iterator iter;
		//make sure this is a low frequency word
		iter = _knownVocabulary->find(word);
		if(iter != _knownVocabulary->end())
			return 0;
		iter = _nameList->find(o->getSymbol());
		if (iter == _nameList->end())
			return 0;
		//std::cout<<"List Feature: "<<state.getTag().to_debug_string()<<" "
		//	<<o->getSymbol().to_debug_string()<<" "
		//	<<(*iter).second.to_debug_string()<<std::endl;
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), (*iter).second);
		return 1;
	}
};
bool IdFSingleWordNameListFeatureType::_initialized = false;
IdFSingleWordNameListFeatureType::Table* IdFSingleWordNameListFeatureType::_nameList = 0;
IdFSingleWordNameListFeatureType::Table* IdFSingleWordNameListFeatureType::_knownVocabulary = 0;
#endif
