// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/StatEventTrainer.h"
#include "Generic/events/stat/EventTriggerFinder.h"
#include "Generic/events/stat/EventArgumentFinder.h"
#include "Generic/events/stat/EventModalityClassifier.h"
#include "Generic/docRelationsEvents/DocEventHandler.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include <boost/scoped_ptr.hpp>

Symbol StatEventTrainer::SENT_TAG_SYM = Symbol(L"S");
Symbol StatEventTrainer::EVENT_TAG_SYM = Symbol(L"EVENT");

StatEventTrainer::StatEventTrainer(int mode_) : MODE(mode_),
	_triggerFinder(0), _argumentFinder(0), _modalityClassifier(0)
{
	TRAIN_TRIGGERS = ParamReader::getRequiredTrueFalseParam("train_event_trigger_model");
	TRAIN_ARGS = ParamReader::getRequiredTrueFalseParam("train_event_aa_model");
	TRAIN_MODALITY = ParamReader::getRequiredTrueFalseParam("train_event_modality_model");
	DEVTEST_MODALITY= false;
	bool train_maxent = false;
	bool train_p1 = false;
	if (TRAIN_ARGS) {
		train_maxent = ParamReader::getRequiredTrueFalseParam("train_event_aa_maxent_model");
		train_p1 = ParamReader::getRequiredTrueFalseParam("train_event_aa_p1_model");
		if (!train_maxent && !train_p1)
			TRAIN_ARGS = false;
	}

	if (MODE == TRAIN) {
		if (TRAIN_TRIGGERS) 
			_triggerFinder = _new EventTriggerFinder(EventTriggerFinder::TRAIN);
		if (TRAIN_ARGS) {
			if (train_maxent && train_p1)
				_argumentFinder = _new EventArgumentFinder(EventArgumentFinder::TRAIN_BOTH);
			else if (train_maxent)
				_argumentFinder = _new EventArgumentFinder(EventArgumentFinder::TRAIN_MAXENT);
			else if (train_p1)
				_argumentFinder = _new EventArgumentFinder(EventArgumentFinder::TRAIN_P1);
		}
		if (TRAIN_MODALITY){
			TWO_LAYER_MODALITY = ParamReader::getOptionalTrueFalseParamWithDefaultVal("train_two_layer_modality_model", false);
			if (TWO_LAYER_MODALITY){	
				_modalityClassifier = _new EventModalityClassifier(EventModalityClassifier::TRAIN_TWO_LAYER);
			}else{
				_modalityClassifier = _new EventModalityClassifier(EventModalityClassifier::TRAIN);
			}
		}

	} else if (MODE == ROUNDROBIN) {
		if (TRAIN_TRIGGERS) 
			_triggerFinder = _new EventTriggerFinder(EventTriggerFinder::DECODE);
		if (TRAIN_ARGS)
			_argumentFinder = _new EventArgumentFinder(EventArgumentFinder::DECODE);
	} else if (MODE == SELECT_ANNOTATION) {
		if (TRAIN_TRIGGERS)
			_triggerFinder = _new EventTriggerFinder(EventTriggerFinder::DECODE);
	} else if (MODE == DEVTEST){
		if (ParamReader::isParamTrue("devtest_event_modality_model")) {
			DEVTEST_MODALITY = true;
			TWO_LAYER_MODALITY = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_two_layer_modality_model", false);
			if (TWO_LAYER_MODALITY){	
				_modalityClassifier = _new EventModalityClassifier(EventModalityClassifier::DEVTEST_TWO_LAYER);
			}else{
				_modalityClassifier = _new EventModalityClassifier(EventModalityClassifier::DEVTEST);
			}
		} 
	} else if (MODE == AADEVTEST) {
		_triggerFinder = _new EventTriggerFinder(EventTriggerFinder::DECODE);
		_argumentFinder = _new EventArgumentFinder(EventArgumentFinder::DECODE);
	}

	_morphAnalysis = MorphologicalAnalyzer::build();

}

