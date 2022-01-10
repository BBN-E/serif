// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "SerifHTTPServer/IOConnection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <boost/bind.hpp>

#ifdef WIN32
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

namespace {
	#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
	int _stdin_fileno = STDIN_FILENO;
	int _stdout_fileno = STDOUT_FILENO;
	#else
	int _stdin_fileno = -1;
	int _stdout_fileno = -1;
	#endif
}

IOConnection::IOConnection(boost::asio::io_service &ioService, bool is_subprocess)
: _ioService(ioService), _closed(false)
{
	if (is_subprocess) {
		#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
		_input.reset(_new boost::asio::posix::stream_descriptor(ioService, ::dup(_stdin_fileno)));
		_output.reset(_new boost::asio::posix::stream_descriptor(ioService, ::dup(_stdout_fileno)));
		#endif
		#ifdef WIN32
		if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
			throw InternalInconsistencyException("IOConnection::IOConnection"," Unable to set stdin to binary mode");
		}
		if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
			throw InternalInconsistencyException("IOConnection::IOConnection", "Unable to set stdout to binary mode");
		}
		#endif
	} else {
		_socket.reset(_new boost::asio::ip::tcp::socket(ioService));
	}
}

void IOConnection::dupStdinAndStdout() {
	#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
	_stdin_fileno = ::dup(STDIN_FILENO);
	_stdout_fileno = ::dup(STDOUT_FILENO);
	#endif
}


bool IOConnection::isClosed() {
    if (_socket) {
        return !_socket->is_open();
    } else {
		#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
		return (!(_output->is_open() && _input->is_open()));
		#endif
	}
	return _closed;
}

void IOConnection::close() {
	if (_socket) _socket->close();
	_closed = true;
}

void IOConnection::shutdown() {
	requireSocket();
    boost::system::error_code ignored_ec;
    _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void IOConnection::requireSocket() const {
	if (!_socket)
		throw InternalInconsistencyException("IOConnection::connect",
			"This method is only available for socket-based connections");
}

size_t IOConnection::read_some(const boost::asio::mutable_buffer &buffer) {
	char* buffer_data = boost::asio::buffer_cast<char*>(buffer);
	size_t buffer_size = boost::asio::buffer_size(buffer);
	if (fgets(buffer_data, static_cast<int>(buffer_size), stdin)) {
		return strlen(buffer_data);
	} else {
		return 0;
	}
}

void IOConnection::write(const boost::asio::const_buffer &buffer) {
	const char* buffer_data = boost::asio::buffer_cast<const char*>(buffer);
	size_t buffer_size = boost::asio::buffer_size(buffer);
	fwrite(buffer_data, sizeof(char), buffer_size, stdout);
	fflush(stdout);
}

std::string IOConnection::getEndpointAddress() const {
	if (_socket) {
		return _socket->remote_endpoint().address().to_string();
	} else {
		return std::string("STDIN");
	}
}
