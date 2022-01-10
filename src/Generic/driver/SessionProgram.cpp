// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <stdio.h>
#include <iostream>
#include <string>
#include <time.h>
#include <wchar.h>
#include <sstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/icews/EventMentionFinder.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/ICEWSDriver.h"
#include "Generic/icews/ICEWSDocumentReader.h"
#include "Generic/icews/ICEWSOutputWriter.h"
#include "Generic/icews/ActorMentionPattern.h"
#include "Generic/icews/EventMentionPattern.h"
#include "Generic/icews/ICEWSQueueFeeder.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/driver/Batch.h"
#include "Generic/driver/DiskQueueDriver.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"

#include <boost/algorithm/string/predicate.hpp>

SessionProgram::SessionProgram(bool use_state_files): _use_state_files(use_state_files) {
	determineSessionName();
    readParamsFromParamReader();
}

SessionProgram::~SessionProgram() {
}

//======================================================================
// Setter Methods
//======================================================================

void SessionProgram::readParamsFromParamReader() {
	readZonerStages();
	initializeICEWSStages();
	checkForStageReordering();
	readBatchFile();
	readStageRange();
	readStateSaverStages();
	readOutputTypes();
	readExperimentDir();

	// This sets all the paths that are dependent on the experiment dir:
	setExperimentDir(_experiment_dir);

	// Save state together, or seperated by file?
	_save_single_doc_state_files = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("save_single_doc_state_files", false);
}

void SessionProgram::setExperimentDir(const std::wstring& dir) {
	_experiment_dir = dir;
	_has_experiment_dir = !_experiment_dir.empty();

	// Recompute all paths that are relative to the experiment dir.
	if (_has_experiment_dir) {
		readLogFile();
		readStateSaverFilenames();
		readInitialStateFile();
		readDumpDir();
		readOutputDir();
	} else {
		_log_file = L"";
		_dump_dir = L"";
		_output_dir = L"";
		_initial_state_file = L"";
		_state_saver_files.clear();
	}
}

void SessionProgram::setStageRange(Stage startStage, Stage endStage) {
	Stage lastValidStage(ParamReader::getParam("last_valid_stage", "end").c_str());
	if (endStage > lastValidStage) {
		std::stringstream err;
		err << "End stages after " << lastValidStage.getName() << " are not supported.";
		throw UnrecoverableException("SessionProgram::setStageRange()", err.str());
	}
	_startStage = startStage;
	_endStage = endStage;
	_stage_activeness.clear();
	for (Stage s=startStage; s<=endStage; ++s)
		_stage_activeness[s] = true;
}

void SessionProgram::setStageActiveness(Stage stage, bool active) {
	_stage_activeness[stage] = active;

	// Set _startStage to be the first active stage, and _endStage to
	// be the last active stage.
	_startStage = Stage();
	for (Stage s=Stage::getFirstStage(); s<Stage::getEndStage(); ++s) {
		if (_stage_activeness[s]) {
			if (_startStage == Stage())
				_startStage = s;
			_endStage = s;
		}
	}
	// If the start state changed, then we may need to update the value
	// of _initial_state_file.
	readInitialStateFile();
}

void SessionProgram::setStateSaverStage(Stage stage, bool save_this_stage) {
	if (save_this_stage && !stateCanBeSavedFor(stage)) {
		std::stringstream err;
		err << "The stage " << stage.getName() << " does not support state-saving.";
		throw UnexpectedInputException("SessionProgram::setStateSaverStage", 
			err.str().c_str());
	}
	_stage_state_saved[stage] = save_this_stage;
}

//======================================================================
// Accessor Methods (const)
//======================================================================

bool SessionProgram::stateCanBeSavedFor(Stage stage) {
	return stage.okToSaveStateAfterThisStage();
}

const wchar_t *SessionProgram::getExperimentDir() const { 
	requireExperimentDir();
	return _experiment_dir.c_str(); 
}

void SessionProgram::requireExperimentDir() const {
	if (!_has_experiment_dir)
		throw InternalInconsistencyException("SessionProgram::requireExperimentDir",
			"Attempt to access a file in the experiment directory when the "
			"experiment directory is disabled.");
}

/** Get the session log file path. */
const wchar_t *SessionProgram::getLogFile() const { 
	requireExperimentDir();
	return _log_file.c_str(); 
}


bool SessionProgram::includeStage(Stage stage) const{
	Stage::HashMap<bool>::const_iterator it = _stage_activeness.find(stage);
	return ((it!=_stage_activeness.end()) && ((*it).second));
}

bool SessionProgram::saveStageState(Stage stage) const{
	if (!hasExperimentDir())
		return false;
	if (stateCanBeSavedFor(stage)) {
		Stage::HashMap<bool>::const_iterator it = _stage_state_saved.find(stage);
		return ((it!=_stage_state_saved.end()) && ((*it).second));
	} else {
		return 0;
	}
}

