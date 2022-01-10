// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_XML_WORK_QUEUE_H
#define SERIF_XML_WORK_QUEUE_H

#include <xercesc/dom/DOM.hpp>
#pragma warning(push, 0)
#include <boost/thread.hpp>
#pragma warning(pop)
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <list>
#include <deque>

#include "Generic/patterns/PatternSet.h"

//#define ENABLE_LEAK_DETECTION

class DocumentDriver;
class Stage;
class IncomingHTTPConnection;
typedef boost::shared_ptr<IncomingHTTPConnection> IncomingHTTPConnection_ptr;



/** The SerifWorkQueue is a singleton class used to manage tasks 
  * that require SERIF.  When the SerifWorkQueue is created, it 
  * starts a new thread.  All tasks are executed in this thread.  
  * Since SERIF is not thread-safe, the work queue executes a single
  * task at a time, waiting for each task to complete before 
  * beginning the next task.  
  *
  * If you use SerifWorkQueue, then you should *never* run SERIF 
  * from the main thread, since the work queue might be running
  * SERIF at the same time in its own thread.
  *
  * Each SERIF-dependant task is defined using a subclass of 
  * SerifWorkQueue::Task.
  */
class SerifWorkQueue: private boost::noncopyable {
public:

	/** Return a pointer to the Serif Work Queue singleton instance. */
	static SerifWorkQueue *getSingletonWorkQueue();

	typedef std::map<std::string, std::string> ParamOverrideMap;

	/** Initialize the work queue.  This must be called after the global
	  * ParamReader has been initialized, and before any call to addTask().
	  * It should be called exactly once. */
	void initialize();

	/** A single SERIF-dependant task that should be performed by the 
	  * work queue.  Each task should define a run() method, which 
	  * takes a DocumentDriver, and performs the task.  The run() method
	  * should return true if the task was successfully performed, and
	  * false if it failed.
	  *
	  * The run() method should only communicate with the originating
	  * connection object by using the protected methods reportError()
	  * and sendResponse().  The subclass should *not* keep a reference 
	  * to _connection, or call any of its methods, since it is running 
	  * in a different thread!
	  */
	class Task: private boost::noncopyable {
	public:
		Task(boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection);
		virtual ~Task() {}
		virtual bool run(DocumentDriver *documentDriver) = 0;
	protected:
		void reportError(size_t error_code, std::string explanation);
		void sendResponse(const std::string &contents);
		void sendResponse(xercesc::DOMDocument *xmldoc);
	private:
		boost::asio::io_service &_ioService;
		IncomingHTTPConnection_ptr _connection;
	};
	typedef boost::shared_ptr<Task> Task_ptr;

	/** A special type of task that expects a collection of pattern
	 * sets instead of a document driver. */
	class PatternSetTask: public Task {
	public:
		PatternSetTask(boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection):
			Task(ioService, connection) {}
		virtual bool run(DocumentDriver *documentDriver) {
			throw InternalInconsistencyException("PatternSetTask::run(DocumentDriver)",
							 "PatternSetTasks should use run(PatternSets) instead");
		}
		virtual bool run(const Symbol::HashMap<PatternSet_ptr>* patternSets) = 0;
	};
	typedef boost::shared_ptr<PatternSetTask> PatternSetTask_ptr;

	/** Register a new task, and return immediately.  The task will be 
	  * performed by the SerifWorkQueue's thread once all other tasks
	  * have been completed. */
	void addTask(boost::shared_ptr<Task> task);

	/** Return the value of the "server_port" parameter from the parameter
	  * file that was supplied by the call to initialize(). */
	int getPortFromParamFile() const;

	/** Return the value of the "server_docs_root" parameter from the 
	  * parameterfile that was supplied by the call to initialize(). */
	std::string getServerDocsRootFromParamFile() const;

	/** Return the number of tasks that the SerifWorkQueue still needs to
	  * complete, including the task that it is currently performing.  If
	  * the SerifWorkQueue is waiting for a task to perform, return 0. */
	size_t numTasksRemaining() const;
	size_t numTasksProcessed() const;
	size_t numTasksFailed() const;
	float getThroughputIncludingLoadTime() const;
	float getThroughputExcludingLoadTime() const;

