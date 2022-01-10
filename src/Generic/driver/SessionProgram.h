// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SESSION_PROGRAM_H
#define SESSION_PROGRAM_H

#include "Generic/driver/Stage.h"
#include "Generic/driver/Batch.h"

#include <wchar.h>
#include <string>


/** SessionProgram represents the information specifying how a single
  * session is executed. It initializes itself based on the params
  * in the parameter file and provides a bunch of accessors to its
  * contents.
  */

class SessionProgram {
public:
	/** Create a new session program, whose settings are read from the
	  * global parameter file.  If you wish to use custom settings, 
	  * you can use the setter methods, such as setExperimentDir() and
	  * setStageRange(), to modify the session before you start processing
	  * a batch of documents. */
	SessionProgram(bool use_state_files=true);
	~SessionProgram();

	//===================================================================
	// Setter Methods methods

	/* Reset the values of all session program variables based on the
	 * values specified in the parameter reader.  This is used by
	 * QueryProcessor, since the experiment directory can change when
	 * we process a new query. */
    void readParamsFromParamReader();

	void setSessionName(const std::wstring& name) { _session_name = name; }

	/** Set this session's experiment directory.  Use the empty string
	  * to use no experiment directory (which will cause no output files
	  * to be written. */
	void setExperimentDir(const std::wstring& dir);
	
	/** Set the range of active stages to start at startStage and end
	  * at endStage (inclusive). */
	void setStageRange(Stage startStage, Stage endStage);

	/** Turn a specific stage on or off.  The session program's begin
	  * and end states will be modified if necessary. */
	void setStageActiveness(Stage stage, bool active);

	/** Turn state saving on or off for a specific stage. */
	void setStateSaverStage(Stage stage, bool save_this_stage);

	//===================================================================
	// Accessor methods (const)

	/** Does this session use an experiment dir?  If not, then no output 
	  * files should be written anywhere. */
	bool hasExperimentDir() const { return _has_experiment_dir; }

	/** Return true if SERIF can save its state after the given stage. */
	static bool stateCanBeSavedFor(Stage stage);

	/** Get the session name (default value is the time we started running) */
	const wchar_t *getSessionName() const { return _session_name.c_str(); }

	/** Get the batch (a list of document files to process) */
	const Batch *getBatch() const { return &_document_batch; }

	/** Get the experiment directory. */
	const wchar_t *getExperimentDir() const;

	/** Get the session log file path. */
	const wchar_t *getLogFile() const;

	/** Return the first stage that should be run. */
	Stage getStartStage() const { return _startStage; }

	/** Return the last stage that should be run. */
	Stage getEndStage() const { return _endStage; }

	/** Return true if this session includes the specified stage. */
	bool includeStage(Stage stage) const;

	/** Return true if a state-file should be written for this stage. */
	bool saveStageState(Stage stage) const;

	/** Return true if we should generate APF output. */
	bool produceApfOutput() const { return _produceApfOutput; };

	/** Return true if we should generate Fact-Browser output. */
	bool produceFBOutput() const { return _produceFBOutput; };

	/** Return true if we should generate Adent output. */
	bool produceAdeptOutput() const { return _produceAdeptOutput; };

	/** Get the path of the file we load to restore a saved state
	  * (returns 0 if we're starting from scratch). */
	const wchar_t *getInitialStateFile() const;

	/** Get the file path where the state after given stage should be
	  * saved. */
	const wchar_t *getStateFileForStage(Stage stage) const;

	/** Get path to directory where human-readable doc theory dumps
	  * should be saved. */
	const wchar_t *getDumpDir() const;

	/** Get path to directory where results should be saved. */
	const wchar_t *getOutputDir() const;

	/** Return true if each document should have its own state files,
	  * or false if all documents should be saved in the same state
	  * file. */
	bool saveSingleDocumentStateFiles() const { return _save_single_doc_state_files; }
	
	/** Get path to file for single-document state file.  This path has the
	  * form: "<DocName>-<StageNum>-<StageName>".  If 'reading' is true, and
	  * this file does not exist, then check if any files exist with the same
	  * DocName and StageName, but a different StageNum.  If such a file is
	  * found, then return it.  (When we make changes to the stage sequence,
	  * the stage numbers corresponding to each stage name may change.) */
	std::wstring constructSingleDocumentStateFile( const std::wstring &docname, Stage stage, bool reading=false ) const;

	/** Return "<outdir>/<file_name>", where <outdir> is the session's
	  * output directory, and <file_name> is the value of the given string. */
	std::wstring constructOutputPath( const std::wstring &file_name ) const {
		return concatPath(_output_dir, file_name); }

	/** Make sure zoner stages are loaded; this may be called from outside SessionProgam
	  * because of order dependencies between this and DocumentDriver (SerifHTTPServer
	  * is inconsistent with main Serif.) */
	static void readZonerStages();

private:
	bool _use_state_files;

	/** Do we have an experiment dir?  If not, then any attempt to ask for a path
	  * that would be inside the experiment dir will given an exception. */
	bool _has_experiment_dir;

	std::wstring _session_name;

	std::wstring _experiment_dir;

	std::wstring _log_file;

	Batch _document_batch;

	Stage _startStage;
	std::wstring _initial_state_file;

	Stage _endStage;

	Stage::HashMap<std::wstring> _state_saver_files;

	Stage::HashMap<bool> _stage_state_saved;
	Stage::HashMap<bool> _stage_activeness;
	bool _save_single_doc_state_files;

	std::wstring _dump_dir;
	std::wstring _output_dir;

	bool _produceApfOutput;
	bool _produceFBOutput;
	bool _produceAdeptOutput;

	//===================================================================
	// Initializer methods

	void readExperimentDir();
	void readLogFile();
	void readBatchFile();
	void readStageRange();
	void readStateSaverFilenames();
	void readInitialStateFile();
	void readStateSaverStages();
	void readDumpDir();
	void readOutputDir();
	void readOutputTypes();
	void initializeICEWSStages();

	//===================================================================
	// Helper methods

	void determineSessionName();
	std::wstring concatPath( const std::wstring& dir_name, const std::wstring& file_name ) const;
	void checkForStageReordering();
	void requireExperimentDir() const;
};


#endif