const wchar_t *SessionProgram::getDumpDir() const { 
	requireExperimentDir();
	return (_dump_dir.empty()?0:_dump_dir.c_str()); 
}

const wchar_t *SessionProgram::getInitialStateFile() const{
	if (_startStage <= Stage::getFirstStage() ||
		_startStage == Stage("score") ||
		saveSingleDocumentStateFiles() )
	{
		// for per-document state files, the initial statefile
		//   is computed later on a per-document basis
		return 0;
	}
	else if (_initial_state_file.empty()) {
		return 0;
	} else {
		requireExperimentDir();
		return _initial_state_file.c_str();
	}
}

const wchar_t *SessionProgram::getStateFileForStage(Stage stage) const{
	// If we're not saving state for the given stage, return NULL.
	if( !saveStageState(stage))
		return 0;
	
	requireExperimentDir();
	Stage::HashMap<std::wstring>::const_iterator it = _state_saver_files.find(stage);
	if ((it == _state_saver_files.end()) || ((*it).second.empty())) {
		return 0;
	} else {
		requireExperimentDir();
		return (*it).second.c_str();
	}
}

const wchar_t *SessionProgram::getOutputDir() const{
	requireExperimentDir();
	return  (_output_dir.empty() ? 0 : _output_dir.c_str());
}

namespace {
	bool fileExists(const std::wstring& filename) {
		return (!std::ifstream(UnicodeUtil::toUTF8StdString(filename).c_str()).fail());
	}
}

std::wstring SessionProgram::constructSingleDocumentStateFile( const std::wstring& docname, Stage stage, bool reading) const
{
	requireExperimentDir();
	std::wstringstream file_name;
	file_name << docname << "-state-" << stage.getSequenceNumber() << "-" << stage.getName();
	std::wstring path = concatPath(_experiment_dir, file_name.str());
	if (reading && !fileExists(path)) {
		for (int i=0; i<Stage::getEndStage().getSequenceNumber()*2; ++i) {
			std::wstringstream alt_name;
			alt_name << docname << "-state-" << i << "-" << stage.getName();
			std::wstring alt_path = concatPath(_experiment_dir, alt_name.str());
			if (fileExists(alt_path)) return alt_path;
		}
	}
	return path;
}

//======================================================================
// Static Initializer Methods (all public)
//======================================================================

void SessionProgram::readZonerStages() {
	std::vector<std::string> zonerNames = ParamReader::getStringVectorParam("document_zoners");
	BOOST_FOREACH(std::string zonerName, zonerNames) {
		// Insert these stages in order before sentence breaking
		std::stringstream stageName;
		stageName << "zoning-" << zonerName;
		std::stringstream stageDesc;
		stageDesc << "Identify regions or zones at the document level using " << zonerName << " method";
		try {
			// Check if this zoner stage is defined
			Stage(stageName.str().c_str());
		} catch (UnexpectedInputException) {
			// Stage doesn't exist, insert it
			Stage zonerStage = Stage::addNewStageBefore(Stage("sent-break"), stageName.str().c_str(), stageDesc.str().c_str());
		}
	}
}

//======================================================================
// Initializer Methods (all private)
//======================================================================

void SessionProgram::readExperimentDir() {

	std::string experiment_dir = ParamReader::getParam("experiment_dir");
	// Note: Empty experiment dir is now ok -- means do not generate file output.
	_experiment_dir = std::wstring(experiment_dir.begin(), experiment_dir.end());
}

// Must be called after _experiment_dir is set.
void SessionProgram::readLogFile() {
	_log_file = concatPath(_experiment_dir, L"session-log.txt");
}

// Must be called after _experiment_dir is set.
void SessionProgram::readDumpDir() {
	_dump_dir = concatPath(_experiment_dir, L"dumps");
}

// Must be called after _experiment_dir is set.
void SessionProgram::readOutputDir() {
	_output_dir = concatPath(_experiment_dir, L"output");
}

// Must be called after _experiment_dir is set.
void SessionProgram::readStateSaverFilenames() {
	for (Stage stage=Stage::getFirstStage(); stage<Stage::getEndStage(); ++stage) {
		if (_use_state_files && stateCanBeSavedFor(stage)) {
			std::wstringstream file_name;
			file_name << "state-" << stage.getSequenceNumber() << "-" << stage.getName();
			_state_saver_files[stage] = concatPath(_experiment_dir, file_name.str());
		} else {
			_state_saver_files[stage] = L"";
		}
	}
}

