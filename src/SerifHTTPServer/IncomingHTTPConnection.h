// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_HTTP_CONNECTION_H
#define SERIF_HTTP_CONNECTION_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include <xercesc/dom/DOM.hpp>
#include "SerifHTTPServer/HTTPConnection.h"
#include "SerifHTTPServer/OutgoingHTTPConnection.h"
#include "SerifHTTPServer/Doc2OutTask.h"

class SerifHTTPServer;

/** An incoming HTTP connection from a client.  This class is responsible
  * for reading the client's request message, and responding appropriately.
  * The IncomingHTTPConnection class supports both GET and POST request
  * messages:
  *
  *   - The connection responds to "GET" messages by translating the URL
  *     to a filename (using SerifHTTPServer::getDocumentFilename()),
  *     and returning the contents of that file.
  *
  *   - The connection responds to "POST SerifXMLRequest" messages by
  *     performing the command(s) listed in the XML-formatted request
  *     message contents.
  *
  *   - The connection responds to "POST sgm2apf" messages by processing
  *     the request message contents using SERIF, and returning the
  *     resulting APF file.
  *
  *   - etc. (fill this in as we add more request messages)
  *
  */
class IncomingHTTPConnection
	: public boost::enable_shared_from_this<IncomingHTTPConnection>,
	  public HTTPConnection
{
public: // ============== Public Interface ================

	/** Create a new connection for the server.  When a new connection is created,
	  * its socket is not yet connected to a client.  The start() method should
	  * be called once a client connects to the socket. */
	IncomingHTTPConnection(SerifHTTPServer *server, boost::asio::io_service &ioService, int verbosity, bool is_subprocess,
						   const char* logdir_root=NULL);

	~IncomingHTTPConnection();

	/** Start reading a response from the connection.  This should be called 
	  * once the connection's socket is connected to a client. */
	void start();

	/** Close this connection.  This will hang up on the client if one is
	  * connected. */
	virtual void stop();

	/** Send a response indicating that there was an error that prevented 
	  * the request from being satisfied.  'error_code' is a standard HTTP
	  * error code (eg 404 for NOT FOUND), and 'explanation' is a string 
	  * explaining the problem.  If the optional 'extra' argument is given,
	  * then it will be appended (in quotes, and separated by a colon) to
	  * the explanation.  */
	bool reportError(size_t error_code, std::string explanation, std::string extra="");

	/** Send an XML document as a response to the client.  Once the 
	  * XML document has been sent, this method will automatically
	  * call xmldoc->release(). */
	void sendResponse(xercesc::DOMDocument* xmldoc);

	// Send the the given string back to the client.  The parameter 'out'
	// is modified (in particular, its contents are swapped into the
	// member variable _response).
	void sendResponse(std::string &out);

	std::string getConnectionDescription() const { return _connectionDescription; }

	template<typename AcceptHandler>
	void accept_from(boost::asio::ip::tcp::acceptor &acceptor, AcceptHandler handler) {
		getIOConnection().accept_from(acceptor, handler);
	}


private: // ============ Member Variables ============

	// An enum used to keep track of the current state that the connection
	// is in.
	typedef enum {
		LISTENING,                // Not yet connected to a client
		READING_REQUEST,          // Reading the request message from the client
		PROCESSING_REQUEST,       // Processing the client's request
		DOWNLOADING_REMOTE_FILE,  // Downloading file(s) from remote servers
		SENDING_RESPONSE,         // Sending a response back to the client
		CLOSED                    // All done.
	} ConnectionState;
	ConnectionState _state;

	// A pointer to the server that owns this connection.
	SerifHTTPServer *_server;

	// The XML request document that was parsed from the contents of a
	// "POST SerifXMLRequest" request.   
	// This is owned by the IncomingHTTPConnection, but will be used by
	// the SerifWorkQueue, so it should not be deleted until the
	// connection is deleted. [XX] This may change soon.
	xercesc::DOMDocument *_xmlRequest;

	// Buffer for our outgoing response:
	std::string _response;

	// asyncronous io service.
	boost::asio::io_service &_ioService;

	// A map used to keep track of outgoing connections that we're using
	// to download files (in order to fulfill a SerifXMLRequest request).
	class DownloadFileResponseHandler;
	typedef boost::shared_ptr<DownloadFileResponseHandler> DownloadFileResponseHandler_ptr;
	typedef boost::shared_ptr<OutgoingHTTPConnection> OutgoingHTTPConnection_ptr;
	std::map<DownloadFileResponseHandler_ptr, OutgoingHTTPConnection_ptr> _downloadConnections;

	// How noisy should we be?
	int _verbosity;

	// Are we a subprocess?
	bool _is_subprocess;

	// Unique identifier.  This maps to a subdirectory where the session log
	// and any other output from this session is stored.  We may also allow
	// caching of output files, etc, and this would allow the user to get 
	// access to those.  The session id is usually generated by the server.
	// The user can specify one if they want, but if it's already used we'll
	// report back an error (or should we overwrite?)
	std::wstring _sessionId;
	bool _user_supplied_session_id;

	// Description of this connection (for the status page)
	std::string _connectionDescription;

	// Root directory that should be used for logging requests and
	// responses, or NULL if logging is not enabled.
	const char* _logdir;

	// Prefix of filename used to log this connection's request & response.
	std::string _logfile_prefix;

private: // ============ Helper Methods ============
	virtual boost::shared_ptr<HTTPConnection> getSharedPointerToThis() {
		return shared_from_this(); }

	// Handlers used to process the request message
	virtual void handleMessage(const HTTPMessage& message);
	void handleGetRequest(const HTTPMessage& message);
	void handleGetDocumentRequest(std::string path);
	void handleStatusRequest();
	void handleShutdownRequest(bool force);
	void handleRestartRequest(bool force);
	void handleXMLContents(const HTTPMessage& message);
	void handleSerifXMLRequest();
	void handleSerifXMLCommands();
	void handleProcessDocumentCommand(xercesc::DOMElement* command);
	void handlePatternMatchCommand(xercesc::DOMElement* command);
	std::map<std::wstring, float> readSlotWeights(xercesc::DOMElement *slot_weights);
	void handleDoc2OutCommand(const HTTPMessage& message, Doc2OutTask::OutputFormat output_format);
	void handleGetReply(const HTTPMessage& message);
	xercesc::DOMDocument* getDOMDocumentFromCommand(xercesc::DOMElement* command, std::string &errorString);
	xercesc::DOMDocument* getDOMDocumentFromElement(xercesc::DOMElement* element, std::string &errorString);

	std::string getServerStatus();
	std::string getServerActions();
	void sendHTMLResponse(const std::string& html_contents, const char* content_type="text/html");

	/** Check if the given DOMElement contains any references to remote 
	  * files; and if so, download the contents of those files.  */
	void insertRemoteFiles(xercesc::DOMElement* root, const XMLCh* tag);

	/** Insert the string 'file_contents' into the given target element.
	  * The target element should be either a <Document> element or an
	  * <OriginalText> element. */
	bool insertRemoteFileContents(xercesc::DOMElement* targetElem, const std::string& file_contents);
	bool insertRemoteDocTheory(xercesc::DOMElement* targetElem, const std::string& file_contents);
	bool insertRemoteOriginalText(xercesc::DOMElement* targetElem, const std::string& file_contents);

	/** Create an OutgoingHTTPConnection and use it to download a 
	  * specified remote file, for use as the contents of a given
	  * DOMElement target.  The target should be either a 
	  * <Document> element or an <OriginalText> element. */
	void downloadRemoteFile(xercesc::DOMElement* target, std::string uri, std::string server, std::string path);

	/** Handler that is called when we're done downloading a remote
	  * file.  Insert the file's contents into the DOM tree; and if
	  * there are no more files to download, then resume processing
	  * the XML message. */
	void handleDownloadRemoteFileResponse(const HTTPMessage& message);

	// Error handler used by readMessage().
	virtual void handleReadMessageError(const std::string &description);
	virtual void handleReadMessageEOF(const std::string& description);

	// Handle completion of a write operation.
	void handleWrite(const boost::system::error_code& e);

	virtual void reset();

	// Helper methods
	std::vector<xercesc::DOMElement*> getDOMChildren(xercesc::DOMElement* parent);
	void getSessionId(const XMLCh* userSuppliedId=0);

	void pick_logfile_prefix(const HTTPMessage &message);
	void logHTTPMessage(const char* message_type, const char* message);

	// Generic logging methods.
	template<typename T1>
	void writeLogMessage(int min_verbosity, T1 m1);
	template<typename T1, typename T2>
	void writeLogMessage(int min_verbosity, T1 m1, T2 m2);
	template<typename T1, typename T2, typename T3>
	void writeLogMessage(int min_verbosity, T1 m1, T2 m2, T3 m3);

};

#endif
