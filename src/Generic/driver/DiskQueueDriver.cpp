// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/driver/DiskQueueDriver.h"

#include "Generic/common/ParamReader.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/linuxPort/serif_port.h"

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#define usleep(x) Sleep((x)/1000)
#else
#include <unistd.h>
#endif

namespace {
	// We use file extensions to mark the status of files in the queue
	// directories.  To change that status of a file, you *must* use
	// rename (aka mv), which we assume is atomic.  This ensures that
	// we can run multiple queue workers in parallel, without having
	// to worry about using locks.
	const std::string READY_EXTENSION(".ready");
	const std::string WORKING_EXTENSION(".working");
	const std::string WRITING_EXTENSION(".writing.xml");
	const std::string FAILED_EXTENSION(".failed");
	const std::string GAVE_UP_EXTENSION(".failed_twice");
	const std::string DONE_FILENAME("done");

	void logException(const char* docid=0, const char* what=0, const char* src=0) {
		std::ostringstream msg;
		msg << "[DiskQueueDriver] Exception ";
		if (docid)
			msg << "while processing" << docid;
		msg << "\n  ";
		if (src) msg << "in " << src << ": ";
		if (what) msg << what << "\n";
		SessionLogger::err("disk_queue") << msg;
		std::cerr << msg;
	}
}

DiskQueueDriver::~DiskQueueDriver() {}

DiskQueueDriver::DiskQueueDriver(): _retryingFailedDocument(false) {
	// Source and destination queue directories.  We will read *.READY
	// files from the source directory, and write them to the dst
	// directory.
	if (hasSource()) {
		_src = ParamReader::getRequiredParam("disk_queue_src");
		_doneFile = _src+SERIF_PATH_SEP+DONE_FILENAME;
	}

	_dst = ParamReader::getRequiredParam("disk_queue_dst");

	// Unique file extension for this worker.  This is appended just
	// before WORKING_EXTENSION or WRITING_EXTENSION, allowing us to
	// keep track of which worker is responsible for a given file.
	// When a worker crashes, this lets us determine which files it
	// was processing.
	_workerExt = ParamReader::getParam("disk_queue_worker_ext");

	// Create our result collector(s)
	// Always generate serifxml output for now.
	std::string outputFormat = ParamReader::getParam("output_format");
	if (outputFormat.empty() || boost::iequals(outputFormat, "none")) {
		_resultCollector.reset(); // no result collector.
	} else if (boost::iequals(outputFormat, "serifxml")) {
		_resultCollector.reset(_new SerifXMLResultCollector());
	} else {
		throw UnexpectedInputException("DiskQueueDriver::DiskQueueDriver",
					"DiskQueueDriver only supports serifxml (or none) as output_format.  Got: ", 
					outputFormat.c_str());
	}

	// Limits on the size of the destination directory
	_max_dst_files = ParamReader::getOptionalIntParamWithDefaultValue("disk_queue_max_dst_files", 10);
	_max_dst_bytes = ParamReader::getOptionalIntParamWithDefaultValue("disk_queue_max_dst_bytes", 0);

	_timerFile = ParamReader::getParam("disk_queue_timer_file");
	_quitFile = ParamReader::getParam("disk_queue_quit_file");

	// Check if directories exist, and create the destination directory
	// if necessary
	if ( hasSource() && !boost::filesystem::is_directory(_src) ) 
		throw UnexpectedInputException("DiskQueueCLIHook::run",
			"Source directory does not exist or is not a directory", _src.c_str());
	if ( !boost::filesystem::exists(_dst) ) {
		if (!boost::filesystem::create_directories(_dst)) {
			throw UnexpectedInputException("DiskQueueCLIHook::run",
				"Unable to create destination directory", _dst.c_str());
		}
	} else if ( !boost::filesystem::is_directory(_dst) ) 
		throw UnexpectedInputException("DiskQueueCLIHook::run",
			"Destination directory is not a directory", _dst.c_str());

	std::string expt_dir = ParamReader::getParam("experiment_dir");
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

	SessionLogger::info("disk_queue") 
		<< "Creating DiskQueueDriver:\n" 
		<< "  src=" << _src << "\n"
		<< "  dst=" << _dst << "\n"
		<< "  quit=" << _quitFile;

}

