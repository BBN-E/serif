// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_XML_SERVER_H
#define SERIF_XML_SERVER_H

#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <xercesc/dom/DOM.hpp>

class DocumentDriver;
class Stage;

/** HTTP server responsible for handling requests */
class SerifXMLServer {
public:
	explicit SerifXMLServer(const char* parameter_file, int port);
	~SerifXMLServer();

	// Start the server.  (blocking)
	void run();

	// Stop the server.  
	void stop();

private:
	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& e);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service _ioService;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor _acceptor;

	/** A single connection.  Handles processing the HTTP message, 
	  * and delegating it to the proper place.
	  */
	class Connection : public boost::enable_shared_from_this<Connection>, private boost::noncopyable
{
	public:
		Connection(SerifXMLServer *server): 
		  _server(server), socket(server->_ioService), _state(READ_REQUEST_LINE),
		  _end_of_request_line(0), _end_of_header_lines(0), _content_length(0) {}
		void start();
		void stop();
		boost::asio::ip::tcp::socket socket;
	private:
		// Our server
		SerifXMLServer *_server;
		// Buffer for incoming data.
		boost::array<char, 8192> _buffer;
		// The HTTP request message:
		std::string _request;
		size_t _end_of_request_line;
		size_t _end_of_header_lines;

		// Information extracted from the HTTP request information:
		std::string _request_method;
		std::string _request_uri;
		std::string _content_encoding;
		size_t _content_length;

		// Buffer for outgoing response:
		std::string _response;

		// Handle completion of a read operation.
		void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		// Handle completion of a write operation.
		void handleWrite(const boost::system::error_code& e);
		// Send the _response string to back to the client.
		void sendResponse();

		// Handle different parts of the HTTP request message.  Return true for success.
		bool handleRequestLine();
		bool handleHeaders();
		bool handleContents();
		bool handleXMLContents();
		bool handleSerifXMLRequest(xercesc::DOMDocument *xmldoc);

		bool handleProcessDocumentCommand(xercesc::DOMDocument *xmldoc, xercesc::DOMElement *cmd);
		bool sendResponse(xercesc::DOMDocument *xmldoc);

		// Misc helpers
		bool checkLanguageAttribute(xercesc::DOMElement *cmd);
		Stage getStageAttribute(xercesc::DOMElement *elt, const XMLCh *attr, Stage default_value);

		// Error reporting.
		bool reportError(size_t error_code, const std::string &explanation);
		bool reportError(size_t error_code, const std::string &explanation, const std::string &extra);

		typedef enum {
			READ_REQUEST_LINE, READ_HEADERS, READ_CONTENTS,
			WORKING, SEND_RESPONSE} ConnectionState;
		ConnectionState _state;
	};
	typedef boost::shared_ptr<Connection> Connection_ptr;

	// Active connections
	std::set<Connection_ptr> _activeConnections;

	// The next connection we'll use.
	Connection_ptr _nextConnection;

	DocumentDriver *_documentDriver;

};

#endif // SERIF_XML_SERVER_H
