// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "SerifHTTPServer/IncomingHTTPConnection.h"
#include "SerifHTTPServer/OutgoingHTTPConnection.h"
#include "SerifHTTPServer/SerifHTTPServer.h"
#include "SerifHTTPServer/SerifWorkQueue.h"
#include "SerifHTTPServer/ProcessDocumentTask.h"
#include "SerifHTTPServer/PatternMatchDocumentTask.h"
#include "SerifHTTPServer/Doc2OutTask.h"

#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/common/version.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/common/version.h"
#include "Generic/linuxPort/serif_port.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <list>
#include <iomanip>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#pragma warning(pop)
#include <boost/foreach.hpp>

#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time.hpp>
#pragma warning(pop)

#include <xercesc/framework/MemBufFormatTarget.hpp>

using namespace SerifXML;

using namespace xercesc;

// Private regular expressions.
namespace {
	boost::regex acceptableSessionId("[\\-a-zA-Z_0-9]+");
	// Regexp Groups: server, port, path
	boost::regex httpUrlRegexp("http://([^:/]+)(:\\d+)?(/.*)", boost::regex::perl);
	// Regexp Groups: path
	boost::regex fileUrlRegexp("file://(.+)", boost::regex::perl);
}

IncomingHTTPConnection::IncomingHTTPConnection(SerifHTTPServer *server, boost::asio::io_service &ioService, int verbosity, bool is_subprocess, const char* logdir)
: HTTPConnection(INCOMING, ioService, is_subprocess), _server(server), 
  _state(LISTENING), _xmlRequest(0), _ioService(ioService), _verbosity(verbosity), _is_subprocess(is_subprocess),
  _connectionDescription("Reading Request Message"), _user_supplied_session_id(false),
  _logdir(logdir)
{}

IncomingHTTPConnection::~IncomingHTTPConnection() {
	if (_xmlRequest) _xmlRequest->release();
	_xmlRequest = 0;
}

void IncomingHTTPConnection::start()
{
	assert(_state == LISTENING);
	_state = READING_REQUEST;
	// Start reading the request message.  If we get unknown headers with
	// a get request, just ignore them -- we're probably responding to a
	// browser, and they can send a wide variety of headers.  But for post
	// requests, be more stringent.
	readMessage(IGNORE_UNKNOWN_HEADERS_FOR_GET_REQUESTS);
	writeLogMessage(0, "Reading request message");
}

void IncomingHTTPConnection::stop()
{
	writeLogMessage(0, "Shutting down HTTP Connection");
	// Cancell all associated outgoing connections.
	typedef std::pair<DownloadFileResponseHandler_ptr, OutgoingHTTPConnection_ptr> DownloadPair;
	BOOST_FOREACH(DownloadPair downloadPair, _downloadConnections)
		downloadPair.second->stop();
	HTTPConnection::stop();
	_state = CLOSED;
}

void IncomingHTTPConnection::reset() {
	HTTPConnection::reset();
	if (_xmlRequest) _xmlRequest->release();
	_xmlRequest = 0;
	_state = LISTENING;
	_response.clear();
}

//========================== Request Processing =============================

void IncomingHTTPConnection::handleReadMessageError(const std::string& description) {
	writeLogMessage(0, "HTTP Connection error:");
	writeLogMessage(0, "  "+description);
    reportError(400, description);
}

void IncomingHTTPConnection::handleReadMessageEOF(const std::string& description) {
    if (_is_subprocess) {
        writeLogMessage(0, "Input pipe closed; Shutting down the server");
        _server->stop(true);
    } else {
        handleReadMessageError(description);
    }
}

void IncomingHTTPConnection::handleMessage(const HTTPMessage& message) {
	if (_state == CLOSED) return;
	writeLogMessage(0, "Processing request:", message.request_method, message.request_uri);
	_connectionDescription = "Processing request: " + message.request_method + " " + message.request_uri;
	assert(_state == READING_REQUEST);
	_state = PROCESSING_REQUEST;
	bool is_get_request = (message.request_method == std::string("GET"));

	// Log the request message.  (But don't bother to log GET requests.)
	if (_logdir && !is_get_request) {
		pick_logfile_prefix(message);
		logHTTPMessage("request", getMessageStr().c_str());
	}

	if (is_get_request) {
		handleGetRequest(message);
	} else {
		// Strip leading '/' from uri, if it's there.
		std::string uri = message.request_uri;
		if (uri.size() > 0 && uri[0] == '/')
			uri = uri.substr(1);
		// Check for known POST request URIs.
		if (boost::iequals(uri, std::string("SerifXMLRequest"))) {
			handleXMLContents(message);
		} else if (boost::iequals(uri, std::string("sgm2apf")) || boost::iequals(uri, std::string("rawtext2apf"))) {
			handleDoc2OutCommand(message, Doc2OutTask::APF);
		} else if (boost::iequals(uri, std::string("sgm2serifxml")) || boost::iequals(uri, std::string("rawtext2serifxml"))) {
			handleDoc2OutCommand(message, Doc2OutTask::SERIFXML);
		} else if (boost::iequals(uri, std::string("Shutdown"))) {
			handleShutdownRequest(false);
		} else if (boost::iequals(uri, std::string("ImmediateShutdown"))) {
			handleShutdownRequest(true);
		} else if (boost::iequals(uri, std::string("Restart"))) {
			handleRestartRequest(false);
		} else if (boost::iequals(uri, std::string("ImmediateRestart"))) {
			handleRestartRequest(true);
		} else if (boost::iequals(uri, std::string("ping"))) {
			sendHTMLResponse("pong "+message.content, 
					 "text/plain");
		} else {
			reportError(404, "Unknown URI", message.request_uri);
		}
	}
}

