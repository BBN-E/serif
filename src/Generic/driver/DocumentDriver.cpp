// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/driver/DocumentDriver.h"

#include "dynamic_includes/common/SerifRestrictions.h"
#include "Generic/driver/SentenceDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/NullSessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/HeapStatus.h"
#include "Generic/common/IStringStream.h"

#include "Generic/commandLineInterface/CommandLineInterface.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/ParamReader.h"
#include "Generic/driver/Stage.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/actors/ActorMentionFinder.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/icews/EventMentionFinder.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/ICEWSDriver.h"
#include "Generic/icews/ICEWSDocumentReader.h"
#include "Generic/icews/ICEWSOutputWriter.h"
#include "Generic/icews/ActorMentionPattern.h"
#include "Generic/icews/EventMentionPattern.h"
#include "Generic/icews/ICEWSQueueFeeder.h"
#include "Generic/driver/DiskQueueDriver.h"

#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/reader/DocumentSplitter.h"
#include "Generic/reader/DocumentZoner.h"
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/results/ResultCollector.h"
#include "Generic/results/MTResultCollector.h"
#include "Generic/results/MSANamesResultCollector.h"
#include "Generic/docentities/DocEntityLinker.h"
#include "Generic/generics/GenericsFilter.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/docRelationsEvents/DocRelationEventProcessor.h"
#include "Generic/values/DocValueProcessor.h"
#include "Generic/clutter/ClutterFilter.h"
#include "Generic/xdoc/XDocClient.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/PropTree/PropForestFactory.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/propositions/PropositionStatusClassifier.h"
#include "Generic/factfinder/FactFinder.h"
#include "Generic/actors/ActorMentionFinder.h"
#include "Generic/confidences/ConfidenceEstimator.h"
#include "Generic/causeEffect/CauseEffectRelationFinder.h"

#include "Generic/state/XMLSerializedDocTheory.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#if defined(_WIN32)
#include <direct.h>
#endif
#include <time.h>

#include "Generic/linuxPort/serif_port.h"

extern void sgml_encode(char* input, char* output);
extern size_t convertWideCharsToUTF8(const wchar_t* wideChars, char* utf8);
extern size_t convertUTF8ToWideChars(const char* utf8, wchar_t* wideChars);

using namespace std;


// session logging stuff
const wchar_t *CONTEXT_NAMES[] = {L"Session",
								  L"Document",
								  L"Sentence",
								  L"Stage"};


DocumentDriver::DocumentDriver(const SessionProgram *sessionProgram,
							   ResultCollector *resultCollector,
							   SentenceDriver *sentenceDriver)
	: _dump_theories(false), _ignore_errors(false), _use_serifxml_source_format(false),
	  _localSessionLogger(0), _sentenceDriver(sentenceDriver), _sessionProgram(0),
	  _documentReader(0), _resultCollector(0), _alternateResultCollectors(0),
	  _sentenceBreaker(0), _docEntityLinker(0), _genericsFilter(0), _clutterFilter(0),
	  _docRelationEventProcessor(0), _docValueProcessor(0), _confidenceEstimator(0),
	  _docActorProcessor(0), _factFinder(0), _xdocClient(0), _propStatusClassifier(0),
	  _causeEffectRelationFinder(0), _maxSymbolTableSize(0), _num_docs_processed(0), 
	  _num_docs_per_cleanup(0), _globalSessionLoggerIsLocalSessionLogger(false)
{

	totalBytesProcessed = 0;
	_dump_theories = ParamReader::getRequiredTrueFalseParam("dump_theories");

	PRINT_COREF_FOR_OUTSIDE_SYSTEM = ParamReader::isParamTrue("print_coref_for_outside_system");
	PRINT_SENTENCE_SELECTION_INFO_UNTOK = ParamReader::isParamTrue("print_sentence_selection_info_untokenized");
	PRINT_PARSES = ParamReader::isParamTrue("print_parses");
	PRINT_MT_SEGMENTATION = ParamReader::isParamTrue("print_sentence_breaks_for_mt");
	PRINT_TOKENIZATION = ParamReader::isParamTrue("print_tokenization");
	PRINT_SENTENCE_SELECTION_INFO = ParamReader::isParamTrue("print_sentence_selection_info");
	PRINT_NAME_ENAMEX_TOKENIZED = ParamReader::isParamTrue("print_name_enamex_tokenized");	
	PRINT_NAME_ENAMEX_ORIGINAL_TEXT = ParamReader::isParamTrue("print_name_enamex_original_text");	
	PRINT_NAME_VALUE_ENAMEX_ORIGINAL_TEXT = ParamReader::isParamTrue("print_name_value_enamex_original_text");	
	PRINT_HTML_DISPLAY = ParamReader::isParamTrue("print_html_display");
	PRINT_PROPOSITON_TREES= ParamReader::isParamTrue("print_proposition_trees");
	_num_docs_per_cleanup = ParamReader::getOptionalIntParamWithDefaultValue("num_docs_per_cleanup", 1000);
	_maxSymbolTableSize = ParamReader::getOptionalIntParamWithDefaultValue("max_symbol_table_size", 2000000);

	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("check_memory", false))
		HeapStatus::makeActive();
	else
		HeapStatus::makeInactive();

	_ignore_errors = ParamReader::isParamTrue("ignore_errors");

	Token::_saveLexicalTokensAsDefaultTokens = ParamReader::isParamTrue("save_lexical_tokens_as_default");

	if (ParamReader::isParamTrue("delay_state_file_check")) 
		_delay_state_file_check = true;
	else 
		_delay_state_file_check = false;

	int open_file_retries = ParamReader::getOptionalIntParamWithDefaultValue("open_file_retries", 0);
	if (open_file_retries > 0)
		UTF8InputStream::setOpenFileRetries(open_file_retries);

	Symbol::initializeSymbolsFromFile();

	_documentReader = createDocumentReader();

	_documentSplitter = DocumentSplitter::build();

	std::vector<std::string> zonerNames = ParamReader::getStringVectorParam("document_zoners");
	BOOST_FOREACH(std::string zonerName, zonerNames) {
		// Define the handlers for the document zoning stages
		std::stringstream stageName;
		stageName << "zoning-" << zonerName;
		setStageHandler(Stage(stageName.str().c_str()), DocumentZoner::getFactory(zonerName.c_str()));
	}

	
	if (_sentenceDriver == 0) {
		_sentenceDriver = _new SentenceDriver();
	}

	_max_document_processing_milliseconds = ParamReader::getOptionalIntParamWithDefaultValue("max_document_processing_seconds", 0)*1000.0;

	// Create timers
	for (Stage stage=Stage::getStartStage(); stage<Stage::getEndStage(); ++stage) {
		stageLoadTimer[stage] = GenericTimer();
		stageProcessTimer[stage] = GenericTimer();
	}
	documentProcessTimer = GenericTimer();

	// Always use the en_US.utf8 locale -- this defines the behavior
	// of functions like iswspace(), isupper() etc.
	setlocale(LC_ALL, "en_US.utf8");
	if (sessionProgram != 0) {
		bool use_lexicon_file = (!_use_serifxml_source_format);
		beginBatch(sessionProgram, resultCollector, use_lexicon_file);
	}
}