StatEventTrainer::~StatEventTrainer() {
	delete _morphAnalysis;
	delete _argumentFinder;
	delete _triggerFinder;
	delete _modalityClassifier;
}

// add for devTest 2008-01-23
void StatEventTrainer::devTest(){
	std::string file_list = ParamReader::getRequiredParam("event_training_file_list");

	TrainingLoader *trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
#if defined(_WIN32)
	HeapChecker::checkHeap("TL created");
#endif
	int max_sentences = trainingLoader->getMaxSentences();

	bool add_selected_annotation = false;
	TrainingLoader *selectedTrainingLoader = 0;
	if (ParamReader::isParamTrue("event_training_add_selected_annotation")) {
		add_selected_annotation = true;
		file_list = ParamReader::getRequiredParam("event_training_selected_file_list");
		selectedTrainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
		max_sentences += selectedTrainingLoader->getMaxSentences();
	}

	_docIds = _new Symbol[max_sentences];
	_documentTopics = _new Symbol[max_sentences];
	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_eventMentionSets = _new EventMentionSet * [max_sentences];

	int num_sentences = loadTrainingData(trainingLoader);
	
	if (add_selected_annotation) {
		int total_sentences = num_sentences;
		file_list = ParamReader::getRequiredParam("event_training_selected_annotation");
		total_sentences += loadTrainingData(selectedTrainingLoader, num_sentences);
		num_sentences += loadAllSelectedAnnotation(file_list.c_str(), num_sentences, total_sentences);
	}

	delete trainingLoader;
	delete selectedTrainingLoader;

	SessionLogger::info("SERIF") << "done with loading\n";

	if (DEVTEST_MODALITY){
		_modalityClassifier->devTest(_tokenSequences, _parses, _valueMentionSets, _mentionSets, _propSets, _eventMentionSets, num_sentences);
	}

	for (int i = 0; i < num_sentences; i++) {
		delete _tokenSequences[i];
		delete _parses[i];
		delete _secondaryParses[i];
		delete _npChunks[i];
		delete _mentionSets[i];
		delete _valueMentionSets[i];
		delete _propSets[i];
		delete _eventMentionSets[i];
	}
	delete[] _docIds;
	delete[] _documentTopics;
	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _eventMentionSets;

	SessionLogger::info("SERIF") << "done with devtest\n";
}

void StatEventTrainer::train() {

	//if (!TRAIN_TRIGGERS && !TRAIN_ARGS)
	if (!TRAIN_TRIGGERS && !TRAIN_ARGS && !TRAIN_MODALITY)
		return;
	
	std::string file_list = ParamReader::getRequiredParam("event_training_file_list");

	TrainingLoader *trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
#if defined(_WIN32)
	HeapChecker::checkHeap("TL created");
#endif
	int max_sentences = trainingLoader->getMaxSentences();

	bool add_selected_annotation = false;
	TrainingLoader *selectedTrainingLoader = 0;
	if (ParamReader::isParamTrue("event_training_add_selected_annotation")) {
		add_selected_annotation = true;
		file_list = ParamReader::getRequiredParam("event_training_selected_file_list");
		selectedTrainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
		max_sentences += selectedTrainingLoader->getMaxSentences();
	}

	_docIds = _new Symbol[max_sentences];
	_documentTopics = _new Symbol[max_sentences];
	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_eventMentionSets = _new EventMentionSet * [max_sentences];

	int num_sentences = loadTrainingData(trainingLoader);
	
	if (add_selected_annotation) {
		int total_sentences = num_sentences;
		file_list = ParamReader::getRequiredParam("event_training_selected_annotation");
		total_sentences += loadTrainingData(selectedTrainingLoader, num_sentences);
		num_sentences += loadAllSelectedAnnotation(file_list.c_str(), num_sentences, total_sentences);
	}

	delete trainingLoader;
	delete selectedTrainingLoader;

	SessionLogger::info("SERIF") << "done with loading\n";
	
	if (TRAIN_TRIGGERS) 
		_triggerFinder->train(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
			_propSets, _eventMentionSets, _documentTopics, num_sentences);

	if (TRAIN_ARGS) 
		_argumentFinder->train(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
			_propSets, _eventMentionSets, num_sentences);
	
	if (TRAIN_MODALITY){
		if (TWO_LAYER_MODALITY){
			_modalityClassifier->train_2layer(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
			_propSets, _eventMentionSets, num_sentences);
		}else{
			_modalityClassifier->train(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
			_propSets, _eventMentionSets, num_sentences);
		}
	}

	for (int i = 0; i < num_sentences; i++) {
		delete _tokenSequences[i];
		delete _parses[i];
		delete _secondaryParses[i];
		delete _npChunks[i];
		delete _mentionSets[i];
		delete _valueMentionSets[i];
		delete _propSets[i];
		delete _eventMentionSets[i];
	}
	delete[] _docIds;
	delete[] _documentTopics;
	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _eventMentionSets;

	SessionLogger::info("SERIF") << "done with training\n";
}

