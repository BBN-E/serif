// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/version.h"
#include "SerifHTTPServer/SerifHTTPServer.h"
#include "SerifHTTPServer/IncomingHTTPConnection.h"
#include "SerifHTTPServer/SerifWorkQueue.h"


#include <iostream>
#include <fstream>

#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time.hpp>
#pragma warning(pop)

SerifHTTPServer::SerifHTTPServer(int port, int verbosity, bool is_subprocess,
								 const char* logdir)
	: _ioService(), _acceptor(_ioService), _verbosity(verbosity), _exitStatus(EXIT), 
	  _is_subprocess(is_subprocess), _stopped(false), _logdir(logdir), _logPrefix("")
{

	_logPrefix = _logPrefix + "[" + SerifVersion::getProductName() + "HTTPServer]";

	if (verbosity > 0)
		std::cerr << _logPrefix << " Initializing " << SerifVersion::getProductName() << " Work Queue" << std::endl;
	SerifWorkQueue *workQueue = SerifWorkQueue::getSingletonWorkQueue();
	_fatalErrorCallback = _new FatalErrorCallback(this);
	workQueue->setShutdownCallback(_fatalErrorCallback);
	workQueue->initialize();

	// If no port was specified, then get our port number from the 
	// parameter file.
	if (port == -1)
		port = workQueue->getPortFromParamFile();

	// Create a connection object for the next connection we get.
	_nextConnection.reset(_new IncomingHTTPConnection(this, _ioService, _verbosity, _is_subprocess, _logdir));

	if (_is_subprocess) {
		_activeConnections.insert(_nextConnection);
		_ioService.post(boost::bind(&IncomingHTTPConnection::start, _nextConnection));
		_nextConnection.reset(); // We create one connection & re-use it; so no next connection.
	} else {
		// Set the acceptor up to listen on the given port.
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), static_cast<unsigned short>(port));
		_acceptor.open(endpoint.protocol());
		_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		_acceptor.bind(endpoint);
		_acceptor.listen();
		
		boost::asio::ip::tcp::endpoint local_endpoint = _acceptor.local_endpoint(); 
		port = local_endpoint.port(); // If we passed in 0 is a port, record what port we were actually assigned.

		// Tell the acceptor to call this->handle_accept() when a connection is made.
		_nextConnection->accept_from(_acceptor,
			boost::bind(&SerifHTTPServer::handle_accept, this, boost::asio::placeholders::error));
	}
	workQueue->setAssignedPort(port);
	if (verbosity > 0) {
		std::cerr << _logPrefix << " " << SerifVersion::getProductName() << " XML Server listening on " << boost::asio::ip::host_name() << ":" << port << std::endl;
	}
}

SerifHTTPServer::~SerifHTTPServer() {
	delete _fatalErrorCallback;
	//SerifWorkQueue::getSingletonWorkQueue()->shutdown();
}

// Noop used for busy-waiting callback in SerifHTTPServer::run
namespace {	void noop() {} }

SerifHTTPServer::SERVER_EXIT_STATUS SerifHTTPServer::run()
{
	boost::asio::deadline_timer timer(_ioService);

	// Run the io service main loop.  
	_exitStatus = EXIT;
	while (!_stopped) {
		_ioService.run(); 
		if (!_stopped) {
			// This occurs if we are a subprocess, and the only thing we're doing
			// is waiting for the serif work queue to finish.  (If we are not a 
			// subprocess, then the acceptor will always have an async request 
			// outstanding, so _ioService.run won't exit until we're stopped.)
			timer.expires_from_now(boost::posix_time::seconds(1));
			timer.async_wait(boost::bind(&noop));
			_ioService.reset();
		}
	}

	// If we reach here, then we either received a shutdown command or a 
	// restart command.  But we still want to wait for all active connections
	// to finish up before we actually exit.
	while (numActiveConnections() > 0) {
		waitForAllConnectionsToClose(&timer);
		_ioService.reset();
		_ioService.run(); 
	}

	// Send a shut-down message to the work queue (but don't wait for
	// it to finish shutting down -- if it's in the middle of a job,
	// that could take a while.)
	SerifWorkQueue::getSingletonWorkQueue()->shutdown(false);

	return _exitStatus;
}