void DocumentDriver::beginBatch(const SessionProgram *sessionProgram, ResultCollector *resultCollector, bool use_lexicon_file) {
	if ((_sessionProgram != 0) || (_localSessionLogger != 0) || (_resultCollector != 0))
		throw InternalInconsistencyException("SentenceDriver::beginBatch",
			"Expected session program, session logger, and "
			"result collector to be NULL -- was endSession() not called?");

	// Record pointers to the session program & result collector.  We do *not* own
	// either of these.
	_sessionProgram = sessionProgram;
	_resultCollector = resultCollector;

	try {
		// Do some sanity checks on parameters.
		if (PRINT_SENTENCE_SELECTION_INFO && _sessionProgram->getEndStage() != Stage("names"))
			throw UnrecoverableException("DocumentDriver::DocumentDriver()", 
			"When running Serif to print sentence selection info, the end stage must be 'names'");
		if (PRINT_NAME_ENAMEX_TOKENIZED && _sessionProgram->getEndStage() != Stage("names"))
			throw UnrecoverableException("DocumentDriver::DocumentDriver()", 
			"When running Serif to print names, the end stage must be 'names'");
		if (PRINT_NAME_ENAMEX_ORIGINAL_TEXT && _sessionProgram->getEndStage() != Stage("names"))
			throw UnrecoverableException("DocumentDriver::DocumentDriver()", 
			"When running Serif to print names, the end stage must be 'names'");
		if (PRINT_NAME_VALUE_ENAMEX_ORIGINAL_TEXT && _sessionProgram->getEndStage() != Stage("values"))
			throw UnrecoverableException("DocumentDriver::DocumentDriver()", 
			"When running Serif to print names and values, the end stage must be 'values'");
		if (PRINT_TOKENIZATION && _sessionProgram->getEndStage() != Stage("tokens"))
			throw UnrecoverableException("DocumentDriver::DocumentDriver()", 
			"When running Serif to print tokenization, the end stage must be 'tokens'");

		// make sure parent-experiment dir, experiment dir, and subdirs exist
		if (_sessionProgram->hasExperimentDir()) {
			wstring experimentDir(_sessionProgram->getExperimentDir());
			size_t index = experimentDir.find_last_of(LSERIF_PATH_SEP);
			OutputUtil::makeDir(experimentDir.substr(0, index).c_str());
			OutputUtil::makeDir(experimentDir.c_str());
			if (_dump_theories)
				OutputUtil::makeDir(_sessionProgram->getDumpDir());
			if (_sessionProgram->getOutputDir() != 0)
				OutputUtil::makeDir(_sessionProgram->getOutputDir());
		}

		// create the session logger
		if(_sessionProgram->hasExperimentDir()) {
			_localSessionLogger = _new FileSessionLogger(_sessionProgram->getLogFile(),
													N_CONTEXTS, CONTEXT_NAMES);
			
			// If we don't have a global session logger yet, set it to be our local one.
			if (!SessionLogger::globalLoggerExists()) {
				_globalSessionLoggerIsLocalSessionLogger = true;
				SessionLogger::setGlobalLogger(_localSessionLogger);
			}
		} else {
			// otherwise, log messages will be discarded
			_localSessionLogger = _new NullSessionLogger();
		}

		// If we're expecting to load state from an initial state file, then check
		// to make sure it exists and is readable (before we spend lots of time 
		// loading models).
		StateLoader *stateLoader = 0;
		const wchar_t *initial_state_file = _sessionProgram->getInitialStateFile();
		if (initial_state_file != 0 && !_delay_state_file_check)
			stateLoader = _new StateLoader(_sessionProgram->getInitialStateFile());

		// Load all the models we'll need to process this session
		if (!(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_lazy_model_loading", false))) {
			Stage startStage = _sessionProgram->getStartStage();
			Stage endStage = _sessionProgram->getEndStage();
			for (Stage stage=startStage; stage<=endStage; ++stage) {
				if (_sessionProgram->includeStage(stage))
					loadModelsForStage(stage);
			}
		}

		// If we haven't created a state loader yet (because _delay_state_file_check
		// was true), then do it now.
		if (initial_state_file != 0 && _delay_state_file_check)
			stateLoader = _new StateLoader(_sessionProgram->getInitialStateFile());

		// Let the sentence driver know that we're beginning a new batch.  This gives
		// ownership of the state loader (if we created one) to the sentence driver.
		_sentenceDriver->beginBatch(_sessionProgram, _localSessionLogger, stateLoader, use_lexicon_file);
	}
	catch (UnexpectedInputException &e) {
		SessionLogger::err("dd_exc_0") << e;
		throw;
	}
}

bool DocumentDriver::inBatch() {
	return _sessionProgram != 0;
}

void DocumentDriver::endBatch() {
	if (_sessionProgram == 0)
		return;

	// Let the sentence driver know we're ending this batch.
	_sentenceDriver->endBatch();

	if (_localSessionLogger) {
		delete _localSessionLogger;
		_localSessionLogger = 0;
		if (_globalSessionLoggerIsLocalSessionLogger) {
			// Prev global logger may be null, but that's okay for us (as long as we set a
			// logger in beginBatch()). We just want to make sure that if there was a 
			// logger in effect before we called startBatch(), it gets restored.
			SessionLogger::restorePrevGlobalLogger(); 
		}
	}

	// Release our session pointers. 
	_sessionProgram = 0;
	_resultCollector = 0;
	_globalSessionLoggerIsLocalSessionLogger = false;
}



bool DocumentDriver::stageModelsAreLoaded(Stage stage) {
	if (_docTheoryStageHandlers.find(stage) != _docTheoryStageHandlers.end())
		return (_docTheoryStageHandlers[stage] != 0);
	return (((stage == Stage("sent-break")) && (_sentenceBreaker != 0)) ||
		((stage == Stage("doc-entities")) && (_docEntityLinker != 0)) ||
		((stage == Stage("doc-relations-events")) && (_docRelationEventProcessor != 0)) ||
		((stage == Stage("doc-values")) && (_docValueProcessor != 0)) ||
		((stage == Stage("confidences")) && (_confidenceEstimator != 0)) ||
		((stage == Stage("prop-status")) && (_propStatusClassifier != 0)) ||
		((stage == Stage("doc-actors")) && (_docActorProcessor != 0)) ||
		((stage == Stage("factfinder")) && (_factFinder != 0)) ||
		((stage == Stage("generics")) && (_genericsFilter != 0)) ||
		((stage == Stage("clutter")) && (_clutterFilter != 0)) ||
		((stage == Stage("xdoc")) && (_xdocClient != 0)) ||
		((stage == Stage("cause-effect")) && (_causeEffectRelationFinder != 0)) ||
		((stage == Stage("sent-level-end"))));
}

void DocumentDriver::loadModelsForStage(Stage stage) {
	if ((stage >= Stage("tokens")) && (stage < Stage("sent-level-end"))) {
		_sentenceDriver->loadModelsForStage(stage);
		return; // Use the sentence driver to load it.
	}
	if (stage == Stage("start"))
		return; // Nothing to load for this stage.
	if (stage == Stage("end"))
		return; // Nothing to load for this stage.
	if (stage == Stage("sent-level-end"))
		return; // Nothing to load for this stage.
	if (stage >= Stage("output"))
		return; // Nothing to load for this stage.
	if (stageModelsAreLoaded(stage))
		return; // We've already loaded this stage.
	try {
		cout << "Initializing Stage " << stage.getSequenceNumber() 
			<< " (" << stage.getName() << ")..." << std::endl;
		stageLoadTimer[stage].startTimer();

		if (stage == Stage("sent-break")) loadSentenceBreakerModels();
		else if (stage == Stage("prop-status")) loadPropStatusModels();
		else if (stage == Stage("doc-entities")) loadDocEntityModels();
		else if (stage == Stage("doc-relations-events")) loadDocRelationsEventsModels();
		else if (stage == Stage("doc-values")) loadDocValuesModels();
		else if (stage == Stage("confidences")) loadConfidenceModels();
		else if (stage == Stage("doc-actors")) loadDocActorModels();
		else if (stage == Stage("factfinder")) loadFactFinder();
		else if (stage == Stage("generics")) loadGenericsModels();
		else if (stage == Stage("clutter")) loadClutterModels();
		else if (stage == Stage("xdoc")) loadXDocModels();
		else if (stage == Stage("cause-effect")) loadCauseEffectModels();
		else if (_docTheoryStageHandlerFactories().find(stage) != _docTheoryStageHandlerFactories().end()) {
			_docTheoryStageHandlers[stage] = _docTheoryStageHandlerFactories()[stage]->build();
		} else {
			throw InternalInconsistencyException("DocumentDriver::loadModelsForStage",
					"Unexpected value for 'stage'");
		}

		stageLoadTimer[stage].stopTimer();
	}
	catch (UnrecoverableException &e) {
		std::stringstream prefix;
		prefix << "While initializing for stage " << stage.getName() << ": ";
		e.prependToMessage(prefix.str().c_str());
		throw;
	}
 }

DocumentDriver::~DocumentDriver() {
	if (_sessionProgram != 0)
		endBatch();
	delete _localSessionLogger;
	delete _sentenceDriver;
	delete _documentReader;
	delete _documentSplitter;
	delete _sentenceBreaker;
	delete _docEntityLinker;
	delete _genericsFilter;
	if (_clutterFilter)
		delete _clutterFilter;
	delete _docRelationEventProcessor;
	delete _docValueProcessor;
	delete _confidenceEstimator;
	delete _docActorProcessor;
	delete _factFinder;
	delete _propStatusClassifier;
	delete _xdocClient;
	delete _causeEffectRelationFinder;
	typedef std::pair<Stage, DocTheoryStageHandler*> DocTheoryStageHandlerPair;
	BOOST_FOREACH(DocTheoryStageHandlerPair pair, _docTheoryStageHandlers)
		delete pair.second;
	_docTheoryStageHandlers.clear();	
}

void DocumentDriver::giveDocumentReader(DocumentReader *documentReader) {
	delete _documentReader;
	_documentReader = documentReader;
}

// Temporary procedure during serifxml development -- this is basically a 
// drop-in replacement for the run() procedure, except that for testing
// purposes it serializes and deserializes after every stage.
void DocumentDriver::serifXMLTest() {
	if (_sessionProgram->getStartStage() < Stage("score")) {
		Stage startStage = _sessionProgram->getStartStage();
		Stage endStage = _sessionProgram->getEndStage();
		for (Stage stage = startStage; stage <= endStage; ++stage) {
			const_cast<SessionProgram*>(_sessionProgram)->setStageRange(stage, stage); // Do one stage at a time.

			const Batch *batch = _sessionProgram->getBatch();
			for (size_t doc_num = 0; doc_num < batch->getNDocuments(); ++doc_num) {	
				const wchar_t *document_path = batch->getDocumentPath(doc_num);
				boost::filesystem::wpath docPathBoost(document_path);
				std::wstring document_filename = UnicodeUtil::toUTF16StdString(BOOST_FILESYSTEM_PATH_GET_FILENAME(docPathBoost));
                if (_localSessionLogger) {
					_localSessionLogger->updateContext(DOCUMENT_CONTEXT, document_filename.c_str());
                }

				Stage loadStage = stage.getPrevStage();

				std::pair<const Document*, DocTheory*> docPair;
				if (stage == Stage::getStartStage()) {
					Document *doc = loadDocument(document_path);
					docPair = std::make_pair(doc, getInitialDocTheory(doc));
				} else {
					std::wstringstream fn;
					fn << _sessionProgram->getOutputDir() 
						<< L"/" << document_filename
						<< "-" << loadStage.getSequenceNumber() 
						<< "-" << loadStage.getName() << L".xml";
					std::wstring fname = fn.str();
					docPair = SerifXML::XMLSerializedDocTheory(fname.c_str()).generateDocTheory();
				}
				DocTheory* docTheory = docPair.second;
				const Document* document = docPair.first;

				runOnDocTheory(docTheory, document_filename.c_str());

				// Save to xml.
				std::wstringstream fn;
				fn << _sessionProgram->getOutputDir() 
					<< L"/" << document_filename 
					<< "-" << stage.getSequenceNumber() 
					<< "-" << stage.getName() << L".xml";
				std::wstring fname = fn.str();
				SerifXML::XMLSerializedDocTheory(docTheory).save(fname.c_str());

				delete docTheory;
				delete document;
			}
			
			Symbol::discardUnusedSymbols();
		}
	}
	// spawn an exec that scores
	if (_sessionProgram->includeStage(Stage("score"))) {
		invokeScoreScript();
	}
}

void DocumentDriver::run() {
	if (_sessionProgram == 0)
		throw InternalInconsistencyException("DocumentDriver::run",
			"Call DocumentDriver::beginBatch() before calling run()");

	HeapStatus heapStatus;
	heapStatus.takeReading("Before batch");

	// time monitoring
	time_t preBatchTime, postBatchTime;
	time(&preBatchTime);

	// Write a description of this session to the logger.
	logSessionStart();

	try {
		if (_sessionProgram->getStartStage() < Stage("score")) {
			#ifdef ENABLE_LEAK_DETECTION
				const int NUM_MEM_STATES = 3;
				_CrtMemState _mem_state[NUM_MEM_STATES]; // 0=current, 1=prev, 2=prev-prev, etc
				_CrtMemState _mem_baseline; // memory usage after first document.
				_CrtMemCheckpoint(&_mem_state[0]);
				int document_number = 0; // counter for debug messages.
			#endif

			const Batch *batch = _sessionProgram->getBatch();
			for (size_t doc_num = 0; doc_num < batch->getNDocuments(); ++doc_num) {
				#ifdef ENABLE_LEAK_DETECTION
					for (int i=NUM_MEM_STATES-1; i>0; --i)
						_mem_state[i] = _mem_state[i-1];
				#endif
				const wchar_t *document_path = 0;
					try {
					document_path = batch->getDocumentPath(doc_num);
					boost::filesystem::wpath docPathBoost(document_path);
					std::wstring document_filename = UnicodeUtil::toUTF16StdString(BOOST_FILESYSTEM_PATH_GET_FILENAME(docPathBoost));
                    if (_localSessionLogger) {
					   _localSessionLogger->updateContext(DOCUMENT_CONTEXT, document_filename.c_str());
                    }
					if (_use_serifxml_source_format) {
						std::pair<const Document*, DocTheory *> docPair;
						docPair = SerifXML::XMLSerializedDocTheory(document_path).generateDocTheory();
						try {
							if (_sessionProgram->saveSingleDocumentStateFiles() && _sessionProgram->hasExperimentDir()) {		
								_sentenceDriver->resetStateSavers(docPair.first->getName().to_string());  // Copied from getInitialDocTheory()
							}
							runOnDocTheory(docPair.second, document_filename.c_str());
						} catch (...) {
							delete docPair.second;
							throw;
						}
						delete docPair.first;
						delete docPair.second;
					} else {
						runOnDocument(loadDocument(document_path), document_filename.c_str());
					}
				}
				catch (UnrecoverableException &e) {
					
						stringstream whodunit;
						stringstream message;

						wstring document_path_as_wstring(document_path);
						whodunit << "\nFailed while in document: ";
						whodunit << std::string(document_path_as_wstring.begin(), document_path_as_wstring.end());
						whodunit << "\n";

						// Print message to console
						message << whodunit.str();
						message << "Error Source: " << e.getSource() << "\n";
						message << e.getMessage();
						cerr << message.str() << std::endl << std::endl;
						
						if (_localSessionLogger) {
						    _localSessionLogger->reportError() << e;
                        }

						e.markAsLogged();

						// This error has now been sufficiently logged, so we'd like to make sure it doesn't
						//  get further logged after it gets thrown. TO do this, we set its message

						if (_ignore_errors) {
							cout << "Skipping failed document because ignore_errors is set to 'true'\n\n";
						} else {
							throw;
						}	
					
				}
				#ifdef ENABLE_LEAK_DETECTION
					// Clear the wordnet caches.
					std::string result = ParamReader::getParam("word_net_dictionary_path");
					if (!result.empty())
						WordNet::getInstance()->cleanup();
					// Clear various other caches.
					if (_resultCollector)
						_resultCollector->finalize();
					if (_sentenceDriver)
						_sentenceDriver->cleanup();
					if (_docEntityLinker)
						_docEntityLinker->cleanup();
					if (_docRelationEventProcessor)
						_docRelationEventProcessor->cleanup();

					_CrtMemCheckpoint(&_mem_state[0]);
					_RPT0(_CRT_ERROR, "============================================\n"); 
					if (document_number == 0) {
						_mem_baseline = _mem_state[0];
						_RPT2(_CRT_ERROR, "Memory Usage (doc %3ld): %8.2fMB (=baseline)\n", 
							document_number, _mem_baseline.lSizes[_NORMAL_BLOCK]/1024.0/1024.0);
					} else {
						_CrtMemState _mem_diff;
						long _mem_baseline_bytes = _mem_baseline.lSizes[_NORMAL_BLOCK];
						long _mem_after_bytes = _mem_state[0].lSizes[_NORMAL_BLOCK];
						double diff_lTotalCount = 100.0*(_mem_after_bytes-_mem_baseline_bytes)/_mem_baseline_bytes;
						_RPT3(_CRT_ERROR, "Memory Usage (doc %3ld): %8.2fMB (%+6.4f%% change from baseline)\n", 
							document_number, _mem_after_bytes/1024.0/1024.0, diff_lTotalCount);
						if (document_number > 1) {
							if (_CrtMemDifference(&_mem_diff, &_mem_state[1], &_mem_state[0])) {
								_CrtMemDumpStatistics(&_mem_diff);
							}
						}
						/*
						if (_CrtMemDifference(&_mem_diff, &_mem_state[1], &_mem_state[0])) {
							_RPT0(_CRT_ERROR, "New objects created for this document:\n");
							_CrtMemDumpAllObjectsSince(&_mem_state[1]); // <-- Much more verbose...
						}
						*/
					}
					++document_number;
				#endif
			}
		}

		// spawn an exec that scores
		if (_sessionProgram->includeStage(Stage("score"))) {
			invokeScoreScript();
		}
	}
	catch (UnrecoverableException &e) {
		if (!e.isLogged())
			_localSessionLogger->reportError() << e;
		throw;
	}

	heapStatus.takeReading("After batch");
	heapStatus.displayReadings();
	heapStatus.flushReadings();

	time(&postBatchTime);
	double total_time = difftime(postBatchTime, preBatchTime);
	_localSessionLogger->reportInfoMessage() << "Session elapsed time: " << total_time << " seconds\n";

	cout << "\n";
	_localSessionLogger->displaySummary();
	cout << "\n";

}

void DocumentDriver::runOnString(const wchar_t *docString, std::wstring *results) {
	if (_sessionProgram == 0)
		throw InternalInconsistencyException("DocumentDriver::runOnString",
			"Call DocumentDriver::beginBatch() before calling runOnString()");
	if (_use_serifxml_source_format)
		throw InternalInconsistencyException("DocumentDriver::runOnString",
			"SerifXML doc_format not supported.");
	if (_documentReader == 0)
		throw InternalInconsistencyException("DocumentDriver::runOnString",
			"_documentReader not initialized");

	_localSessionLogger->updateLocalContext(SESSION_CONTEXT,
								  _sessionProgram->getSessionName());
	_localSessionLogger->updateLocalContext(DOCUMENT_CONTEXT, L"Document");

	Document *doc = _documentReader->readDocumentFromWString(docString, NULL);
	_sentenceDriver->makeNewStateSavers();

	runOnDocument(doc, 0, results);
}

void DocumentDriver::run(const wchar_t *document_path) {
	if (_sessionProgram == 0)
		throw InternalInconsistencyException("DocumentDriver::run",
			"Call DocumentDriver::beginBatch() before calling run()");

	HeapStatus status;
	status.takeReading("Before processing");

	// time monitoring
	time_t preBatchTime, postBatchTime;
	time(&preBatchTime);

	// Write a description of this session to the logger.
	logSessionStart();

	try {
		boost::filesystem::wpath docPathBoost(document_path);
		std::wstring document_filename = UnicodeUtil::toUTF16StdString(BOOST_FILESYSTEM_PATH_GET_FILENAME(docPathBoost));
		_localSessionLogger->updateLocalContext(DOCUMENT_CONTEXT, document_filename.c_str());

		runOnDocument(loadDocument(document_path), document_filename.c_str());

		// spawn an exec that scores
		if (_sessionProgram->includeStage(Stage("score"))) {
			invokeScoreScript();
		}
	}
	catch (UnrecoverableException &e) {
		if (!e.isLogged())
			_localSessionLogger->reportError() << e;
		throw;
	}

	status.takeReading("After processing");
	status.displayReadings();
	status.flushReadings();

	time(&postBatchTime);
	double total_time = difftime(postBatchTime, preBatchTime);
	_localSessionLogger->reportInfoMessage() << "Session elapsed time: " << total_time << " seconds\n";

	cout << "\n";
	_localSessionLogger->displaySummary();
	cout << "\n";
}

void DocumentDriver::runOnSingletonBatch(Document *document, wstring *results) {
	if (_sessionProgram == 0)
		throw InternalInconsistencyException("DocumentDriver::runOnSingletonBatch",
			"Call DocumentDriver::beginBatch() before calling runOnSingletonBatch()");

	HeapStatus status;
	status.takeReading("Before processing");

	// time monitoring
	time_t preBatchTime, postBatchTime;
	time(&preBatchTime);

	// First we do some basic session-management stuff:
	_localSessionLogger->updateContext(SESSION_CONTEXT,
								  _sessionProgram->getSessionName());

	_localSessionLogger->reportInfoMessage() << "Processing document: "
					<< document->getName().to_string() << "\n";

	//ParamReader::logParams(*_localSessionLogger);

	try {
		const char *document_name = document->getName().to_debug_string();
		_localSessionLogger->updateContext(DOCUMENT_CONTEXT, document_name);
		runOnDocument(document, 0, results);
	}
	catch (UnrecoverableException &e) {
		if (!e.isLogged())
			_localSessionLogger->reportError() << e;
		throw;
	}

	status.takeReading("After processing");
	status.displayReadings();
	status.flushReadings();

	time(&postBatchTime);
	double total_time = difftime(postBatchTime, preBatchTime);
	_localSessionLogger->reportInfoMessage() << "Document elapsed time: " << total_time << " seconds\n";

	cout << "\n";
	_localSessionLogger->displaySummary();
	cout << "\n";
}


void DocumentDriver::runOnDocument(Document *document,
								   const wchar_t *document_filename,
								   wstring *results)
{
	if (_sessionProgram == 0)
		throw InternalInconsistencyException("DocumentDriver::runOnDocument",
			"Call DocumentDriver::beginBatch() before calling runOnDocument()");

	HeapStatus heapStatus;
	heapStatus.takeReading("Before document");

	// Process the document.  Once we're done, we can delete the DocTheory -- we've
	// already saved the results (either to a state file if we're doing partial 
	// processing, or to the output file, or both).
	std::vector<Document*> splitDocuments = _documentSplitter->splitDocument(document);
	std::vector<DocTheory*> splitDocTheories;
	BOOST_FOREACH(Document* splitDocument, splitDocuments) {
		DocTheory* splitDocTheory = getInitialDocTheory(splitDocument);
		runOnDocTheory(splitDocTheory);
		splitDocTheories.push_back(splitDocTheory);
	}
	DocTheory* mergedDocTheory = _documentSplitter->mergeDocTheories(splitDocTheories);
	if (document_filename && _sessionProgram->hasExperimentDir()) {
		if (_sessionProgram->includeStage(Stage("output")))
			outputResults(document_filename, mergedDocTheory, _sessionProgram->getOutputDir());
		else
			outputSerifXMLResults(document_filename, mergedDocTheory, _sessionProgram->getOutputDir());
		outputMTResults(document_filename, mergedDocTheory, _sessionProgram->getOutputDir());
		outputMSAResults(document_filename, mergedDocTheory, _sessionProgram->getOutputDir());
	} else if (results) {
		if (_sessionProgram->includeStage(Stage("output")))
			outputResults(results, mergedDocTheory);
		else
			outputSerifXMLResults(results, mergedDocTheory);
		outputMTResults(results, mergedDocTheory);
	}
	delete mergedDocTheory->getDocument();
	delete mergedDocTheory;

	heapStatus.takeReading("After document");
	heapStatus.displayReadings();
	heapStatus.flushReadings();
}

DocTheory *DocumentDriver::getInitialDocTheory(Document *document) {
	Stage startStage = _sessionProgram->getStartStage();
	Stage endStage = _sessionProgram->getEndStage();
	DocTheory *docTheory = _new DocTheory(document);
	wstring document_name( document->getName().to_string() );

	if (startStage > Stage::getFirstStage() && !_sessionProgram->hasExperimentDir()) {
		std::ostringstream err;
		err << "Unable to load state files to start from stage " 
			<< startStage.getName() 
			<< " because no experiment dir was specified.";
		throw UnexpectedInputException("DocumentDriver::getInitialDocTheory", err.str().c_str());
	}

	Stage loadStage = startStage;
	--loadStage; // load output of previous stage

	// If we're saving state and we're using a separate state file for each document,
	// then create a new state loader and register it with the sentence driver.  The
	// sentence driver will be responsible for deleting it.  Additionally, set up the
	// appropriate state saver(s) for this document.
	if (_sessionProgram->saveSingleDocumentStateFiles()){
		wstring state_file = _sessionProgram->constructSingleDocumentStateFile( document_name, loadStage, true);
		StateLoader *stateLoader = (startStage <= Stage::getFirstStage()) ? 0 : _new StateLoader(state_file.c_str());
		_sentenceDriver->resetStateLoader(stateLoader);
		_sentenceDriver->resetStateSavers(document_name.c_str());
	}

	_sentenceDriver->printDocName(document_name.c_str());

	// If we we're starting after the sentence breaking stage, then load sentence break
	// information from the input state file.
	if (startStage > Stage("sent-break"))
		
		docTheory->loadSentenceBreaksFromStateFile(_sentenceDriver->getStateLoader());

	// If we're starting from a saved state file from a per-sentence state, then load
	// the state beam for each sentence.
	if ((startStage > Stage("tokens")) && (startStage <= Stage::getLastSentenceLevelStage())) {
		for (int sent_no=0; sent_no < docTheory->getNSentences(); ++sent_no) {
			_sentenceDriver->clearLine();
			cout << "Loading beam state for sentence "
				 << sent_no << " after stage "
				 << loadStage.getName() << "...";
			docTheory->setSentenceTheoryBeam(sent_no, _sentenceDriver->loadBeamState(loadStage, sent_no, docTheory));
		}
	}

	// If we're starting after the sentence level stages, then fill in the doc theory
	// by reading the input state file.
	if (startStage > Stage::getLastSentenceLevelStage())
		loadDocTheory(docTheory, loadStage);

	return docTheory;
}

void DocumentDriver::runOnDocTheory(DocTheory *docTheory,
									const wchar_t *document_filename,
									wstring *results)
{
	if (_sessionProgram == 0)
		throw InternalInconsistencyException("DocumentDriver::runOnDocTheory",
			"Call DocumentDriver::beginBatch() before calling runOnDocTheory()");

	// Perform cleanup periodically.
	++_num_docs_processed;
	if ((_num_docs_per_cleanup && ((_num_docs_processed % _num_docs_per_cleanup) == 0)) ||
		(_maxSymbolTableSize != 0 && Symbol::defaultSymbolTable()->size() > _maxSymbolTableSize)) {
		_localSessionLogger->updateContext(SESSION_CONTEXT, _sessionProgram->getSessionName());
		_localSessionLogger->dbg("cache-cleanup") << "Triggering cache cleanup after " << _num_docs_processed
			<< " documents processed (cleanup every " << _num_docs_per_cleanup << "); symbol table size "
			<< Symbol::defaultSymbolTable()->size() << " (max " << _maxSymbolTableSize << ")";
		std::string result = ParamReader::getParam("word_net_dictionary_path");
		if (!result.empty())
			WordNet::getInstance()->cleanup();
		Symbol::discardUnusedSymbols();
		if (_resultCollector)
			_resultCollector->finalize();
		if (_sentenceDriver)
			_sentenceDriver->cleanup();
		if (_docEntityLinker)
			_docEntityLinker->cleanup();
		if (_docRelationEventProcessor)
			_docRelationEventProcessor->cleanup();
	}

	Stage startStage = _sessionProgram->getStartStage();
	Stage endStage = _sessionProgram->getEndStage();

	// If we're using lazy model loading, then make sure the models are loaded now.
	for (Stage stage=startStage; stage<=endStage; ++stage) {
		if (_sessionProgram->includeStage(stage))
			loadModelsForStage(stage);
	}

	Stage currentStage = startStage;
	if (startStage == Stage("score"))
		return;

	// Clear the timer each document, since we're not collecting overall profiling for timeouts
	documentProcessTimer.resetTimer();

	const Document *document = docTheory->getDocument();
	wstring document_name( document->getName().to_string() );
	_localSessionLogger->updateContext(DOCUMENT_CONTEXT, document_name.c_str());

	// Record number of bytes processed.
	totalBytesProcessed += document->getOriginalText()->length();

	// If we're just converting state files do that now
	if (ParamReader::isParamTrue("only_convert_serifxml_to_state")) {
		saveSentenceBreakState(docTheory);
		saveDocTheoryState(docTheory, startStage);
		return;
	}

	while (currentStage < Stage("sent-break")) {
		if (!_sessionProgram->includeStage(currentStage))
			continue;
		_localSessionLogger->updateContext(STAGE_CONTEXT, currentStage.getName());
		stageProcessTimer[currentStage].startTimer();
		documentProcessTimer.startTimer();
		if (currentStage == Stage ("start")) {
			// do nothing... this is just a bookkeeping stage
		} else if (_docTheoryStageHandlers.find(currentStage) != _docTheoryStageHandlers.end()) {
			_docTheoryStageHandlers[currentStage]->process(docTheory);
		} else {
			std::ostringstream err;
			err << "Unexpected or unknown stage: " << currentStage.getName()
				<< " (before sent-break)";
			throw InternalInconsistencyException("DocumentDriver::runOnDocTheory", err.str().c_str());
		}
		stageProcessTimer[currentStage].stopTimer();
		documentProcessTimer.stopTimer();
		if (_max_document_processing_milliseconds > 0 && documentProcessTimer.getTime() > _max_document_processing_milliseconds) {
			std::ostringstream err;
			err << "Document " << document_name << " timed out after stage " << currentStage.getName();
			throw UnrecoverableException("DocumentDriver::runOnDocTheory", err.str().c_str());
		}
		saveDocTheoryState(docTheory, currentStage);
		++currentStage;
	}

	// Sentence Breaking
	if ((currentStage == Stage("sent-break") && 
		 _sessionProgram->includeStage(currentStage))) {
		stageProcessTimer[currentStage].startTimer();
		documentProcessTimer.startTimer();
		_sentenceBreaker->resetForNewDocument(document);
		Sentence *sentences[MAX_DOCUMENT_SENTENCES];
		int n_sentences= _sentenceBreaker->getSentences(
			sentences, MAX_DOCUMENT_SENTENCES,
			document->getRegions(), document->getNRegions());
		
		docTheory->setSentences(n_sentences, sentences);
		stageProcessTimer[currentStage].stopTimer();
		documentProcessTimer.stopTimer();
		if (_max_document_processing_milliseconds > 0 && documentProcessTimer.getTime() > _max_document_processing_milliseconds) {
			std::ostringstream err;
			err << "Document " << document_name << " timed out after stage " << currentStage.getName();
			throw UnrecoverableException("DocumentDriver::runOnDocTheory", err.str().c_str());
		}
		++currentStage;
	}
	saveSentenceBreakState(docTheory);


	// Sentence-level processing.
	if ((currentStage <= Stage::getLastSentenceLevelStage()) &&
		(endStage >= Stage("sent-break")))
	{
		_sentenceDriver->beginDocument(docTheory);
		// main sentence loop -- all sentence-level stages
		for (int sent_no = 0; sent_no < docTheory->getNSentences(); sent_no++) {\
			// update session logger with sentence info
			char sent_str[11];
			sprintf(sent_str, "%d", sent_no);
			_localSessionLogger->updateContext(SENTENCE_CONTEXT, sent_str);
			SentenceTheoryBeam *sentenceTheoryBeam;
			documentProcessTimer.startTimer();
			sentenceTheoryBeam = _sentenceDriver->run(docTheory, sent_no, startStage, endStage);
			//std::cout << document->getName().to_debug_string() << ":" << sent_no << ": " << std::hex << (int) sentenceTheoryBeam << " " << (int) sentenceTheoryBeam->getBestTheory() << std::dec << std::endl;
			docTheory->setSentenceTheoryBeam(sent_no, sentenceTheoryBeam);
			documentProcessTimer.stopTimer();
			if (_max_document_processing_milliseconds > 0 && documentProcessTimer.getTime() > _max_document_processing_milliseconds) {
				std::ostringstream err;
				err << "Document " << document_name << " timed out after sentence " << (sent_no + 1) << "/" << docTheory->getNSentences();
				throw UnrecoverableException("DocumentDriver::runOnDocTheory", err.str().c_str());
			}
		}

		// If any sentences provided an entity set, then adopt the last such entity
		// set as the entity set for the entire document.  (When we have per-sentence 
		// entity detection turned on, the entity set for each sentence includes the 
		// entities for all previous sentences in the document.)
		for (int sent_no = docTheory->getNSentences()-1; sent_no >=0 ; --sent_no) {
			SentenceTheoryBeam *sentenceTheoryBeam = docTheory->getSentenceTheoryBeam(sent_no);
			if ((sentenceTheoryBeam != 0) &&
				(sentenceTheoryBeam->getNTheories() > 0) &&
				(sentenceTheoryBeam->getBestTheory()->getEntitySet() != 0)) 
			{
				docTheory->setEntitySet(_new EntitySet(*sentenceTheoryBeam->getBestTheory()->getEntitySet()));
				break;
			}
		}

		// Output those readable debug dumps -- pre-doc-level
		if (_dump_theories) {
			if (document_filename) {
				dumpDocumentTheory(docTheory, document_filename);
			} else {
				dumpDocumentTheory(docTheory, document->getName().to_string());
			}
		}
		_sentenceDriver->endDocument();
	}

	// Now that we're done with sentence-level stages, move on to
	// document-level stages:
	if (startStage <= Stage::getLastSentenceLevelStage()) {
		startStage = Stage::getLastSentenceLevelStage();
		++startStage;
	}

	for (Stage stage = startStage; stage < endStage.getNextStage(); ++stage) {
		if (!_sessionProgram->includeStage(stage))
			continue;

		_localSessionLogger->updateContext(STAGE_CONTEXT, stage.getName());

		stageProcessTimer[stage].startTimer();
		documentProcessTimer.startTimer();

		// notify each subtheory generator that we're starting a new sentence
		if (stage == Stage ("sent-level-end")) {
			// do nothing... this is just a bookkeeping stage
		} else if (stage == Stage("prop-status")) {
			_propStatusClassifier->augmentPropositionTheory(docTheory);
		} else if (stage == Stage("doc-entities")) {
			_docEntityLinker->linkEntities(docTheory);
		} else if (stage == Stage("doc-relations-events")) {
			if (!ParamReader::getRequiredTrueFalseParam("use_sentence_level_event_finding"))
				_docRelationEventProcessor->doDocRelationsAndDocEvents(docTheory);
		} else if (stage == Stage("doc-values")) {
			_docValueProcessor->doDocValues(docTheory);
		} else if (stage == Stage("confidences")) {
			_confidenceEstimator->process(docTheory);
		} else if (stage == Stage("doc-actors")) {
			_docActorProcessor->doDocActors(docTheory);
		} else if (stage == Stage("factfinder")) {
			_factFinder->findFacts(docTheory);
		} else if (stage == Stage("generics")) {
			_genericsFilter->filterGenerics(docTheory);
		} else if (stage == Stage("clutter")) {
			_clutterFilter->filterClutter(docTheory);
		} else if (stage == Stage("xdoc")) {
			_xdocClient->performXDoc(docTheory);
		} else if (stage == Stage("cause-effect")) {
			_causeEffectRelationFinder->findCauseEffectRelations(docTheory);
		} else if (stage == Stage("output")) {
			if (document_filename && _sessionProgram->hasExperimentDir())
				outputResults(document_filename, docTheory, _sessionProgram->getOutputDir());
			if (results)
				outputResults(results, docTheory);
		} else if (stage == Stage ("score")) {
			// This stage is handled outside of runOnDocTheory, though
			// it probably *could* be moved here.
		} else if (stage == Stage ("end")) {
			// do nothing... this is just a bookkeeping stage
		} else if (_docTheoryStageHandlers.find(stage) != _docTheoryStageHandlers.end()) {
			_docTheoryStageHandlers[stage]->process(docTheory);
		} else {
			std::ostringstream err;
			err << "Unexpected or unknown stage: " << stage.getName()
				<< " (after sent-level-end)";
			throw InternalInconsistencyException("DocumentDriver::runOnDocTheory", err.str().c_str());
		}

		stageProcessTimer[stage].stopTimer();
		documentProcessTimer.stopTimer();

		saveDocTheoryState(docTheory, stage);

		if (_max_document_processing_milliseconds > 0 && documentProcessTimer.getTime() > _max_document_processing_milliseconds) {
			std::ostringstream err;
			err << "Document " << document_name << " timed out after stage " << stage.getName();
			throw UnrecoverableException("DocumentDriver::runOnDocTheory", err.str().c_str());
		}
	}

	// If the doc_format is SerifXML, then generate output even if we didn't
	// make it all the way to the "output" stage.
	if (document_filename && _sessionProgram->hasExperimentDir()) {
		outputSerifXMLResults(document_filename, docTheory, _sessionProgram->getOutputDir());
	} else if (results) {
		outputSerifXMLResults(results, docTheory);
	}

	// Output those readable debug dumps -- doc-level
	//   if doc-level processes don't fill in sentence-level processes,
	//   some fields may be blank (e.g. entities, relations, events)
	if (_dump_theories) {
		if (document_filename) {
			dumpDocumentTheory(docTheory, document_filename);
		} else { 
			dumpDocumentTheory(docTheory, document->getName().to_string());
		}
	}

	// 11/13/06 by JSG - MT is a somewhat special output format, which wants partial Serif processing to be output
	if (document_filename && _sessionProgram->hasExperimentDir()) {
		outputMTResults(document_filename, docTheory, _sessionProgram->getOutputDir());
	} else if (results) {
		outputMTResults(results, docTheory);
	}
}

void DocumentDriver::saveDocTheoryState(DocTheory *docTheory, Stage stage) {

	if (!_sessionProgram->hasExperimentDir())
		return;
	
	if( !_sessionProgram->saveStageState(stage))
		return;

	StateSaver *stateSaver = 0;
	
	if(_sessionProgram->saveSingleDocumentStateFiles())
	{
		wstring dname( docTheory->getDocument()->getName().to_string() );
		wstring state_file = _sessionProgram->constructSingleDocumentStateFile(dname.c_str(), stage);
		
        stateSaver = _new StateSaver(state_file.c_str(), ParamReader::isParamTrue("binary_state_files"));
		docTheory->saveSentenceBreaksToStateFile(stateSaver);
	} else {
		stateSaver = _sentenceDriver->getStageStateSaver(stage);
	}

		std::wstring wstr;
		if (stage == Stage("sent-level-end"))
			wstr = L"DocTheory following stage: sent-level-end";
		else if (stage == Stage("doc-entities"))
			wstr = L"DocTheory following stage: doc-entities";
		else if (stage == Stage("doc-relations-events"))
			wstr = L"DocTheory following stage: doc-relations-events";
		else if (stage == Stage("doc-values"))
			wstr = L"DocTheory following stage: doc-values";
		else return;

		ObjectIDTable::initialize();
		docTheory->updateObjectIDTable();
		stateSaver->beginStateTree(wstr.c_str());
		stateSaver->saveInteger(docTheory->getNSentences());
		docTheory->saveState(stateSaver);
		stateSaver->endStateTree();
		ObjectIDTable::finalize();

	// delete above allocation
	if (_sessionProgram->saveSingleDocumentStateFiles())
		delete stateSaver;
}

void DocumentDriver::loadDocTheory(DocTheory *docTheory, Stage stage) {
	std::wstring wstr;
	if (stage == Stage("sent-level-end"))
		wstr = L"DocTheory following stage: sent-level-end";
	else if (stage == Stage("doc-entities"))
		wstr = L"DocTheory following stage: doc-entities";
	else if (stage == Stage("doc-relations-events"))
		wstr = L"DocTheory following stage: doc-relations-events";
	else if (stage == Stage("doc-values"))
		wstr = L"DocTheory following stage: doc-values";
	else {
		char message[500];
		sprintf(message, "Serif cannot restart in stage %s", stage.getName());
		throw UnrecoverableException("DocumentDriver::loadDocTheoryBeforeStage()", message);
	}

	StateLoader *stateLoader = _sentenceDriver->getStateLoader();
	stateLoader->beginStateTree(wstr.c_str());
	stateLoader->loadInteger();
	docTheory->loadDocTheory(stateLoader);
	stateLoader->endStateTree();
	docTheory->resolvePointers(stateLoader);
}

Document *DocumentDriver::loadDocument(const wchar_t *document_filename) {
	if (_use_serifxml_source_format)
		throw InternalInconsistencyException("DocumentDriver::loadDocument",
			"SerifXML doc_format not supported.");
	if (_documentReader == 0)
		throw InternalInconsistencyException("DocumentDriver::loadDocument",
			"_documentReader not initialized");
	return _documentReader->readDocumentFromFile(document_filename);
}

DocumentReader *DocumentDriver::createDocumentReader() {
	//Check the document input format
	std::string source_format = ParamReader::getParam("source_format");
	boost::algorithm::to_lower(source_format); // case-normalize.

	if (source_format.empty() || boost::iequals(source_format, "default")) 
		source_format = "sgm"; // default value
	//Support old MT parameter
	if (ParamReader::isParamTrue("mt_format"))
		source_format = "segments";

	if (boost::iequals(source_format, "serifxml")) {
		// If source_format is SerifXML, then we don't actually use a document
		// reader, since the SerifXML format encodes both a document *and* a
		// DocTheory; and the document reader interface doesn't allow for 
		// that.  Instead, the DocumentDriver directly creates an 
		// XMLSerializedDocTheory, and calls its generateDocTheory() method.
		_use_serifxml_source_format = true;
		return 0;
	} else {
		return DocumentReader::build(source_format.c_str());
	}
}

//Save the sentence breaks for all sentences in a document at for each active state saver
//Note: The sentence breaks are their own 'state-tree' that occurs before the sentence
//beams of each sentence level state or the entire document for document level states
void DocumentDriver::saveSentenceBreakState(DocTheory *docTheory){
	Stage startStage = _sessionProgram->getStartStage();
	Stage endStage = _sessionProgram->getEndStage();
	for(Stage stage = startStage; stage < endStage.getNextStage(); ++stage){
		StateSaver *stateSaver = _sentenceDriver->getStageStateSaver(stage);
		if (stateSaver != 0) {
			docTheory->saveSentenceBreaksToStateFile(stateSaver);
		}
	}
}

void DocumentDriver::outputResults(const wchar_t *document_filename
								   ,DocTheory *docTheory
								   ,const wchar_t *output_dir)
{
	if (_resultCollector) {
		wstring output_dir_as_wstring(output_dir);
		_resultCollector->loadDocTheory(docTheory);
		_resultCollector->produceOutput(output_dir_as_wstring.c_str(), document_filename);
	}
    if (_alternateResultCollectors) {
      wstring output_dir_as_wstring(output_dir);
      for (size_t i=0; i<_alternateResultCollectors->size(); i++) {
        _alternateResultCollectors->at(i)->loadDocTheory(docTheory);
        _alternateResultCollectors->at(i)->produceOutput(output_dir_as_wstring.c_str(), document_filename);
      }
    }
}

void DocumentDriver::outputResults(wstring *results,
								   DocTheory *docTheory)
{
	if (_resultCollector) {
		_resultCollector->loadDocTheory(docTheory);
		_resultCollector->produceOutput(results);
	}
    if (_alternateResultCollectors) {
      for (size_t i=0; i<_alternateResultCollectors->size(); i++) {
        _alternateResultCollectors->at(i)->loadDocTheory(docTheory);
        _alternateResultCollectors->at(i)->produceOutput(results);
      }
    }
}

void DocumentDriver::outputSerifXMLResults(const wchar_t *document_filename,
										   DocTheory *docTheory,
										   const wchar_t *output_dir)
{
	if ((_sessionProgram->getEndStage() < Stage("output"))) {
		std::vector<ResultCollector*> collectors;
		if (_alternateResultCollectors)
			collectors.assign(_alternateResultCollectors->begin(), _alternateResultCollectors->end());
		collectors.push_back(_resultCollector);
		BOOST_FOREACH(ResultCollector* collector, collectors) {
			if (SerifXMLResultCollector* rc = dynamic_cast<SerifXMLResultCollector*>(collector)) {
				rc->loadDocTheory(docTheory);
				rc->produceOutput(output_dir, document_filename);
			}
		}
	}
}

void DocumentDriver::outputSerifXMLResults(std::wstring *results,
										   DocTheory *docTheory)
{
	if ((_sessionProgram->getEndStage() < Stage("output"))) {
		std::vector<ResultCollector*> collectors;
		if (_alternateResultCollectors)
			collectors.assign(_alternateResultCollectors->begin(), _alternateResultCollectors->end());
		collectors.push_back(_resultCollector);
		BOOST_FOREACH(ResultCollector* collector, collectors) {
			if (SerifXMLResultCollector* rc = dynamic_cast<SerifXMLResultCollector*>(collector)) {
				rc->loadDocTheory(docTheory);
				rc->produceOutput(results);
			}
		}
	}
}

void DocumentDriver::outputMTResults(const wchar_t *document_filename,
									 DocTheory *docTheory,
									 const wchar_t *output_dir)
{
	if (ParamReader::getParam(L"output_format") == Symbol(L"MT") || 
		ParamReader::isParamTrue("output_MT_format_in_addition_to_chosen_output_format")) {
		MTResultCollector collector;
		collector.loadDocTheory(docTheory);
		collector.produceOutput(output_dir, document_filename);
	}
}

void DocumentDriver::outputMTResults(std::wstring *results,
									 DocTheory *docTheory)
{
	if (ParamReader::getParam(L"output_format") == Symbol(L"MT") || 
		ParamReader::isParamTrue("output_MT_format_in_addition_to_chosen_output_format")) {
		throw UnexpectedInputException("DocumentDriver::outputMTResults", "MTResultCollector only supports writing to files");
	}
}

void DocumentDriver::outputMSAResults(const wchar_t *document_filename,
									 DocTheory *docTheory,
									 const wchar_t *output_dir)
{
	if (ParamReader::getParam(L"output_format") == Symbol(L"MSAXML") && ParamReader::getParam(L"end_stage") == Symbol(L"names")) {
		MSANamesResultCollector collector;
		collector.loadDocTheory(docTheory);
		collector.produceOutput(output_dir, document_filename);
	}
}

void DocumentDriver::dumpDocumentTheory(DocTheory *docTheory,
										const wchar_t *document_filename)
{
	
	#ifdef BLOCK_FULL_SERIF_OUTPUT

	throw UnexpectedInputException("DocumentDriver::dumpDocumentTheory", "Dump theories not supported");

	#endif

	if (!_sessionProgram->hasExperimentDir())
		return;

	bool print_special = false;
	wstring dump_dir(_sessionProgram->getDumpDir());
	wstring doc_name = docTheory->getDocument()->getName().to_string();

	if (PRINT_TOKENIZATION) {
		wstring toks_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".tokens";
	
		UTF8OutputStream toks_out;
		toks_out.open(toks_file.c_str());
		
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			TokenSequence *tseq = docTheory->getSentenceTheory(i)->getTokenSequence();
			for (int i = 0; i < tseq->getNTokens(); i++) {
				toks_out << tseq->getToken(i)->getSymbol();
				toks_out << L" ";
			}
			toks_out << "\n";
		}

		toks_out.close();
		print_special = true;
	}
	
	if (PRINT_SENTENCE_SELECTION_INFO) {
		wstring ss_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".ss";

		UTF8OutputStream ss_out;
		ss_out.open(ss_file.c_str());

		
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			if (docTheory->getSentenceTheory(i)->getNameTheory() == 0)
				continue;
			TokenSequence *tseq = docTheory->getSentenceTheory(i)->getTokenSequence();
			NameTheory *names = docTheory->getSentenceTheory(i)->getNameTheory();
			if(PRINT_SENTENCE_SELECTION_INFO_UNTOK){
				const LocatedString* origstr = docTheory->getSentence(i)->getString();
				LocatedString* str = origstr->substring(0);
				str->replace(L")", L" -RRB- ");
				str->replace(L"(", L" -LRB- ");
				str->replace(L"\n", L" ");
				ss_out <<"(( " << str->toString() <<L"  ) " <<names->getScore() <<L" (";
				delete str;
			}
			else{
				ss_out << L"(" << tseq->toString() << L" " << names->getScore() << L" (";
			}
			for (int j = 0; j < names->getNNameSpans(); j++) {
				NameSpan *span = names->getNameSpan(j);
				ss_out << L"(";
				for (int index = span->start; index <= span->end; index++) {
					ss_out << tseq->getToken(index)->getSymbol() << " ";
				}
				ss_out << L")";
			}
			ss_out << L"))\n";
		}

		ss_out.close();
		print_special = true;
	}

	if (PRINT_HTML_DISPLAY) {

		// Prints an HTML file suitable for viewing by a non-SERIF-expert
		// This was created by Liz Boschee in March 2011 for a particular project, so it only includes
		//   what I needed there (e.g. it does not include events).
		
		wstring html_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".html";
		UTF8OutputStream html_out;
		html_out.open(html_file.c_str());

		html_out << "<html>\n<h2>Key</h2>\n";
		html_out << "<font color=\"red\">Person: <b>red</b></font><br>\n";
		html_out << "<font color=\"blue\">Organization: <b>blue</b></font><br>\n";
		html_out << "<font color=\"green\">Location or Facility: <b>green</b></font><br>\n";
		html_out << "<font color=\"purple\">Date/Time: <b>purple</b></font><br>\n";
		html_out << "<font color=\"orange\">Contact Info (emails, URLs, or phone numbers): <b>orange</b></font><br>\n";
		html_out << "<font color=\"gray\">Other: <b>gray</b></font><br><br>\n";
		html_out << "<html>\n<h2>Document</h2>\n";

		LocatedString docText(*(docTheory->getDocument()->getOriginalText()));
		addNamesToDocString(docTheory, &docText, true);
		addValuesToDocString(docTheory, &docText, true);

		for (int i = 0; i < docTheory->getNSentences(); i++) {
			TokenSequence *tseq = docTheory->getSentenceTheory(i)->getTokenSequence();			
			if (tseq == 0)
				continue;
			EDTOffset start_edt_offset = tseq->getToken(0)->getStartEDTOffset();
			EDTOffset end_edt_offset = tseq->getToken(tseq->getNTokens() - 1)->getEndEDTOffset();
			++end_edt_offset; // I think this is correct, but it's all a little scary
			int start = docText.positionOfStartOffset(start_edt_offset);
			int end = docText.positionOfEndOffset(end_edt_offset);
			LocatedString *sentString = docText.substring(start, end);
			html_out << sentString->toString();
			delete sentString;
			
			html_out << "<br>\n";
			RelMentionSet *rms = docTheory->getSentenceTheory(i)->getRelMentionSet();
			if (rms != 0 && rms->getNRelMentions() != 0) {
				html_out << "<br><table>\n";
				for (int r = 0; r < rms->getNRelMentions(); r++) {
					RelMention *rm = rms->getRelMention(r);
					std::wstring typeStr = rm->getType().to_string();
					html_out << L"<tr><td width=\"30\"></td><td><i>";
					if (typeStr == L"ORG-AFF.Employment") html_out << L"Employment";
					else if (typeStr == L"ORG-AFF.Membership") html_out << L"Membership";
					else if (typeStr == L"ORG-AFF.Student-Alum") html_out << L"Student/Alumni";
					else if (typeStr == L"ORG-AFF.Founder") html_out << L"Founder";
					else if (typeStr == L"ORG-AFF.Sports-Affiliation") html_out << L"Affiliation (sports)";
					else if (typeStr == L"ORG-AFF.Investor-Shareholder") html_out << L"Investor/Shareholder";
					else if (typeStr == L"PER-SOC.Family") html_out << L"Family";
					else if (typeStr == L"PER-SOC.Lasting-Personal") html_out << L"Personal Relationship (non-family)";
					else if (typeStr == L"PER-SOC.Business") html_out << L"Professional Relationship";
					else if (typeStr == L"GEN-AFF.Org-Location") html_out << L"Location";
					else if (typeStr == L"PHYS.Located") html_out << L"Location";
					else if (typeStr == L"PHYS.Near") html_out << L"Location (nearby)";
					else if (typeStr == L"PART-WHOLE.Geographical") html_out << L"Location";
					else if (typeStr == L"PART-WHOLE.Artifact") html_out << L"Part-Whole";
					else if (typeStr == L"PART-WHOLE.Subsidiary") html_out << L"Subsidiary";
					else if (typeStr == L"ART.User-Owner-Inventor-Manufacturer") html_out << L"User/Owner/Manufacturer";
					else if (typeStr == L"GEN-AFF.Citizen-Resident-Religion-Ethnicity") html_out << L"Citizenship/Residence/Religion/Ethnicity";
					else html_out << typeStr;
					html_out << L":<br>\n";
					std::vector<const Mention *> mentionsToPrint;
					mentionsToPrint.push_back(rm->getLeftMention());
					mentionsToPrint.push_back(rm->getRightMention());
					for (std::vector<const Mention *>::iterator iter = mentionsToPrint.begin(); iter != mentionsToPrint.end(); iter++) {
						const Mention *ment = (*iter);
						html_out << L"* " << tseq->toString(ment->getNode()->getStartToken(), ment->getNode()->getEndToken());
						if (ment->getMentionType() != Mention::NAME && docTheory->getEntitySet() != 0) {
							Entity *entity = docTheory->getEntitySet()->getEntityByMention(ment->getUID());							
							std::wstring first_name = L"";
							std::wstring first_desc = L"";
							if (entity != 0) {
								for (int mentno = 0; mentno < entity->getNMentions(); mentno++) {
									const Mention *new_ment = docTheory->getEntitySet()->getMention(entity->getMention(mentno));
									TokenSequence* t = docTheory->getSentenceTheory(new_ment->getSentenceNumber())->getTokenSequence();								
									if (first_name.length() == 0 && new_ment->getMentionType() == Mention::NAME) {
										const SynNode *nameNode = new_ment->getNode()->getHeadPreterm()->getParent();
										if (nameNode != 0) {
											first_name = t->toString(nameNode->getStartToken(), nameNode->getEndToken());
											break;
										}
									} else if (first_desc.length() == 0 && 
										(new_ment->getMentionType() == Mention::DESC || new_ment->getMentionType() == Mention::PART))
									{
										first_desc = t->toString(new_ment->getNode()->getStartToken(), new_ment->getNode()->getEndToken());									
									}
								}	
							}
							if (first_name.length() != 0)
								html_out << L" [" << first_name << L"]<br>\n";
							else if (first_desc.length() != 0 && ment->getMentionType() == Mention::PRON)
								html_out << L" [" << first_desc << L"]<br>\n";
							else html_out << L"<br>\n";
						} else html_out << L"<br>\n";
					}
					html_out << L"</i></td></tr>\n";
				}
				html_out << L"</table>\n";
			}
			EventMentionSet *evms = docTheory->getSentenceTheory(i)->getEventMentionSet();
			html_out << "<br><table>\n";
			for (int eno = 0; eno < evms->getNEventMentions(); eno++) {
				EventMention* ev = evms->getEventMention(eno);
				if(ev->getNArgs() == 0 && ev->getNValueArgs() == 0)
					continue;
				std::wstring typeStr = ev->getEventType().to_string();
				html_out << L"<tr><td width=\"30\"></td><td><i>";
				html_out << typeStr;
				html_out << L", ";
				html_out << "(anchor word- "<<ev->getAnchorNode()->getHeadWord().to_string()<<"):<br>\n";
				for(int ano = 0; ano < ev->getNArgs(); ano++){
					const Mention* ment = ev->getNthArgMention(ano);
					html_out <<"*" <<ev->getNthArgRole(ano)<<": "<<ment->getAtomicHead()->toTextString();
					if (ment->getMentionType() != Mention::NAME && docTheory->getEntitySet() != 0) {
						Entity *entity = docTheory->getEntitySet()->getEntityByMention(ment->getUID());							
						std::wstring first_name = L"";
						std::wstring first_desc = L"";
						if (entity != 0) {
							for (int mentno = 0; mentno < entity->getNMentions(); mentno++) {
								const Mention *new_ment = docTheory->getEntitySet()->getMention(entity->getMention(mentno));
								TokenSequence* t = docTheory->getSentenceTheory(new_ment->getSentenceNumber())->getTokenSequence();								
								if (first_name.length() == 0 && new_ment->getMentionType() == Mention::NAME) {
									const SynNode *nameNode = new_ment->getNode()->getHeadPreterm()->getParent();
									if (nameNode != 0) {
										first_name = t->toString(nameNode->getStartToken(), nameNode->getEndToken());
										break;
									}
								} else if (first_desc.length() == 0 && 
									(new_ment->getMentionType() == Mention::DESC || new_ment->getMentionType() == Mention::PART))
								{
									first_desc = t->toString(new_ment->getNode()->getStartToken(), new_ment->getNode()->getEndToken());									
								}
							}	
						}
						if (first_name.length() != 0)
							html_out << L" [" << first_name << L"]<br>\n";
						else if (first_desc.length() != 0 && ment->getMentionType() == Mention::PRON)
							html_out << L" [" << first_desc << L"]<br>\n";
						else html_out << L"<br>\n";
					} else html_out << L"<br>\n";				
				}
				for(int ano = 0; ano < ev->getNValueArgs(); ano++){
					const ValueMention* vm = ev->getNthArgValueMention(ano);
					html_out  <<"*" <<ev->getNthArgValueRole(ano)<<": "<<vm->toCasedTextString(docTheory->getSentenceTheory(i)->getTokenSequence())<<"<br>\n";
				}
				html_out << L"</i></td></tr>\n";
			}
			html_out << L"</table>\n";
			html_out << "<br>\n";

		}

		html_out.close();
	}
	
	if (PRINT_NAME_ENAMEX_TOKENIZED) {

		// prints name info in ENAMEX format, as tokenized by Serif,
		//  one sentence per line
		
		wstring enamex_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".names.tokenized.enamex";

		UTF8OutputStream enamex_out;
		enamex_out.open(enamex_file.c_str());
		
		for (int i = 0; i < docTheory->getNSentences(); i++) {			
			if (docTheory->getSentenceTheory(i)->getNameTheory() == 0)
				continue;
			TokenSequence *tseq = docTheory->getSentenceTheory(i)->getTokenSequence();			
			NameTheory *names = docTheory->getSentenceTheory(i)->getNameTheory();
			for (int t = 0; t < tseq->getNTokens(); t++) {
				std::wstring prefix = L"";
				std::wstring suffix = L"";
				for (int n = 0; n < names->getNNameSpans(); n++) {
					NameSpan *ns = names->getNameSpan(n);
					if (ns->start == t) {
						prefix += L"<ENAMEX TYPE=\"";
						prefix += ns->type.getName().to_string();
						prefix += L"\">";
					}
					if (ns->end == t) {
						suffix += L"</ENAMEX>";
					}
				}
				enamex_out << prefix << tseq->getToken(t)->getSymbol() << suffix << L" ";
			}
			enamex_out << L"\n";
		}

		enamex_out.close();
		print_special = true;
	}
	
	if (PRINT_NAME_ENAMEX_ORIGINAL_TEXT || PRINT_NAME_VALUE_ENAMEX_ORIGINAL_TEXT) {

		// prints name (and possibly value) info in ENAMEX format, inserted into the original text
		
		wstring enamex_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".names.enamex";

		UTF8OutputStream enamex_out;
		enamex_out.open(enamex_file.c_str());

		LocatedString docText(*(docTheory->getDocument()->getOriginalText()));
		addNamesToDocString(docTheory, &docText);
		if (PRINT_NAME_VALUE_ENAMEX_ORIGINAL_TEXT) {
			addValuesToDocString(docTheory, &docText);
		}
		enamex_out << docText.toString();
		enamex_out.close();
		print_special = true;
	}
	if(PRINT_PROPOSITON_TREES){
		wstring pt_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".proptrees";
		UTF8OutputStream pt_out;
		pt_out.open(pt_file.c_str());
		DocPropForest::ptr docPropForest = PropForestFactory::getDocumentForest(docTheory);
		for(int sno =0; sno < docTheory->getNSentences(); sno++){
			const TokenSequence* toks = docTheory->getSentenceTheory(sno)->getTokenSequence();
			pt_out<<"Sentence "<<sno<<": "<<toks->toString()<<"\n";
			PropNodes_ptr sentenceNodes = (*(docPropForest.get()))[sno];
			PropNode::PropNodes roots;
			PropNode::enumRootNodes(*sentenceNodes, roots);	
			for(size_t c_it = 0; c_it != roots.size(); c_it++){
				std::wstringstream wstrstream;
				(*roots[c_it]).compactPrint(wstrstream, false, false, true, 1);
				pt_out<<"TreeRoot "<<c_it<<"\n"<< wstrstream.str()<<"\n";
			}
		}
		pt_out.close();
		print_special = true;
	}
	
	// don't print real dump files if we're in another mode
	if (print_special)
		return;

	wstring dump_file = dump_dir + LSERIF_PATH_SEP + L"dump-" + wstring(document_filename) + L".txt";

	ofstream dump;