	std::string getStatus() const;

	std::string getMemoryUsageGraph() const;

	class FatalErrorCallback: private boost::noncopyable {
		public: 
		virtual ~FatalErrorCallback() {}
		virtual void operator()() = 0;
	};

	// Sets _assigned_port.  This is useful if the port in the param file is 0.
	void setAssignedPort(int assigned_port);

	/** Set a callback that should be called if we encounter a fatal 
	  * error.  This callback will be called in the serif thread, so
	  * if it needs to communicate w/ the server thread, then it should
	  * do so using asyncio. */
	void setShutdownCallback(FatalErrorCallback *fatalErrorCallback);

	void shutdown(bool wait=false);

private: // ============ Member Variables ============

	// The thread used by the work queue to perform tasks.
	boost::thread *_thread;

	// A mutex used to guard access to all member variables.  
	mutable boost::mutex _mutex;

	// The list of tasks that we still need to perform.  (This list does
	// not include the task that we are currently performing.)
	std::list<Task_ptr> _tasks;

	// A condition variable that is used by addTask() to let the work
	// queue's thread know when tasks are available for processing.
	boost::condition_variable _tasks_ready;

	// A condition variable that is used to let the work queue know 
	// when we're ready to load the document driver.
	bool _initialization_complete;
	boost::condition_variable _initialized;

	// The document driver used to run SERIF.  We use a single document
	// driver for all process document tasks, to avoid having to reload 
	// models.
	DocumentDriver *_documentDriver;

	// The pattern sets used for pattern matching during pattern match
	// tasks.
	Symbol::HashMap<PatternSet_ptr> *_patternSets;

	// Statistics about how many tasks we've performed successfully or
	// and how many have failed.
	size_t _num_tasks_processed;
	size_t _num_tasks_failed;
	float _throughput_including_load_time;
	float _throughput_excluding_load_time;

	// A boolean that can be used to check whether the queue is currently
	// waiting for a new task.  If false, then it's currently working on
	// a task.
	bool _waiting_for_task;

	// Values read from the parameter file.  We record a copy of these
	// values when we read the parameter file, because it may not be
	// safe to call ParamReader::readParam() while SERIF is running.
	int _port_from_param_file;
	std::string _server_info_file_from_param_file;
	std::string _server_docs_root_from_param_file;

	// Our assigned port.  This is set by calling setAssignedPort()
	int _assigned_port;

	// What are we working on right now?
	std::string _status;

	FatalErrorCallback *_fatalErrorCallback;

	// Set this to true when you want to shut down.  (Shut down will
	// occur after the current task completes)
	bool _shutdown;
	boost::condition_variable _shutdown_complete;

	// If ENABLE_LEAK_DETECTION is defined, then we keep track of how much
	// memory the server is using.
	struct MemoryUsageRecord {
		size_t normal_block_size;
		size_t crt_block_size;
		time_t time_checked;
		size_t task_num;
		size_t totalUsage() const { return normal_block_size+crt_block_size; }
		MemoryUsageRecord(): normal_block_size(0), crt_block_size(0), 
			time_checked(0), task_num(0) {}
		void check(); // set the record variables based on current mem usage.
	};
	MemoryUsageRecord _baseline_memory_usage;
	std::deque<MemoryUsageRecord> _memory_usage_history;

private:  // ============ Helper Methods ============

	bool loadDocumentDriver(); // return false for failure.
	bool loadPatternSets(); // return false for failure.
	void run();
	Task_ptr getNextTask();

	// The constructor and destructor are both private, because this is
	// a singleton class -- the only instance should be created by the
	// static method getSingletonWorkQueue().
	SerifWorkQueue();
	~SerifWorkQueue();

	void recordMemoryUsage();
	size_t addMemoryUsageGraphRow(const MemoryUsageRecord &mem_record, size_t max_memory_usage, size_t prev_mem, std::ostream &out) const;

	void writeToServerInfoFile();
};


#endif
