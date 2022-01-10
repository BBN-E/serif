// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "SerifHTTPServer/SerifWorkQueue.h"
#include "SerifHTTPServer/IncomingHTTPConnection.h"

#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include <time.h>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <iomanip>

//#define WINDOWS_PSAPI

#ifdef WINDOWS_PSAPI
#include <Psapi.h>
#endif

#define MEMORY_USAGE_HISTORY_SIZE 10

// Return a pointer to the singleton SerifWorkQueue instance.
SerifWorkQueue *SerifWorkQueue::getSingletonWorkQueue() {
	static SerifWorkQueue singletonQueue;
	return &singletonQueue;
}

// Constructor -- this should only be called by getSingletonWorkQueue().
SerifWorkQueue::SerifWorkQueue()
: _num_tasks_processed(0), _num_tasks_failed(0),
  _documentDriver(0), _patternSets(0), _waiting_for_task(false),
  _port_from_param_file(-1), _server_info_file_from_param_file(""), _throughput_including_load_time(-1),
  _throughput_excluding_load_time(-1), _initialization_complete(false),
  _status("Initializing Serif"),
  _memory_usage_history(MEMORY_USAGE_HISTORY_SIZE),
  _fatalErrorCallback(0), _shutdown(false)
{
	// Start the main thread.
	_thread = _new boost::thread(boost::bind(&SerifWorkQueue::run, this)); 
}

void SerifWorkQueue::setAssignedPort(int assigned_port) {
	_assigned_port = assigned_port;
}

void SerifWorkQueue::setShutdownCallback(SerifWorkQueue::FatalErrorCallback *fatalErrorCallback) {
	delete _fatalErrorCallback;
	_fatalErrorCallback = fatalErrorCallback;
}

SerifWorkQueue::~SerifWorkQueue() {
	delete _thread;
	delete _documentDriver;
	delete _patternSets;
}

void SerifWorkQueue::initialize() {
	boost::mutex::scoped_lock lock(_mutex);
	if (_documentDriver != 0)
		throw InternalInconsistencyException("SerifWorkQueue::initialize",
			"Already initialized.");
	_port_from_param_file = ParamReader::getOptionalIntParamWithDefaultValue("server_port", 8000);
	_server_info_file_from_param_file = ParamReader::getParam("server_info_file");
	_server_docs_root_from_param_file = ParamReader::getParam("server_docs_root");

	// Make sure zoner stages are defined, if any
	SessionProgram::readZonerStages();

	// Check that we have an experiment directory.
	std::string expt_dir = ParamReader::getParam("experiment_dir");
	if (expt_dir.empty()) {
		// Do not generate any output files.
	}
	if (expt_dir[0] == '+' && expt_dir[expt_dir.size()-1] == '+') {
		throw UnexpectedInputException("SerifWorkQueue::initialize", 
			("The experiment_dir parameter must be specified!  (\""+expt_dir+
			"\" looks like a placeholder value)").c_str());
	}

	// make sure parent-experiment dir and experiment dir exist.
	if (expt_dir.empty()) {
		SessionLogger::setGlobalLogger(new ConsoleSessionLogger(N_CONTEXTS, CONTEXT_NAMES));
	} else {
		size_t index = expt_dir.find_last_of(SERIF_PATH_SEP);
		if (index != std::string::npos)
			OutputUtil::makeDir(expt_dir.substr(0, index).c_str());
		OutputUtil::makeDir(expt_dir.c_str());
		std::string session_logfile = expt_dir + SERIF_PATH_SEP + "session-log.txt";
		SessionLogger::setGlobalLogger(new FileSessionLogger(session_logfile.c_str(), N_CONTEXTS, CONTEXT_NAMES));
	}
	_initialization_complete = true;
	_initialized.notify_one();
}

