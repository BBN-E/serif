// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DISK_QUEUE_DRIVER_H
#define DISK_QUEUE_DRIVER_H

#include <string>
#include "Generic/driver/QueueDriver.h"

/** A queue driver that uses directories as source and destination
 * locations.  "Locking" files is done using file renaming, which is
 * assumed to be atomic.  WARNING: This is true on most UNIX variants,
 * but is not always guaranteed on Windows filesystems!
 *
 */
class DiskQueueDriver: public QueueDriver {
public:
	DiskQueueDriver();
	virtual ~DiskQueueDriver();

protected:
	// See QueueDriver.h for documentation of these methods:
	virtual bool gotQuitSignal();
	virtual bool dstIsFull();
	virtual Element_ptr next();
	virtual void writeResults(Element_ptr elt);
	virtual void saveTimers(double workTime, double waitTime, double blockTime, double overheadTime, size_t numDocs);
	virtual void handleFailure(Element_ptr elt);

	/* This can be used by subclasses (eg ICEWSQueueFeeder) to
	 * indicate that there is no source directory. */
	virtual bool hasSource() { return true; }

	virtual void markDstAsDone();
	virtual void markSrcAsDone();

private:
	Element_ptr readDocument(const std::string& docId);
	Element_ptr next(const std::string& extension);

	std::string _sourceFormat;
	std::string _src;
	std::string _dst;
	std::string _timerFile;
	std::string _quitFile;
	std::string _doneFile;
	std::string _workerExt;
	size_t _max_dst_files;
	size_t _max_dst_bytes;
	boost::scoped_ptr<ResultCollector> _resultCollector;

	bool _retryingFailedDocument;
};

#endif