#if defined(_WIN32)
	dump.open(dump_file.c_str());
#else
	string dump_file_narrow = OutputUtil::convertToUTF8BitString(dump_file.c_str());
	dump.open(dump_file_narrow.c_str());
#endif

	if (dump) {
		dump << *docTheory << "\n";
	} else {
		SessionLogger::warn("dump_document_theories") << 
			"Cannot open " << dump_file << " to write a document theory dump. This path is constructed from the document ID; please make sure that the document ID can be a part of a valid pathname." << std::endl;
	}

	dump.close();
	
	if (PRINT_COREF_FOR_OUTSIDE_SYSTEM) {
		wstring parse_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".parses";

		UTF8OutputStream p_out;
		p_out.open(parse_file.c_str());

		for (int i = 0; i < docTheory->getNSentences(); i++) {
			if (docTheory->getSentenceTheory(i)->getPrimaryParse() == 0)
				continue;
			const SynNode *root = docTheory->getSentenceTheory(i)->getPrimaryParse()->getRoot();
			p_out << root->toAugmentedParseString(docTheory->getSentenceTheory(i)->getTokenSequence(),
				docTheory->getSentenceTheory(i)->getMentionSet(), 0);
			p_out << L"\n";
		}

		p_out.close();

		EntitySet *eset = docTheory->getEntitySet();
		if (eset != 0) {
			wstring entities_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".entities";

			UTF8OutputStream e_out;
			e_out.open(entities_file.c_str());

			wchar_t buffer[10];
			for (int i = 0; i < eset->getNEntities(); i++) {
				Entity *ent = eset->getEntity(i);
				std::wstring mentstr = L"";
				for (int j = 0; j < ent->getNMentions(); j++) {
					Mention *ment = eset->getMention(ent->getMention(j));
					if (ment->getMentionType() == Mention::NAME ||
						ment->getMentionType() == Mention::DESC ||
						ment->getMentionType() == Mention::PART ||
						ment->getMentionType() == Mention::PRON)
					{
						if (mentstr.length() != 0)
							mentstr += L" ";
						_snwprintf(buffer, 10, L"%d", ment->getUID().toInt());
						mentstr += buffer;
					}
				}
				if (mentstr.length() != 0) {
					e_out << L"(" << ent->getID() << L" ";
					e_out << ent->getType().getName().to_string() << L" ";
					e_out << mentstr << L")\n";
				}
			}

			e_out.close();
		}

	}
	if (PRINT_PARSES){
		wstring parse_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".parses";

		UTF8OutputStream p_out;
		p_out.open(parse_file.c_str());

		for (int i = 0; i < docTheory->getNSentences(); i++) {
			if (docTheory->getSentenceTheory(i)->getFullParse() == 0)
				continue;
			const SynNode *root = docTheory->getSentenceTheory(i)->getFullParse()->getRoot();
			p_out << root->toString(0);
			p_out << L"\n";
		}
		p_out.close();

		wstring pos_file = dump_dir + LSERIF_PATH_SEP + doc_name + L".pos";
		
		p_out.open(pos_file.c_str());
		for (int j = 0; j < docTheory->getNSentences(); j++) {
			if (docTheory->getSentenceTheory(j)->getFullParse() == 0)
				continue;
			const SynNode *root = docTheory->getSentenceTheory(j)->getFullParse()->getRoot();
			p_out << root->toPOSString(docTheory->getSentenceTheory(j)->getTokenSequence());
			p_out << L"\n";
		}

		p_out.close();


	}
	
	if (PRINT_MT_SEGMENTATION) {
		if (docTheory->getNSentences() > 0) {
			std::wstring base = docTheory->getDocument()->getName().to_string();
			size_t pos = base.find('-');
			pos = (pos == string::npos) ? 0 : pos + 1;
			base = base.substr(pos,9);
			std::wstring dir = dump_dir + LSERIF_PATH_SEP + base;
			OutputUtil::makeDir(dir.c_str());
			std::wstring mt_file = dir + LSERIF_PATH_SEP + docTheory->getDocument()->getName().to_string() + L".mt";
			UTF8OutputStream mt_out;
			mt_out.open(mt_file.c_str());

			//print each segment, wrapped with <SEGMENT>...</SEGMENT>\n; include ACE offsets for each segment since we are adding new lines
			for (int i = 0; i < docTheory->getNSentences(); i++) {
				const LocatedString* origstr = docTheory->getSentence(i)->getString();
			
				std::wstring buf = origstr->toString();
				std::map<wchar_t,std::wstring> reps;
				//reps[L'&'] = L"&amp;";
				//reps[L'\"'] = L"&quot;";
				//reps[L'\''] = L"&apos;";
				reps[L'<'] = L"&lt;";
				reps[L'>'] = L"&gt;";
				reps[L'\n'] = L" ";  //for Arabic. In Chinese we might delete the symbol.
				reps[L'\r'] = L"";
				for (std::map<wchar_t,std::wstring>::const_iterator mwwci=reps.begin(); mwwci != reps.end(); mwwci++) {
					size_t s = 0;
					while ((s=buf.find(mwwci->first,s)) != buf.npos) {
						buf.replace(s,1,mwwci->second);
						s += mwwci->second.length();
					}
				}
				mt_out << L"<SEGMENT>\n\t<CHUNKS></CHUNKS>\n"
						<< "\t<GUID>[" << doc_name << "][" << i << "]</GUID>\n"
						<< "\t<START_OFFSET>" << origstr->firstStartOffsetStartingAt<EDTOffset>(0) << "</START_OFFSET>\n"
						<< "\t<END_OFFSET>" << origstr->lastEndOffsetEndingAt<EDTOffset>(origstr->length()-1) << "</END_OFFSET>\n"
						<< "\t<RAW_SOURCE>" << buf << "</RAW_SOURCE>\n"
						<< "</SEGMENT>\n";
			}
			//pritn the post </TEXT> stufff
			//mt_out << suffix->toString();
			mt_out.close();
			//delete prefix;
			//delete suffix;
		}
	}
	
}

