// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/limits.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/driver/SentenceDriver.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/theories/SentenceTheoryBeam.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"

// Stage-specific components
#include "Generic/tokens/Tokenizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/xx_MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/normalizer/MTNormalizer.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Generic/names/NameRecognizer.h"
#include "Generic/nestedNames/NestedNameRecognizer.h"
#include "Generic/values/ValueRecognizer.h"
#include "Generic/parse/Parser.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/descriptors/DescriptorRecognizer.h"
#include "Generic/propositions/PropositionFinder.h"
#include "Generic/metonymy/MetonymyAdder.h"
#include "Generic/edt/DummyReferenceResolver.h"
#include "Generic/relations/RelationFinder.h"
#include "Generic/events/EventFinder.h"
#include "Generic/PNPChunking/NPChunkFinder.h"
#include "Generic/dependencyParse/DependencyParser.h"
#include "Generic/results/MTResultSaver.h"
#include "Generic/actors/ActorMentionFinder.h"

// Stage-specific subtheories
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include "dynamic_includes/common/ProfilingDefinition.h"

using namespace std;


int SentenceDriver::_n_docs_processed = 0;
static const Symbol _symbol_sentence = Symbol(L"sentence");


SentenceDriver::SentenceDriver()
	: _START_Stage("START"),
	  _tokens_Stage("tokens"),
	  _partOfSpeech_Stage("part-of-speech"),
	  _names_Stage("names"),
	  _nestedNames_Stage("nested-names"),
  	  _values_Stage("values"),
	  _parse_Stage("parse"),
	  _npchunk_Stage("npchunk"),
	  _dependencyParse_Stage("dependency-parse"),
	  _mentions_Stage("mentions"),
	  _actorMatch_Stage("actor-match"),
	  _props_Stage("props"),
	  _metonymy_Stage("metonymy"),
	  _entities_Stage("entities"),
	  _relations_Stage("relations"),
	  _events_Stage("events"),
	  _END_Stage("END"),

	  _sessionLogger(0),
	  _sessionProgram(0),

	  _tokenizer(0),
	  _posRecognizer(0),
	  _nameRecognizer(0),
	  _nestedNameRecognizer(0),
  	  _valueRecognizer(0),
	  _parser(0),
	  _descriptorRecognizer(0),
	  _actorMentionFinder(0),
	  _propositionFinder(0),
	  _metonymyAdder(0),
	  _dummyReferenceResolver(0),
	  _eventFinder(0),
	  _relationFinder(0),

	  _tokenSequenceBuf(0),
	  _partOfSpeechSequenceBuf(0),
	  _max_token_sequences(0),
	  _nameTheoryBuf(0),
	  _max_name_theories(0),
	  _nestedNameTheoryBuf(0),
	  _max_nested_name_theories(0),
	  _valueSetBuf(0),
	  _max_value_sets(0),
	  _parseBuf(0),
	  _max_parses(0),
	  _mentionSetBuf(0),
	  _actorMentionSetBuf(0),
	  _max_mention_sets(0),
	  _propositionSetBuf(0),
	  _entitySetBuf(0),
	  _relationSetBuf(0),
	  _eventSetBuf(0),
	  _morphAnalysis(0),
	  _morphSelector(0),
	  _mtNormalizer(0),
	  _npChunkFinder(0),
	  _mtResultSaver(0),
	  _dependencyParser(0),
	  _stateLoader(0),
	  _npChunkBuf(0),
	  _dependencyParseBuf(0),

	  _use_sentence_level_relation_finding(false),
	  _use_sentence_level_event_finding(false),
      _use_npchunker_constraints(false),
	  _do_morph_selection(false),
	  _use_lexicon_file(false)

{
	initIntentionalFailures();
	_beam_width = ParamReader::getRequiredIntParam("beam_width");
	_tokens_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("tokens_beam_width", _beam_width);
	_pos_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("pos_beam_width", _beam_width);
	_names_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("names_beam_width", _beam_width);
	_nested_names_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("nested_names_beam_width", _beam_width);
	_values_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("values_beam_width", _beam_width);
	_parse_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("parse_beam_width", _beam_width);
	_npchunk_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("npchunk_beam_width", _beam_width);
	_dependency_parse_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("dependency_parse_beam_width", _beam_width);
	_mentions_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("mentions_beam_width", _beam_width);
	_actor_match_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("actor_match_beam_width", _beam_width);
	_props_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("props_beam_width", _beam_width);
	_relations_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("relations_beam_width", _beam_width);
	_events_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("events_beam_width", _beam_width);
	_entities_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("entities_beam_width", _beam_width);

	_primary_parse = ParamReader::getParam(Symbol(L"primary_parse"));
	if (!SentenceTheory::isValidPrimaryParseValue(_primary_parse))
		throw UnrecoverableException("SentenceDriver::SentenceDriver()",			
			"Parameter 'primary_parse' has an invalid value");

	_use_sentence_level_relation_finding = ParamReader::getRequiredTrueFalseParam("use_sentence_level_relation_finding");
	_use_sentence_level_event_finding = ParamReader::getRequiredTrueFalseParam("use_sentence_level_event_finding");

	// Perform all stage-specific initializations
	_use_npchunker_constraints = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_npchunker_constraints", false);

	// Create timers
	for (Stage stage=Stage::getStartStage(); stage<Stage::getEndStage(); ++stage) {
		stageLoadTimer[stage] = GenericTimer();
		stageProcessTimer[stage] = GenericTimer();
	}
}