// This must be called after _experiment_dir is set, and after
// readStateSaverFilenames() is called.
void SessionProgram::readInitialStateFile() {
	if (!_use_state_files)
		_initial_state_file = L"";

	// for START stage we start right from the document, so there is
	// no initial state file
	else if (_startStage <= Stage::getFirstStage())
		_initial_state_file = L"";
	
	// If we've already done output, and all that's left is to do scoring,
	// then we don't need to bother to load the state.
	else if (_startStage == Stage("score")) {
		_initial_state_file = L"";
	}

	// If we're using SerifXML as our input, then there is no initial
	// state file -- the doctheory input comes from SerifXML.
	else if (boost::iequals(ParamReader::getParam("source_format"), "serifxml")) {
		_initial_state_file = L"";
	}

	// Otherwise, we should load our state from the stage just before the
	// start stage.
	else {
		Stage savedStateStage = _startStage.getPrevStage();
		_initial_state_file = _state_saver_files[savedStateStage];
		// If it doesn't exist, then try looking for it with different stagenums.
		if (!fileExists(_initial_state_file)) {
			for (int i=0; i<Stage::getEndStage().getSequenceNumber()*2; ++i) {
				std::wstringstream alt_name;
				alt_name << "state-" << i << "-" << savedStateStage.getName();
				std::wstring alt_path = concatPath(_experiment_dir, alt_name.str());
				if (fileExists(alt_path)) 
					_initial_state_file = alt_path;
			}
		}
		if (_initial_state_file.empty()) {
			char message[500];
			sprintf(message, "Serif cannot restart in stage %s", _startStage.getName());
			throw UnexpectedInputException("SessionProgram::readInitialStateFile()",
				message);
		}
	}
}
void SessionProgram::initializeICEWSStages () {
	// do this init only once, even if called multiple times
	static bool icews_init = false;
	if (icews_init) return;
	icews_init = true;

	// code here was once the ICEWS module, now included always but only turned on with the "run_icews" par
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("run_icews", false)){	
		//cout << "DEBUG -- run_icews true so adding new stages \n ";

		// Add a new stage for actor mention detection
		Stage icewsActorsStage = Stage::addNewStageBefore(Stage("output"), "icews-actors",
			"ICEWS actor mention detection");
		DocumentDriver::addDocTheoryStage<ActorMentionFinder>(icewsActorsStage);

		if (!ParamReader::isParamTrue("skip_icews_events")) {
			// Add a new stage for event mention detection
			Stage icewsEventsStage = Stage::addNewStageBefore(Stage("output"), "icews-events",
				"ICEWS event mention detection");
			DocumentDriver::addDocTheoryStage<ICEWSEventMentionFinder>(icewsEventsStage);
		}

		// Add a new stage for writing events to the database.
		Stage icewsOutputStage = Stage::addNewStageBefore(Stage("output"), "icews-output",
			"Save ICEWS event mentions to the knowledge database");
		DocumentDriver::addDocTheoryStage<ICEWSOutputWriter>(icewsOutputStage);

		// Register ICEWS pattern types
		Pattern::registerPatternType<ICEWSActorMentionPattern>(Symbol(L"icews-actor"));
		Pattern::registerPatternType<ICEWSEventMentionPattern>(Symbol(L"icews-event"));

		// Register queue feeder
		QueueDriver::addImplementation<ICEWSQueueFeeder<DiskQueueDriver> >("icews-disk-feeder");

		// Register the command-line hook.
		addModifyCommandLineHook(boost::shared_ptr<ModifyCommandLineHook>(
			_new ICEWSDriver::ICEWSCommandLineHook()));

		//cout << "DEBUG -- run_icews end of stage adding code\n ";
	}
}

