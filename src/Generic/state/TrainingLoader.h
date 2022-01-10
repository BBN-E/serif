// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TRAINING_LOADER_H
#define TRAINING_LOADER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include <boost/scoped_ptr.hpp>

class DocTheory;
class SentenceTheory;
class StateLoader;


class TrainingLoader {
public:
	TrainingLoader(const char *file, const wchar_t* stageString);
	//skip reading the documents to count sentences, and set _n_sentences from the parameter
	TrainingLoader(const char *file, const wchar_t* stageString, int n_sentences);
	TrainingLoader(const wchar_t *file, const wchar_t* stageString, bool single_file_mode = false);
	~TrainingLoader();

	int getMaxSentences() { return _n_sentences; }
	SentenceTheory *getNextSentenceTheory();
	const class EntitySet *getCurrentEntitySet() { return _copyOfCurrentEntitySet; }

	static int countFilesInFileList(const char *filename);
	static int countSentencesInFileList(const char *filename);
	static int countDocumentsInFileList(const char *filename);
	static int countSentencesInFile(const wchar_t *filename);
	static int countSentencesInFile(const char *filename);
	static int countDocumentsInFile(const wchar_t *filename); 
	static int countDocumentsInFile(const char *filename); 

	//new (testing)
	const DocTheory* getCurrentDocTheory() const { return _currentDocTheory; }
	int getCurrentSentenceIndex() const { return _current_sent_index; }
	//new (testing)

private:
	void initStateLoader(const char *filename);
	void initFromList(const char *file);
	void initFromSingleFile(const wchar_t *file);
	wchar_t _stage_string[100];
	enum { LIST_MODE, SINGLE_FILE_MODE };
	bool _mode;

	int _n_sentences;
	int _precounted_sentences;
	boost::scoped_ptr<UTF8InputStream> _fileListStream;
	StateLoader *_currentStateLoader;
	DocTheory *_currentDocTheory;
	class EntitySet *_copyOfCurrentEntitySet;
	int _current_doc_index;
	int _n_docs_in_file;
	int _current_sent_index;
	wchar_t state_tree_name[100];

	SentenceTheory *getNextFromDocTheory();
	SentenceTheory *getNextFromStateLoader();

};

#endif
