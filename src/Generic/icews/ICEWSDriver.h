// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_DRIVER_H
#define ICEWS_DRIVER_H

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/regex.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/commandLineInterface/CommandLineInterface.h"

// Forward declarations
class DocumentDriver;
class FileSessionLogger;
class Document;


/** Replacement for the document driver that is used if the 
* icews_read_stories_from_database is set.  */
class ICEWSDriver: private boost::noncopyable {
public:
	ICEWSDriver();
	~ICEWSDriver();

	/** For each story returned by Stories::getStories(), read the
	* story from the database, and process it through the stage
	* specified by the end_stage parameter.  If the
	* icews_save_final_serifxml parameter is set, then write the
	* final output to the experiment directory. */
	void processStories();
	
	/** A command-line hook that checks if the icews_read_stories_from_database 
     * parameter is set, and if so it uses ICEWSDriver for the main loop, rather 
     * than the default DocumentDriver. */
	struct ICEWSCommandLineHook: public ModifyCommandLineHook {
		WhatNext run(int verbosity) { 
			if (ParamReader::isParamTrue("icews_read_stories_from_database") && !ParamReader::hasParam("queue_driver")) {
				ICEWSDriver().processStories();
				return SUCCESS;
			} else {
				return CONTINUE; // use the normal document driver.
			}
		}
	};


private:
	boost::scoped_ptr<DocumentDriver> _docDriver;
	FileSessionLogger *_sessionLogger;
	// We can save our final output to disk or to the database.
	bool _save_final_serifxml_in_experiment_dir;

	// Skip regexp
	boost::scoped_ptr<boost::wregex> _documentTextFilter;
	bool documentShouldBeSkipped(Document *document);
};

#endif
