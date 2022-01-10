// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "SerifHTTPServer/OutgoingHTTPConnection.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

OutgoingHTTPConnection::OutgoingHTTPConnection(boost::asio::io_service &ioService,
											   const std::string &server, const std::string &request, 
											   OutgoingHTTPConnection::ResponseHandler_ptr responseHandler)
: _resolver(ioService), _server(server), _requestMessage(request),
  _responseHandler(responseHandler),
  HTTPConnection(HTTPConnection::OUTGOING, ioService, false)
{}

void OutgoingHTTPConnection::connect() {
	// Use the resolver to translate the server into a list of possible endpoints.
    boost::asio::ip::tcp::resolver::query query(_server, "http");
    _resolver.async_resolve(query,
        boost::bind(&OutgoingHTTPConnection::handleResolve, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator));
}

void OutgoingHTTPConnection::handleResolve(const boost::system::error_code& err,
				   boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!err) {
		// Attempt a connection to the first endpoint in the list. Each endpoint
		// will be tried until we successfully establish a connection.
		boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
		getIOConnection().async_connect(endpoint,
			boost::bind(&OutgoingHTTPConnection::handleConnect, shared_from_this(),
		boost::asio::placeholders::error, ++endpoint_iterator));
	} else {
		_responseHandler->handleResponseError(
			"Error while resolving server name "+_server+": "+err.message());
    }
}

void OutgoingHTTPConnection::handleConnect(const boost::system::error_code& err,
									  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!err) {
		// The connection was successful. Send the request.
		getIOConnection().async_write(boost::asio::buffer(_requestMessage),
			boost::bind(&OutgoingHTTPConnection::handleWrite, shared_from_this(),
				boost::asio::placeholders::error));
	} else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
		// The connection failed. Try the next endpoint in the list.
		getIOConnection().close();
		boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
		getIOConnection().async_connect(endpoint,
			boost::bind(&OutgoingHTTPConnection::handleConnect, shared_from_this(),
				boost::asio::placeholders::error, ++endpoint_iterator));
	} else {
		_responseHandler->handleResponseError(
			"Error while connecting to "+_server+": "+err.message());
	}
}

void OutgoingHTTPConnection::handleWrite(const boost::system::error_code& err) {
	if (!err) {
		readMessage(IGNORE_UNKNOWN_HEADERS);
	} 
	else {
		_responseHandler->handleResponseError(
			"Error while sending request to "+_server+": "+err.message());
	}
}

void OutgoingHTTPConnection::handleMessage(const HTTPMessage& response) {
	if (response.response_status == 200)
		_responseHandler->handleResponse(response); 
	else
		_responseHandler->handleResponseError(
			"Error while sending request to "+_server+": " +
			boost::lexical_cast<std::string>(response.response_status)+
			" -- " + response.response_status_descr);
}