// Local helper function
namespace {
	bool readFileIntoString(const std::string& filename, std::string& file_contents) {
		std::ifstream is(filename.c_str(), std::ios::in | std::ios::binary);
		if (!is) return false;
		char buf[512];
		while (is.read(buf, sizeof(buf)).gcount() > 0)
			file_contents.append(buf, is.gcount());
		return (is.eof() && !is.bad());
	}
}

void IncomingHTTPConnection::handleGetRequest(const HTTPMessage& message) {
	std::string path = message.request_uri;

	// Check for some special URLs
	//if (path == "/session-log.txt") 
	//	foo...
	if (path == "/status.html")
		handleStatusRequest(); 
	else
		handleGetDocumentRequest(path);
}

void IncomingHTTPConnection::handleGetDocumentRequest(std::string path) {

	// Get the server docs root.  If it's not defined, then reject all GET
	// requests.
	SerifWorkQueue *workQueue = SerifWorkQueue::getSingletonWorkQueue();
	std::string server_docs_root = workQueue->getServerDocsRootFromParamFile();
	if (server_docs_root.empty()) {
		reportError(404, "This Serif HTTP server's parameter file does not "
			"define a root directory for server documents.  Please set "
			"the server_docs_root parameter if you wish for the Serif HTTP "
			"server to respond to GET requests.");
		return;
	}

	// Reject any URI that doesn't point at an absolute path:
	if (path.empty() || path[0] != '/') {
		reportError(404, "URL path must be absolute (i.e. begin with '/')", path); 
		return;
	}

	// Reject any URI that contains "..":
	if (path.find("..") != std::string::npos) {
		reportError(404, "URL may not contain \"..\"", path); 
		return;
	}

	// If the path ends in a slash, add "index.html" to the end.
	if (path[path.size() - 1] == '/')
		path += "index.html";

	_connectionDescription = "Retrieving webpage: " + path;

	// Treat the given path as relative to the server docs root.
	if (server_docs_root[server_docs_root.size() - 1] != '/' &&
		server_docs_root[server_docs_root.size() - 1] != '\\')
		server_docs_root += '/';
	std::string file_path = server_docs_root + path;

	// Read the file
	std::string file_contents;
	if (!readFileIntoString(file_path, file_contents)) {
		reportError(400, "File not found or error reading file", file_path);
		return;
	}

	if ((path == "/index.html") || (path == "/shutdown.html")) {
		boost::algorithm::replace_all(file_contents, "<!--**VERSION**-->", SerifVersion::getVersionString());
		boost::algorithm::replace_all(file_contents, "<!--**STATUS_SECTION**-->", getServerStatus());
		boost::algorithm::replace_all(file_contents, "<!--**ACTION_SECTION**-->", getServerActions());
		#ifdef ENABLE_LEAK_DETECTION
		boost::algorithm::replace_all(file_contents, "<!--**MEMORY_USAGE_SECTION**-->", SerifWorkQueue::getSingletonWorkQueue()->getMemoryUsageGraph());
		#endif
	}

	const char* content_type = "text/html";
	size_t dotpos = path.rfind('.');
	if (dotpos != std::string::npos) {
		std::string ext = path.substr(dotpos+1);
		if (boost::iequals(ext, "css"))
			content_type = "text/css";
		if (boost::iequals(ext, "js"))
			content_type = "text/javascript";
	}

	// Send the response
	sendHTMLResponse(file_contents, content_type);
}

void IncomingHTTPConnection::sendHTMLResponse(const std::string& html_contents, 
					      const char* content_type) {
	std::stringstream out;
	out << "HTTP/1.0 200 OK\r\n";
	out << "Content-Length: " << html_contents.size() << "\r\n";
	out << "Content-Type: " << content_type << "\r\n";
	out << "\r\n";
	out << html_contents;
	std::string out_str = out.str();
	sendResponse(out_str);
}

void IncomingHTTPConnection::handleRestartRequest(bool force) {
	_connectionDescription = "Handling Restart Request";
	if (_is_subprocess) {
		// Don't send a response -- doing so might cause us to block
		// (waiting for the response to get read), and so not shut down.
		_server->restart(true);
	} else {
		std::stringstream out;
		out << "<html>\r\n<head>\r\n<title>Serif HTTP Server: Restart</title>\r\n";
		out << "<meta http-equiv=\"refresh\" content=\"5;url=/\"></head>\r\n";
		out << "<body>\r\n<h1>Restarting the Serif HTTP Server...</h1>\r\n";
		out << "<p>Please wait while the server restarts.</p>\r\n";
		out << "<p><a href=\"/\">Click here to go to the Serif HTTP Server homepage</a></p>\r\n";
		out << "</body></html>\r\n";
		std::string html_str = out.str();
		sendHTMLResponse(html_str);
		// Wait one second, to give us a chance to actually respond to the client.
		boost::asio::deadline_timer t(_ioService, boost::posix_time::seconds(1));
		t.async_wait(boost::bind(&SerifHTTPServer::restart, _server, force));
	}
}

void IncomingHTTPConnection::handleShutdownRequest(bool force) {
	_connectionDescription = "Handling Shutdown Request";
	if (_is_subprocess) {
		// Don't send a response -- doing so might cause us to block
		// (waiting for the response to get read), and so not shut down.
		_server->stop(true);
	} else {
		std::stringstream out;
		out << "<html>\r\n<head>\r\n<title>Serif HTTP Server: Shutdown</title>\r\n";
		out << "</head>\r\n";
		out << "<body>\r\nShutting down the Serif HTTP Server...\r\n";
		out << "</body>\r\n</html>\r\n";
		std::string html_str = out.str();
		sendHTMLResponse(html_str);
		// Wait one second, to give us a chance to actually respond to the client.
		boost::asio::deadline_timer t(_ioService);
		t.expires_from_now(boost::posix_time::seconds(1));
		t.async_wait(boost::bind(&SerifHTTPServer::stop, _server, force));
	}
}