bool SerifWorkQueue::loadDocumentDriver() {
	{
		boost::mutex::scoped_lock lock(_mutex);
		while (!_initialization_complete)
			_initialized.wait(lock);
	}

	DocumentDriver *documentDriver = 0;
	try {
	//SessionLogger::logger = new ConsoleSessionLogger(N_CONTEXTS, CONTEXT_NAMES);

		std::cerr << "[WorkQueue] Loading document driver..." << std::endl;
		documentDriver = _new DocumentDriver();
		if (!(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_lazy_model_loading", false))) {
			Stage startStage = Stage(ParamReader::getParam("start_stage").c_str());
			Stage endStage = Stage(ParamReader::getParam("end_stage").c_str());
			// Count how many models we need to load.
			int num_models = 1;
			for (Stage stage=startStage; stage<=endStage; ++stage,++num_models);
			// Load the models
			int cur_model = 1;
			for (Stage stage=startStage; stage<=endStage; ++stage,++cur_model) {
				int progress = (cur_model*100+50)/num_models;
				std::ostringstream status;
				status << "Initializing Serif (" << cur_model << "/"
					<< num_models << " stages initialized)";
				_status = status.str();
				documentDriver->loadModelsForStage(stage);
			}
		}
		std::cerr << "[WorkQueue] Done loading document driver." << std::endl;

		// If we have a server_info_file, write our hostname and port to it
		if (_server_info_file_from_param_file.size() > 0) {
			writeToServerInfoFile();
		}

		//delete SessionLogger::logger;
		//SessionLogger::logger = 0;
	} catch (UnrecoverableException &e) {
		delete documentDriver;
		std::cerr << "\n[WorkQueue] " << e.getMessage() << std::endl;
		std::cerr << "[WorkQueue] Error Source: " << e.getSource() << std::endl;
		(*_fatalErrorCallback)();
		return false;
	} catch (std::exception &e) {
		delete documentDriver;
		std::cerr << "\n[WorkQueue] " <<e.what() << std::endl;
		(*_fatalErrorCallback)();
		return false;
	}

	{
		boost::mutex::scoped_lock lock(_mutex);
		_documentDriver = documentDriver;
		_baseline_memory_usage.check();
	}
	return true;
}

bool SerifWorkQueue::loadPatternSets() {
	{
		boost::mutex::scoped_lock lock(_mutex);
		while (!_initialization_complete)
			_initialized.wait(lock);
	}

	std::string patternSetsFile = ParamReader::getParam("pattern_sets");
	if (patternSetsFile.length() == 0) 
		return true; // no pattern sets to load.

	Symbol::HashMap<PatternSet_ptr> *patternSets = 0;
	try {
		std::cerr << "[WorkQueue] Loading pattern sets." << std::endl;
		patternSets = new Symbol::HashMap<PatternSet_ptr>();

		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(patternSetsFile));
		UTF8InputStream& stream(*stream_scoped_ptr);
		std::wstring line;
		while (stream) {
			stream.getLine(line);
			if (!stream) 
				break;
			if (line.size()>0 && line[0]!=L'#') {
				size_t pos = line.find(L' ');
				Symbol patternSetName(line.substr(0, pos).c_str());
				std::string patternFile = ParamReader::expand(UnicodeUtil::toUTF8StdString(line.substr(pos + 1)));
				std::cerr << "[WorkQueue] " << patternSetName.to_debug_string() << ": " << patternFile << "\n";
				PatternSet_ptr ps = boost::make_shared<PatternSet>(patternFile.c_str());
				(*patternSets)[patternSetName] = ps;
			}
		}
		std::cerr << "[WorkQueue] Done loading pattern sets." << std::endl;
	} catch (UnrecoverableException &e) {
		delete patternSets;
		std::cerr << "\n" <<e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		(*_fatalErrorCallback)();
		return false;
	} catch (std::exception &e) {
		delete patternSets;
		std::cerr << "\n" <<e.what() << std::endl;
		(*_fatalErrorCallback)();
		return false;
	}

	{
		boost::mutex::scoped_lock lock(_mutex);
		_patternSets = patternSets;
		_baseline_memory_usage.check();
	}

	return true;
}

int SerifWorkQueue::getPortFromParamFile() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _port_from_param_file;
}

std::string SerifWorkQueue::getServerDocsRootFromParamFile() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _server_docs_root_from_param_file;
}

size_t SerifWorkQueue::numTasksRemaining() const {
	boost::mutex::scoped_lock lock(_mutex);
	if (_waiting_for_task)
		return 0;
	else {
		if (_documentDriver == 0)
			return _tasks.size();
		else
			return 1 + _tasks.size(); // include the current task
	}
}

size_t SerifWorkQueue::numTasksProcessed() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _num_tasks_processed;
}

size_t SerifWorkQueue::numTasksFailed() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _num_tasks_failed;
}

float SerifWorkQueue::getThroughputIncludingLoadTime() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _throughput_including_load_time;
}

float SerifWorkQueue::getThroughputExcludingLoadTime() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _throughput_excluding_load_time;
}

