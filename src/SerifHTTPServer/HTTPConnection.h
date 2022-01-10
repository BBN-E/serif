// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include "SerifHTTPServer/IOConnection.h"


/** Abstract base class for both incoming and outgoing HTTP connections used
  * by the HTTP server.  Incoming connections handle requests from clients;
  * and outgoing connections are used to connect with remote servers (e.g.
  * to retrieve the contents of a document that we wish to process).
  *
  * Each HTTP connection wraps a single Boost tcp::socket object, which it 
  * uses to communicate with the client or remote server.  All communication 
  * is performed using asynchronous IO.  This means that all IO commands 
  * (such as reading from or writing to the socket) are done by registering 
  * callbacks, which are called when the IO completes.  See the documentation
  * for boost::asio for more information.
  *
  * The socket is initially closed -- it is the responsibility of the subclass 
  * to connect it with either a client (for incoming connections) or a remote 
  * sever (for outgoing connections).
  *
  * HTTPConnection defines the protected method readMessage(), which can be
  * called by subclasses to read an HTTP message from the socket, parse it 
  * into a Message object, and call the message handler method.  The 
  * readMessage() method may be used to read both HTTP request messages (for 
  * incoming connections) and HTTP response messages (for outgoing connections).
  *
  * HTTPConnection objects should always be stored using boost::shared_ptr,
  * and subclasses are expected to use boost::enable_shared_from_this to
  * allow the HTTPConnection to get access to its own shared pointer.  Using
  * a boost::shared_ptr lets us ensure that this object will still exist
  * when boost::asio callbacks are called, even if the original owner no 
  * longer has a copy of the connection object.
  */
class HTTPConnection : private boost::noncopyable
{
public: // ============== Public Interface ================

	/** Return a unique identifier for this connection.  This is intended
	  * to be used for logging purposes only. */
	size_t getConnectionId() { return _connection_id; }

	/** Close this connection.  This will hang up on the remote socket
	  * or socket if the connection is currently open. */
	virtual void stop();

	/** Return true if this connection is closed. */
	bool isClosed() { return _ioConnection->isClosed(); }

	/** Utility function: replace "<" with "&lt;" etc. */
	static std::string encodeCharEntities(const char* src);
	static std::string encodeCharEntities(const std::string &src) {
		return encodeCharEntities(src.c_str()); }

protected: // ============== Subclass Interface ================
	typedef enum {INCOMING, OUTGOING} ConnectionType;
	HTTPConnection(ConnectionType connectionType, boost::asio::io_service &ioService, bool is_subprocess);
	virtual ~HTTPConnection() {}

	IOConnection& getIOConnection() { return *_ioConnection; }

	/** Structure used to hold the contents of an HTTP message. */
	struct HTTPMessage {
		// Information from the request line
		std::string request_method; // GET or POST.
		std::string request_uri;

		// Information from the response line
		int response_status;
		std::string response_status_descr;

		// Information from headers.
		std::string content_encoding;

		// The content block.
		std::string content;
	};

	//
	typedef enum {REJECT_UNKNOWN_HEADERS, IGNORE_UNKNOWN_HEADERS, 
		IGNORE_UNKNOWN_HEADERS_FOR_GET_REQUESTS} UnknownHeaderBehavior;

	/** Start reading the message. */
	void readMessage(UnknownHeaderBehavior unknownHeaderBehavior=REJECT_UNKNOWN_HEADERS);

	virtual void reset();

	const std::string& getMessageStr() { return _message_str; }

protected: // ============ Abstract Virtual Methods ============

	// Subclasses must define these methods:
	virtual void handleMessage(const HTTPMessage& message) = 0;
	virtual void handleReadMessageError(const std::string &description) = 0;
	virtual void handleReadMessageEOF(const std::string &description) {
	    handleReadMessageError(description); }

	// Subclasses should make use of boost::enable_shared_from_this, and
	// define getSharedPointerToThis() to return shared_from_this().
	virtual boost::shared_ptr<HTTPConnection> getSharedPointerToThis() = 0;

private: // ================= Member Variables =================

	size_t _connection_id;
	static size_t _next_connection_id;

	// Socket used to communicate.  
	boost::shared_ptr<IOConnection> _ioConnection;
	bool _is_subprocess;

	// Is this an incoming or an outgoing connection?  This affects
	// how we treat the first line of the message.
	ConnectionType _connectionType;

	// The message object that gets filled in by readMessage().
	HTTPMessage _message;

	// An enum to keep track of which parts of the message we've
	// read so far.
	typedef enum {
		NOT_STARTED, READING_FIRST_LINE, READING_HEADERS, READING_CONTENTS, 
		DONE} MessageReaderState;
	MessageReaderState _messageReaderState;

	// A buffer used to store incoming data for the message.
	boost::array<char, 8192> _buffer;

	// The complete HTTP message string.
	std::string _message_str;

	// The indexes where different parts of the HTTP message end.
	size_t _end_of_first_line;
	size_t _end_of_header_lines;

	// The value of the content-length header
	size_t _content_length;

	// Should we reject unknown headers or silently ignore them?
	UnknownHeaderBehavior _unknownHeaderBehavior;

private: // ============ Helper Methods ============

	// Handle completion of a read operation.
	void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);

	// Handle different parts of the HTTP request message.  
	// Return true for success, or false for failure.
	bool handleRequestLine();     // Called after we find our first newline
	bool handleResponseLine();    // Called after we find our first newline
	bool handleHeaders();         // Called after we find our first blank line
	bool handleRequest();         // Called after we read the request contents (if any)

	// Return an unescaped version of encoded_uri -- e.g. "+" is replaced with a space.
	static std::string decodeRequestURI(const std::string& encoded_uri);
};

#endif