bool DiskQueueDriver::gotQuitSignal() {
	try {
		// If our source directory has a "done" file, and it contains
		// no files that are ready, writing, failed, or working, then
		// we're done.
		if ((!_doneFile.empty()) && boost::filesystem::exists(_doneFile)) {
			boost::filesystem::directory_iterator endItr;
			boost::filesystem::directory_iterator itr(_src);
			for (; itr!=endItr; ++itr) {
				std::string filename = BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(itr);
				if (boost::algorithm::ends_with(filename, READY_EXTENSION) ||
					boost::algorithm::ends_with(filename, WRITING_EXTENSION) ||
					boost::algorithm::ends_with(filename, FAILED_EXTENSION) ||
					boost::algorithm::ends_with(filename, WORKING_EXTENSION)) {
					return false; // Not entirely done yet. 
				}
			}
			// Mark the destination stage as done, and signal that we
			// can quit.
			markDstAsDone();
			return true;
		}

		// If we see the quit file, then exit.
		if (boost::filesystem::exists(_quitFile)) {
			//markDstAsDone(); -- should we do this or not?
			return true;
		}

		// Otherwise, keep running.
		return false;
	} catch (std::exception &) {
		return false;
	}
}

void DiskQueueDriver::markDstAsDone() {
	std::ofstream out((_dst+SERIF_PATH_SEP+DONE_FILENAME).c_str());
	out.close();
}

void DiskQueueDriver::markSrcAsDone() {
	std::ofstream out((_src+SERIF_PATH_SEP+DONE_FILENAME).c_str());
	out.close();
}

bool DiskQueueDriver::dstIsFull() {
	size_t dst_files = 0;
	size_t dst_bytes = 0;

	// Find all files in the destination directory whose status is
	// either "ready" or "writing".  The total number/size of these
	// files determines whether we consider the output queue directory
	// to be "full".  Note that we may still go over these limits,
	// since we check the limits *before* adding a new file, and 
	// multiple workers might all decide that the dst is not full, and
	// proceed to generate an output file.
	boost::filesystem::directory_iterator endItr;
	boost::filesystem::directory_iterator itr(_dst);
	for (; itr!=endItr; ++itr) {
		std::string filename = BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(itr);
		if (boost::algorithm::ends_with(filename, READY_EXTENSION) ||
			boost::algorithm::ends_with(filename, WRITING_EXTENSION) ||
			boost::algorithm::ends_with(filename, FAILED_EXTENSION)) {
			dst_files += 1;
			if (_max_dst_bytes)
				dst_bytes += static_cast<size_t>(boost::filesystem::file_size(itr->path()));
		}
	}

	if (_max_dst_files && (dst_files >= _max_dst_files))
		return true;
	if (_max_dst_bytes && (dst_bytes >= _max_dst_bytes))
		return true;
	return false;
}

void DiskQueueDriver::saveTimers(double workTime, double waitTime, double blockTime, double overheadTime, size_t numDocs) {
	if (_timerFile.empty()) return;
	std::ofstream out((_timerFile+".tmp").c_str());
	out << "Work\t" << workTime << "\n"
		<< "Wait\t" << waitTime << "\n"
		<< "Block\t" << blockTime << "\n"
		<< "Overhead\t" << overheadTime << "\n"
		<< "Docs\t" << numDocs << "\n";
	out.close();
	if (boost::filesystem::exists(_timerFile))
		boost::filesystem::remove(_timerFile);
	boost::filesystem::rename(_timerFile+".tmp", _timerFile);
}

QueueDriver::Element_ptr DiskQueueDriver::next() {
	QueueDriver::Element_ptr result;
	if ((result = next(READY_EXTENSION))) {
		_retryingFailedDocument = false;
		return result;
	} else if ((result = next(FAILED_EXTENSION))) {
		_retryingFailedDocument = true;
		return result;
	} else {
		return result;
	}
}