void IncomingHTTPConnection::handleStatusRequest() {
	_connectionDescription = "Handling Status Request";
	std::stringstream out;
	// HTML Header
	out << "<html>\r\n<head>\r\n<title>Serif HTTP Server Status</title>\r\n";
	out << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/serif.css\">\r\n";
	out << "</head>\r\n";
	out << "<body>\r\n";
	out << "<div class=\"body\">\r\n";
	out << "<h1>Serif HTTP Server Status</h1>\r\n";
	out << getServerStatus() << "\r\n";
	out << "<p><a href=\"/\">Serif HTTP Server Home</a></p>\r\n";
	out << "</div>\r\n";
	out << "</body>\r\n";
	out << "</html>\r\n";
	std::string html_str = out.str();
	sendHTMLResponse(html_str);
}

std::string IncomingHTTPConnection::getServerStatus() {
	SerifWorkQueue *workQueue = SerifWorkQueue::getSingletonWorkQueue();
	std::vector<IncomingHTTPConnection_ptr> active_connections = _server->getActiveConnectionList();
	float throughputIncludingLoadTime = workQueue->getThroughputIncludingLoadTime();
	float throughputExcludingLoadTime = workQueue->getThroughputExcludingLoadTime();

	std::stringstream out;
	out << "<h2>Current Status: " << workQueue->getStatus() << "</h2>\r\n";
	out << "<table>\r\n";
	if (active_connections.size() > 0) {
		out << "<tr><th colspan=\"3\">Open Client Requests</th></tr>\r\n";
		BOOST_FOREACH(IncomingHTTPConnection_ptr conn, active_connections) {
			out << "  <tr><td>c" << conn->getConnectionId() << "</td><td colspan=\"2\">" 
				<< conn->_connectionDescription << "</td></tr>\r\n";
		}
		out << "<tr><td class=\"noborder\" width=\"1\">&nbsp;</td><td class=\"noborder\">"
			<< "&nbsp;</td><td class=\"noborder\" width=\"1\">&nbsp;</td></tr>\r\n";
	}
	out << "<tr><th colspan=\"3\">Statistics</th></tr>\r\n";
	out << "<tr><td colspan=\"2\">Successfully completed tasks</td><td>" << workQueue->numTasksProcessed() << "</td></tr>\r\n";
	out << "<tr><td colspan=\"2\">Failed tasks</td><td>" << workQueue->numTasksFailed() << "</td></tr>\r\n";
	out << "<tr><td colspan=\"2\">Tasks queued</td><td>" << workQueue->numTasksRemaining() << "</td></tr>\r\n";
	if (throughputIncludingLoadTime > 0) {
		out << std::setprecision(2)
			<< "<tr><td colspan=\"2\">Average throughput</td><td>" << throughputExcludingLoadTime 
			<< "MB/hr</td></tr>\r\n";
	}
	out << "</table>\r\n";
	return out.str();
}

std::string IncomingHTTPConnection::getServerActions() {
	std::stringstream out;
	out << "<h2> Server Control </h2>\r\n"
		<< "<form action=\"Shutdown\" id=\"ShutdownForm\" method=\"post\" style=\"display: none;\"></form>\r\n"
		<< "<form action=\"ImmediateShutdown\" id=\"ImmediateShutdownForm\" method=\"post\" style=\"display: none;\"></form>\r\n"
		<< "<form action=\"Restart\" id=\"RestartForm\" method=\"post\" style=\"display: none;\"></form>\r\n"
		<< "<form action=\"ImmediateRestart\" id=\"ImmediateRestartForm\" method=\"post\" style=\"display: none;\"></form>\r\n"
		<< "<ul>\r\n"
		<< "  <li> Shut down the server...\r\n"
		<< "    <ul>\r\n"
		<< "      <li> <a href=\"javascript: document.getElementById('ImmediateShutdownForm').submit()\">Immediately</a> </li>\r\n"
		<< "      <li> <a href=\"javascript: document.getElementById('ShutdownForm').submit()\">After it finishes any pending requests</a> </li>\r\n"
		<< "    </ul>\r\n"
		<< "  </li>\r\n"
		<< "  <li> Restart the server...\r\n"
		<< "    <ul>\r\n"
		<< "      <li> <a href=\"javascript: document.getElementById('ImmediateRestartForm').submit()\">Immediately</a> </li>\r\n"
		<< "      <li> <a href=\"javascript: document.getElementById('RestartForm').submit()\">After it finishes any pending requests</a> </li>\r\n"
		<< "    </ul>\r\n"
		<< "  </li>\r\n"
		<< "</ul>\r\n";
	return out.str();
}


void IncomingHTTPConnection::handleXMLContents(const HTTPMessage& message) {
	_connectionDescription = "Parsing SerifXMLRequest Body";
	writeLogMessage(0, "Parsing XML Message");
	try {
		// Take ownership of the document from the parser.  We will wait to delete
		// it until the connection is closed (when we know we have no further use
		// for it).
		_xmlRequest = XMLUtil::loadXercesDOMFromString(message.content.c_str(), message.content.size());
	} catch (InternalInconsistencyException &e) {
		reportError(500, e.getMessage());
		return;
	} catch (UnexpectedInputException &e) {
		std::ostringstream err;
		err << "<pre>\r\n" << HTTPConnection::encodeCharEntities(e.getMessage()) << "</pre>\r\n";
		reportError(400, err.str().c_str());
		return;
	}

	handleSerifXMLRequest();
}

