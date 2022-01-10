// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SGM_2_APF_TASK_H
#define SGM_2_APF_TASK_H

#include "SerifHTTPServer/SerifWorkQueue.h"
#include "Generic/state/XMLStrings.h"

class DocumentDriver;

/** This SerifWorkQueue Task handles the following client requests:
  *   - sgm2apf
  *   - sgm2serifxml
  *   - rawtext2apf
  *   - rawtext2serifxml
  */
struct Doc2OutTask: public SerifWorkQueue::Task {
	typedef enum {APF, SERIFXML} OutputFormat;
	Doc2OutTask(const std::string &document, OutputFormat outputFormat, boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection): 
	Task(ioService, connection), _document(document), _outputFormat(outputFormat)  {}
private:
	std::string _document;
	OutputFormat _outputFormat;
	virtual bool run(DocumentDriver *documentDriver);
};

#endif