// The batch of files can be specified using one of the following parameters:
//   - input_files: A comma-separated list of document file names
//   - batch_file: The name of a file containing a list of document file names (one per line).
//   - batch_directory: A directory, all of whose files will be processed, optionally subject to...
//   - batch_directory_file_extensions: Valid file extensions to be selected when running with batch_directory (comma-separated list)
void SessionProgram::readBatchFile() {
	std::string batch_file = ParamReader::getParam("batch_file");
	std::string batch_directory = ParamReader::getParam("batch_directory");
	std::vector<std::string> batch_directory_file_suffixes = ParamReader::getStringVectorParam("batch_directory_file_suffixes");
	std::string batch_file_type = ParamReader::getParam("batch_file_type", "file-list");
	std::vector<Symbol> input_files = ParamReader::getSymbolVectorParam("input_files");

	if (!batch_directory.empty()) {
		if (!batch_file.empty() || !input_files.empty()) {
			throw UnexpectedInputException("SessionProgram::readBatchFile()", 
										   "'batch_directory' cannot be specified alongside 'batch_file' or 'input_files' (or input files on the command line)");
		}
		boost::filesystem::path batch_directory_path(batch_directory);
		if (!boost::filesystem::exists(batch_directory_path) || !boost::filesystem::is_directory(batch_directory_path)) {
			std::stringstream errStr;
			errStr << "batch_directory " << batch_directory << " does not exist or is not a directory";
			throw UnexpectedInputException("SessionProgram::readBatchFile()", errStr.str().c_str());
		}
		for (boost::filesystem::directory_iterator itr(batch_directory_path); itr!=boost::filesystem::directory_iterator(); ++itr)
		{
			if (boost::filesystem::is_regular_file(itr->path())) {
				if (batch_directory_file_suffixes.size() != 0) {
					bool has_valid_suffix = false;
					BOOST_FOREACH(std::string ext, batch_directory_file_suffixes) {
						if (boost::algorithm::ends_with(itr->path().string(), ext))
							has_valid_suffix = true;
					}
					if (!has_valid_suffix)
						continue;
				}
				_document_batch.addDocument(UnicodeUtil::toUTF16StdString(itr->path().string()));
			}
		}
	}

	// Note: when Serif is running as a service, batch_file, batch_directory and the
	// input_files parameters will be undefined.  So just leave the 
	// _document_batch empty.
	if (!input_files.empty()) {
		for (size_t i=0; i<input_files.size(); ++i)
			_document_batch.addDocument(input_files[i].to_string());
	}
	
	if (!batch_file.empty()) {
		if (batch_file_type == std::string("file-list"))
			_document_batch.loadFromFile(batch_file.c_str());
		else if (batch_file_type == std::string("segment-list"))
			_document_batch.loadFromSegmentFile(batch_file.c_str());
		else
			throw UnexpectedInputException("SessionProgram::readBatchFile()", 
										   "Parameter 'batch_file_type' must be set to 'file-list' or 'segment-list'");
	}

}

void SessionProgram::readStageRange() {
	std::string start_stage_name = ParamReader::getRequiredParam("start_stage");
	std::string end_stage_name = ParamReader::getRequiredParam("end_stage");
	setStageRange(Stage(start_stage_name.c_str()), Stage(end_stage_name.c_str()));
}

void SessionProgram::readStateSaverStages() {
	std::vector<Symbol> stages = ParamReader::getSymbolVectorParam("state_saver_stages");

	// initialize state-saver files array
	_stage_state_saved.clear();

	for (size_t i=0; i<stages.size(); ++i) {
		if (stages[i] == Symbol(L"NONE")) continue; // To allow for an explicit empty list.
		Stage stage(stages[i].to_debug_string()); // This can throw an exception if it's a bad stage name.
		if (includeStage(stage) && _use_state_files)
			_stage_state_saved[stage] = true;
	}
}

void SessionProgram::readOutputTypes() {
	_produceApfOutput = false;
	_produceFBOutput = false;
	_produceAdeptOutput = false;

	std::vector<Symbol> output_types = ParamReader::getSymbolVectorParam("output_types");
	for (size_t i=0; i<output_types.size(); ++i) {
		if      (output_types[i] == Symbol(L"apf")        ) _produceApfOutput = true;
		else if (output_types[i] == Symbol(L"factbrowser")) _produceFBOutput = true;
		else if (output_types[i] == Symbol(L"adept")      ) _produceAdeptOutput = true;
		else if (output_types[i] == Symbol(L"NONE")       ) continue; 
		else
			throw UnexpectedInputException("SessionProgram::readOutputTypes()", 
				"Bad 'output_types' parameter value (must be a comma"
				"separated list, which can contain the following values:"
				"'apf', 'factbrowser', or 'adept')");
	}
}

//======================================================================
// Helper Methods (all private)
//======================================================================

void SessionProgram::determineSessionName() {
	time_t ltime;
	time(&ltime);

	wchar_t buf[512];
	wcsftime(buf, 512, L"%Y-%m-%d %H:%M:%S", localtime(&ltime));
	_session_name = buf;

	return;
}

std::wstring SessionProgram::concatPath( const std::wstring& dir_name, const std::wstring& file_name ) const
{
	if( dir_name.empty() ) {
		return file_name;
	}
	else {
		std::wstring ret_val(dir_name);
		if( ret_val.substr(ret_val.size()-1,1) != LSERIF_PATH_SEP )
			ret_val += LSERIF_PATH_SEP;
		return ret_val + file_name;
	}
}

void SessionProgram::checkForStageReordering() {
	Stage chunkStage("npchunk");
	Stage parseStage("parse");
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_npchunker_constraints", false)) {
		if (chunkStage > parseStage)
			Stage::swapStageOrder(chunkStage, parseStage);
	} else {
		if (parseStage > chunkStage)
			Stage::swapStageOrder(chunkStage, parseStage);
	}
}