std::string SerifWorkQueue::getStatus() const {
	boost::mutex::scoped_lock lock(_mutex);
	return _status;
}

void SerifWorkQueue::addTask(SerifWorkQueue::Task_ptr task) {
	boost::mutex::scoped_lock lock(_mutex);
	_tasks.push_back(task);
	_tasks_ready.notify_one();
}

SerifWorkQueue::Task_ptr SerifWorkQueue::getNextTask() {
	boost::mutex::scoped_lock lock(_mutex);
	if (_shutdown) return Task_ptr();
	while (_tasks.empty()) {
		_waiting_for_task = true;
		_tasks_ready.wait(lock);
		_waiting_for_task = false;
		if (_shutdown) return Task_ptr();
	}
	Task_ptr task = _tasks.front();
	_tasks.pop_front();
	return task;
}

// This is what the SerifWorkQueue's thread runs:
void SerifWorkQueue::run() {
	if (!loadDocumentDriver() || !loadPatternSets()) {
		return;
	}

	while (true) {
		std::cerr << "[WorkQueue] Waiting for a task." << std::endl;
		_status = "Waiting for a task.";
		Task_ptr task = getNextTask();
		if (!task) break; // Shutdown was requested.
		std::cerr << "[WorkQueue] Performing a task..." << std::endl;
		_status = "Performing a task";
		if (PatternSetTask_ptr psTask = boost::dynamic_pointer_cast<PatternSetTask>(task)) {
			if (psTask->run(_patternSets))
				++_num_tasks_processed;
			else
				++_num_tasks_failed;
		} else {
			if (task->run(_documentDriver))
				++_num_tasks_processed;
			else
				++_num_tasks_failed;
		}
		_throughput_including_load_time = _documentDriver->getThroughput(true);
		_throughput_excluding_load_time = _documentDriver->getThroughput(false);
		recordMemoryUsage();
	}
    // Make sure no condition variables are blocking threads -- otherwise, 
    // destroying them will have undefined effects.
	_tasks_ready.notify_one();
    _initialized.notify_one();
    // Record the fact that we've shut down (in case the user wants to call
    // shutdown(true)).
	_shutdown_complete.notify_one();
}

// This should *not* be called from the queue's thread; only from 
// the server's thread.
void SerifWorkQueue::shutdown(bool wait) {
    boost::mutex::scoped_lock lock(_mutex);
    _shutdown = true;
	_tasks_ready.notify_one();
    if (wait)
        _shutdown_complete.wait(lock);
}

void SerifWorkQueue::recordMemoryUsage() {
	MemoryUsageRecord mem_record;
	mem_record.check();
	mem_record.task_num = _num_tasks_processed+_num_tasks_failed;
	_memory_usage_history.push_back(mem_record);
	_memory_usage_history.pop_front();
}

void SerifWorkQueue::MemoryUsageRecord::check() {
#if defined(WINDOWS_PSAPI)
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
	if ( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) ) {
		crt_block_size = 0;
		normal_block_size = pmc.WorkingSetSize;
	}
    CloseHandle( hProcess );
	time(&time_checked);
#elif defined(ENABLE_LEAK_DETECTION)
#ifdef _WIN32
	std::cout << "Checking memory usage" << std::endl;
	_CrtMemState mem_state = {0,0,0,0,0};
	_CrtMemCheckpoint(&mem_state);
	crt_block_size = mem_state.lSizes[_CRT_BLOCK];
	normal_block_size = mem_state.lSizes[_NORMAL_BLOCK];
	time(&time_checked);
#endif
#endif
}


std::string SerifWorkQueue::getMemoryUsageGraph() const {
	boost::mutex::scoped_lock lock(_mutex);
	std::ostringstream result;

	size_t max_memory_usage = _baseline_memory_usage.totalUsage();
	if (max_memory_usage==0)
		return "";
	BOOST_FOREACH(const MemoryUsageRecord& mem_record, _memory_usage_history) {
		if (mem_record.totalUsage() > max_memory_usage)
			max_memory_usage = mem_record.totalUsage();
	}
	

	result << std::fixed << std::setprecision(1);
	result << "<h2> Memory Usage History </h2>\n";
	//result << "<div class=\"memory-usage\">\n";
	result << "<table class=\"memory-usage\">\n";
	result << "<tr><th>Task#</th><th>Time</th><th>Memory</th><th>Diff vs Baseline</th><th>Bar graph</th></tr>\n";
	size_t prev_mem = addMemoryUsageGraphRow(_baseline_memory_usage, max_memory_usage, _baseline_memory_usage.totalUsage(), result);
	BOOST_FOREACH(const MemoryUsageRecord& mem_record, _memory_usage_history) {
		prev_mem = addMemoryUsageGraphRow(mem_record, max_memory_usage, prev_mem, result);
	}
	//result << "<div style=\"clear: both;\"> </div>\n";
	//result << "</div>\n";
	result << "</table>\n";
	return result.str();
}