void StatEventTrainer::roundRobin() {

	if (!TRAIN_TRIGGERS && !TRAIN_ARGS)
		return;

	std::string setup_file = ParamReader::getRequiredParam("event_round_robin_setup");
	std::string output_file = ParamReader::getRequiredParam("event_round_robin_results");

	UTF8OutputStream triggerResultStream;
	UTF8OutputStream triggerHRresultStream;
	UTF8OutputStream triggerHTMLStream;
	UTF8OutputStream triggerScoreStream;
	UTF8OutputStream triggerKeyStream;
	UTF8OutputStream argsResultStream;

	if (TRAIN_TRIGGERS) {
		std::string str(output_file);
		str += ".triggers.decoded";
		triggerResultStream.open(str.c_str());
		std::string str2(output_file);
		str2 += ".triggers.HR.decoded";
		triggerHRresultStream.open(str2.c_str());
		std::string str3(output_file);
		str3 += ".triggers.scores";
		triggerScoreStream.open(str3.c_str());
		std::string str4(output_file);
		str4 += ".triggers.key";
		triggerKeyStream.open(str4.c_str());
		std::string str5(output_file);
		str5 += ".triggers.html";
		triggerHTMLStream.open(str5.c_str());
		triggerResultStream << L"<DOC>\n<DOCID>0</DOCID>\n";
		triggerHRresultStream << L"<DOC>\n<DOCID>0</DOCID>\n";
		triggerKeyStream << L"<DOC>\n<DOCID>0</DOCID>\n";
	}
	if (TRAIN_ARGS) {
		std::string str(output_file);
		str += ".args.html";
		argsResultStream.open(str.c_str());		
	}

	UTF8Token triggerModelToken;
	UTF8Token argsModelToken;
	UTF8Token messageToken;
	int max_sentences = 0;

	boost::scoped_ptr<UTF8InputStream> countStream_scoped_ptr(UTF8InputStream::build(setup_file.c_str()));
	UTF8InputStream& countStream(*countStream_scoped_ptr);
	while (!countStream.eof()) {
		countStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		countStream >> triggerModelToken;
		if (wcscmp(triggerModelToken.chars(), L"") == 0)
			break;
		countStream >> argsModelToken;
		if (wcscmp(argsModelToken.chars(), L"") == 0)
			break;
		TrainingLoader *trainingLoader = _new TrainingLoader(messageToken.chars(),L"doc-relations-events", true);
		int batch_max_sentences = trainingLoader->getMaxSentences();
		delete trainingLoader;
		max_sentences = (batch_max_sentences > max_sentences) ? batch_max_sentences : max_sentences;	
	}
	countStream.close();

	_docIds = _new Symbol[max_sentences];
	_documentTopics = _new Symbol[max_sentences];
	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_eventMentionSets = _new EventMentionSet * [max_sentences];

	boost::scoped_ptr<UTF8InputStream> setupFileStream_scoped_ptr(UTF8InputStream::build(setup_file.c_str()));
	UTF8InputStream& setupFileStream(*setupFileStream_scoped_ptr);

	if (TRAIN_TRIGGERS) _triggerFinder->resetRoundRobinStatistics();
	if (TRAIN_ARGS)	_argumentFinder->resetRoundRobinStatistics();

	while (!setupFileStream.eof()) {
		setupFileStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		setupFileStream >> triggerModelToken;
		if (wcscmp(triggerModelToken.chars(), L"") == 0)
			break;
		setupFileStream >> argsModelToken;
		if (wcscmp(argsModelToken.chars(), L"") == 0)
			break;

		TrainingLoader *trainingLoader = _new TrainingLoader(messageToken.chars(),L"doc-relations-events", true);
		int num_sentences = loadTrainingData(trainingLoader);
		delete trainingLoader;

		char char_str[501];
		if (TRAIN_TRIGGERS) {
			StringTransliterator::transliterateToEnglish(char_str, 
				triggerModelToken.chars(), 500);
			_triggerFinder->replaceModel(char_str);
			_triggerFinder->roundRobinDecode(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
				_propSets, _eventMentionSets, num_sentences,
				triggerResultStream, triggerHRresultStream, triggerKeyStream, triggerHTMLStream);
		}
		if (TRAIN_ARGS) {
			StringTransliterator::transliterateToEnglish(char_str, 
				argsModelToken.chars(), 500);
			_argumentFinder->replaceModel(char_str);
			_argumentFinder->decode(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
				_propSets, _eventMentionSets, num_sentences, argsResultStream);
		}

		for (int i = 0; i < num_sentences; i++) {
			delete _tokenSequences[i];
			delete _parses[i];
			delete _secondaryParses[i];
			delete _npChunks[i];
			delete _valueMentionSets[i];
			delete _mentionSets[i];
			delete _propSets[i];
			delete _eventMentionSets[i];
		}
	}

	if (TRAIN_TRIGGERS) {
		_triggerFinder->printRoundRobinStatistics(triggerScoreStream);
		triggerResultStream << L"</DOC>";
		triggerHRresultStream << L"</DOC>";
		triggerKeyStream << L"</DOC>";
		triggerResultStream.close();
		triggerHRresultStream.close();
		triggerScoreStream.close();
		triggerKeyStream.close();
	}
	if (TRAIN_ARGS) {
		_argumentFinder->printRoundRobinStatistics(argsResultStream);
		argsResultStream.close();
	}
	delete[] _docIds;
	delete[] _documentTopics;
	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _eventMentionSets;

}