void DocumentDriver::addNamesToDocString(DocTheory *docTheory, LocatedString *docText, bool output_html) {
	for (int i = 0; i < docTheory->getNSentences(); i++) {					
		TokenSequence *tseq= docTheory->getSentenceTheory(i)->getTokenSequence();
		NameTheory *names = docTheory->getSentenceTheory(i)->getNameTheory();
		if (names == 0)
			continue;
		for (int n = 0; n < names->getNNameSpans(); n++) {
			NameSpan *ns = names->getNameSpan(n);
			EDTOffset start_edt_offset = tseq->getToken(ns->start)->getStartEDTOffset();
			int start_offset = docText->positionOfStartOffset(start_edt_offset);
			std::wstring prefix = L"";
			if (output_html) {
				prefix += L"<font color=\"";
				if (ns->type.matchesPER()) prefix += L"red";
				else if (ns->type.matchesORG()) prefix += L"blue";
				else if (ns->type.matchesLOC() || ns->type.matchesGPE() || ns->type.matchesFAC()) prefix += L"green";
				else prefix += L"gray";
				prefix += L"\"><b>";
			} else {
				prefix += L"<ENAMEX TYPE=\"";
				prefix += ns->type.getName().to_string();
				prefix += L"\">";
			}
			docText->insertAtPOSWithSameOffsets(prefix.c_str(), start_offset);
			EDTOffset end_edt_offset = tseq->getToken(ns->end)->getEndEDTOffset();
			++end_edt_offset; //we want the SGML Tag at one after the last offset
			int end_offset = docText->positionOfEndOffset(end_edt_offset);
			std::wstring suffix;
			if (output_html) {
				suffix = L"</b></font>";
			} else {
				suffix = L"</ENAMEX>";
			}
			if (end_offset >= docText->length())
				docText->append(suffix.c_str());
			else docText->insertAtPOSWithSameOffsets(suffix.c_str(), end_offset);
		}
	}
}
void DocumentDriver::addValuesToDocString(DocTheory *docTheory, LocatedString *docText, bool output_html) {
	for (int i = 0; i < docTheory->getNSentences(); i++) {	
		TokenSequence *tseq= docTheory->getSentenceTheory(i)->getTokenSequence();
		ValueMentionSet *values = docTheory->getSentenceTheory(i)->getValueMentionSet();
		if (values == 0)
			continue;
		for (int v = 0; v < values->getNValueMentions(); v++) {
			ValueMention *vm = values->getValueMention(v);
			EDTOffset start_edt_offset = tseq->getToken(vm->getStartToken())->getStartEDTOffset();
			int start_offset = docText->positionOfStartOffset(start_edt_offset);
			std::wstring prefix = L"";
			if (output_html) {
				prefix += L"<font color=\"";
				if (vm->isTimexValue()) prefix += L"purple";
				else if (vm->getType() == Symbol(L"Contact-Info")) prefix += L"orange";
				else prefix += L"gray";
				prefix += L"\"><b>";
			} else {
				prefix += L"<ENAMEX TYPE=\"";
				prefix += vm->getFullType().getNameSymbol().to_string();
				prefix += L"\">";
			}
			docText->insertAtPOSWithSameOffsets(prefix.c_str(), start_offset);
			EDTOffset end_edt_offset = tseq->getToken(vm->getEndToken())->getEndEDTOffset();
			++end_edt_offset; //we want the SGML Tag at one after the last offset
			int end_offset = docText->positionOfEndOffset(end_edt_offset);
			std::wstring suffix;
			if (output_html) {
				suffix = L"</b></font>";
			} else {
				suffix = L"</ENAMEX>";
			}		
			if (end_offset >= docText->length())
				docText->append(suffix.c_str());
			else docText->insertAtPOSWithSameOffsets(suffix.c_str(), end_offset);
		}
	}
}