bool SentenceDriver::stageModelsAreLoaded(Stage stage) {
	return (((stage == _tokens_Stage) && (_tokenizer != 0)) ||
		    ((stage == _partOfSpeech_Stage) && (_posRecognizer != 0)) ||
		    ((stage == _names_Stage) && (_nameRecognizer != 0)) ||
		    ((stage == _nestedNames_Stage) && (_nestedNameRecognizer != 0)) ||
		    ((stage == _values_Stage) && (_valueRecognizer != 0)) ||
		    ((stage == _parse_Stage) && (_parser != 0)) ||
		    ((stage == _npchunk_Stage) && (_npChunkFinder != 0)) ||
			((stage == _dependencyParse_Stage) && (_dependencyParser != 0)) ||
		    ((stage == _mentions_Stage) && (_descriptorRecognizer != 0)) ||
			((stage == _actorMatch_Stage) && (_actorMentionFinder != 0)) ||
		    ((stage == _props_Stage) && (_propositionFinder != 0)) ||
		    ((stage == _metonymy_Stage) && (_metonymyAdder != 0)) ||
		    ((stage == _entities_Stage) && (_dummyReferenceResolver != 0)) ||
		    ((stage == _events_Stage) && (_eventFinder != 0)) ||
		    ((stage == _relations_Stage) && (_relationFinder != 0)));
}