void StatEventTrainer::selectAnnotation() { 

	if (!TRAIN_TRIGGERS)
		return;

	std::string input_file = ParamReader::getRequiredParam("event_select_annotation_input_list");

	std::string output_file = ParamReader::getRequiredParam("event_select_annotation_results");
	UTF8OutputStream outputStream(output_file.c_str());

	UTF8Token messageToken;
	int max_sentences = 0;

	boost::scoped_ptr<UTF8InputStream> countStream_scoped_ptr(UTF8InputStream::build(input_file.c_str()));
	UTF8InputStream& countStream(*countStream_scoped_ptr);
	while (!countStream.eof()) {
		countStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		TrainingLoader *trainingLoader = _new TrainingLoader(messageToken.chars(),L"doc-relations-events", true);
		int batch_max_sentences = trainingLoader->getMaxSentences();
		delete trainingLoader;
		max_sentences = (batch_max_sentences > max_sentences) ? batch_max_sentences : max_sentences;	
	}
	countStream.close();

	_docIds = _new Symbol[max_sentences];
	_documentTopics = _new Symbol[max_sentences];
	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_eventMentionSets = _new EventMentionSet * [max_sentences];

	boost::scoped_ptr<UTF8InputStream> inputFileStream_scoped_ptr(UTF8InputStream::build(input_file.c_str()));
	UTF8InputStream& inputFileStream(*inputFileStream_scoped_ptr);

	while (!inputFileStream.eof()) {
		inputFileStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		
		TrainingLoader *trainingLoader = _new TrainingLoader(messageToken.chars(),L"doc-relations-events", true);
		int num_sentences = loadTrainingData(trainingLoader);
		delete trainingLoader;
		
		_triggerFinder->selectAnnotation(_docIds, _documentTopics, _tokenSequences, _parses, _valueMentionSets, 
				_mentionSets, _propSets, _eventMentionSets, num_sentences, outputStream);

		for (int i = 0; i < num_sentences; i++) {
			delete _tokenSequences[i];
			delete _parses[i];
			delete _secondaryParses[i];
			delete _npChunks[i];
			delete _valueMentionSets[i];
			delete _mentionSets[i];
			delete _propSets[i];
			delete _eventMentionSets[i];
		}
	}
	inputFileStream.close();
	
	outputStream.close();
	delete[] _docIds;
	delete[] _documentTopics;
	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _eventMentionSets;

}

