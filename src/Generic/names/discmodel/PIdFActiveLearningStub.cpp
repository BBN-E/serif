// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "names/discmodel/PIdFActiveLearning.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

wstring PIdFActiveLearning::Initialize(char* param_file) {
	return getRetVal(false, "PIdFActiveLearningStub: Initialize called");
}

wstring PIdFActiveLearning::Close(){
	return getRetVal(false, "PIdFActiveLearningStub: Close called");
}

wstring PIdFActiveLearning::Save(){
	return getRetVal(false, "PIdFActiveLearningStub: Save called");
}

wstring PIdFActiveLearning::ReadCorpus(char* corpus_file) {
	return getRetVal(false, "PIdFActiveLearningStub: ReadCorpus called");
}

wstring PIdFActiveLearning::ChangeCorpus(char* corpus_file) {
	return ReadCorpus(corpus_file);
}

wstring PIdFActiveLearning::SaveSentences(const wchar_t* ann_sentences, int str_length, const wchar_t* token_sents, int token_sents_length, const wchar_t * tokens_file) {
	return getRetVal(false, "PIdFActiveLearningStub: SaveSentences called");
}

wstring PIdFActiveLearning::Train(const wchar_t* ann_sentences, int str_length, const wchar_t* token_sents, int token_sents_length, int epochs, bool incremental){
	return getRetVal(false, "PIdFActiveLearningStub: Train called");
}

wstring PIdFActiveLearning::AddToTestSet(const wchar_t* ann_sentences, int str_length, 
										 const wchar_t* token_sents, int token_sents_length) 
{
	return getRetVal(false, "PIdFActiveLearningStub: AddToTestSet called");
}

wstring PIdFActiveLearning::DecodeFile(const wchar_t * input_file) {
	return getRetVal(false, "PIdFActiveLearningStub: DecodeFile called");
}

wstring PIdFActiveLearning::DecodeTraining(const wchar_t* ann_sentences, int length) {
	return getRetVal(false, "PIdFActiveLearningStub: DecodeTraining called");
}

wstring PIdFActiveLearning::DecodeTestSet(const wchar_t* ann_sentences, int length) {
	return getRetVal(false, "PIdFActiveLearningStub: DecodeTestSet called");
}

wstring PIdFActiveLearning::DecodeFromCorpus(const wchar_t* ann_sentences, int length) {
	return getRetVal(false, "PIdFActiveLearningStub: DecodeFromCorpus called");
}


wstring PIdFActiveLearning::SelectSentences(int training_pool_size, int num_to_select,
											int context_size, int min_positive_results)
{
	return getRetVal(false, "PIdFActiveLearningStub: SelectSentences called");
}

wstring PIdFActiveLearning::GetNextSentences(int num_to_select, int context_size)
{
	return getRetVal(false, "PIdFActiveLearningStub: GetNextSentences called");
}

wstring PIdFActiveLearning::getRetVal(bool ok, const wchar_t* txt) {
	wstring retval(L"<RETURN>\n");
	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";
	retval += txt;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}
wstring PIdFActiveLearning::getRetVal(bool ok, const char* txt) {
	wstring retval(L"<RETURN>\n");
	if(strlen(txt) >999){
		char errbuffer[1000];
		strcpy(errbuffer, "Invalid Conversion, String too long\nFirst 900 characters: ");
		strncat(errbuffer, txt, 900);
		return getRetVal(false, errbuffer);
	}
	wchar_t conversionbuffer[1000];
	mbstowcs(conversionbuffer, txt, 1000);

	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";

	retval += conversionbuffer;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}

std::wstring PIdFActiveLearning::GetCorpusPointer() {
	return getRetVal(true, "PIdFActiveLearningStub: GetCorpusPointer called");
}
