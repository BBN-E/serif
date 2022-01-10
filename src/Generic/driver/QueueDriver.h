// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef QUEUE_DRIVER_H
#define QUEUE_DRIVER_H

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/reader/DocumentReader.h"

/** An abstract base class for queue-based drivers for SERIF.  Queue
 * based drivers read input documents from a source location (e.g., a
 * directory or database table), process them, and write the results
 * to a destination location.  Multiple queue drivers can be run in
 * parallel (with the same source locations and destination
 * locations).  The queue driver continues to run, monitoring its
 * source location for new input documents, until it receives a quit
 * signal.
 *
 * If and when the destination location becomes "full", the queue
 * driver will pause processing until it is no longer full.  This
 * allows for "back pressure" when multiple queue drivers are arranged
 * in a pipeline.
 *
 * Subclasses are expected to implement various virtual methods that
 * are responsible for interacting with the queue itself.  See also:
 * DiskQueueDriver.
 *
 * Documents are identified using "document identifier" strings. The
 * content and formatting of these identifiers is left up to the
 * subclasses.
 */
class QueueDriver {
public:
	virtual ~QueueDriver();

	/** Start the queue driver.  This will read files from the source
	 * location, process them, and write them to the destination
	 * location.  It will continue running (and checking the source
	 * location for new file) until a "quit" signal is recieved.  The
	 * mechanism used to send quit signals is left up to subclasses
	 * to define.*/
	virtual void processQueue();

	//======================================================================
	// Factory Method & Hook
	//======================================================================

	/** Create and return a new QueueDriver of the given type. */
	static QueueDriver *build(std::string name);

	/** Hook for changing queue driver implementation. */
	template<typename QueueDriverClass>
	static void addImplementation(std::string name) {
		_factory()[name] = boost::make_shared<FactoryFor<QueueDriverClass> >();
	}

protected:
	/** Construct a new QueueDriver.  Initializes _docDriver etc. */
	QueueDriver();

	/** An element in the queue for processing. */
	struct Element { 
		std::string uid;
		boost::scoped_ptr<const Document> document;
		boost::scoped_ptr<DocTheory> docTheory;
		/** Takes ownership of document & docTheory. */
		Element(std::string uid, const Document *document, DocTheory *docTheory)
			:uid(uid), document(document), docTheory(docTheory) {}
	};
	typedef boost::shared_ptr<Element> Element_ptr;

	/** Process a single document, given its identifier.  This will be
	 * called on identifiers that were returned by findSrcDocument(),
	 * so the queue driver can assume that the given document is
	 * "locked." */
	virtual double process(Element_ptr elt);

	//======================================================================
	// Abstract methods that subclasses must implement
	//======================================================================

	/** Return true if the queue driver recieved a quit signal. */
	virtual bool gotQuitSignal() = 0;

	/** Return true if the destination location is full.  The queue
	 * driver will not begin processing a new document until this
	 * returns false.  */
	virtual bool dstIsFull() = 0;

	/** Select a new input document from the source location, and
	 * "lock" it, so that no other queue drivers will process the same
	 * document.  The exact mechanism used for locking is left up to
	 * subclasses of QueueDriver.  */
	virtual Element_ptr next() = 0;

	/** Save SERIF's results (typically as a serifxml file) in the
	 * destination location.  The result file must be "locked" until
	 * it is completely written, in order to avoid downstream
	 * QueueDrivers from reading the file prematurely.  The exact
	 * mechanism used for locking is left up to subclasses of
	 * QueueDriver.  */
	virtual void writeResults(Element_ptr elt) = 0;

	/** This method is called when an exception is raised while
	 * processing a document. */
	virtual void handleFailure(Element_ptr elt) = 0;

	/** Record the amount of time spent working, waiting for new input
	 * files, and blocking because the destination location is
	 * full. */
	virtual void saveTimers(double workTime, double waitTime, double blockTime, double overheadTime, size_t numDocs) = 0;

protected:
	boost::scoped_ptr<DocumentReader> _docReader;
	boost::scoped_ptr<DocumentDriver> _docDriver;
	boost::scoped_ptr<SessionProgram> _sessionProgram;

private:
	std::string _sourceFormat;
	
private:
	struct Factory { virtual QueueDriver *build() = 0; };
	template<typename QueueDriverClass>
	struct FactoryFor: public Factory {
		QueueDriver *build() { return new QueueDriverClass(); }
	};
	static std::map<std::string, boost::shared_ptr<Factory> > &_factory();

};

#endif