int StatEventTrainer::loadTrainingData(TrainingLoader *trainingLoader, int start_index) {

	int max_sentences = trainingLoader->getMaxSentences();
	int i;
	for (i = 0; i < max_sentences; i++) {
		int j = i + start_index;
		SentenceTheory *theory = trainingLoader->getNextSentenceTheory();
		if (theory == 0)
			break;

		wchar_t id_str[100];
		_docIds[j] = theory->getDocID();
		swprintf(id_str, sizeof(id_str)/sizeof(id_str[0]), L"%ls-%d", _docIds[j].to_string(), theory->getTokenSequence()->getSentenceNumber());
		_sentIdMap[Symbol(id_str)] = j;

		_tokenSequences[j] = theory->getTokenSequence();
		_tokenSequences[j]->gainReference();
		_parses[j] = theory->getPrimaryParse();
		_parses[j]->gainReference();
		//have to add a reference to the np chunk theory, because it may be 
		//the source of the parse, if the reference isn't gained, the parse 
		//will be deleted when result is deleted
		_npChunks[j] = theory->getNPChunkTheory();
		if(_npChunks[j] != 0){
			_npChunks[j]->gainReference();
			// is this what we want?
			_secondaryParses[j] = theory->getFullParse();
			_secondaryParses[j]->gainReference();
		} else _secondaryParses[j] = 0;
		_mentionSets[j] = theory->getMentionSet();
		_mentionSets[j]->gainReference();
		_valueMentionSets[j] = theory->getValueMentionSet();
		_valueMentionSets[j]->gainReference();
		_propSets[j] = theory->getPropositionSet();
		_propSets[j]->gainReference();
		_propSets[j]->fillDefinitionsArray();
		_eventMentionSets[j] = theory->getEventMentionSet();
		_eventMentionSets[j]->gainReference();
	}

	Symbol currTopic;
	for (int k = 0; k < i; k++) {
		int j = k + start_index;
		if (_tokenSequences[j]->getSentenceNumber() == 0) 
			currTopic = DocEventHandler::assignTopic(_tokenSequences, _parses, j, i);
		_documentTopics[j] = currTopic;
	}

	return i;
}

int StatEventTrainer::loadAllSelectedAnnotation(const char *file, 
												int start_index, int end_index) 
{
	boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
	UTF8Token token;
	int i;

	// zero out the predicted EventMentionSets
	for (i = start_index; i < end_index; i++) {
		_eventMentionSets[i]->loseReference();
		_eventMentionSets[i] = 0;
	}

	while (!tempFileListStream.eof()) {
		tempFileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadSelectedAnnotationFile(token.chars());
	}
	tempFileListStream.close();	

	int j = start_index;
	for (i = start_index; i < end_index; i++) {
		wchar_t id_str[100];
		swprintf(id_str, sizeof(id_str)/sizeof(id_str[0]), L"%ls-%d", _docIds[i].to_string(), _tokenSequences[i]->getSentenceNumber());
		if (_eventMentionSets[i] != 0) {
			_docIds[j] = _docIds[i];
			_documentTopics[j] = _documentTopics[i];
			_sentIdMap[Symbol(id_str)] = j;
			_tokenSequences[j] = _tokenSequences[i];
			_parses[j] = _parses[i];
			_npChunks[j] = _npChunks[i];
			_secondaryParses[j] = _secondaryParses[i];
			_mentionSets[j] = _mentionSets[i];
			_valueMentionSets[j] = _valueMentionSets[i];
			_propSets[j] = _propSets[i];
			_eventMentionSets[j] = _eventMentionSets[i];
			j++;
		}
		else {		
			_sentIdMap[Symbol(id_str)] = -1;
			_tokenSequences[i]->loseReference();
			_parses[i]->loseReference();
			if (_npChunks[i] != 0) {
				_npChunks[i]->loseReference();
				_secondaryParses[i]->loseReference();
			}
			_mentionSets[i]->loseReference();
			_valueMentionSets[i]->loseReference();
			_propSets[i]->loseReference();
		}
	}

	return j - start_index;
}