/** Handle a <SerifXMLRequest> message from the client. */
void IncomingHTTPConnection::handleSerifXMLRequest() {
	if (_state == CLOSED) return;
	_connectionDescription = "Handling SerifXML Request";
	assert(_state == PROCESSING_REQUEST);
	writeLogMessage(0, "Handling XML Message");
	DOMElement* root = _xmlRequest->getDocumentElement();
	if (XMLString::compareIString(root->getTagName(), X_SerifXMLRequest)!=0) {
		reportError(400, "Expected a <SerifXMLRequest> root element; got", 
			transcodeToStdString(root->getTagName()));
		return;
	}

	// Choose a unique identifier for this session
	getSessionId(root->getAttribute(X_session_id));
	if (_state == SENDING_RESPONSE || _state == CLOSED) return; // we encountered an error

	// Check if any elements contain references to remote content; if so,
	// then download that remote content and insert it into the DOM tree.
	insertRemoteFiles(root, X_Document);
	if (_state == SENDING_RESPONSE || _state == CLOSED) return; // we encountered an error
	insertRemoteFiles(root, X_OriginalText);
	if (_state == SENDING_RESPONSE || _state == CLOSED) return; // we encountered an error

	if (_state == PROCESSING_REQUEST) {
		assert(_downloadConnections.empty());
		handleSerifXMLCommands();
	}
}

/** Handle the command(s) in a <SerifXMLRequest> message. */
void IncomingHTTPConnection::handleSerifXMLCommands() {
	DOMElement* root = _xmlRequest->getDocumentElement();
	std::vector<DOMElement*> commands = getDOMChildren(root);

	if (commands.size() == 0) {
		reportError(400, "Expected at least one command");
	} else if (commands.size() > 1) {
		reportError(400, "Multiple commands not currently supported");
	} else {
		BOOST_FOREACH(DOMElement* command, commands) {
			if (XMLString::compareIString(command->getTagName(), X_ProcessDocument)==0) {
				handleProcessDocumentCommand(command);
			} else if (XMLString::compareIString(command->getTagName(), X_PatternMatch)==0) {
				handlePatternMatchCommand(command);
			} else {
				reportError(400, "Unknown command", transcodeToStdString(command->getTagName()));
			}
			if (_state == SENDING_RESPONSE || _state == CLOSED) 
				return; // we encountered an error
		}
	}
	// We're all done with the XML document now.
	_xmlRequest->release();
	_xmlRequest = 0;
}

void IncomingHTTPConnection::handlePatternMatchCommand(DOMElement* command) {
	SessionLogger::dbg("IncomingHTTPConnection") << "IncomingHTTPConnection::handlePatternMatchCommand()";
	_connectionDescription = "Pattern matching against document";
	std::vector<DOMElement*> childElts = getDOMChildren(command);

	// Read our document and slots
	DOMDocument* document = 0;
	DOMDocument* slot1 = 0;
	DOMDocument* slot2 = 0;
	DOMDocument* slot3 = 0;
	std::map<std::wstring, float> slot1_weights;
	std::map<std::wstring, float> slot2_weights;
	std::map<std::wstring, float> slot3_weights;
	std::string message = "";

	DOMElement* equivNamesElt = 0;

	if (childElts.size() < 1) {
		reportError(400, "Expected command to contain at least one <Document>");
		return;
	}
	document = getDOMDocumentFromElement(childElts[0], message);

	bool first = true;
	BOOST_FOREACH(DOMElement* childElt, childElts) {
		if (first) {
			// skip first child element (it's the document)
			first = false;
			continue;
		}
		if (XMLString::compareIString(childElt->getTagName(), X_EquivalentNames) == 0) {
			equivNamesElt = childElt;
			continue;
		} 

		if (XMLString::compareIString(childElt->getTagName(), X_slot_weights) == 0) {
			std::string query_slot = transcodeToStdString(childElt->getAttribute(X_query_slot));
			if (query_slot == "slot1")
				slot1_weights = readSlotWeights(childElt);
			else if (query_slot == "slot2")
				slot2_weights = readSlotWeights(childElt);
			else if (query_slot == "slot3")
				slot3_weights = readSlotWeights(childElt);
			else {
				reportError(400, "slot_weights element with invalid query_slot attribute");
				return;
			}
		} else {
			DOMDocument* thing = getDOMDocumentFromElement(childElt, message);		
			if (message.size() > 0) {
				reportError(400, message);
				return;
			}
			if (slot1 == 0) {
				slot1 = thing;
			} else if (slot2 == 0) {
				slot2 = thing;
			} else if (slot3 == 0) {
				slot3 = thing;
			} else {
				reportError(400, "too many child elements");
				return;
			}
		}

	}

	// Extract our equivalent names
	typedef std::map<std::wstring,double> NameSynonyms;
	typedef std::map<std::wstring,NameSynonyms> NameDictionary;
	NameDictionary equiv_names;
	if (equivNamesElt) {
		DOMNodeList* names = equivNamesElt->getChildNodes();
		for (size_t i = 0; i < names->getLength(); i++) {
			DOMElement* name = dynamic_cast<DOMElement*>(names->item(i));
			if (name && XMLString::compareIString(name->getTagName(), X_Name) == 0) {
				std::wstring name_text = transcodeToStdWString(name->getAttribute(X_text));
				std::map<std::wstring,double>& map_for_this_name = equiv_names[name_text];
				DOMNodeList* equivs_for_this_name = name->getChildNodes();
				for (size_t j = 0; j < equivs_for_this_name->getLength(); j++) {
					DOMElement* equiv_name = dynamic_cast<DOMElement*>(equivs_for_this_name->item(j));
					if (equiv_name && XMLString::compareIString(equiv_name->getTagName(), X_EquivalentName) == 0) {
						std::wstring equiv_name_text = transcodeToStdWString(equiv_name->getAttribute(X_text));
						double score = boost::lexical_cast<double>(transcodeToStdString(equiv_name->getAttribute(X_score)));
						map_for_this_name[equiv_name_text] = score;
					}
				}
			}
		}
	}	
	
	// Debugging code to print out equiv names
	if (SessionLogger::dbg_or_msg_enabled("IncomingHTTPConnection")) {
		BOOST_FOREACH(const NameDictionary::value_type& outer_pair, equiv_names) {
			const std::wstring& name = outer_pair.first;
			const NameSynonyms& equivs_for_this_name = outer_pair.second;
			BOOST_FOREACH(const NameSynonyms::value_type& inner_pair, equivs_for_this_name) {
				const std::wstring& equiv_name = inner_pair.first;
				double score = inner_pair.second;
				SessionLogger::dbg("IncomingHTTPConnection") << name << ": (" << equiv_name << ", " << score << ")\n";
			}
		}
	}

	// Extract all attributes of the <PatternMatch> command, so we can
	// supply them as options for the PatternMatchDocumentTask.
	PatternMatchDocumentTask::OptionMap *optionMap = _new PatternMatchDocumentTask::OptionMap();
	DOMNamedNodeMap *attrMap = command->getAttributes();
	for (size_t i=0; i<attrMap->getLength(); ++i) {
		DOMAttr* attr = static_cast<DOMAttr*>(attrMap->item(i));
		(*optionMap)[attr->getName()] = attr->getValue();
	}

	// Add a task to the SERIF work queue to process this document.  The
	// ProcessDocumentTask takes ownership of both the document and the
	// optionMap.
	SerifWorkQueue::Task_ptr task(_new PatternMatchDocumentTask(
		document, slot1, slot2, slot3, slot1_weights, slot2_weights, slot3_weights, equiv_names, optionMap, _ioService, shared_from_this()));
	SerifWorkQueue::getSingletonWorkQueue()->addTask(task);
}