void SentenceDriver::loadModelsForStage(Stage stage) 
{
	if ((stage < _tokens_Stage) || (stage >= Stage("sent-level-end")))
		return; // We're not responsible for this stage.
	if (stageModelsAreLoaded(stage))
		return; // We've already loaded this stage.
	if (stage == _npchunk_Stage && 
		!ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_np_chunk", false))
		return; // NP chunk stage is explicitly disabled.
	if (stage == _dependencyParse_Stage &&
		!ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_dependency_parsing", false))
		return; 
	if (stage == _actorMatch_Stage &&
		!ParamReader::isParamTrue("do_actor_match"))
		return;
	try {
		cout << "Initializing Stage " << stage.getSequenceNumber() 
			<< " (" << stage.getName() << ")..." << std::endl;
		stageLoadTimer[stage].startTimer();

		if (stage == _tokens_Stage) loadTokenizerModels();
		else if (stage == _partOfSpeech_Stage) loadPartOfSpeechModels();
		else if (stage == _names_Stage) loadNameModels();
		else if (stage == _nestedNames_Stage) loadNestedNameModels();
		else if (stage == _values_Stage) loadValueModels();
		else if (stage == _parse_Stage) loadParseModels();
		else if (stage == _npchunk_Stage) loadNPChunkModels();
		else if (stage == _dependencyParse_Stage) loadDependencyParseModels();
		else if (stage == _mentions_Stage) loadMentionModels();
		else if (stage == _actorMatch_Stage) loadActorMatchModels();
		else if (stage == _props_Stage) loadPropositionModels();
		else if (stage == _metonymy_Stage) loadMetonymyModels();
		else if (stage == _entities_Stage) loadEntityModels();
		else if (stage == _events_Stage) loadEventModels();
		else if (stage == _relations_Stage) loadRelationModels();
		else
			throw InternalInconsistencyException("SentenceDriver::loadModelsForStage",
				"Unexpected value for 'stage'");

		stageLoadTimer[stage].stopTimer();
	}
	catch (UnrecoverableException &e) {
		std::stringstream prefix;
		prefix << "While initializing for stage " << stage.getName() << ": ";
		e.prependToMessage(prefix.str().c_str());
		throw;
	}
}

void SentenceDriver::loadTokenizerModels() {
	_max_token_sequences = ParamReader::getRequiredIntParam("token_branch");
	_tokenizer = Tokenizer::build();
	_tokenSequenceBuf = _new TokenSequence*[_max_token_sequences];
	//Take care of 'bigram' morph selection with tokenization
	_do_morph_selection = ParamReader::isParamTrue("do_bigram_morph_selection");
	if (_do_morph_selection)
		_morphSelector = _new MorphSelector();
	_do_mt_normalization = ParamReader::isParamTrue("do_mt_normalization");
	if (_do_mt_normalization)
		_mtNormalizer = MTNormalizer::build();
}

void SentenceDriver::loadPartOfSpeechModels() {
	_max_pos_sequences = ParamReader::getRequiredIntParam("pos_branch");
	_posRecognizer = PartOfSpeechRecognizer::build();
	_partOfSpeechSequenceBuf = _new PartOfSpeechSequence*[_max_pos_sequences];
}

void SentenceDriver::loadNameModels() {
	_max_name_theories = ParamReader::getRequiredIntParam("name_branch");
	_nameRecognizer = NameRecognizer::build();
	_nameTheoryBuf = _new NameTheory*[_max_name_theories];
}

void SentenceDriver::loadNestedNameModels() {
	_max_nested_name_theories = ParamReader::getRequiredIntParam("nested_name_branch");
	_nestedNameRecognizer = NestedNameRecognizer::build();
	_nestedNameTheoryBuf = _new NestedNameTheory*[_max_nested_name_theories];
}

void SentenceDriver::loadValueModels() {
	_max_value_sets = ParamReader::getRequiredIntParam("value_branch");
	_valueRecognizer = ValueRecognizer::build();
	_valueSetBuf = _new ValueMentionSet*[_max_value_sets];
}

void SentenceDriver::loadParseModels() {
	_max_parses = ParamReader::getRequiredIntParam("parse_branch");
	_parser = Parser::build();
	_parseBuf = _new Parse*[_max_parses];
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("mt_result_parse_n_best", false))
		_mtResultSaver = _new MTResultSaver();
}

void SentenceDriver::loadNPChunkModels() {
	_npChunkFinder = NPChunkFinder::build();
	_npChunkBuf = _new NPChunkTheory*[1];
	_npChunkBuf[0] = 0;
	_max_np_chunk = 1;
}

void SentenceDriver::loadDependencyParseModels() {
	_dependencyParser = _new DependencyParser();
	_dependencyParseBuf = _new DependencyParseTheory*[1];
	_dependencyParseBuf[0] = 0;
	_max_dependency_parses = 1;
}

void SentenceDriver::loadMentionModels() {
	_max_mention_sets = ParamReader::getRequiredIntParam("desc_branch");
	bool doStage = ParamReader::getRequiredTrueFalseParam("do_desc_recognition");
	_descriptorRecognizer = _new DescriptorRecognizer(doStage);
	_mentionSetBuf = _new MentionSet*[_max_mention_sets];
}

void SentenceDriver::loadActorMatchModels() {
	_actorMentionFinder = _new ActorMentionFinder(ActorMentionFinder::ACTOR_MATCH);
}

void SentenceDriver::loadPropositionModels() {
	_propositionFinder = _new PropositionFinder();
}

void SentenceDriver::loadMetonymyModels() {
	_metonymyAdder = MetonymyAdder::build();
}

void SentenceDriver::loadEntityModels() {
	_dummyReferenceResolver = _new DummyReferenceResolver();
}

void SentenceDriver::loadEventModels() {
	_eventFinder = _new EventFinder(_symbol_sentence);
	_eventFinder->allowMentionSetChanges();
}

void SentenceDriver::loadRelationModels() {
	_relationFinder = RelationFinder::build();
	_relationFinder->allowMentionSetChanges();
}

SentenceDriver::~SentenceDriver() {
	if (_sessionProgram != 0)
		endBatch();
	if (_parser != 0) {
        _parser->writeCaches();
	}
	delete _tokenizer;
	delete _posRecognizer;
	delete _morphAnalysis;
	delete _nameRecognizer;
	delete _nestedNameRecognizer;
	delete _valueRecognizer;
	delete _parser;
	delete _descriptorRecognizer;
	delete _actorMentionFinder;
	delete _propositionFinder;
	delete _metonymyAdder;
	delete _dummyReferenceResolver;
	delete _relationFinder;
	delete _eventFinder;
	delete _morphSelector;
	delete _mtNormalizer;
	delete _npChunkFinder;
	delete _mtResultSaver;
	delete _dependencyParser;

	delete[] _tokenSequenceBuf;
	delete[] _nameTheoryBuf;
	delete[] _nestedNameTheoryBuf;
	delete[] _valueSetBuf;
	delete[] _parseBuf;
	delete[] _mentionSetBuf;
	delete[] _npChunkBuf;
	delete[] _dependencyParseBuf;
	delete[] _partOfSpeechSequenceBuf;
	// _propositionSetBuf is not an array, so there's nothing to delete
	// _entitySetBuf is no longer an array, so there's nothing to delete
	// _relationSetBuf is not an array, so there's nothing to delete
	// _eventSetBuf is not an array, so there's nothing to delete
	// _actorMentionSetBuf is not an array, so there's nothing to delete
}

void SentenceDriver::initIntentionalFailures() {
	for (int i = 0; i < MAX_INTENTIONAL_FAILURES; i++)
		_intentional_failures[i] = false;

	Symbol intentional_failure_val =
		ParamReader::getParam(L"intentional_failures");
	if (intentional_failure_val.is_null())
		return;

	cout << "Intenionally failing on docs: ";

	wstring str(intentional_failure_val.to_string());
	while (str.length() > 0) {
		unsigned i;
		swscanf(str.c_str(), L"%u", &i);
		cout << i << " ";
		if (i < MAX_INTENTIONAL_FAILURES)
			_intentional_failures[i] = true;

		wstring::size_type next = str.find(L',');
		if (next == str.npos)
			str = L"";
		else
			str = str.substr(next + 1);
	}
	cout << "\n";
}



void SentenceDriver::clearLine() { cout << "\r                                                                        \r"; }
//static void clearLine() { }

void SentenceDriver::beginBatch(const SessionProgram *sessionProgram, SessionLogger *sessionLogger, StateLoader *stateLoader, bool use_lexicon_file) {
	if ((_sessionProgram != 0) || (_sessionLogger != 0) || (_stateLoader != 0))
		throw InternalInconsistencyException("SentenceDriver::beginBatch",
			"Expected session program, session logger, and state loader to be NULL"
			" -- was endBatch() not called?");

	// Record the session program, session logger, and state loader.  We are taking ownership of
	// the state loader, but we do *not* own the session program or the session logger.
	_sessionProgram = sessionProgram;
	_sessionLogger = sessionLogger;
	_stateLoader = stateLoader;
	_use_lexicon_file = use_lexicon_file;

	// If we don't have an experiment directory, then disable the lexicon file --
	// there would be no where to save it.
	if (!_sessionProgram->hasExperimentDir())
		_use_lexicon_file = false;

	//Always Initialize the Morphological Analyzer,
	//Because we need access to the lexion in all stages
	if (_morphAnalysis == 0)
		_morphAnalysis = MorphologicalAnalyzer::build();

	// Initialize any state savers we'll be using
	makeNewStateSavers();

	// Load any models that we'll need to process this batch.
	if (!(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_lazy_model_loading", false))) {
		Stage startStage = _sessionProgram->getStartStage();
		Stage endStage = _sessionProgram->getEndStage();
		for (Stage stage=startStage; stage<=endStage; ++stage) {
			if (_sessionProgram->includeStage(stage))
				loadModelsForStage(stage);
		}
	}

	//Now read in extra lexical info if needed
	if ((Stage("tokens") < _sessionProgram->getStartStage())) {
        _morphAnalysis->resetDictionary(); // RPB: Distillation can run multiple batches, so we need to start with a clean slate
		//read in the extra dictionary
		if (_use_lexicon_file) {
			std::wostringstream dict_file;
			dict_file << _sessionProgram->getExperimentDir() << "/lexical-state";
			std::string dict_file_narrow = OutputUtil::convertToUTF8BitString(dict_file.str().c_str());
			_morphAnalysis->readNewVocab(dict_file_narrow.c_str());
		}
	}
	//cout << "\n";
}

void SentenceDriver::clearStageStateSavers() {
	Stage::HashMap<StateSaver*>::iterator it;
	for (it=_stageStateSavers.begin(); it!=_stageStateSavers.end(); ++it)
		delete (*it).second;
	_stageStateSavers.clear();
}

void SentenceDriver::endBatch() {
	if (_sessionProgram == 0)
		return;

	// get rid of the stuff we made in beginBatch()
	delete _stateLoader;
	_stateLoader = 0;

	clearStageStateSavers();

	//save the new lexicon here,
	if (_use_lexicon_file) {
		wstringstream dict_file;
		dict_file << _sessionProgram->getExperimentDir() << "/lexical-state";
		UTF8OutputStream uos(dict_file.str().c_str());
		SessionLexicon::getInstance().getLexicon()->dumpDynamicVocab(uos);
		uos.close();
	}

	// zero-out our pointers to the session program and session logger, 
	// since we should never access these outside of a batch.  (We don't
	// own either of these objects.)
	_sessionProgram = 0;
	_sessionLogger = 0;
}


void SentenceDriver::printDocName(const wchar_t *document_name) {
	boost::posix_time::ptime currentTime(boost::posix_time::second_clock::local_time());
	string timeStamp = boost::posix_time::to_simple_string(currentTime);
	timeStamp = timeStamp.substr(timeStamp.find(" ") + 1, timeStamp.find("."));
	cout << "Processing #" << _n_docs_processed 
		 << " (" << timeStamp << "): " 
		 << OutputUtil::convertToChar(document_name) << "..." 
		 << std::endl;
}

void SentenceDriver::beginDocument(DocTheory *docTheory) {

	// put here a call to any component that needs to
	// be initialized before document processing
	if (_nameRecognizer != 0)
		_nameRecognizer->resetForNewDocument(docTheory);
	if (_nestedNameRecognizer != 0)
		_nestedNameRecognizer->resetForNewDocument(docTheory);
	if (_valueRecognizer != 0)
		_valueRecognizer->resetForNewDocument(docTheory);
	if (_descriptorRecognizer != 0)
		_descriptorRecognizer->resetForNewDocument(docTheory);
	if (_parser != 0)
		_parser->resetForNewDocument(docTheory);
	if (_actorMentionFinder != 0)
		_actorMentionFinder->resetForNewDocument();
	if (_metonymyAdder != 0)
		_metonymyAdder->resetForNewDocument(docTheory);
	if (_dummyReferenceResolver != 0)
		_dummyReferenceResolver->resetForNewDocument(docTheory);
	if(_mtResultSaver != 0 && _sessionProgram->hasExperimentDir())
		_mtResultSaver->resetForNewDocument(docTheory, _sessionProgram->getOutputDir());
}

void SentenceDriver::endDocument() {
	_n_docs_processed++;

	//mrf -8-04, allow dictionary reset to prevent memory problems
	_morphAnalysis->resetDictionary();

#ifdef SERIF_SHOW_PROGRESS
	clearLine();
#endif

	// put here a call to any component that needs to
	// be cleaned up after document processing
	if (_nameRecognizer != 0)
		_nameRecognizer->cleanUpAfterDocument();
	if (_mtResultSaver != 0)
		_mtResultSaver->cleanUpAfterDocument();
}

SentenceTheory *SentenceDriver::run(DocTheory *docTheory, int sent_no,
									const Sentence *sentence)
{
	if (docTheory->getSentence(sent_no) != sentence)
		throw InternalInconsistencyException("SentenceDriver::run",
			"Sentence doesn't match sentence from docTheory");

	Stage startStage = _sessionProgram->getStartStage();
	Stage endStage = _sessionProgram->getEndStage();
	Stage loadStage = startStage;
	--loadStage; // load output of previous stage

	// we don't belong here... get out!
	if (Stage::getLastSentenceLevelStage() < startStage)
		return 0;

	SentenceTheoryBeam *currentBeam = 0;

	if (startStage > _tokens_Stage) {
		// Starting from saved state file, so build
		// beam from state tree
		clearLine();
		cout << "Loading beam state for sentence "
			 << sentence->getSentNumber() << " after stage "
			 << loadStage.getName() << "...";

		currentBeam = loadBeamState(loadStage, sent_no, docTheory);
	}
	docTheory->setSentenceTheoryBeam(sent_no, currentBeam);

	SentenceTheoryBeam *beam = run(docTheory, sent_no, startStage, endStage);

	// Now we return the best theory.
	// We also have to save it from getting deleted when the beam goes away
	// so we must call extractBestTheory(), which takes it out of the beam,
	// and deletes the rest of the beam.
	SentenceTheory *result = beam->extractBestTheory();
	delete beam;
	return result;

}

// Todo: check to make sure that we're initialized for the given range
// of stages -- e.g. it would be bad if we were called with a startstage
// of names if we only initialized the stages after mentions.
SentenceTheoryBeam *SentenceDriver::run(DocTheory *docTheory, int sent_no,
									Stage startStage, Stage endStage)
{
	// If we're using lazy model loading, then make sure the models are loaded now.
	for (Stage stage=startStage; stage<=endStage; ++stage) {
		if (_sessionProgram->includeStage(stage))
			loadModelsForStage(stage);
	}

	const Sentence* sentence = docTheory->getSentence(sent_no);

	if (_n_docs_processed < MAX_INTENTIONAL_FAILURES &&
		_intentional_failures[_n_docs_processed])
	{
		_n_docs_processed++; // because endDocument() gets no chance to
		throw UnexpectedInputException(
			"SentenceDriver::run()",
			"Skipping prespecified document");
	}

	SentenceTheoryBeam *currentBeam = docTheory->getSentenceTheoryBeam(sent_no);

	// If we're starting from scratch, then create a beam with an empty theory.
	if (currentBeam == 0) {
		currentBeam = _new SentenceTheoryBeam(sentence, _beam_width);
		currentBeam->addTheory(_new SentenceTheory(sentence, _primary_parse, docTheory->getDocument()->getName()));
		docTheory->setSentenceTheoryBeam(sent_no, currentBeam);
	}

	// limit stage range to sentence-level stages
	if (startStage < _tokens_Stage)
		startStage = _tokens_Stage;
	if (Stage::getLastSentenceLevelStage() < endStage)
		endStage = Stage::getLastSentenceLevelStage();

	// This loop takes the sentence through each stage
	for (Stage stage = startStage; stage <= endStage; ++stage) {
		if (!_sessionProgram->includeStage(stage))
			continue;

		_sessionLogger->updateLocalContext(STAGE_CONTEXT, stage.getName());

		char source[100];
		sprintf(source, "SentenceDriver::run(); before stage %s",
						stage.getName());
#if defined(_WIN32)
//		HeapChecker::checkHeap(source);
#endif
#ifdef SERIF_SHOW_PROGRESS
		clearLine();
		cout << "Processing sentence " << sentence->getSentNumber()
			 << "/" << docTheory->getNSentences()
			 << ": " << stage.getName() << "...";
#endif

		stageProcessTimer[stage].startTimer();

		// notify each subtheory generator that we're starting a new sentence
		if (stage == _tokens_Stage) {
			_tokenizer->resetForNewSentence(docTheory->getDocument(),
											sentence->getSentNumber());
			_morphAnalysis->resetForNewSentence();

			if (_do_morph_selection)
				_morphSelector->resetForNewSentence();

			if (_do_mt_normalization)
				_mtNormalizer->resetForNewSentence();
		}

		else if (stage == _partOfSpeech_Stage) {
			_posRecognizer->resetForNewSentence();
		}

		else if (stage == _names_Stage) {
			_nameRecognizer->resetForNewSentence(sentence);
		}
		else if (stage == _nestedNames_Stage) {
			_nestedNameRecognizer->resetForNewSentence(sentence);
		}
		else if (stage == _values_Stage) {
			_valueRecognizer->resetForNewSentence();
		}
		else if (stage == _parse_Stage) {
			_parser->resetForNewSentence();
		}
		else if (stage == _npchunk_Stage) {
			if (_npChunkFinder)
				_npChunkFinder->resetForNewSentence();
		}
		else if (stage == _dependencyParse_Stage) {
			if (_dependencyParser)
				_dependencyParser->resetForNewSentence();
		}
		else if (stage == _mentions_Stage) {
			_descriptorRecognizer->resetForNewSentence();
		}
		else if (stage == _actorMatch_Stage) {
			if (_actorMentionFinder)
				_actorMentionFinder->resetForNewSentence();
		}
		else if (stage == _props_Stage) {
			_propositionFinder->resetForNewSentence(sentence->getSentNumber());
		}
		else if (stage == _metonymy_Stage) {
			_metonymyAdder->resetForNewSentence();
		}
		else if (stage == _entities_Stage) {
			_dummyReferenceResolver->resetForNewSentence(docTheory, sentence->getSentNumber());
		}
		else if (stage == _events_Stage && _use_sentence_level_event_finding) {
			_eventFinder->resetForNewSentence(docTheory, sentence->getSentNumber());
		}
		else if (stage == _relations_Stage && _use_sentence_level_relation_finding) {
			_relationFinder->resetForNewSentence(docTheory, sentence->getSentNumber());
		}


		// For each old theory, combine with new subtheory(ies) and put
		// resulting theory into new beam.
		SentenceTheoryBeam *nextBeam = 0;

		for (int theory_no = 0;
			 theory_no < currentBeam->getNTheories();
			 theory_no++)
		{
			sprintf(source, "SentenceDriver::run(); stage %s; before theory %d",
							stage.getName(), theory_no);
#if defined(_WIN32)
//			HeapChecker::checkHeap(source);
#endif
			SentenceTheory *currentTheory = currentBeam->getTheory(theory_no);
			//always set the lexicon
			currentTheory->setLexicon(SessionLexicon::getInstance().getLexicon());


			// create new subtheory(ies) depending on what stage we're on
			if (stage == _tokens_Stage) {         // *** Tokenization
				_n_token_sequences = _tokenizer->getTokenTheories(
					_tokenSequenceBuf, _max_token_sequences,
					sentence->getString());
				for(int i =0; i< _n_token_sequences; i++){
					_morphAnalysis->getMorphTheories(_tokenSequenceBuf[i]);
				}
				
				if (_do_morph_selection) {
			 		for(int i=0; i<_n_token_sequences; i++){
						_morphSelector->selectTokenization(sentence->getString(), _tokenSequenceBuf[i]);
					}
				}

				if (_do_mt_normalization) {
			 		for(int i=0; i<_n_token_sequences; i++){
						_mtNormalizer->normalize(_tokenSequenceBuf[i]);
					}
				}

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _tokens_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::TOKEN_SUBTHEORY, _n_token_sequences,
					(SentenceSubtheory **) _tokenSequenceBuf);

			}

			else if (stage == _partOfSpeech_Stage) {     // *** Part of Speech Recognition
				_n_pos_sequences = _posRecognizer->getPartOfSpeechTheories(
					_partOfSpeechSequenceBuf, _max_pos_sequences,
					currentTheory->getTokenSequence());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _pos_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::POS_SUBTHEORY, _n_pos_sequences,
					(SentenceSubtheory **) _partOfSpeechSequenceBuf);
			}

			else if (stage == _names_Stage) {     // *** Name Recognition
				_n_name_theories = _nameRecognizer->getNameTheories(
					_nameTheoryBuf, _max_name_theories,
					currentTheory->getTokenSequence());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _names_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::NAME_SUBTHEORY, _n_name_theories,
					(SentenceSubtheory **) _nameTheoryBuf);
			}
			else if (stage == _nestedNames_Stage) {     // *** NestedName Recognition
				_n_nested_name_theories = _nestedNameRecognizer->getNestedNameTheories(
					_nestedNameTheoryBuf, _max_nested_name_theories,
					currentTheory->getTokenSequence(),
					currentTheory->getNameTheory());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _nested_names_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::NESTED_NAME_SUBTHEORY, _n_nested_name_theories,
					(SentenceSubtheory **) _nestedNameTheoryBuf);
			}
			else if (stage == _values_Stage) {  // *** Value Recognition
				_n_value_sets = _valueRecognizer->getValueTheories(
					_valueSetBuf, _max_value_sets,
					currentTheory->getTokenSequence());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _values_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::VALUE_SUBTHEORY, _n_value_sets,
					(SentenceSubtheory **) _valueSetBuf);
			}
			else if (stage == _parse_Stage) {     // *** Parsing
				TokenSequence* ts = currentTheory->getTokenSequence();
				//_morphAnalysis->getMorphTheories(ts);


				if (_use_npchunker_constraints && currentTheory->getNPChunkTheory()) {
					int num_chunk_constraints = 0;
					Constraint* chunk_constraints = LanguageSpecificFunctions::getConstraints(currentTheory->getNPChunkTheory(), ts, num_chunk_constraints);
					_n_parses = _parser->getParses(
						_parseBuf, _max_parses,
						currentTheory->getTokenSequence(),
						currentTheory->getPartOfSpeechSequence(),
						currentTheory->getNameTheory(),
						currentTheory->getNestedNameTheory(),
						currentTheory->getValueMentionSet(),
						chunk_constraints,
						num_chunk_constraints);
					delete [] chunk_constraints;

				} else {
					_n_parses = _parser->getParses(
						_parseBuf, _max_parses,
						currentTheory->getTokenSequence(),
						currentTheory->getPartOfSpeechSequence(),
						currentTheory->getNameTheory(),
						currentTheory->getNestedNameTheory(),
						currentTheory->getValueMentionSet());
				}

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _parse_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::PARSE_SUBTHEORY, _n_parses,
					(SentenceSubtheory **) _parseBuf);

			}
			else if (stage == _npchunk_Stage) {     // *** NPChunking
				if (_npChunkFinder) {
					if (_use_npchunker_constraints) {

						_n_np_chunk= _npChunkFinder->getNPChunkTheories(_npChunkBuf, 1,
							currentTheory->getTokenSequence(), 
							currentTheory->getPartOfSpeechSequence(),
							currentTheory->getNameTheory());
					} else {
						_n_np_chunk= _npChunkFinder->getNPChunkTheories(_npChunkBuf, 1,
							currentTheory->getTokenSequence(), 
							currentTheory->getFullParse(),
							currentTheory->getNameTheory());
					}

					if (nextBeam == 0)
						nextBeam = _new SentenceTheoryBeam(sentence, _npchunk_beam_width);
					addSubtheories(nextBeam, currentTheory,
						SentenceTheory::NPCHUNK_SUBTHEORY, _n_np_chunk,
						(SentenceSubtheory **) _npChunkBuf);
				}
			}
			else if (stage == _dependencyParse_Stage) {     // *** Dependency Parsing
				if (_dependencyParser) {
					//_n_dependency_parses = _dependencyParser->convertFullParse(_dependencyParseBuf, 1, currentTheory->getFullParse());
					_n_dependency_parses = _dependencyParser->parse(_dependencyParseBuf, 1, currentTheory->getFullParse(), currentTheory->getTokenSequence(), docTheory->getDocument()->getName());

					if (nextBeam == 0)
						nextBeam = _new SentenceTheoryBeam(sentence, _dependency_parse_beam_width);
					addSubtheories(nextBeam, currentTheory,
						SentenceTheory::DEPENDENCY_PARSE_SUBTHEORY, _n_dependency_parses,
						(SentenceSubtheory **) _dependencyParseBuf);
				}
			}
			else if (stage == _mentions_Stage) {  // *** Desc Recognition
				if (currentTheory->getPrimaryParse() == 0){
					SessionLogger::info("SERIF")<<"no primary parse!"<<std::endl;
				}
				_n_mention_sets = _descriptorRecognizer->getDescriptorTheories(
					_mentionSetBuf, _max_mention_sets,
					currentTheory->getPartOfSpeechSequence(),
					currentTheory->getPrimaryParse(),
					currentTheory->getNameTheory(),
					currentTheory->getNestedNameTheory(),
					currentTheory->getTokenSequence(),
					sentence->getSentNumber());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _mentions_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::MENTION_SUBTHEORY, _n_mention_sets,
					(SentenceSubtheory **) _mentionSetBuf);
				_descriptorRecognizer->cleanup();

			}
			else if (stage == _actorMatch_Stage) {
				if (_actorMentionFinder)
					_actorMentionSetBuf = _actorMentionFinder->process(currentTheory, docTheory);
				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _actor_match_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::ACTOR_MENTION_SET_SUBTHEORY, 1,
					(SentenceSubtheory **) &_actorMentionSetBuf);
			}
			else if (stage == _props_Stage) {  // *** Proposition Finding
				_propositionSetBuf = _propositionFinder->getPropositionTheory(
					currentTheory->getPrimaryParse(),
					currentTheory->getMentionSet());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _props_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::PROPOSITION_SUBTHEORY, 1,
					(SentenceSubtheory **) &_propositionSetBuf);
			}
			else if (stage == _metonymy_Stage) {  // *** Metonymy Adding
				_metonymyAdder->addMetonymyTheory(
					currentTheory->getMentionSet(),
					currentTheory->getPropositionSet());
				// modifies the existing beam, so no need to create a new one
			}
			else if (stage == _entities_Stage) {  // *** Reference Resolution
				_entitySetBuf = _dummyReferenceResolver->createDefaultEntityTheory(
					currentTheory->getMentionSet());
				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _entities_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::ENTITY_SUBTHEORY, 1,
					(SentenceSubtheory **) &_entitySetBuf);


			}
			else if (stage == _events_Stage) {  // *** Event Finding
				if (!_use_sentence_level_event_finding)
					_eventSetBuf = _new EventMentionSet(currentTheory->getPrimaryParse());
				else _eventSetBuf = _eventFinder->getEventTheory(
					currentTheory->getTokenSequence(),
					currentTheory->getValueMentionSet(),
					currentTheory->getPrimaryParse(),
					currentTheory->getMentionSet(),
					currentTheory->getPropositionSet());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _events_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::EVENT_SUBTHEORY, 1,
					(SentenceSubtheory **) &_eventSetBuf);
			}
			else if (stage == _relations_Stage) {  // *** Relation Finding
				if (!_use_sentence_level_relation_finding)
					_relationSetBuf = _new RelMentionSet();
				else _relationSetBuf = _relationFinder->getRelationTheory(
					0, currentTheory,
					currentTheory->getPrimaryParse(),
					currentTheory->getMentionSet(),
					currentTheory->getValueMentionSet(),
					currentTheory->getPropositionSet(),
					currentTheory->getFullParse());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _relations_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::RELATION_SUBTHEORY, 1,
					(SentenceSubtheory **) &_relationSetBuf);
			}
			else {
				// We don't do anything for this stage, so there's no point
				// in continuing for the other theories
				break;
			}
		}

		// Now that we're out of the stage loop, let the session logger know
		// not to print out stage context info
		_sessionLogger->updateLocalContext(STAGE_CONTEXT, "");

		// If a new beam was created, adopt it as the current beam.
		// (Note: the docTheory, as the owner of the old beam, is
		// responsible for deleting it.)
		if (nextBeam != 0) {
			docTheory->setSentenceTheoryBeam(sent_no, nextBeam);
			currentBeam = nextBeam;
		}

		// When a component needs to bail out, but manages not to crash, it
		// returns 0 subtheories. This results in a 0-sized beam. We need to
		// add a "default" theory, which simply says nothing, so there is at
		// least a valid beam.
		// IH, 3/3/11: 
		// A change in the logic of addSubtheories makes this step unnecessary
		// and removes the side-effect of previous subtheories being erased.
		// We don't ever want this to be called, if it is, exit Serif.
		if (currentBeam->getNTheories() == 0) {
			std::string message("Found no subtheories for stage <"	);
			message.append(stage.getName());
			message.append(">. Is there a model specified?\n");
			SessionLogger::err("no_subtheories") << message;
			throw 
				InternalInconsistencyException("SentenceDriver::run",
					message.c_str());
		}
		
		
		stageProcessTimer[stage].stopTimer();

		saveBeamState(stage, currentBeam, sentence->getSentNumber());
		if (stage == _parse_Stage && _mtResultSaver != 0) 
			_mtResultSaver->produceSentenceOutput(sent_no, currentBeam);
	}

	return currentBeam;
}

