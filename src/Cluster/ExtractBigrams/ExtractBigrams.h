#ifndef EXTRACT_BIGRAMS_H
#define EXTRACT_BIGRAMS_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "WSTokenizer.h"
#include "Generic/common/hash_map.h"

using namespace std;

class ExtractBigrams
{
public:
	ExtractBigrams();
	~ExtractBigrams();

	struct Bigram {
		Bigram(int h, int f) {
			_h = h;
			_f = f;
		}
	    
		Bigram() {
			_h = 0;
			_f = 0;
		}
	    
		int _h;
		int _f;
	};

	struct BigramHash
	{
		size_t operator()(const Bigram& b) const
		{
			return (b._h << 15) ^ b._f;
		}
	};

	struct BigramEq
	{
		bool operator()(const Bigram& b1, const Bigram& b2) const
		{
			return (b1._h == b2._h) && (b1._f == b2._f);
		}
	};

	struct StringHash
	{
		size_t operator()(const wstring& s) const
		{
			size_t result = 0;
			for (size_t i = 0; i < s.size(); i++) {
				result = (result << 2) ^ s[i];
			}
			return result;
		}
	};

	struct StringEq
	{
		bool operator()(const wstring& s1, const wstring& s2) const
		{
			return s1 == s2;
		}
	};

	typedef serif::hash_map<wstring, int, StringHash, StringEq> VocMap;
	typedef serif::hash_map<Bigram, int, BigramHash, BigramEq> BigramCount;

	void extractBigrams(vector <wstring> tokens);
	int printFiles(string prefix);
	vector <wstring> getVocabulary();
	BigramCount * getBigrams();
private:
	BigramCount * bigramCount;
	VocMap * voc;
	int vMax;

	int lookupVocCode(wstring& str, VocMap * vocMap, int& vocMax);
	void addBigram(Bigram& bigram);
	void printVocabulary(VocMap * vocMap, int vocMax, wofstream& out);
	void printBigrams(ofstream& out);


};


#endif