std::map<std::wstring, float> IncomingHTTPConnection::readSlotWeights(xercesc::DOMElement *slot_weights) {
	std::map<std::wstring, float> results;
	
	if (slot_weights == 0)
		return results;

	DOMNodeList* predicates = slot_weights->getChildNodes();
	for (size_t i = 0; i < predicates->getLength(); i++) {
		DOMNode *n = predicates->item(i);
		if (n->getNodeType() == DOMNode::ELEMENT_NODE) {
			DOMElement* pred = dynamic_cast<DOMElement*>(predicates->item(i));
			if (XMLString::compareIString(pred->getTagName(), X_predicate) == 0) {
				float weight = boost::lexical_cast<float>(transcodeToStdString(pred->getAttribute(X_weight)));
				std::wstring text = transcodeToStdWString(pred->getTextContent());
				results[text] = weight;
			}
		}
	}
	return results;
}

void IncomingHTTPConnection::handleProcessDocumentCommand(DOMElement* command) {
	_connectionDescription = "Processing document";

	// Look for Serif <Document> under the command and put it in 
	// a DOMDocument
	std::string errorString;
	DOMDocument *document = getDOMDocumentFromCommand(command, errorString);
	if (!document) {
		reportError(400, errorString.c_str());
		return;
	}

	// Extract all attributes of the <ProcessDocument> command, so we can
	// supply them as options for the ProcessDocumentTask.
	ProcessDocumentTask::OptionMap *optionMap = _new ProcessDocumentTask::OptionMap();
	DOMNamedNodeMap *attrMap = command->getAttributes();
	for (size_t i=0; i<attrMap->getLength(); ++i) {
		DOMAttr* attr = static_cast<DOMAttr*>(attrMap->item(i));
		(*optionMap)[attr->getName()] = attr->getValue();
	}

	// Add a task to the SERIF work queue to process this document.  The
	// ProcessDocumentTask takes ownership of both the document and the
	// optionMap.
	SerifWorkQueue::Task_ptr task(_new ProcessDocumentTask(
		document, optionMap, _sessionId, 
		_user_supplied_session_id, _ioService, shared_from_this()));
	SerifWorkQueue::getSingletonWorkQueue()->addTask(task);
}

std::vector<DOMElement*> IncomingHTTPConnection::getDOMChildren(DOMElement* parent) {
	std::vector<DOMElement*> result;
	DOMNodeList *children = parent->getChildNodes();
	for (size_t i=0; i<children->getLength(); ++i) {
		DOMElement *elt = dynamic_cast<DOMElement*>(children->item(i));
		if (elt) result.push_back(elt);
	}
	return result;
}

void IncomingHTTPConnection::getSessionId(const XMLCh* userSuppliedId) {
	_user_supplied_session_id = (userSuppliedId && userSuppliedId[0]);
	if  (_user_supplied_session_id) {
		if (!boost::regex_match(transcodeToStdString(userSuppliedId), acceptableSessionId))
			reportError(400, "Unacceptable session id: must be alphanumeric");
		else
			_sessionId = transcodeToStdWString(userSuppliedId);
	} else {
		_sessionId = L"session_" + boost::lexical_cast<std::wstring>(getConnectionId());
	}
}

void IncomingHTTPConnection::handleDoc2OutCommand(const HTTPMessage& message, Doc2OutTask::OutputFormat output_format) {
	SerifWorkQueue::Task_ptr task(_new Doc2OutTask(message.content, output_format, _ioService, shared_from_this()));
	SerifWorkQueue::getSingletonWorkQueue()->addTask(task);
}

DOMDocument* IncomingHTTPConnection::getDOMDocumentFromCommand(DOMElement* command, std::string &errorString) {
	std::vector<DOMElement*> childElts = getDOMChildren(command);

	// Make sure there's only one child
	if (childElts.size() != 1) {
		errorString = "Expected command to contain exactly one <Document>";
		return 0;
	}
	DOMDocument *document = getDOMDocumentFromElement(childElts[0], errorString);
	return document;
}