/** This populates nextBeam with new theories based on currentTheory, plus
  * one of the subtheories specified in newSubtheories.
  * subtheoryType gives the type of those subtheories, and n_new_subtheories
  * gives their number (the size of the array), which determines the number
  * of new theories that it tries to add to nextBeam.
  */
void SentenceDriver::addSubtheories(
	SentenceTheoryBeam *nextBeam, const SentenceTheory *currentTheory,
	SentenceTheory::SubtheoryType subtheoryType,
	int n_new_subtheories, SentenceSubtheory *const *newSubtheories)
{
		
	for (int i = 0; i < n_new_subtheories; i++) {
		// create new sentence theory based on currentTheory, plus
		// subtheory i:
		SentenceTheory *newTheory = _new SentenceTheory(*currentTheory);	
		newTheory->adoptSubtheory(subtheoryType, newSubtheories[i]);


		// Due to an unfortunate design wart, it is necessary to use the
		// MentionSet as well as the EntitySet returned by the reference
		// recognizer.
		if (subtheoryType == SentenceTheory::ENTITY_SUBTHEORY) {
			const EntitySet *entitySet =
						static_cast<EntitySet *>(newSubtheories[i]);
			newTheory->adoptSubtheory(SentenceTheory::MENTION_SUBTHEORY,
									  _new MentionSet(*entitySet->getLastMentionSet()));
		
		}
		nextBeam->addTheory(newTheory);

	}
}