// run a perl script with the specified variables
// TODO: these probably shouldn't be unrecoverable, but for now they are
void DocumentDriver::invokeScoreScript()
{
	stageProcessTimer[Stage("score")].startTimer();

	// score script is the actual perl file that does the scoring
	std::string scoreScript = ParamReader::getRequiredParam("score_script");

	// key dir has the key apfs and the original sgms in directories
	// it's usually the directory that batch file is in
	std::string keyDirStr = ParamReader::getRequiredParam("key_dir");

	// expdir is the same root experiment directory used by other stuff
	std::string expDirStr = ParamReader::getRequiredParam("experiment_dir");

	string perl_binary = ParamReader::getParam("perl_binary");

	string runLine = (perl_binary.empty() ? "perl" : perl_binary) + string(" ");

	runLine += scoreScript;
	runLine += " " + keyDirStr;
	runLine += " " + expDirStr;
	int procRes = system(runLine.c_str());
	if(procRes!=0) {
		runLine += " failed!\n return value: ";
		char in[10];
		runLine += _itoa(procRes,in,10);
		throw UnrecoverableException("DocumentDriver::invokeScoreScript()", runLine);
	}
	stageProcessTimer[Stage("score")].stopTimer();
}

void DocumentDriver::setMaxParserSeconds(int maxsecs) {
	_sentenceDriver->setMaxParserSeconds(maxsecs);
}