DOMDocument* IncomingHTTPConnection::getDOMDocumentFromElement(DOMElement* element, std::string &errorString) {

	// It's ok if there's a <SerifXML> wrapper element.
    if (XMLString::compareIString(element->getTagName(), X_SerifXML) == 0) {
		std::vector<DOMElement*> childElts = getDOMChildren(element);
        if (childElts.size() != 1) {
			errorString = "<SerifXML> should contain at most one child <Document> element.";
            return 0;
		}
		element = childElts[0];
    }

	// Make sure there's a single <Document> element.
	if (XMLString::compareIString(element->getTagName(), X_Document) != 0) {
		errorString = "Expected <ProcessDocument> command to contain exactly one <Document> element";
		return 0;
	}

	// Create a new DOMDocument to hold the document.
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);
	DOMDocument *document = impl->createDocument(0, X_SerifXML, 0);
	document->getDocumentElement()->appendChild(document->importNode(element, true));
	return document;
}

//========================== Response Sending ==============================

void IncomingHTTPConnection::sendResponse(std::string &out) {
	// If we already sent a response (e.g. because of an error), then don't 
	// send another one.  This could happen, for example, if we're downloading
	// several files at once, and multiple files cause us to respond with
	// an error message.
	if (_state == SENDING_RESPONSE || _state == CLOSED)
		return;
	if (!_response.empty()) 
		return; 

	if (_logdir && !_logfile_prefix.empty()) {
		logHTTPMessage("response", out.c_str());
	}

	writeLogMessage(0, "Sending Response");
	_state = SENDING_RESPONSE;

	// Steal the contents of the response message, and store them
	// in a local buffer.
	out.swap(_response);
	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(boost::asio::buffer(_response));

	// Initiate an asyncronous write with the buffer.
	getIOConnection().async_write(buffers,
		boost::bind(&IncomingHTTPConnection::handleWrite, shared_from_this(),
		            boost::asio::placeholders::error));
}

void IncomingHTTPConnection::handleWrite(const boost::system::error_code& e)
{
	if (_state == CLOSED) return;
	assert(_state == SENDING_RESPONSE);
	if (e == boost::asio::error::operation_aborted) {
		stop();
	} else if (e == boost::asio::error::eof) {
		_state = CLOSED;
	} else if (e) {
		writeLogMessage(0, "Unknown error during handleWrite: ", e);
		stop();
	} else {
		if (_is_subprocess) {
			// Reset and read the next message!
			reset();
			start();
			writeLogMessage(0, "Success");
		} else {
			// Shut down the socket -- we're done!
			getIOConnection().shutdown();
			_state = CLOSED;
			writeLogMessage(0, "Success");
		}
	}
}

bool IncomingHTTPConnection::reportError(size_t error_code, 
									 std::string explanation,
									 std::string extra)
{
	_connectionDescription = "Reporting error:" + explanation + " " + extra;
	std::string message;
	if (error_code == 400) message = "BAD REQUEST";
	else if (error_code == 404) message = "NOT FOUND";
	else if (error_code == 500) message = "SERVER ERROR";
	else message = "ERROR";

	if (_verbosity >= 0) {
		std::stringstream err1, err2;
		err1 << "HTTP Connection error: " << error_code
		     << " (" << message << ")";
		err2 << "    " << explanation;
		if (!extra.empty())
			err2 << ": \"" << extra << "\"";
		writeLogMessage(0, err1.str());
		writeLogMessage(0, err2.str());
	}

	std::stringstream body; // The body of the error message
	if (_is_subprocess) {
		body << "[" << message << "] " << explanation;
		if (!extra.empty())
			body << ": \"" << extra << "\"";
		body << "\r\n";
	} else {
		body << "<html>\r\n";
		body << "<head><title>Serif XML Server: " 
			 << encodeCharEntities(message) << "</title></head>\r\n";
		body << "<body>\r\n";
		body << "<h1>Serif XML Server: " << error_code 
			 << " " << encodeCharEntities(message) << "</h1>\r\n";
		body << "<p>" << encodeCharEntities(explanation);
		if (!extra.empty())
			body << ": \"" << encodeCharEntities(extra) << "\"";
		body << "</p>" << "\r\n";
		body << "</body>\r\n";
		body << "</html>\r\n";
	}

	std::stringstream out;
	out << "HTTP/1.0 " << error_code << " " << message << "\r\n";
	out << "Content-length: " << body.str().size() << "\r\n\r\n";
	out << body.str();
	std::string out_str = out.str();

	sendResponse(out_str);
	return false;
}

void IncomingHTTPConnection::sendResponse(DOMDocument *xmldoc)
{
	std::string body;
	XMLUtil::saveXercesDOMToString(xmldoc, body);
	std::stringstream out;
	out << "HTTP/1.0 200 OK\r\n";
	out << "Content-length: " << body.size() << "\r\n";
	out << "Content-type: text/xml" << "\r\n";
	out << "\r\n"; // End of headers
	out << body;
	xmldoc->release();
	std::string out_str = out.str();
	sendResponse(out_str);
}

// Return false if we encounter an error.

//========================== Remote File Download ===========================
void IncomingHTTPConnection::insertRemoteFiles(DOMElement* root, const XMLCh* tag) {
	DOMNodeList *documentElements = root->getElementsByTagName(tag);
	for (size_t i=0; i<documentElements->getLength(); ++i) {
		DOMElement* elem = dynamic_cast<DOMElement*>(documentElements->item(i));
		if (elem->hasAttribute(X_href)) {
			std::string uri = transcodeToStdString(elem->getAttribute(X_href));
			boost::smatch match;

			// Does the href contain an HTTP URL?  If so, then download it.
			if (boost::regex_match(uri, match, httpUrlRegexp)) {
				std::string server = match.str(1);
				std::string port = match.str(2);
				std::string path = match.str(3);
				if (!port.empty()) {
					reportError(400, "Remote HTTP file URLs with ports not supported yet");
					return;
				}
				downloadRemoteFile(elem, uri, server, path);
			}
			// Does the href contain a file:// url?  If so, then read it.
			else if (boost::regex_match(uri, match, fileUrlRegexp)) {
				std::string file_path = match.str(1);
				if (!OutputUtil::isAbsolutePath(file_path.c_str())) {
					reportError(400, "File URLs must be absolute", file_path);
					return;
				}
				std::string file_contents;
				if (!readFileIntoString(file_path, file_contents)) {
					reportError(400, "File not found or error reading file", file_path);
					return;
				}
				if (!insertRemoteFileContents(elem, file_contents))
					return;
			}
			else {
				reportError(400, "Unsupported href URL", uri);
				return;
			}
		}
	}
}