SentenceTheoryBeam *SentenceDriver::loadBeamState(Stage stage,
												  int sent_no,
												  DocTheory *docTheory)
{
	if (_stateLoader == 0) {
		throw InternalInconsistencyException(
			"SentenceDriver::loadBeamState()",
			"State loader needed but not available.");
	}

	wchar_t state_tree_name[100];
	getStateTreeName(state_tree_name, stage);
	_stateLoader->beginStateTree(state_tree_name);

	int file_sent_no = _stateLoader->loadInteger();
	if (file_sent_no != sent_no) {
		throw UnexpectedInputException("SentenceDriver::loadBeamState()",
			"State file has wrong sentence number");
	}

	// EMB 6/20/05: we can't use the regular _beam_width here
	int current_beam_width = _beam_width;
	if (stage == _tokens_Stage) {				//MRF: 7/5/05 tokens is no longer = start
		current_beam_width = _tokens_beam_width;
	}
	else if (stage == _partOfSpeech_Stage) {
		current_beam_width = _pos_beam_width;
	}
	else if (stage == _names_Stage) {
		current_beam_width = _names_beam_width;
	} else if (stage == _nestedNames_Stage) {
		current_beam_width = _nested_names_beam_width;
	} else if (stage == _values_Stage) {
		current_beam_width = _values_beam_width;
	} else if (stage == _parse_Stage) {
		current_beam_width = _parse_beam_width;
	} else if(stage == _npchunk_Stage){
		current_beam_width = _npchunk_beam_width;
	} else if (stage == _mentions_Stage) {
		current_beam_width = _mentions_beam_width;
	} else if (stage == _props_Stage) {
		current_beam_width = _props_beam_width;
	} else if (stage == _metonymy_Stage) {
		current_beam_width = _props_beam_width;
	} else if (stage == _entities_Stage) {
		current_beam_width = _entities_beam_width;
	} else if (stage == _events_Stage) {
		current_beam_width = _events_beam_width;
	} else if (stage == _relations_Stage) {
		current_beam_width = _relations_beam_width;
	}

	SentenceTheoryBeam *result = _new SentenceTheoryBeam(_stateLoader,
														sent_no,
														docTheory,
														current_beam_width);


	_stateLoader->endStateTree();

	result->resolvePointers(_stateLoader);

	return result;
}