void DocumentDriver::logTrace() const {

	_sentenceDriver->logTrace();

	Stage i;
	SessionLogger::info("profiling") << "DocumentDriver Load Time: " << endl;
	for (i = Stage::getFirstStage(); i < Stage::getEndStage(); ++i) {
		if ((i>Stage("sent-break")) && (i<=Stage("sent-level-end"))) continue;
		SessionLogger::info("profiling") << i.getName() << "\t" << stageLoadTimer[i].getTime() << " msec" << endl;
	}
	SessionLogger::info("profiling") << endl;
	SessionLogger::info("profiling") << "DocumentDriver Processing Time: " << endl;
	for (i = Stage::getFirstStage(); i < Stage::getEndStage(); ++i) {
		if ((i>Stage("sent-break")) && (i<=Stage("sent-level-end"))) continue;
		SessionLogger::info("profiling") << i.getName() << "\t" << stageProcessTimer[i].getTime() << " msec" << endl;
	}
	SessionLogger::info("profiling") << endl;
	if (_docRelationEventProcessor)
		_docRelationEventProcessor->logTrace();
}

float DocumentDriver::getThroughput(bool include_load_times) {
	double total_time = 0;
	Stage::HashMap<GenericTimer>::iterator it;
	for (it=stageProcessTimer.begin(); it!=stageProcessTimer.end(); ++it)
		total_time += (*it).second.getTime();
	if (include_load_times) {
		for (it=stageLoadTimer.begin(); it!=stageLoadTimer.end(); ++it)
			total_time += (*it).second.getTime();
	}
	// If we haven't processed anything yet, return -1 (undefined)
	if (totalBytesProcessed == 0) return -1;
	// If we've spent less than 10msec total so far, then any numbers are unreliable.
	if (total_time < 10) total_time = 10;
	// Convert from bytes/msec -> mb/hr
	double convert_ratio = (1024.0*1024.0)/(3600.0*1000.0);
	return static_cast<float>(totalBytesProcessed / total_time * convert_ratio);
}

