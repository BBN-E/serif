// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFTrainer.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/names/IdFSentence.h"
#include "Generic/names/IdFModel.h"
#include "Generic/names/IdFListSet.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include <boost/scoped_ptr.hpp>

/** initializes IdFTrainer with nameClassTags object and optional list_file_name */
IdFTrainer::IdFTrainer(const NameClassTags *nameClassTags, 
					   const char *list_file_name,
					   int word_features_mode)
{
	IdFListSet *listSet = list_file_name == 0 ? 0 : _new IdFListSet(list_file_name);
	new(this) IdFTrainer(nameClassTags, IdFWordFeatures::build(word_features_mode, listSet));
}

/** initializes IdFTrainer with nameClassTags object, IdFWordFeatures object,
 *	and optional IdFListSet. IdFListSet inside wordFeatures should be the same
 *  as listSet, but that's not enforced.
 */
IdFTrainer::IdFTrainer(const NameClassTags *nameClassTags, 
					   const IdFWordFeatures *wordFeatures) :
	FIRSTWORD(Symbol(L"FW")), NOTFIRSTWORD(Symbol(L"NFW")), 
	_nameClassTags(nameClassTags)
{

	// storage for raw event counts
	_tagEventCounts = _new NgramScoreTable(4,10000);
	_wordEventCounts = _new NgramScoreTable(8,10000);

	// storage for vocab relevant to whatever half of data we're training on
	_temporaryVocabTable = _new NgramScoreTable(1,10000);

	// storage + unknown words
	// smaller, because there's no need to store firstword info anymore
	// These are the tables that will be passed to IdFModel to train on.
	_tagEventCountsPlusUnknown = _new NgramScoreTable(3,10000);
	_wordEventCountsPlusUnknown = _new NgramScoreTable(6,10000);
	
	_sentence = _new IdFSentence(_nameClassTags, wordFeatures->getListSet());		
	_model = _new IdFModel(_nameClassTags, wordFeatures);
}

/** Reset the event counts between jack knife portions,  
  * re-allocate, throwing out old ones.
  */
void IdFTrainer::resetCounts() {
	delete _tagEventCounts;
	delete _wordEventCounts;
	delete _temporaryVocabTable;

	_tagEventCounts = _new NgramScoreTable(4,10000);
	_wordEventCounts = _new NgramScoreTable(8,10000);
	_temporaryVocabTable = _new NgramScoreTable(1,10000);
}

/** connect vocabulary from IdFSentence for use in standalone IdF */
void IdFTrainer::collectVocabulary(IdFSentenceTokens * sentence) {
	for (int i = 0; i < sentence->getLength(); i++) {
		Symbol s = sentence->getWord(i);
			_model->addToVocab(sentence->getWord(i));
	}
}