void SentenceDriver::saveBeamState(Stage stage,
								   SentenceTheoryBeam *theoryBeam,
								   int sent_no)
{
	StateSaver *stateSaver = _stageStateSavers[stage];
	if (stateSaver != 0) {
		// Collect ID/pointer pairs from all the objects we're going to save:
		ObjectIDTable::initialize();
		theoryBeam->updateObjectIDTable();

		// Save state tree
		wchar_t state_tree_name[100];
		getStateTreeName(state_tree_name, stage);
		stateSaver->beginStateTree(state_tree_name);

//		stateSaver->saveString(document_name);
		stateSaver->saveInteger(sent_no);

		theoryBeam->saveState(stateSaver);

		stateSaver->endStateTree();
		ObjectIDTable::finalize();
	}
}




StateSaver *SentenceDriver::getStageStateSaver(Stage stage) {
	return _stageStateSavers[stage];
}

void SentenceDriver::getStateTreeName(wchar_t *result, Stage stage) {
	wcscpy(result, L"Sentence theory beam following stage: ");
	const char *p = stage.getName();
	wchar_t *q = result + wcslen(result);
	while ((*q++ = (wchar_t) *p++));
}

void SentenceDriver::setMaxParserSeconds(int maxsecs) {
	_parser->setMaxParserSeconds(maxsecs);
}


