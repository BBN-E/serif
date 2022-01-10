// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/InternalInconsistencyException.h"
#include "SerifHTTPServer/HTTPConnection.h"

#include <iostream>
#include <fstream>
#include <limits>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

size_t HTTPConnection::_next_connection_id = 0;

// windows.h defines a "max" macro, which interferes with our use
// of std::numeric_limits<>::max; so undefine it if it's been defined.
#ifdef max
#undef max
#endif

// Define some private regular expressions that we can use to help
// parse the HTTP request.
namespace {
	boost::regex newlineRegex("\r?\n", boost::regex::perl);
	boost::regex doubleNewlineRegex("\r?\n\r?\n", boost::regex::perl);
	boost::regex requestLineRegex("^\\s*(GET|POST)\\s+(\\S+)\\s+HTTP/(\\d+\\.\\d+)\r?\n$");
	boost::regex responseLineRegex("^HTTP/(\\d+\\.\\d+)\\s+(\\d+) (.*)\r?\n$");
	boost::regex headerContinuation("\r?\n\\s+");
	boost::regex spacesRegex("[ \t]+");
	boost::regex headerLineRegex("^([-\\w]+)\\s*:\\s*(.*\\S)\\s*$");

	// This constant is used for _content_length when no "content
	// length" header is recieved; in that case, we just read until
	// the input stream is closed.
	const size_t UNSPECIFIED_LENGTH = std::numeric_limits<std::size_t>::max();
}

HTTPConnection::HTTPConnection(ConnectionType connectionType, boost::asio::io_service &ioService, bool is_subprocess)
: _messageReaderState(NOT_STARTED), _connectionType(connectionType),
  _connection_id(++_next_connection_id), _is_subprocess(is_subprocess)
{
	_ioConnection.reset(_new IOConnection(ioService, is_subprocess));
	std::fill(_buffer.begin(), _buffer.end(), '\0');
}

void HTTPConnection::stop()
{
	_ioConnection->close();
	_messageReaderState = DONE;
}

void HTTPConnection::reset() {
	_messageReaderState = NOT_STARTED;
	_connection_id = ++_next_connection_id;
	_message_str.clear();
	// no need to clear _message here -- that's done by readMessage().
}

