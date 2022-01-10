// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// QueueDriver.cpp

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/driver/QueueDriver.h"

#include "Generic/driver/DiskQueueDriver.h"

#include "Generic/common/ParamReader.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/linuxPort/serif_port.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>
#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time_types.hpp>
#pragma warning(pop)

#ifdef _WIN32
#include <Windows.h>
#define usleep(x) Sleep((x)/1000)
#else
#include <unistd.h>
#include <ctime>
#endif

namespace {
	// When there's nothing in our input queue, or our output queue is
	// empty, we sleep until something appears.  We start out sleeping
	// at MIN_SLEEP_TIME, and then sleep progressively longer periods
	// until we're sleeping MAX_SLEEP_TIME.
	const int MIN_SLEEP_TIME = 10;   // Time in millisecs
	const int MAX_SLEEP_TIME = 5000; // Time in millisecs
	const int SLEEP_DELTA = 100; // dt in millisecs

	void logException(const char* docid=0, const char* what=0, const char* src=0) {
		std::ostringstream msg;
		msg << "Exception ";
		if (docid)
			msg << "while processing" << docid;
		msg << "\n  ";
		if (src) msg << "in " << src << ": ";
		if (what) msg << what << "\n";
		SessionLogger::err("queue-driver") << msg;
		std::cerr << msg;
	}
}

QueueDriver::QueueDriver() {
	// Build our document reader.
	_sourceFormat = ParamReader::getParam("source_format");
	if (!boost::iequals(_sourceFormat, "serifxml")) {
		_docReader.reset(DocumentReader::build(_sourceFormat.c_str()));
	}

	// Create the document driver.
	_sessionProgram.reset(_new SessionProgram());
	_docDriver.reset(_new DocumentDriver(_sessionProgram.get(), 0));
	_docDriver->endBatch();
}

QueueDriver::~QueueDriver() {}

void QueueDriver::processQueue() { 
	// How much time to we spend waiting for input, waiting for 
	// space in the output queue, and actually working?
	double waitTime=0;  // msec
	double blockTime=0; // msec
	double workTime=0;  // msec
	double realWorkTime = 0;
	size_t numDocs=0;

	// The longer it's been since we've done something useful, the
	// longer we sleep when polling to see if we're ready to do some
	// more work.
	int sleep_time = MIN_SLEEP_TIME;

	boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::local_time();
	while (true) {
		try {
			saveTimers(realWorkTime, waitTime, blockTime, 
					   workTime-realWorkTime, numDocs);
			if (gotQuitSignal()) return;
			double *timerDst = 0;
			if (dstIsFull()) {
				timerDst = &blockTime;
				usleep(sleep_time*1000);
			} else {
				Element_ptr elt = next();
				if (elt) {
					timerDst = &workTime;
					realWorkTime += process(elt);
					++numDocs;
				} else {
					timerDst = &waitTime;
					usleep(sleep_time*1000);
				}
			}
			// Update the timer for whatever action we took.
			boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::local_time();
			boost::posix_time::time_duration dt = t1-t0;
			(*timerDst) += dt.total_milliseconds();
			t0 = t1;
			// Adjust sleep interval.
			if (timerDst == &workTime) {
				sleep_time = MIN_SLEEP_TIME; 
			} else {
				sleep_time = std::min(MAX_SLEEP_TIME, 
									  sleep_time + SLEEP_DELTA);
			}
		} catch (UnrecoverableException &e) {
			logException(0, e.getMessage(), e.getSource());
		} catch (std::exception &e) {
			logException(0, e.what());
		} catch (...) {
			logException();
		}
	}
}

double QueueDriver::process(Element_ptr elt) 
{
	_docDriver->endBatch();
	double real_work_time = 0;
	bool success = false;
	try {
		// Run serif.
		_docDriver->beginBatch(_sessionProgram.get(), 0, false);
		boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::local_time();
		_docDriver->runOnDocTheory(elt->docTheory.get(), 0);
		boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::local_time();
		real_work_time = static_cast<double>((t1-t0).total_milliseconds());
		_docDriver->endBatch();
		// Save results.
		writeResults(elt);
		success = true;
	} catch (UnrecoverableException &e) {
		logException(elt->uid.c_str(), e.getMessage(), e.getSource());
	} catch (std::exception &e) {
		logException(elt->uid.c_str(), e.what());
	} catch (...) {
		logException(elt->uid.c_str());
	}
	if (!success) {
		try {
			handleFailure(elt);
		} catch (UnrecoverableException &e) {
			logException(elt->uid.c_str(), e.getMessage(), e.getSource());
		} catch (std::exception &e) {
			logException(elt->uid.c_str(), e.what());
		} catch (...) {
			logException(elt->uid.c_str());
		}
	}
	return real_work_time;
}

std::map<std::string, boost::shared_ptr<QueueDriver::Factory> > &QueueDriver::_factory() {
	static std::map<std::string, boost::shared_ptr<Factory> > factory;
	if (factory.empty()) {
		factory["disk"] = boost::make_shared<FactoryFor<DiskQueueDriver> >();
	}
	return factory;
}

QueueDriver *QueueDriver::build(std::string name) {
	boost::shared_ptr<Factory> factory = _factory()[name];
	if (factory) {
		return factory->build();
	} else {
		throw UnexpectedInputException("QueueDriver::build",
				   "Unknown queue driver type", name.c_str());
	}
}