/** connect temporary vocabulary from IdFSentence for use in standalone IdF */
void IdFTrainer::collectTemporaryVocab(IdFSentenceTokens * sentence) {
	for (int i = 0; i < sentence->getLength(); i++) {
                Symbol word(sentence->getWord(i));
		_temporaryVocabTable->add(&word);
	}
}
/** collect training event counts from files in training_file_list */ 
void IdFTrainer::collectCountsFromFiles(const char *training_file_list) {
	boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& listStream(*listStream_scoped_ptr);
	listStream.open(training_file_list);

	int num_files;
	listStream >> num_files;
	if (num_files <= 1) 
		throw UnexpectedInputException("IdFTrainer::collectCountsFromFiles", 
		"number of training files must be > 1");

	Symbol *files = _new Symbol[num_files];
	
	UTF8Token token;
	int i = 0;
	for (i = 0; i < num_files; i++) {
		if (listStream.eof())
			throw UnexpectedInputException("IdFTrainer::collectCountsFromFiles", 
				"fewer training files than specified in file");
		listStream >> token;
		if (listStream.eof())
			throw UnexpectedInputException("IdFTrainer::collectCountsFromFiles", 
				"fewer training files than specified in file");
		files[i] = token.symValue();
	}

	listStream.close();

	// TRAINING METHOD:
	// First, gather all real vocabulary (for use in word features). Then,
	// gather vocab from first half (store in _temporaryVocabTable); train on 
	// second half, considering a word unknown if it has not been seen in 
	// the first half. Reverse. 

	std::cerr << "gathering real vocabulary" << "\n";
	for (i = 0; i < num_files; i++) {
		std::cerr << "gathering vocab from " << files[i].to_debug_string() << "\n";
		boost::scoped_ptr<UTF8InputStream> vocabStream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& vocabStream(*vocabStream_scoped_ptr);
		vocabStream.open(files[i].to_string());
		while (_sentence->readTrainingSentence(vocabStream)) {
			for (int i = 0; i < _sentence->getLength(); i++) {
				_model->addToVocab(_sentence->getWord(i));
			}
		}
		vocabStream.close();
	}

	std::cerr << "\ntraining on first half with second half vocab" << "\n";
	for (i = 0; i < num_files; i++) {
		if (i % 2 == 1) {
			std::cerr << "gathering vocab from " << files[i].to_debug_string() << "\n";
			boost::scoped_ptr<UTF8InputStream> vocabStream_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& vocabStream(*vocabStream_scoped_ptr);
			vocabStream.open(files[i].to_string());
			collectTemporaryVocab(vocabStream);
			vocabStream.close();
		}
	}

	for (i = 0; i < num_files; i++) {
		if (i % 2 == 0) {
			std::cerr << "training off of " << files[i].to_debug_string() << "\n";
			boost::scoped_ptr<UTF8InputStream> trainingStream_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& trainingStream(*trainingStream_scoped_ptr);
			trainingStream.open(files[i].to_string());
			collectCounts(trainingStream);
			trainingStream.close();
		}
	}
	addUnknownWords();

	// re-allocate, throwing out old ones (memory leak, but I don't care much in training)
	_tagEventCounts = _new NgramScoreTable(4,10000);
	_wordEventCounts = _new NgramScoreTable(8,10000);
	_temporaryVocabTable = _new NgramScoreTable(1,10000);

	std::cerr << "training on second half with first half vocab" << "\n";
	for (i = 0; i < num_files; i++) {
		if (i % 2 == 0) {
			std::cerr << "gathering vocab from " << files[i].to_debug_string() << "\n";
			boost::scoped_ptr<UTF8InputStream> vocabStream_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& vocabStream(*vocabStream_scoped_ptr);
			vocabStream.open(files[i].to_string());
			collectTemporaryVocab(vocabStream);
			vocabStream.close();
		}
	}
	for (i = 0; i < num_files; i++) {
		if (i % 2 == 1) {
			std::cerr << "training off of " << files[i].to_debug_string() << "\n";
			boost::scoped_ptr<UTF8InputStream> trainingStream_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& trainingStream(*trainingStream_scoped_ptr);
			trainingStream.open(files[i].to_string());
			collectCounts(trainingStream);
			trainingStream.close();
		}
	}
	addUnknownWords();
	
}

/**
 * puts the vocabulary from this training stream into _temporaryVocabTable
 */	
void IdFTrainer::collectTemporaryVocab(UTF8InputStream &stream) {
	while (_sentence->readTrainingSentence(stream)) {
		for (int i = 0; i < _sentence->getLength(); i++) {
                        Symbol word(_sentence->getWord(i));
			_temporaryVocabTable->add(&word);
		}
	}
}

/**
 * puts the events from this training stream into _wordEventCounts and _tagEventCounts
 */	
void IdFTrainer::collectCounts(UTF8InputStream &stream) {
	//UTF8OutputStream out;
	//out.open("training.sents");
	while (_sentence->readTrainingSentence(stream)) {
		//out << _sentence->to_string() << L"\n";
		collectSentenceCounts(_sentence);
	}
	//out.close();
}

/**
 * puts the events from this training sentence into _wordEventCounts and _tagEventCounts
 */	
