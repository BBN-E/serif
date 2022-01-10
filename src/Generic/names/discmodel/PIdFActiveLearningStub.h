// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_ACTIVE_LEARNING_H
#define P_IDF_ACTIVE_LEARNING_H

#include <iostream>

using namespace std;

class PIdFActiveLearning {

public:
	PIdFActiveLearning() {};
	wstring Initialize(char* param_file);
	wstring ReadCorpus(char* corpus_file = 0);
	wstring ChangeCorpus(char* new_corpus_file);
	wstring Train(const wchar_t* ann_sentences, int str_length, const wchar_t* token_sents, int token_sents_length, int epochs, bool incremental);
	wstring AddToTestSet(const wchar_t* ann_sentences, int str_length, const wchar_t* token_sents, int token_sents_length);
	wstring SelectSentences(int training_pool_size, int num_to_select,
		int context_size, int min_positive_results);
	wstring GetNextSentences(int num_to_select, int context_size);
	wstring DecodeTraining(const wchar_t* ann_sentences, int length);
	wstring DecodeTestSet(const wchar_t* ann_sentences, int length);
	wstring DecodeFromCorpus(const wchar_t* ann_sentences, int length);
	wstring DecodeFile(const wchar_t * input_file);
	wstring Save();
	wstring SaveSentences(const wchar_t* ann_sentences, int str_length, const wchar_t* token_sents, int token_sents_length, const wchar_t * tokens_file);
	wstring Close();
	wstring GetCorpusPointer();

private:
	wstring getRetVal(bool ok, const wchar_t* txt =L"");
	wstring getRetVal(bool ok, const char* txt);
};
#endif

