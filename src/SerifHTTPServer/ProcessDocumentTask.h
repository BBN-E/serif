// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_PROCESS_DOCUMENT_TASK_H
#define SERIF_PROCESS_DOCUMENT_TASK_H

#include "SerifHTTPServer/SerifWorkQueue.h"
#include "Generic/state/XMLStrings.h"

namespace SerifXML { class XMLSerializedDocTheory; }

class DocTheory;
class DocumentDriver;

/** A SerifWorkQueue::Task class that is responsible for handling
  * a "ProcessDocument" message from a client.  This message supplies
  * a single document, along with some information about what should
  * be performed; and returns an annotated document.
  *
  * <pre>
  *    &lt;ProcessDocument language="English" start_stage="START" end_stage="output">
  *      &lt;Document>...&lt;/Document>
  *    &lt;/ProcessDocument>
  * </pre>
  * 
  */
class ProcessDocumentTask: public SerifWorkQueue::Task {
public:
	typedef std::map<SerifXML::xstring, SerifXML::xstring> OptionMap;

	/** Create a new task that is responsible for handling the 
	  * "ProcessDocument" message in 'cmd'.  Once the task is complete,
	  * send a response to the client using the given connection. 
	  *
	  * This ProcessDocumentTask assumes ownership of both the 
	  * <code>document</code> and the <code>optionMap</code> -- i.e.,
	  * the caller should not delete them, and should not access them
	  * any further after constructing the ProcessDocumentTask. */
	ProcessDocumentTask(xercesc::DOMDocument *document, 
		OptionMap *optionMap, const std::wstring &sessionId, bool user_supplied_session_id,
		boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection);

	~ProcessDocumentTask();

	/** Handle the "ProcessDocument" message and send a response 
	  * back to the client.  The given documentDriver can be used to
	  * run SERIF. */
	virtual bool run(DocumentDriver *documentDriver);

private:
	xercesc::DOMDocument *_document;
	OptionMap *_optionMap;
	std::wstring _sessionId;
	bool _user_supplied_session_id;
	void getStageRange(Stage& startStage, Stage& endStage, DocTheory *docTheory);

};



#endif