int StatEventTrainer::loadSelectedAnnotationFile(const wchar_t *file) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& in(*in_scoped_ptr);
	LocatedString *source = _new LocatedString(in);

	SGMLTag openTag, closeTag;
	LocatedString *sent = 0;
	int index = 0;
	int start, end;
	
	do {
		openTag = SGMLTag::findOpenTag(*source, SENT_TAG_SYM, index);
		if (!openTag.notFound()) {
			start = openTag.getEnd();
			closeTag = SGMLTag::findCloseTag(*source, SENT_TAG_SYM, openTag.getEnd());
			if (closeTag.notFound()) {
				end = source->length() - 1;
				index = source->length();
			}
			else {
				end = closeTag.getStart() - 1;
				index = closeTag.getEnd();
			}

			// Make a substring from just the sentence text and markup
			sent = source->substring(start, end);

			// Extract the doc id and sentence number
			Symbol fullID = openTag.getAttributeValue(L"ID");
			const wchar_t *full_id_str = fullID.to_string();
			int sent_no = _wtoi(wcsrchr(full_id_str, L'-') + 1);
			SentIdMap::iterator it = _sentIdMap.find(fullID);
			if (it == _sentIdMap.end())
				continue;
			int sent_idx = (*it).second;

			_eventMentionSets[sent_idx] = processTriggerAnnotationSentence(sent, sent_no,
										_parses[sent_idx], _propSets[sent_idx]);
		}
	} while (!openTag.notFound());
	
	return 0;
}

EventMentionSet* StatEventTrainer::processTriggerAnnotationSentence(LocatedString *sent,
																	int sent_no,
																	Parse *parse, 
																	PropositionSet *propSet) 
{
	std::vector<EventMention*> ementions;
	SGMLTag openTag, closeTag;
	int index = 0;
	int start, end;
	int tok_num = 0;
	int start_tok, end_tok;
	
	// Skip over any leading whitespace
	while (iswspace(sent->charAt(index)))
		index++;

	do {
		openTag = SGMLTag::findOpenTag(*sent, EVENT_TAG_SYM, index);
		if (!openTag.notFound()) {

			// count tokens before tag
			int i = index;
			while (i < openTag.getStart()) {
				if (iswspace(sent->charAt(i)))
					tok_num++;
				i++;
			}
			start_tok = tok_num;

			// find close tag
			start = openTag.getEnd();
			closeTag = SGMLTag::findCloseTag(*sent, EVENT_TAG_SYM, start);
			if (closeTag.notFound()) {
				throw UnexpectedInputException(
					"StatEventTrainer::processTriggerAnnotationSentence()",
					"Unclosed tigger annotation tag found");
			}
			end = closeTag.getStart();
			
			// count tokens inside tag
			i = start;
			while (i < end) {
				if (iswspace(sent->charAt(i)))
					tok_num++;
				i++;
			}
			end_tok = tok_num;
			
			Symbol typeSym = openTag.getType();
			for (int tok = start_tok; tok <= end_tok; tok++) {
				EventMention *mention = _new EventMention(sent_no, static_cast<int>(ementions.size()));
				mention->setEventType(typeSym);
				
				const SynNode *tNode = parse->getRoot()->getNthTerminal(tok);
				// we want the preterminal node, since that's what we would get if we were
				// going from a proposition to a node
				if (tNode->getParent() != 0)
					tNode = tNode->getParent();
				mention->setAnchor(tNode, propSet);

				ementions.push_back(mention);
			}

			index = closeTag.getEnd();
		}

	} while (!openTag.notFound());

	EventMentionSet *result = _new EventMentionSet(parse);
	for (size_t i = 0; i < ementions.size(); i++) {
		result->takeEventMention(ementions[i]);
		ementions[i] = 0;
	}

	return result;
}

