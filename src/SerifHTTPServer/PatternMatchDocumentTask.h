// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_MATCH_DOCUMENT_TASK_H
#define PATTERN_MATCH_DOCUMENT_TASK_H

#include "SerifHTTPServer/SerifWorkQueue.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/patterns/PatternSet.h"

namespace SerifXML { class XMLSerializedDocTheory; }
class DocTheory;

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
class PatternMatchDocumentTask: public SerifWorkQueue::PatternSetTask {
public:
	typedef std::map<SerifXML::xstring, SerifXML::xstring> OptionMap;

	/** Create a new task that is responsible for handling the 
	* "PatternMatch" message in 'cmd'.  Once the task is complete,
	* send a response to the client using the given connection. 
	*
	* This PatternMatchDocumentTask assumes ownership of both the 
	* <code>document</code> and the <code>optionMap</code> -- i.e.,
	* the caller should not delete them, and should not access them
	* any further after constructing the PatternMatchDocumentTask. */
	PatternMatchDocumentTask(xercesc::DOMDocument *document, xercesc::DOMDocument *slot1,
		xercesc::DOMDocument *slot2, xercesc::DOMDocument *slot3, 
		std::map<std::wstring, float> slot1Weights, std::map<std::wstring, float> slot2Weights,
		std::map<std::wstring, float> slot3Weights,
		const std::map<std::wstring,std::map<std::wstring,double> > &equiv_names,
		OptionMap *optionMap,
		boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection);

	~PatternMatchDocumentTask();

	/** Handle the "PatternMatchDocumentTask" message and send a 
	* response back to the client. */
	virtual bool run(const Symbol::HashMap<PatternSet_ptr> *patternSets);

private:
	xercesc::DOMDocument *_document;
	xercesc::DOMDocument *_slot1;
	xercesc::DOMDocument *_slot2;
	xercesc::DOMDocument *_slot3;	
	std::map<std::wstring, float> _slot1Weights;
	std::map<std::wstring, float> _slot2Weights;
	std::map<std::wstring, float> _slot3Weights;
	const std::map<std::wstring,std::map<std::wstring,double> > _equiv_names;
	const std::map<std::wstring,std::map<std::wstring,double> > _slot_weights;
	OptionMap *_optionMap;
	std::wstring _sessionId;
	bool _user_supplied_session_id;
};

#endif