// switch to this StateLoader, deleting the old loader
void SentenceDriver::resetStateLoader( StateLoader * stateLoader ) {
	if (_stateLoader != stateLoader) {
		delete _stateLoader;
		_stateLoader = stateLoader;
	}
}

void SentenceDriver::makeNewStateSavers() {
	clearStageStateSavers();
	resetStateSavers();
}

void SentenceDriver::resetStateSavers( const wchar_t * document_name ){
	Stage::HashMap<StateSaver*>::iterator it;
	for (Stage stage=Stage::getStartStage(); stage<=Stage::getEndStage(); ++stage) {
		if(!_sessionProgram->saveStageState(stage)) continue;
		
		if(_sessionProgram->saveSingleDocumentStateFiles()){
			
			if(!document_name) continue;

			wstring state_file = _sessionProgram->constructSingleDocumentStateFile(
											document_name, stage);
			delete _stageStateSavers[stage];
			_stageStateSavers[stage] = _new StateSaver(state_file.c_str(), 
				ParamReader::isParamTrue("binary_state_files"));
			
		} else {
			// set only once
			if(_stageStateSavers[stage]) continue;
			
			_stageStateSavers[stage] = _new StateSaver(
				_sessionProgram->getStateFileForStage(stage),
				ParamReader::isParamTrue("binary_state_files"));
		}
	}
}

void SentenceDriver::cleanup() {
	if (_parser) {
		_parser->cleanup();
	}
	if (_relationFinder) {
		_relationFinder->cleanup();
	}
}

void SentenceDriver::logTrace() {
	Stage i;
	SessionLogger::info("profiling") << "SentenceDriver Load Time: " << endl;
	for (i = Stage("tokens"); i <= Stage::getLastSentenceLevelStage(); ++i) {
		SessionLogger::info("profiling") << i.getName() << "\t" << stageLoadTimer[i].getTime() << " msec" << endl;
	}
	SessionLogger::info("profiling") << endl;
	SessionLogger::info("profiling") << "SentenceDriver Processing Time: " << endl;
	for (i = Stage("tokens"); i <= Stage::getLastSentenceLevelStage(); ++i) {
		SessionLogger::info("profiling") << i.getName() << "\t" << stageProcessTimer[i].getTime() << " msec" << endl;
	}
	SessionLogger::info("profiling") << endl;
}