QueueDriver::Element_ptr DiskQueueDriver::next(const std::string& extension) {
	if (!hasSource())
		throw InternalInconsistencyException("DiskQueue::findSrcFile",
			"This type of DiskQueue has no source directory");

	boost::filesystem::directory_iterator itr(_src);
	boost::filesystem::directory_iterator endItr; // default construction yields past-the-end
	for (; itr!=endItr; ++itr) {
		std::string filename = BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(itr);
		if (boost::algorithm::ends_with(filename, extension)) {
			std::string path = BOOST_FILESYSTEM_DIR_ITERATOR_GET_PATH(itr);
			std::string basePath = path.substr(0, path.size()-extension.size());
			std::string baseFilename = filename.substr(0, filename.size()-extension.size());

			// Rename the file to add a special extension that signals 
			// that we're working on this file.  This prevents any other
			// workers from working on the file.  Note that we assume
			// that rename is an atomic operation.
			try {
				std::string workingPath = basePath+_workerExt+WORKING_EXTENSION;
				boost::filesystem::rename(path, workingPath);
			} catch (std::exception&) {
				SessionLogger::warn("disk_queue") << "Unable to lock file " << path;
				// Unable to grab this file -- someone else got to it
				// first.  Just continue scanning.
				continue;
			}
			bool success = false;
			try {
				return readDocument(baseFilename);
			} catch (UnrecoverableException &e) {
				logException(path.c_str(), e.getMessage(), e.getSource());
			} catch (std::exception &e) {
				logException(path.c_str(), e.what());
			} catch (...) {
				logException(path.c_str());
			}
			try {
				_retryingFailedDocument = extension == FAILED_EXTENSION;
				handleFailure(boost::make_shared<Element>(baseFilename, (Document*)0, (DocTheory*)0));
			} catch (UnrecoverableException &e) {
				logException(path.c_str(), e.getMessage(), e.getSource());
			} catch (std::exception &e) {
				logException(path.c_str(), e.what());
			} catch (...) {
				logException(path.c_str());
			}
		}
	}
	return QueueDriver::Element_ptr();

}


QueueDriver::Element_ptr DiskQueueDriver::readDocument(const std::string& docId) {
	std::string path = _src+SERIF_PATH_SEP+
		docId+_workerExt+WORKING_EXTENSION;
	std::wstring wpath = UnicodeUtil::toUTF16StdString(path);
	std::pair<const Document*, DocTheory*> docPair(0,0);
	if (_docReader) {
		Document *doc = _docReader->readDocumentFromFile(wpath.c_str());
		docPair.first = doc;
		try {
			docPair.second = _new DocTheory(doc);
		} catch (...) {
			try { delete doc; }
			catch (...) {}
			throw;
		}
	} else {
		docPair = SerifXML::XMLSerializedDocTheory(path.c_str()).generateDocTheory();
	}
	return boost::make_shared<Element>(docId, docPair.first, docPair.second);
}

void DiskQueueDriver::writeResults(QueueDriver::Element_ptr elt) {
	const std::string &docId = elt->uid;
	if (_resultCollector) {
		// Write output to a temporary file that we "lock" by using a
		// special file extension.  Then use atomic rename to unlock
		// the file (by chaning the extension to ".ready").
		std::string lockedDstFile = docId + _workerExt + WRITING_EXTENSION;
		_resultCollector->loadDocTheory(elt->docTheory.get());
		_resultCollector->produceOutput(UnicodeUtil::toUTF16StdString(_dst).c_str(), 
			UnicodeUtil::toUTF16StdString(lockedDstFile).c_str());
		if (boost::filesystem::exists(_dst+SERIF_PATH_SEP+docId)) {
			boost::filesystem::remove(_dst+SERIF_PATH_SEP+docId);
		}
		std::string unlockedDstFile = docId + READY_EXTENSION;
		// If there's a file in the way, then delete it.  This can happen
		// if we process the same document more than once.
		if (boost::filesystem::exists(_dst+SERIF_PATH_SEP+unlockedDstFile))
			boost::filesystem::remove(_dst+SERIF_PATH_SEP+unlockedDstFile);
		// Unlock the output file.
		boost::filesystem::rename(_dst+SERIF_PATH_SEP+lockedDstFile,
								  _dst+SERIF_PATH_SEP+unlockedDstFile);
	}
	// Remove the locked input file.
	std::string srcFile = docId+_workerExt+WORKING_EXTENSION;
	boost::filesystem::remove(_src+SERIF_PATH_SEP+srcFile);
}

void DiskQueueDriver::handleFailure(QueueDriver::Element_ptr elt) {
	const std::string &docId = elt->uid;
	// Mark the input file as failed.  (Add a parameter that 
	// says to just delete it instead??)
	std::string srcFile = docId+_workerExt+WORKING_EXTENSION;
	std::string failFile = docId+FAILED_EXTENSION;
	if (_retryingFailedDocument) {
		boost::filesystem::remove(_src+SERIF_PATH_SEP+srcFile);
		std::ofstream gaveUpFile((_src+SERIF_PATH_SEP+docId+GAVE_UP_EXTENSION).c_str());
		gaveUpFile.close();
	} else {
		boost::filesystem::rename(_src+SERIF_PATH_SEP+srcFile,
								  _src+SERIF_PATH_SEP+failFile);
	}
}