size_t SerifWorkQueue::addMemoryUsageGraphRow(const MemoryUsageRecord &mem_record, size_t max_memory_usage, size_t prev_mem, std::ostream &out) const {
	if (mem_record.totalUsage()==0)
		return prev_mem;
	int BAR_WIDTH=400; // Pixels
	int w1 = static_cast<int>(double(mem_record.crt_block_size)*BAR_WIDTH/max_memory_usage);
	int w2 = static_cast<int>(double(mem_record.normal_block_size)*BAR_WIDTH/max_memory_usage);
	int w3 = BAR_WIDTH-w1-w2;
	double mem_diff = 100.0*mem_record.totalUsage()/_baseline_memory_usage.totalUsage()-100.0;

	out << "<tr><td>" << mem_record.task_num << "</td>\n" 
		<< "<td>" << ctime(&mem_record.time_checked) << "</td>\n"
		<< "<td>" << mem_record.totalUsage()/1024.0/1024.0 << "&nbsp;MB</td>\n"
		<< "<td>" << mem_diff << "%</td>\n";
	// Draw the bar.
	out << "<td class=\"mu-bar\" style=\"width:" << BAR_WIDTH << "px\">\n"
		<< "  <div class=\"mu-crt-bar\" style=\"width:" << w1 << "px\">&nbsp;</div>\n"
		<< "  <div class=\"mu-normal-bar\" style=\"width:" << w2 << "px\">&nbsp;</div>\n"
		<< "</td>\n";
	out << "</tr>\n";
	return mem_record.totalUsage();
}
	
SerifWorkQueue::Task::Task(boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection)
: _ioService(ioService), _connection(connection) {}


namespace {
	// These helper methods are used to register callbacks with the server's io service.  
	// They will be called in the server's thread, *not* in the serif work queue's thread, 
	// because we use _ioService to post them to the server thread's asyncronous operation 
	// queue.
	void sendXMLResponseToConnection(IncomingHTTPConnection_ptr connection, xercesc::DOMDocument *xmldoc) {
		connection->sendResponse(xmldoc); }
	void reportErrorToConnection(IncomingHTTPConnection_ptr connection, size_t error_code, std::string explanation) {
		connection->reportError(error_code, explanation); }
	void sendStrResponseToConnection(IncomingHTTPConnection_ptr connection, const std::string& contents) {
		std::stringstream out;
		out << "HTTP/1.0 200 OK\r\n";
		out << "Content-type: text/xml" << "\r\n";
		out << "Content-length: " << contents.size() << "\r\n";
		out << "\r\n"; // End of headers
		out << contents;
		std::string out_str = out.str();
		connection->sendResponse(out_str); 
	}
}

void SerifWorkQueue::Task::reportError(size_t error_code, std::string explanation) {
	_ioService.post(boost::bind(&reportErrorToConnection, _connection, error_code, explanation));
}
void SerifWorkQueue::Task::sendResponse(xercesc::DOMDocument *xmldoc) {
	_ioService.post(boost::bind(&sendXMLResponseToConnection, _connection, xmldoc));
}

void SerifWorkQueue::Task::sendResponse(const std::string &contents) {
	_ioService.post(boost::bind(&sendStrResponseToConnection, _connection, contents));
}

void SerifWorkQueue::writeToServerInfoFile() {
	boost::mutex::scoped_lock lock(_mutex);

	// Create the parent directory recursively if we need to
	boost::filesystem::path output_file(_server_info_file_from_param_file);
	if (!boost::filesystem::exists(output_file.branch_path())) {
		boost::filesystem::create_directories(output_file.branch_path());
	}
	std::cerr << "[WorkQueue] Writing server info to file " << _server_info_file_from_param_file << "\n";
	std::ofstream output;
	output.open(_server_info_file_from_param_file.c_str());
	output << boost::asio::ip::host_name() << ":" << _assigned_port << "\n";
	output.close();
}