void DocumentDriver::logSessionStart() {
	// The locale defines the behavior of functions like iswspace(), isupper() etc.
	char *locale = setlocale(LC_ALL, NULL); // The local here is only queried not modified.

	_localSessionLogger->updateContext(SESSION_CONTEXT,
								  _sessionProgram->getSessionName());

	_localSessionLogger->reportInfoMessage() << "_________________________________________________";
	_localSessionLogger->reportInfoMessage() << "Starting session: \""
	                                    << _sessionProgram->getSessionName() << "\"";
	_localSessionLogger->reportInfoMessage() << "Start stage: "
	                                    << _sessionProgram->getStartStage().getName() << "; "
	                                    << "end stage: "
	                                    << _sessionProgram->getEndStage().getName();
	_localSessionLogger->reportInfoMessage() << "Locale: " << locale;
	_localSessionLogger->reportInfoMessage() << "Parameters:";

	ParamReader::logParams();
}

void DocumentDriver::loadSentenceBreakerModels() { _sentenceBreaker = SentenceBreaker::build(); }
void DocumentDriver::loadDocEntityModels() { _docEntityLinker = _new DocEntityLinker(); }
void DocumentDriver::loadDocRelationsEventsModels() { _docRelationEventProcessor = _new DocRelationEventProcessor(); }
void DocumentDriver::loadDocValuesModels() { _docValueProcessor = _new DocValueProcessor(); }
void DocumentDriver::loadConfidenceModels() { _confidenceEstimator = ConfidenceEstimator::build(); }
void DocumentDriver::loadDocActorModels() { _docActorProcessor = _new ActorMentionFinder(ActorMentionFinder::DOC_ACTORS); }
void DocumentDriver::loadPropStatusModels() { _propStatusClassifier = PropositionStatusClassifier::build(); }
void DocumentDriver::loadGenericsModels() { _genericsFilter = GenericsFilter::build(); }
void DocumentDriver::loadFactFinder() { _factFinder = _new FactFinder(); }
void DocumentDriver::loadClutterModels() { _clutterFilter = ClutterFilter::build(); }
void DocumentDriver::loadXDocModels() { _xdocClient = _new XDocClient(); }
void DocumentDriver::loadCauseEffectModels() { _causeEffectRelationFinder = _new CauseEffectRelationFinder(); }


