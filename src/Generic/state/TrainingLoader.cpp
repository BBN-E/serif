// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/TrainingLoader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/state/StateLoader.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/UnicodeUtil.h"
#include <boost/scoped_ptr.hpp>

TrainingLoader::TrainingLoader(const char *file, const wchar_t* stageString): _fileListStream(UTF8InputStream::build()) {
	_mode = LIST_MODE;
	_precounted_sentences = ParamReader::getOptionalIntParamWithDefaultValue("precounted_training_sentences",-1);
	wcscpy(_stage_string, stageString);
	initFromList(file);
	
}
TrainingLoader::TrainingLoader(const wchar_t *file,const wchar_t* stageString,
							   bool single_file_mode) : _mode(LIST_MODE), _fileListStream(UTF8InputStream::build()){
	wcscpy(_stage_string, stageString);
	_precounted_sentences = ParamReader::getOptionalIntParamWithDefaultValue("precounted_training_sentences",-1);
	if (single_file_mode) {
		_mode = SINGLE_FILE_MODE;
		initFromSingleFile(file);
	} else {
		char temp[501];
		StringTransliterator::transliterateToEnglish(temp, file, 500);
		_mode = LIST_MODE;
		initFromList(temp);
	}
}

TrainingLoader::TrainingLoader(const char *file, const wchar_t* stageString, 
							   int n_sentences): _fileListStream(UTF8InputStream::build()) {

	_mode = LIST_MODE;
	_precounted_sentences = n_sentences;
	wcscpy(_stage_string, stageString);
	initFromList(file);
}

void TrainingLoader::initStateLoader(const char *filename){
	boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
	UTF8Token token;
	_currentDocTheory = _new DocTheory(static_cast<Document*>(0));
	while(!tempFileListStream.eof()) {
		tempFileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		else{
			std::wstring wideFile = token.chars();
			std::string narrowFile( wideFile.begin(), wideFile.end() );
			_currentStateLoader = _new StateLoader(narrowFile.c_str());
			_currentDocTheory->loadFakedDocTheory(_currentStateLoader, state_tree_name);
			_currentDocTheory->resolvePointers(_currentStateLoader);
			break;
		}
	}
	delete _currentDocTheory;
	_currentDocTheory = 0;
	delete _currentStateLoader;
	_currentStateLoader = 0;
	tempFileListStream.close();
}

void TrainingLoader::initFromSingleFile(const wchar_t *file) {
	_currentDocTheory = 0;
	_copyOfCurrentEntitySet = 0;
	wcscpy(state_tree_name, L"DocTheory following stage: ");
	wcscat(state_tree_name, _stage_string);	
	_n_sentences = countSentencesInFile(file);
	_n_docs_in_file = countDocumentsInFile(file);
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, file, 500);
	SessionLogger::info("SERIF") << "Loading data from " << state_file_name_str << "\n";
	
	_currentStateLoader = _new StateLoader(state_file_name_str);
	_current_doc_index = 0;
}

void TrainingLoader::initFromList(const char *file) {
	_currentDocTheory = 0;
	_copyOfCurrentEntitySet = 0;
	_currentStateLoader = 0;
	wcscpy(state_tree_name, L"DocTheory following stage: ");
	wcscat(state_tree_name, _stage_string);
	
	initStateLoader(file);
	if (_precounted_sentences != -1)
		_n_sentences = _precounted_sentences;
	else 
		_n_sentences = countSentencesInFileList(file);
	_fileListStream->open(file);
	_current_doc_index = 0;
	_n_docs_in_file = 0;

}

TrainingLoader::~TrainingLoader() {
	if (_mode == LIST_MODE)
		_fileListStream->close();
	delete _currentDocTheory;
	delete _currentStateLoader;
}


SentenceTheory *TrainingLoader::getNextSentenceTheory() {
	if (_currentDocTheory != 0) {
		SentenceTheory *result = getNextFromDocTheory();
		if (result == 0) {
			delete _currentDocTheory;
			if (_copyOfCurrentEntitySet)
				_copyOfCurrentEntitySet->loseReference();
			_copyOfCurrentEntitySet = 0;
			_currentDocTheory = 0;
		} else return result;
	}
	if (_currentStateLoader != 0) {
		if (_current_doc_index == _n_docs_in_file) {
			delete _currentStateLoader;
			_currentStateLoader = 0;
		} else return getNextFromStateLoader();
	}
	if (_mode == SINGLE_FILE_MODE) return 0;
	if (_fileListStream->eof()) return 0;

	UTF8Token token;
	(*_fileListStream) >> token;
	if (wcscmp(token.chars(), L"") == 0) return 0;

	_n_docs_in_file = countDocumentsInFile(token.chars());
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, token.chars(), 500);
	SessionLogger::info("SERIF") << "Loading data from " << state_file_name_str << "\n";
	
	_currentStateLoader = _new StateLoader(state_file_name_str);
	_current_doc_index = 0;
	return getNextFromStateLoader();

}