void IdFTrainer::collectSentenceCounts(IdFSentence* sentence) {

	Symbol ngram[8];

	if (sentence->getLength() == 0)
		return;

	for (int i = 0; i < sentence->getLength(); i++) {
		ngram[0] = sentence->getWord(i);
		if (i == 0)
			ngram[1] = FIRSTWORD;
		else ngram[1] = NOTFIRSTWORD;

		ngram[2] = sentence->getFullTag(i);
		ngram[3] = sentence->getReducedTag(i);
		ngram[4] = sentence->getTagStatus(i);
		if (i == 0) {
			ngram[5] = _nameClassTags->getSentenceStartTag();
			ngram[6] = _nameClassTags->getSentenceStartTag();
		} else {
			ngram[5] = sentence->getFullTag(i - 1);
			ngram[6] = sentence->getWord(i - 1);
		}
		if (i <= 1)
			ngram[7] = FIRSTWORD;
		else ngram[7] = NOTFIRSTWORD;

		// ngram = word, firstword?, tag, reduced-tag, tag-status,
		//         word-minus-1 word-minus-1-firstword?
		_wordEventCounts->add(ngram);

		ngram[0] = sentence->getFullTag(i);
		if (i == 0) {
			ngram[1] = _nameClassTags->getSentenceStartTag();
			ngram[2] = _nameClassTags->getSentenceStartTag();
		} else {
			ngram[1] = sentence->getFullTag(i - 1);
			ngram[2] = sentence->getWord(i - 1);
		}
		if (i <= 1)
			ngram[3] = FIRSTWORD;
		else ngram[3] = NOTFIRSTWORD;
		
		// ngram = tag, tag-minus-1, word-minus-1		
		_tagEventCounts->add(ngram);
	}

	// END (just tag events)
	ngram[0] = _nameClassTags->getSentenceEndTag();
	ngram[1] = sentence->getFullTag(sentence->getLength() - 1);
	ngram[2] = sentence->getWord(sentence->getLength() - 1);
	if (sentence->getLength() - 1 > 0)
		ngram[3] = NOTFIRSTWORD;
	else ngram[3] = FIRSTWORD;

	_tagEventCounts->add(ngram);

}

/**
 * transfers events from _wordEventCounts and _tagEventCounts into 
 * _wordEventCountsPlusUnknown and _tagEventCountsPlusUnknown
 *
 * Every event is added to the relevant table once as is. Then, any words 
 * qualifying as unknown are replaced by their feature-symbol, and
 * the events that contain them are added again, in their modified form, 
 * to the relevant table.
 */	
void IdFTrainer::addUnknownWords() {

	float threshold = 5;
	NgramScoreTable::Table::iterator iter;
	for (iter = _tagEventCounts->get_start(); iter != _tagEventCounts->get_end(); ++iter) {
		Symbol ngram[4];
		ngram[0] = (*iter).first[0];
		ngram[1] = (*iter).first[1];
		ngram[2] = (*iter).first[2];
		ngram[3] = (*iter).first[3];
		_tagEventCountsPlusUnknown->add(ngram, (*iter).second);
		if (_temporaryVocabTable->lookup(&ngram[2]) == 0) {
			ngram[2] = _model->getWordFeatures(ngram[2], 
				ngram[3] == FIRSTWORD, 
				_model->normalizedWordIsInVocab(ngram[2]));
			_tagEventCountsPlusUnknown->add(ngram, (*iter).second);
		}
	}

	for (iter = _wordEventCounts->get_start(); iter != _wordEventCounts->get_end(); ++iter) {
		Symbol ngram[6];
		ngram[0] = (*iter).first[0];
		ngram[1] = (*iter).first[2];
		ngram[2] = (*iter).first[3];
		ngram[3] = (*iter).first[4];
		ngram[4] = (*iter).first[5];
		ngram[5] = (*iter).first[6];
		_wordEventCountsPlusUnknown->add(ngram, (*iter).second);
		
		bool word_not_found = false;
		if (_temporaryVocabTable->lookup(&ngram[0]) == 0) {
			ngram[0] = _model->getWordFeatures(ngram[0],
				(*iter).first[1] == FIRSTWORD, 
				_model->normalizedWordIsInVocab(ngram[0]));
			word_not_found = true;
		}
		if (_temporaryVocabTable->lookup(&ngram[5]) == 0) {
			ngram[5] = _model->getWordFeatures(ngram[5],
				(*iter).first[7] == FIRSTWORD, 				
				_model->normalizedWordIsInVocab(ngram[5]));
			word_not_found = true;
		}
		if (word_not_found)
			_wordEventCountsPlusUnknown->add(ngram, (*iter).second);
	}	

}

/** 
 *  Once _tagEventCountsPlusUnknown and _wordEventCountsPlusUnknown are filled in,
 *  calls upon the model to derive the probability/lambda tables.
 */
void IdFTrainer::deriveTables() {
	_model->calculateOneOverVocabSize();
	_model->deriveTagTables(_tagEventCountsPlusUnknown);
	_model->deriveWordTables(_wordEventCountsPlusUnknown);

}

/** Prints tables (passes through call to IdFModel). */
void IdFTrainer::printTables(const char *model_file_prefix) {
	_model->printTables(model_file_prefix);
}

/** estimates smoothing parameters (passes through call to IdFModel) */
void IdFTrainer::estimateLambdas(const char *smoothing_file) { 
	_model->estimateLambdas(smoothing_file); 
}