void DocumentDriver::setStageHandler(Stage stage, boost::shared_ptr<DocTheoryStageHandlerFactory> factory) {
	// Do a bunch of sanity checks.
	DocTheoryStageHandlerFactoryMap &factories = _docTheoryStageHandlerFactories();
	if (factories.find(stage) != factories.end())
		throw InternalInconsistencyException("DocumentDriver::setStageHandler", "Stage already added");
	if ((stage >= Stage("sent-break")) && (stage <= Stage("sent-level-end")))
		throw InternalInconsistencyException("DocumentDriver::setStageHandler", 
			"Document-level stage must come before sentence breaking or after sent-level-end.");
	if (boost::iequals(stage.getName(), "start") || boost::iequals(stage.getName(), "sent-break") || 
		boost::iequals(stage.getName(), "doc-entities") || boost::iequals(stage.getName(), "doc-relations-events") || 
		boost::iequals(stage.getName(), "doc-values") || boost::iequals(stage.getName(), "confidences") ||
		boost::iequals(stage.getName(), "doc-actors") || boost::iequals(stage.getName(), "generics") || 
		boost::iequals(stage.getName(), "clutter") || boost::iequals(stage.getName(), "xdoc") || 
		boost::iequals(stage.getName(), "output") || boost::iequals(stage.getName(), "score") ||
		boost::iequals(stage.getName(), "cause-effect"))
		throw InternalInconsistencyException("DocumentDriver::setStageHandler", "Stage already added");
	// If everything looks good, then add it.
	factories[stage] = factory;
}

DocumentDriver::DocTheoryStageHandlerFactoryMap &DocumentDriver::_docTheoryStageHandlerFactories() {
	static DocTheoryStageHandlerFactoryMap factories;
	return factories;
}