void SerifHTTPServer::waitForAllConnectionsToClose(boost::asio::deadline_timer *timer) {
	size_t num_connections = numActiveConnections();
	if (num_connections == 0) {
		return; // All done!
	} else {
		// Issue a message once every 20 secs:
		static size_t iter = 0;
		if ((iter++ % 20) == 0) {
			std::cerr << _logPrefix << " Waiting for connection(s) to close:" << std::endl;
			std::vector<IncomingHTTPConnection_ptr> connections = getActiveConnectionList();
			for (size_t i=0; i<connections.size(); ++i) {
				std::cerr << _logPrefix << "   * " << connections[i]->getConnectionDescription() << std::endl;
			}
		}

		// Wait for the connection to finish.
		timer->expires_from_now(boost::posix_time::seconds(1));
		timer->async_wait(boost::bind(&SerifHTTPServer::waitForAllConnectionsToClose, this, timer));
	}
}

size_t SerifHTTPServer::numActiveConnections() {
	updateActiveConnectionList();
	return _activeConnections.size();
}

bool connectionIdLessThan(IncomingHTTPConnection_ptr lhs, IncomingHTTPConnection_ptr rhs) {
	return lhs->getConnectionId() < rhs->getConnectionId();
}

std::vector<IncomingHTTPConnection_ptr> SerifHTTPServer::getActiveConnectionList() {
	std::vector<IncomingHTTPConnection_ptr> result;
	std::set<IncomingHTTPConnection_weakptr>::const_iterator connection_i;
	for (connection_i = _activeConnections.begin(); connection_i != _activeConnections.end(); ++connection_i) {
		if (IncomingHTTPConnection_ptr connection = connection_i->lock()) {
			if (!connection->isClosed()) {
				result.push_back(connection);
			}
		}
	}
	std::sort(result.begin(), result.end(), connectionIdLessThan);
	return result;
}


void SerifHTTPServer::updateActiveConnectionList() {
	std::set<IncomingHTTPConnection_weakptr> newActiveConnections;
	std::set<IncomingHTTPConnection_weakptr>::const_iterator connection_i;
	for (connection_i = _activeConnections.begin(); connection_i != _activeConnections.end(); ++connection_i) {
		if (IncomingHTTPConnection_ptr connection = connection_i->lock()) {
			if (!connection->isClosed()) {
				newActiveConnections.insert(connection);
			}
		}
	}
	_activeConnections.swap(newActiveConnections);
}

void SerifHTTPServer::handle_accept(const boost::system::error_code& e)
{
	updateActiveConnectionList();
	if (e) {
		if (e.value() == boost::system::errc::operation_canceled || e.value() == boost::asio::error::operation_aborted)
			return; // Happens during shutdown.
		if (_verbosity >= 0)
			std::cerr << _logPrefix << "   Error while accepting connection: " << e.message() << std::endl;
		return;
	}

	if (_verbosity >= 0)
		std::cerr << _logPrefix << " " << boost::posix_time::second_clock::local_time() << " Accepting new connection (" 
			<< _nextConnection->getConnectionId() << ")" << std::endl;

	_nextConnection->start();
	_activeConnections.insert(_nextConnection);

	_nextConnection.reset(_new IncomingHTTPConnection(this, _ioService, _verbosity, false, _logdir));
	_nextConnection->accept_from(_acceptor, 
		boost::bind(&SerifHTTPServer::handle_accept, this, boost::asio::placeholders::error));
}

void SerifHTTPServer::stop(bool force)
{
	// Tell the io service to call this->handle_stop().  (We shouldn't call it ourselves,
	// in case we're not in the same thread as the (blocking) server::run().)
	_ioService.post(boost::bind(&SerifHTTPServer::handle_stop, this, force));
}

void SerifHTTPServer::handle_stop(bool force)
{
	_stopped = true;
	if (_verbosity > 0)
		std::cerr << _logPrefix << " Shutting down the server." << std::endl;
	if (!_is_subprocess)
		_acceptor.close();
	if (force) {
		typedef std::set<IncomingHTTPConnection_weakptr>::iterator ConnectionIter;	 
		for (ConnectionIter connection_i = _activeConnections.begin(); connection_i != _activeConnections.end(); ++connection_i) {
			if (IncomingHTTPConnection_ptr connection = connection_i->lock()) {
				if (!connection->isClosed()) {
					connection->stop();
				}
			}
		}
		_activeConnections.clear();
	}
}

void SerifHTTPServer::restart(bool force)
{
	_exitStatus = RESTART;
	stop(force);
}

