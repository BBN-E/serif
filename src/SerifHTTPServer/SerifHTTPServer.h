// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_HTTP_SERVER_H
#define SERIF_HTTP_SERVER_H

#include "SerifHTTPServer/SerifWorkQueue.h"

#include <set>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>

class IncomingHTTPConnection;
typedef boost::shared_ptr<IncomingHTTPConnection> IncomingHTTPConnection_ptr;
typedef boost::weak_ptr<IncomingHTTPConnection> IncomingHTTPConnection_weakptr;

/** Main class for Serif's HTTP server.  The job of this class is to 
  * listen for new connections, and to create a IncomingHTTPConnection
  * object for each connection that's made.  See the IncomingHTTPConnection
  * class for more information about what commands are supported by
  * Serif's HTTP server. 
  *
  * All server communication is performed using Asynchronous IO.  This
  * means that all IO commands (including accepting a new connection)
  * are done by registering callbacks, which are called when the command
  * completes.  See the documentation for boost::asio for more 
  * information.
  */
class SerifHTTPServer: public boost::noncopyable {
public:
	/** Construct a new SerifHTTPServer that listens for connections on 
	  * the specified port.  If no port is specified, then the port number 
	  * is read from the parameter file.  Note: the parameter reader must
	  * be iniitialized before a SerifHTTPServer is constructed. */
	explicit SerifHTTPServer(int port=-1, int verbosity=1,
		bool is_subprocess=false, const char* logdir=0);

	/** Destructor */
	~SerifHTTPServer();

	/** Enum used to indicate what caused the server to exit. */
	typedef enum { EXIT, RESTART, FAILURE } SERVER_EXIT_STATUS;

	/** Start the server.  This will return when the server shuts down. */
	SERVER_EXIT_STATUS run();

	/** Stop the server.  This will close all active server connections, and
	  * cause the server to shut down.  It may be called from any thread. */
	void stop(bool force=false);

	/** Restart the server (using exec).  The method setRestartArguments() 
	  * must be called first, to let the server know what command-line 
	  * arguments should be used when restarting.  This may be called from 
	  * any thread. */
	void restart(bool force=false);

	/** Return the number of active connections. */
	size_t numActiveConnections();

	/** Return a list of the active connections. */
	std::vector<IncomingHTTPConnection_ptr> getActiveConnectionList();

private: // ============ Helper Methods ============

	/** Callback that is called when a new connection is accepted.
	  * Adds the new connection to the list of active connections, 
	  * and initializes a new listening connection. */
	void handle_accept(const boost::system::error_code& e);

	/** Handle a request to stop the server. */
	void handle_stop(bool force);

	/** Check if any of the connections in the _activeConnections
	  * list are closed; and if so, remove them from the list and
	  * delete them. */
	void updateActiveConnectionList();

	void waitForAllConnectionsToClose(boost::asio::deadline_timer *timer);

private: // ============ Member Variables ============

	// The boost io_service used to perform asynchronous operations.
	boost::asio::io_service _ioService;

	// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor _acceptor;

	// The next connection we'll use (currently listening)
	IncomingHTTPConnection_ptr _nextConnection;

	// List of active connections.
	std::set<IncomingHTTPConnection_weakptr> _activeConnections;

	// How noisy should we be?
	int _verbosity;

	// Are we a subprocess?  If so, use stdin/stdout rather than http sockets
	const bool _is_subprocess;

	bool _stopped;
	SERVER_EXIT_STATUS _exitStatus;

	class FatalErrorCallback: public SerifWorkQueue::FatalErrorCallback {
		SerifHTTPServer* _server;
	public: 
		FatalErrorCallback(SerifHTTPServer* server): _server(server) {}
		virtual void operator()() {
			_server->_exitStatus = FAILURE;
			_server->stop(true);
		}
	};
	FatalErrorCallback *_fatalErrorCallback;

	const char* _logdir;

	std::string _logPrefix;
};

#endif // SERIF_XML_SERVER_H