// Return false if we encounter an error.
bool IncomingHTTPConnection::insertRemoteFileContents(DOMElement* targetElem, const std::string& file_contents) {
	assert(targetElem != 0);
	std::string url = transcodeToStdString(targetElem->getAttribute(X_href));
	if (XMLString::compareIString(targetElem->getTagName(), X_Document)==0)
		return insertRemoteDocTheory(targetElem, file_contents);
	else if (XMLString::compareIString(targetElem->getTagName(), X_OriginalText)==0)
		return insertRemoteOriginalText(targetElem, file_contents);
	else {
		reportError(400, "href on unexpected element type", 
			transcodeToStdString(targetElem->getTagName()));
		return false;
	}
}

bool IncomingHTTPConnection::insertRemoteDocTheory(DOMElement* targetElem, const std::string& file_contents) {
	std::string uri = transcodeToStdString(targetElem->getAttribute(X_href));

	// Make sure the document is empty. (also check for text and attribs??)
	if (!getDOMChildren(targetElem).empty()) {
		return reportError(400, "<Document> element may not have both an HREF attribute and child elements");
	}

	// Parse the downloaded file's contents
	DOMDocument* importedDocument = 0;
	try {
		importedDocument = XMLUtil::loadXercesDOMFromString(file_contents.c_str(), file_contents.size());
	} catch (InternalInconsistencyException &e) {
		std::ostringstream err;
		err << "In remote file " << uri << ":\r\n" 
			<< HTTPConnection::encodeCharEntities(e.getMessage());
		return reportError(500, err.str().c_str());
	} catch (UnexpectedInputException &e) {
		std::ostringstream err;
		err << "In remote file " << uri << ":\r\n<pre>\r\n" 
			<< HTTPConnection::encodeCharEntities(e.getMessage()) << "</pre>\r\n";
		return reportError(400, err.str().c_str());
	}
	DOMElement* importedRoot = importedDocument->getDocumentElement();

	// Check that the imported XML file contains <SerifXML>
	if (XMLString::compareIString(importedRoot->getTagName(), X_SerifXML) != 0)
		reportError(400, "Expected remote file to contain a <SerifXML> element", uri);

	// Make sure there's a single <Document> element.
	std::vector<DOMElement*> childElts = getDOMChildren(importedRoot);
	if ((childElts.size() != 1) || (XMLString::compareIString(childElts[0]->getTagName(), X_Document)!=0))
		return reportError(400, "Expected <SerifXML> element to contain exactly one <Document> element");

	// Import the new <Document> element into the target's DOMDocument.
	DOMDocument* targetDoc = targetElem->getOwnerDocument();
	DOMElement* importedElem = dynamic_cast<DOMElement*>(targetDoc->importNode(childElts[0], true));

	// <Document> attributes specified in the original target override the
	// values in the imported <Document>
	DOMNamedNodeMap *attribs = targetElem->getAttributes();
	for (size_t i=0; i<attribs->getLength(); ++i) {
		DOMAttr *attrib = dynamic_cast<DOMAttr*>(attribs->item(i));
		if (attrib != 0) { // Just in case
			if (XMLString::compareIString(attrib->getName(), X_href) != 0)
				importedElem->setAttribute(attrib->getName(), attrib->getValue());
		}
	}

	// Replace the old <Document> element with the new one.
	DOMElement* targetParent = dynamic_cast<DOMElement*>(targetElem->getParentNode());
	if (targetParent == 0)
		return reportError(400, "<Document> element has no parent!"); // shouldn't ever happen.
	targetParent->removeChild(targetElem);
	targetParent->appendChild(importedElem);

	importedDocument->release();
	// [XX] should we call targetElem->release() here?
	return true;
}

bool IncomingHTTPConnection::insertRemoteOriginalText(DOMElement* targetElem, const std::string& file_contents) {
	DOMDocument *domDocument = targetElem->getOwnerDocument();
	DOMNodeList *contentsElements = targetElem->getElementsByTagName(X_Contents);
	if (contentsElements->getLength() == 0) {
		// Create a <Contents> element containing the document contents.
		DOMElement *contentsElem = domDocument->createElement(X_Contents);
		targetElem->appendChild(contentsElem);
		contentsElem->appendChild(
			domDocument->createTextNode(transcodeToXString(file_contents.c_str()).c_str()));
	} else if (contentsElements->getLength() == 1) {
		// Verify that the <Contents> matches.
		xstring oldContent(contentsElements->item(0)->getTextContent());
		xstring newContent = transcodeToXString(file_contents.c_str());
		if (oldContent != newContent) {
			reportError(400, "Document original text's <Content> does not match the "
				"contents from the URL", transcodeToStdString(targetElem->getAttribute(X_href)).c_str());
			return false;
		}
	} else {
		reportError(400, "Document's <OriginalText> element has more than one "
			"<Content> child element");
		return false;
	}
	return true;
}

class IncomingHTTPConnection::DownloadFileResponseHandler: public OutgoingHTTPConnection::ResponseHandler,
				public boost::enable_shared_from_this<DownloadFileResponseHandler>{
public:
	DownloadFileResponseHandler(IncomingHTTPConnection_ptr caller, std::string uri,
		xercesc::DOMElement *target): _incomingConnection(caller), _uri(uri), _target(target) {}
	virtual void handleResponse(const HTTPMessage& message);
	virtual void handleResponseError(const std::string &err) {
		_incomingConnection->reportError(404, 
			"Error while downloading remote URL <"+_uri+">", err); }
private:
	IncomingHTTPConnection_ptr _incomingConnection;
	xercesc::DOMElement *_target;
	std::string _uri;
};