SentenceTheory *TrainingLoader::getNextFromDocTheory() {
	
	while (_current_sent_index < _currentDocTheory->getNSentences()) {
		if (!_currentDocTheory->getSentence(_current_sent_index)->isAnnotated())
			_current_sent_index++;
		else break;
	}
	if (_current_sent_index == _currentDocTheory->getNSentences()){
		return 0;
	}
	else{
		return _currentDocTheory->getSentenceTheory(_current_sent_index++);
	}
}


SentenceTheory *TrainingLoader::getNextFromStateLoader() {
	_currentDocTheory = _new DocTheory(static_cast<Document*>(0));
	_currentDocTheory->loadFakedDocTheory(_currentStateLoader, state_tree_name);
	_currentDocTheory->resolvePointers(_currentStateLoader);
	if (_currentDocTheory->getEntitySet() != 0) {
		_copyOfCurrentEntitySet = _new EntitySet(*(_currentDocTheory->getEntitySet()), false);
		_copyOfCurrentEntitySet->gainReference();
	} else _copyOfCurrentEntitySet = 0;
	_current_sent_index = 0;
	_current_doc_index++;
	// special case for empty docs
	if (_currentDocTheory->getNSentences() == 0) {
		return getNextSentenceTheory();
	}

	return getNextFromDocTheory();
	
}


int TrainingLoader::countFilesInFileList(const char *filename) {

	boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
	UTF8Token token;
	int num_files = 0;
	while (!tempFileListStream.eof()) {
		tempFileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		num_files++;
	}
	tempFileListStream.close();
	SessionLogger::info("SERIF") << "\n" << num_files << " files in " << filename << "\n\n";
	return num_files;
}

int TrainingLoader::countSentencesInFileList(const char *filename) {

	boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
	UTF8Token token;
	int num_sentences = 0;
	while (!tempFileListStream.eof()) {
		tempFileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		num_sentences += countSentencesInFile(token.chars());
	}
	tempFileListStream.close();
 	SessionLogger::info("SERIF") << "\n" << num_sentences << " sentences in " << filename << "\n\n";
	return num_sentences;
}

int TrainingLoader::countDocumentsInFileList(const char *filename) {

	boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
	UTF8Token token;
	int num_documents = 0;
	while (!tempFileListStream.eof()) {
		tempFileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		num_documents += countDocumentsInFile(token.chars());
	}
	tempFileListStream.close();

	SessionLogger::info("SERIF") << "\n" << num_documents << " documents in " << filename << "\n\n";
	return num_documents;
}

int TrainingLoader::countSentencesInFile(const wchar_t *filename) {
	UTF8Token token;
	int num_sentences = 0;
	Symbol sentenceTheorySym(L"SentenceTheory");
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& file(*file_scoped_ptr);
	while (!file.eof()) {
		file >> token;
		if (token.symValue() == sentenceTheorySym){
			num_sentences++;
		}
	}
	file.close();
	return num_sentences;
}

int TrainingLoader::countSentencesInFile(const char *filename) {
	UTF8Token token;
	int num_sentences = 0;
	Symbol sentenceTheorySym(L"SentenceTheory");
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& file(*file_scoped_ptr);
	while (!file.eof()) {
		file >> token;
		if (token.symValue() == sentenceTheorySym){
			num_sentences++;
		}
	}
	file.close();
	return num_sentences;
}

int TrainingLoader::countDocumentsInFile(const wchar_t *filename) {
	UTF8Token token;
	Symbol serifStateSym(L"Serif-state");
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& file(*file_scoped_ptr);
	int ndocs = 0;
	while (!file.eof()) {
		file >> token;
		if (token.symValue() == serifStateSym)
			ndocs++;
	}
	file.close();
	int rem = ndocs % 2;
	if(rem != 0){
        std::wstringstream err;
		err << L"Uneven number of 'Serif-state' tokens in statefile: " << filename << L",  should be 2 for each document";
		throw UnexpectedInputException("TrainingLoader::countDocumentsInFile()", UnicodeUtil::toUTF8StdString(err.str()).c_str());
	}
	return ndocs/2;
}

int TrainingLoader::countDocumentsInFile(const char *filename) {
	UTF8Token token;
	Symbol serifStateSym(L"Serif-state");
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& file(*file_scoped_ptr);
	int ndocs = 0;
	while (!file.eof()) {
		file >> token;
		if (token.symValue() == serifStateSym)
			ndocs++;
	}
	file.close();
	int rem = ndocs % 2;
	if(rem != 0){
        std::stringstream err;
		err << "Uneven number of 'Serif-state' tokens in statefile: " << filename << ",  should be 2 for each document";
		throw UnexpectedInputException("TrainingLoader::countDocumentsInFile()", err.str().c_str());
	}
	return ndocs/2;
}
