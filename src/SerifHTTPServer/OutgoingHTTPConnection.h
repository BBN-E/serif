// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OUTGOING_HTTP_CONNECTION_H
#define OUTGOING_HTTP_CONNECTION_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include "SerifHTTPServer/HTTPConnection.h"

/** An outgoing connection to a remote server (typically used to download a
  * file from a URL over HTTP).  Each outgoing connection is given a
  * ResponseHandler, which is used to handle the response message that
  * the remote server returns.  The ResponseHandler is also used to handle
  * any errors that occur during the transaction.
  */
class OutgoingHTTPConnection 
	: public boost::enable_shared_from_this<OutgoingHTTPConnection>,
	  public HTTPConnection
{
public:
	/** Abstract base class for response handlers.  Define a subclass
	  * of ResponseHandler to define what methods should be called
	  * when the server responds; or when an error occurs. */
	class ResponseHandler {
	public:
		virtual ~ResponseHandler() {}
		virtual void handleResponse(const HTTPMessage& response) = 0;
		virtual void handleResponseError(const std::string &description) = 0;
	};
	typedef boost::shared_ptr<ResponseHandler> ResponseHandler_ptr;

	/** Create a new outgoing HTTP connection that will connect to a
	  * specified server, send it the specified request message, and
	  * use the given responseHandler to handle the server's response
	  * message.  The connection will not actually be initiated until
	  * the connect() method is called. */
	OutgoingHTTPConnection(boost::asio::io_service &ioService,
		const std::string &server, const std::string &request, 
		ResponseHandler_ptr responseHandler);

	/** Initiate this outgoing HTTP connection.  This method should
	  * be called only once for a given OutgoingHTTPConnection object. */
	void connect();

protected:
	virtual void handleMessage(const HTTPMessage& response);
	virtual void handleReadMessageError(const std::string &description) {
		return _responseHandler->handleResponseError(description); }
	virtual boost::shared_ptr<HTTPConnection> getSharedPointerToThis() {
		return shared_from_this(); }

private:
	// Connect to the remote server.
	// Handle completion of a resolve operation.
	void handleResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	// Handle completion of a connect operation.
	void handleConnect(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	// Handle completion of our write operation (sending the request)
	void handleWrite(const boost::system::error_code& err);

	// Handler for response message from remote server.
	ResponseHandler_ptr _responseHandler;
	// The HTTP server we should connect to.
	std::string _server;
	// The request message we should send
	std::string _requestMessage;
	// The TCP resolver used to connect with an HTTP server
	boost::asio::ip::tcp::resolver _resolver;
};

#endif