// test using gold standard entity and value mentions
void StatEventTrainer::eventAADevTestUsingGoldEVMentions() {

	// == load test data from SERIF state files ==
	std::string file_list = ParamReader::getRequiredParam("event_test_file_list");	// a file giving a list of SERIF state files

	TrainingLoader *trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
	int max_sentences = trainingLoader->getMaxSentences();

	_docIds = _new Symbol[max_sentences];
	_documentTopics = _new Symbol[max_sentences];
	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_eventMentionSets = _new EventMentionSet * [max_sentences];

	int num_sentences = loadTrainingData(trainingLoader);
	
	delete trainingLoader;
	// == done loading data ==


	std::string aaModelFile = ParamReader::getParam("event_aa_model_file");
	if(!aaModelFile.empty()) {
		std::string outputFilename = ParamReader::getParam("event_aa_devtest_resultfile");
		UTF8OutputStream outStream;
		outStream.open(outputFilename.c_str());

		_argumentFinder->replaceModel(&aaModelFile[0]);

		// == event-aa with gold triggers ==
		_argumentFinder->resetAllStatistics();
		for(int i=0; i<num_sentences; i++) {
			_argumentFinder->devTestDecode(_tokenSequences[i], _parses[i], _valueMentionSets[i], _mentionSets[i], _propSets[i], _eventMentionSets[i]);
		}
		outStream << "<gold_trigger>\n";
		_argumentFinder->printRecPrecF1(outStream);
		outStream << "</gold_trigger>\n";

		// == event-aa with predicted triggers ==
		std::string triggerModelFile = ParamReader::getParam("event_trigger_model_file");
		if(!triggerModelFile.empty()) {
			_argumentFinder->resetAllStatistics();
			_triggerFinder->resetRoundRobinStatistics();
			_triggerFinder->replaceModel(&triggerModelFile[0]);

			for(int i=0; i<num_sentences; i++) {
				if(_tokenSequences[i]->getSentenceNumber()==0) 
					_triggerFinder->setDocumentTopic( DocEventHandler::assignTopic(_tokenSequences, _parses, i, num_sentences) );

				// predict event mentions for the sentence
				EventMentionSet* ements = _triggerFinder->devTestDecode(_tokenSequences[i], _parses[i], _valueMentionSets[i], 
											_mentionSets[i], _propSets[i], _eventMentionSets[i], num_sentences);

				_argumentFinder->devTestDecode(_tokenSequences[i], _parses[i], _valueMentionSets[i], _mentionSets[i], _propSets[i], 
								_eventMentionSets[i], ements);

				delete ements;
			}

			_triggerFinder->printRecPrecF1(outStream);
			outStream << "<predicted_trigger>\n";
			_argumentFinder->printRecPrecF1(outStream);
			outStream << "</predicted_trigger>\n";
		}

		outStream.close();
	}

	// == cleanup ==
	for (int i = 0; i < num_sentences; i++) {
		delete _tokenSequences[i];
		delete _parses[i];
		delete _secondaryParses[i];
		delete _npChunks[i];
		delete _mentionSets[i];
		delete _valueMentionSets[i];
		delete _propSets[i];
		delete _eventMentionSets[i];
	}
	delete[] _docIds;
	delete[] _documentTopics;
	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _eventMentionSets;
}