void IncomingHTTPConnection::downloadRemoteFile(DOMElement* target, std::string uri, std::string server, std::string path) {
	if (_state == CLOSED) return;
	// Update our state.
	assert(_state == PROCESSING_REQUEST || _state == DOWNLOADING_REMOTE_FILE);
	_state = DOWNLOADING_REMOTE_FILE;
	// Assemble the request
	std::stringstream request;
	request << "GET " << path << " HTTP/1.0\r\n";
	request << "Host: " << server << "\r\n";
	request << "Accept: */*\r\n";
	request << "Connection: close\r\n\r\n";
	// Create a new outgoing connection to download the remote file.
	boost::shared_ptr<DownloadFileResponseHandler> downloadResponseHandler(
		_new DownloadFileResponseHandler(shared_from_this(), uri, target));
	boost::shared_ptr<OutgoingHTTPConnection> downloadConnection(
		_new OutgoingHTTPConnection(_ioService, server, request.str(), downloadResponseHandler));
	// Record the download connection and its associated handler.
	_downloadConnections[downloadResponseHandler] = downloadConnection;
	// Initiate the download.
	downloadConnection->connect();
}

void IncomingHTTPConnection::DownloadFileResponseHandler::handleResponse(const HTTPMessage& response) {
	// If we already encountered an error (eg while downloading another file) 
	// and reported it back to the client, then ignore this download response.
	if (_incomingConnection->_state == SENDING_RESPONSE || 
		_incomingConnection->_state == CLOSED)
		return;

	assert(_incomingConnection->_state == PROCESSING_REQUEST || 
		   _incomingConnection->_state == DOWNLOADING_REMOTE_FILE);

	_incomingConnection->writeLogMessage(0, "Done downloading a Remote File");

	// Remove ourself from the list of downloads in progress.  (We need to do
	// this *before* we call _incomingConnection->insertRemoteFileContents,
	// so the incoming connection will know if it's done downloading files yet.)
	_incomingConnection->_downloadConnections.erase(shared_from_this());

	// Insert the file contents into the XML tree.
	if (!_incomingConnection->insertRemoteFileContents(_target, response.content))
		return;

	// Once there are no documents left to download, then we can proceed with
	// the command.  Otherwise, the last download to finish will do it.  (Note
	// that we rely on the fact that all these connections occur in the same
	// thread, so we don't need to worry about locking here.)
	if (_incomingConnection->_downloadConnections.empty()) {
		_incomingConnection->_state = PROCESSING_REQUEST;
		_incomingConnection->handleSerifXMLCommands();
	}
}

//====================== HTTP Message Logging ==========================

namespace {
	template<class T1, class T2>
	std::string join_paths(T1 p1, T2 p2) {
		std::stringstream s;
		s << p1 << SERIF_PATH_SEP << p2;
		return s.str();
	}
}

void IncomingHTTPConnection::logHTTPMessage(const char* message_type, 
											const char* message) {
	std::string filename = _logfile_prefix + "." + message_type;
	std::fstream out(filename.c_str(), std::fstream::out);
	out << message;
	out.close();
}

/** Choose a vlaue for _logfile_prefix for an incoming http
 * connection.  If the necessary parent directories don't already
 * exist, then create them. */
void IncomingHTTPConnection::pick_logfile_prefix(const HTTPMessage &message) {
	// Set up parent directories.  We use a separate subdirectory for
	// each endpoint address (IP address).
	OutputUtil::makeDir(_logdir);
	std::string parent_dir = join_paths(_logdir, getIOConnection().getEndpointAddress());
	OutputUtil::makeDir(parent_dir.c_str());

	// Pick a unique prefix for this connection, using both the
	// connection id (which is unique within this process) and the
	// current time (in case the server is restarted).
	time_t tim=time(NULL);
	tm *now=localtime(&tim);
	char filename_prefix[128];
	_snprintf(filename_prefix, 127, "%d-%02d-%02d.%02d%02d%02d.%06lu", 
			 now->tm_year+1900, now->tm_mon+1, now->tm_mday, 
			 now->tm_hour, now->tm_min, now->tm_sec, 
			 static_cast<unsigned long>(getConnectionId()));

	// Set the prefix.
	_logfile_prefix = join_paths(parent_dir, filename_prefix);
}


//========================== Logging ===========================
template<typename T1>
void IncomingHTTPConnection::writeLogMessage(int min_verbosity, T1 m1) {
	std::stringstream message;
	message << m1;
	if (_verbosity >= min_verbosity)
		std::cerr << "[" << SerifVersion::getProductName() << "HTTPServer:c" << getConnectionId() << "] " << boost::posix_time::second_clock::local_time() << " " << message.str() << std::endl;
}

template<typename T1, typename T2>
void IncomingHTTPConnection::writeLogMessage(int min_verbosity, T1 m1, T2 m2) {
	std::stringstream message;
	message << m1 << " " << m2;
	if (_verbosity >= min_verbosity)
		std::cerr << "[" << SerifVersion::getProductName() << "HTTPServer:c" << getConnectionId() << "] " << boost::posix_time::second_clock::local_time() << " " << message.str() << std::endl;
}

template<typename T1, typename T2, typename T3>
void IncomingHTTPConnection::writeLogMessage(int min_verbosity, T1 m1, T2 m2, T3 m3) {
	std::stringstream message;
	message << m1 << " " << m2 << " " << m3;
	if (_verbosity >= min_verbosity)
		std::cerr << "[" << SerifVersion::getProductName() << "HTTPServer:c" << getConnectionId() << "] " << boost::posix_time::second_clock::local_time() << " " << message.str() << std::endl;
}
