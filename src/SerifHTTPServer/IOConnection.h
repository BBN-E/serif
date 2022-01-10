// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

/** A class used to encapsulate an I/O connection with a remote party,
  * for use with boost asynchronous IO (boost::asio).  This class was
  * introduced to abstract out the differences between communicating over
  * TCP sockets (as is done in the normal server mode) and communicating
  * over stdin/stdout (as is done when the server is run as a subprocess).
  *
  * Each instance of this class wraps *either* a socket *or* a pair of
  * handles used to communicate with stdin/stdout.  (Which one is controlled
  * by the "is_subprocess" constructor argument).  The I/O methods are 
  * all implemented for both sockets and stdin/stdout; but several methods
  * (such as accept_from() and async_connect()) are only implemented for
  * sockets; attempting to call them with an IOConnection that communicates
  * via stdin/stdout will result in an exception.
  *
  * Originally, I tried implementing this as a single abstract base class 
  * with two concrete subclasses (one for sockets and one for stdin/stdout).
  * However, in order to interoperate easily with boost, I needed to use
  * template methods, and unfortunately there's no such thing as a virtual
  * template method in C++.
  */
class IOConnection: private boost::noncopyable {
public:
	/** Create a new IO Connection object.  If is_subprocess is true, then
	  * the IOConnection will communicate over stdin/stdout.  Only one such
	  * IOConnection object should be created.  If is_subprocess is false,
	  * then the IOConnection will communicate using a private socket object.
	  * Use accpet_from() and async_connect() to connect the socket to a 
	  * client. */
	IOConnection(boost::asio::io_service &ioService, bool is_subprocess);

	bool isClosed();
	void close();
	void shutdown();

	std::string getEndpointAddress() const;

	template <typename MutableBufferSequence, typename ReadHandler>
	void async_read_some(const MutableBufferSequence& buffers, ReadHandler handler) {
		if (_socket) { 
			_socket->async_read_some(buffers, handler);
		} else {
			#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
				_input->async_read_some(buffers, handler);
			#else
				_ioService.post(SyncReadCompletionHandler<MutableBufferSequence, ReadHandler>
				                    (this, buffers, handler));
			#endif
		}
	}

	template <typename ConstBufferSequence, typename WriteHandler>
	void async_write(const ConstBufferSequence& buffers, WriteHandler handler) {
		if (_socket) { 
			boost::asio::async_write(*_socket, buffers, handler);
		} else {
			#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
				boost::asio::async_write(*_output, buffers, handler);
			#else
				_ioService.post(SyncWriteCompletionHandler<ConstBufferSequence, WriteHandler>
					(this, buffers, handler));
			#endif
		}
	}

	template<typename AcceptHandler>
	void accept_from(boost::asio::ip::tcp::acceptor &acceptor, AcceptHandler handler) {
		requireSocket();
		acceptor.async_accept(*_socket, handler);
	}

	template<typename Endpoint, typename ConnectHandler>
	void async_connect(Endpoint &endpoint, ConnectHandler handler) {
		requireSocket();
		_socket->async_connect(endpoint, handler);
	}

	/** Make a private (static) copy of stdout and stderr.  This should
	 * be called if you're using a subprocess IOConnection *before*
	 * you redirect stdout to a file. */
	static void dupStdinAndStdout();

private: 
	// ==================== Member Variables ====================

	// The boost async I/O Service
	boost::asio::io_service &_ioService;

	// A flag used to keep track of whether we've been closed.
	bool _closed;

	// The socket that is wrapped by this IOConnection (only used if 
	// is_subprocess=false)
	boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;

	// The stdin/stdout streams wrapped by this IOConnection (only used
	// if is_subprocess=true)
	#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
	boost::shared_ptr<boost::asio::posix::stream_descriptor> _input;
	boost::shared_ptr<boost::asio::posix::stream_descriptor> _output;
	#endif

private: 
	// ==================== Helper methods ====================

	// Helper: check to make sure we have a socket.
	void requireSocket() const;


	// This completion handler, along with the read_some() method, are used 
	// to implement async_read_some for windows, where we unfortunately
	// can't do non-blocking input on stdin.
	template <typename MutableBufferSequence, typename ReadHandler>
	class SyncReadCompletionHandler {
		IOConnection *_ioConnection; 
		const boost::asio::mutable_buffer& _buffer;
		ReadHandler _handler;
	public:
		SyncReadCompletionHandler(IOConnection *ioConnection, const MutableBufferSequence& buffers, ReadHandler handler)
			: _ioConnection(ioConnection), _buffer(*(buffers.begin())), _handler(handler) {}
		void operator()() {
		    size_t count = _ioConnection->read_some(_buffer);
			_ioConnection->_ioService.post(boost::bind(_handler, boost::system::error_code(), count));
		}
	};
	size_t read_some(const boost::asio::mutable_buffer &buffer);

	// This completion handler, along with the write() method, are used 
	// to implement async_write for windows, where we unfortunately
	// can't do non-blocking output on stdout.
	template <typename ConstBufferSequence, typename WriteHandler>
	class SyncWriteCompletionHandler {
		IOConnection *_ioConnection; 
		typedef std::vector<boost::asio::const_buffer> Buffers;
		const Buffers _buffers;
		WriteHandler _handler;
	public:
		SyncWriteCompletionHandler(IOConnection *ioConnection, const ConstBufferSequence& buffers, WriteHandler handler)
			: _ioConnection(ioConnection), _buffers(buffers.begin(), buffers.end()), _handler(handler) {}
		void operator()() {
			for (Buffers::const_iterator buf = _buffers.begin(); buf!=_buffers.end(); ++buf)
				_ioConnection->write(*buf);
			_ioConnection->_ioService.post(boost::bind(_handler, boost::system::error_code()));
		}
	};
	void write(const boost::asio::const_buffer &buffer);

};
