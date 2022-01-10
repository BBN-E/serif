// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Cluster/ReplaceLowFreq/ReplaceLowFreq.h"
#include "Cluster/ReplaceLowFreq/WordFeatures.h"
#include "Generic/common/ParamReader.h"


ReplaceLowFreq::ReplaceLowFreq()
{
	_countTable = _new CountTable();
}

ReplaceLowFreq::~ReplaceLowFreq()
{
	delete _countTable;
}

void ReplaceLowFreq::pruneToThreshold(int threshold, const char * rare_words_file) {
	_countTable->pruneToThreshold(threshold, rare_words_file);
}

vector <wstring> ReplaceLowFreq::doReplaceLowFreq(vector <wstring> tokens) {
	_result.clear();
	replaceLowFreqWords(tokens);
	return _result;
}

/*
void ReplaceLowFreq::addCounts(TokenSequence * tokenSeq) {
	for (int i = 0; i < tokenSeq->getNTokens(); i++) {
		Symbol word = tokenSeq->getToken(i)->getSymbol();
		_countTable->increment(word);
	}
}
*/
void ReplaceLowFreq::addCounts(Symbol word) {
	_countTable->increment(word);
}

void ReplaceLowFreq::replaceLowFreqWords(vector <wstring> tokens) {
	bool first = true;
	for (size_t i = 0; i < tokens.size(); i++) {
		Symbol word = Symbol(tokens[i].c_str());
		int count = _countTable->getCount(word);
		if (count == -1) {
			int feature = ClusterWordFeatures::lookup(word.to_string(), first);
			wstring unknown_feature = L"UNK";
			wchar_t unk_f[11];
#if defined(_WIN32)
			_itow(feature, unk_f, 10);
#else
			swprintf(unk_f, sizeof(unk_f)/sizeof(unk_f[0]), L"%d", feature);
#endif
			unknown_feature.append(unk_f);
			_result.push_back(unknown_feature);
		} else {
			_result.push_back(word.to_string());
		}
		first = false;
	}
}