void HTTPConnection::readMessage(UnknownHeaderBehavior unknownHeaderBehavior)
{
	if (_messageReaderState != NOT_STARTED)
		throw InternalInconsistencyException("HTTPConnection::readMessage",
			"Already reading a message -- can't read two messages at once!");
	_unknownHeaderBehavior = unknownHeaderBehavior;
	// Reset the message fields
	_message.request_method.clear();
	_message.request_uri.clear();
	_message.response_status = 0;
	_message.response_status_descr.clear();
	_content_length = UNSPECIFIED_LENGTH;
	_message.content_encoding.clear();
	_message.content.clear();
	// Start reading the message
	_end_of_first_line = 0;
	_end_of_header_lines = 0;
	_messageReaderState = READING_FIRST_LINE;
	_ioConnection->async_read_some(boost::asio::buffer(_buffer),
		boost::bind(&HTTPConnection::handleRead, getSharedPointerToThis(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

}

void HTTPConnection::handleRead(const boost::system::error_code& err, std::size_t bytes_transferred)
{
	if (_messageReaderState == DONE)
		return;

	// If the content length was unspecified, then assume that we've reached the 
	// end of the contents when we read an EOF.
	if (err && (err.value() == boost::asio::error::eof) && 
		(_content_length == UNSPECIFIED_LENGTH) && !_message_str.empty()) {
		_messageReaderState = DONE;
		_content_length = _message_str.size()-_end_of_header_lines;
		// Copy the message contents into the message object.
		const char* content_start = &(*(_message_str.begin())) + _end_of_header_lines;
		_message.content.append(content_start, _content_length);
		// Handle the message
		handleMessage(_message);
		return;
	}

	// Report any other error.
	if (err) {
		if (err.value() == 995)
			return; // OPERATION_ABORTED -- i.e., intentional shutdown.
        else if (err.value() == boost::asio::error::eof)
            handleReadMessageEOF("Error during read: "+err.message());
        else
            handleReadMessageError("Error during read: "+err.message());

		return;
	}

	// Read the new data.
	_message_str.append(_buffer.data(), bytes_transferred);

	char rc = '\r';
	char nc = '\n';

	// Check if we've got a complete request line; if so, process it.
	if (_messageReaderState == READING_FIRST_LINE ) {
		boost::smatch match;
		if (boost::regex_search(_message_str, match, newlineRegex)) {
			_end_of_first_line = match.position() + match.length();
			_messageReaderState = READING_HEADERS;
			if (_connectionType == INCOMING) {
				if (!handleRequestLine())
					return;
			} else {
				if (!handleResponseLine())
					return;
			}
		}
	}

	// Check if we've got a complete set of headers; if so, process them.
	if (_messageReaderState == READING_HEADERS) {
		boost::smatch match;
		if (boost::regex_search(_message_str, match, doubleNewlineRegex)) {
			_end_of_header_lines = match.position() + match.length();
			_messageReaderState = READING_CONTENTS;
			if (!handleHeaders())
				return;
		}
	}

	// If we've reached the end of the contents, then process the message,
	// and return -- we're done!
	if (_messageReaderState == READING_CONTENTS) {
		if (_content_length == UNSPECIFIED_LENGTH) {
			// Continue reading until we get an EOF.
		} else if ((_message_str.size() - _end_of_header_lines) >= _content_length) {
			_messageReaderState = DONE;
			// Copy the message contents into the message object.
			const char* content_start = &(*(_message_str.begin())) + _end_of_header_lines;
			_message.content.append(content_start, _content_length);
			// Handle the message
			handleMessage(_message);
			return;
		}
	}

	_ioConnection->async_read_some(boost::asio::buffer(_buffer),
		boost::bind(&HTTPConnection::handleRead, getSharedPointerToThis(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

bool HTTPConnection::handleRequestLine() {
	std::string request_line(_message_str.begin(), _message_str.begin()+_end_of_first_line);
	boost::smatch match;

	// Extract information from the request line.  Should we check the HTTP version?
	if (boost::regex_match(request_line, match, requestLineRegex)) {
		// Ignore HTTP version for now.
		_message.request_method = match.str(1);
		_message.request_uri = decodeRequestURI(match.str(2));
		if (_message.request_uri.empty()) {
			handleReadMessageError("Error while decoding URI: \""+match.str(2)+"\"");
			return false;
		}
		else
			return true;
	} else {
		handleReadMessageError("Bad request line: \""+request_line+"\"");
		return false;
	} 
}

bool HTTPConnection::handleResponseLine() {
	std::string response_line(_message_str.begin(), _message_str.begin()+_end_of_first_line);
	boost::smatch match;

	// Extract information from the request line.  Should we check the HTTP version?
	if (boost::regex_match(response_line, match, responseLineRegex)) {
		// Ignore HTTP version for now.
		_message.response_status = boost::lexical_cast<int>(match.str(2));
		_message.response_status_descr = match.str(3);
		return true;
	} else {
		handleReadMessageError("Bad response line: \""+response_line+"\"");
		return false;
	}
}


bool HTTPConnection::handleHeaders() {
	std::vector<std::string> lines;
	std::string headers(_message_str.begin()+_end_of_first_line, 
		_message_str.begin()+_end_of_header_lines);

	// Normalize the headers
	headers = boost::regex_replace(headers, newlineRegex, "\n");
	headers = boost::regex_replace(headers, headerContinuation, " ");
	headers = boost::regex_replace(headers, spacesRegex, " ");

	std::string content_type;
	_message.content_encoding = "";
	if (_message.request_method == "GET")
		_content_length = 0;
	else
		_content_length = UNSPECIFIED_LENGTH;

	// handle each header line.
	boost::split(lines, headers, boost::is_any_of("\n"));
	for (size_t i=0; i<lines.size(); ++i) {
		if (lines[i].size() == 0) 
			continue;
		boost::smatch match;
		if (boost::regex_search(lines[i], match, headerLineRegex)) {
			std::string field = match.str(1);
			std::string value = match.str(2);
			if (boost::iequals(field, "Accept")) 
				continue; //Ignore for now
			if (boost::iequals(field, "Accept-Charset")) 
				continue; //Ignore for now
			if (boost::iequals(field, "Accept-Encoding")) 
				continue; //Ignore for now
			if (boost::iequals(field, "Accept-Language")) 
				continue; //Ignore for now
			if (boost::iequals(field, "Allow")) 
				continue; // Ignore for now
			else if (boost::iequals(field, "Authorization"))
				continue; // ignore credentials
			else if (boost::iequals(field, "Cache-Control"))
				content_type = value;
			else if (boost::iequals(field, "Content-Language"))
				content_type = value;
			else if (boost::iequals(field, "Content-Type"))
				content_type = value;
			else if (boost::iequals(field, "Connection"))
				continue; // ignore for now
			else if (boost::iequals(field, "Content-Length")) {
				try {
					int content_length = boost::lexical_cast<int>(value);
					if (content_length >= 0)
						_content_length = static_cast<size_t>(content_length);
					else {
						handleReadMessageError("Content-length must be positive: \""+lines[i]+"\"");
						return false;
					}
				} catch(boost::bad_lexical_cast) {
					handleReadMessageError("Content-length must be an integer: \""+lines[i]+"\"");
					return false;
				}
			}
			else if (boost::iequals(field, "Content-Type"))
				content_type = value;
			else if (boost::iequals(field, "Cookie"))
				continue; // we don't care.
			else if (boost::iequals(field, "Date"))
				continue; // we don't care.
			else if (boost::iequals(field, "Expires"))
				continue; // we don't care.
			else if (boost::iequals(field, "From"))
				continue; // we don't care (though we could log this).
			else if (boost::iequals(field, "Host"))
				continue; // we don't care
			else if (boost::iequals(field, "If-Modified-Since"))
				continue; // no caching currently enabled.
			else if (boost::iequals(field, "Keep-Alive")) 
				continue; // Ignore for now
			else if (boost::iequals(field, "Last-Modified"))
				continue; // no caching currently enabled.
			else if (boost::iequals(field, "Link"))
				continue; // Ignore for now
			else if (boost::iequals(field, "Location"))
				continue; // we don't care
			else if (boost::iequals(field, "MIME-Version"))
				continue; // Ignore for now
			else if (boost::iequals(field, "Origin"))
				continue; // Ignore for now
			else if (boost::iequals(field, "Pragma"))
				continue; // we ignore all pragmas.
			else if (boost::iequals(field, "Referer"))
				continue; // we don't care
			else if (boost::iequals(field, "Retry-After"))
				continue; // Ignore for now
			else if (boost::iequals(field, "Server"))
				continue; // we don't care
			else if (boost::iequals(field, "Title"))
				continue; // Ignore for now
			else if (boost::iequals(field, "URI"))
				continue; // Ignore for now
			else if (boost::iequals(field, "User-Agent"))
				continue; // we don't care
			else if (boost::iequals(field, "WWW-Authenticate"))
				continue; // we don't care
			else if (boost::iequals(field, "X-Requested-With"))
				continue; // we don't care
			else {
				if (_unknownHeaderBehavior == IGNORE_UNKNOWN_HEADERS ||
					(_unknownHeaderBehavior == IGNORE_UNKNOWN_HEADERS_FOR_GET_REQUESTS &&
					 _message.request_method == std::string("GET"))) {
					continue; // Ignore unknown headers.
				} else {
					handleReadMessageError("Unknown header: \""+lines[i]+"\"");
					return false;
				}
			}
		} else {
			handleReadMessageError("Unable to parse header line: \""+lines[i]+"\"");
			return false;
		}
	}
	if (_content_length == UNSPECIFIED_LENGTH && _is_subprocess) {
		handleReadMessageError("Content-Length header is required in subprocess mode");
		return false;
	}

	return true; // success
}

// If there's an error, return the empty string.
std::string HTTPConnection::decodeRequestURI(const std::string& encoded_uri) {
	std::stringstream decoded;

	for (size_t i=0; i<encoded_uri.size(); ++i) {
		// If we find a '%'-escaped character, then decode it.
		if (encoded_uri[i] == '%') {
			if (i+3 <= encoded_uri.size()) {
				// Convert from hex -> int -> char
				int value;
		        std::istringstream iss(encoded_uri.substr(i + 1, 2));
				if (iss >> std::hex >> value) {
					decoded << static_cast<char>(value);
					i += 2; // skip the characters we processed.
				} else
					return "";
			} else 
				return "";
		}
		else if (encoded_uri[i] == '+')
			decoded << ' ';
		else
			decoded << encoded_uri[i];
	}
	return decoded.str();
}


std::string HTTPConnection::encodeCharEntities(const char* src) {
	std::string s(src);
	boost::replace_all(s, "&", "&amp;");
	boost::replace_all(s, "<", "&lt;");
	boost::replace_all(s, ">", "&gt;");
	boost::replace_all(s, "\"", "&quot;");
	boost::replace_all(s, "'", "&apos;");
	return s;
}

#pragma warning(pop)
