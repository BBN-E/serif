// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "SerifXMLServer/SerifXMLServer.h"
#include "Generic/driver/DocumentDriver.h"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/Stage.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLDocument.h"
#include "Generic/apf/APF4ResultCollector.h"
#include "Generic/theories/DocTheory.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SaxParseException.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>

#include "Generic/common/version.h"

// Temporary hack:
#define X(s) (boost::lexical_cast<xstring>(s).c_str())

using boost::asio::ip::tcp;
using namespace xercesc;
using namespace SerifXML;

// Private regexps
namespace {
	boost::regex newlineRegex("\r?\n", boost::regex::perl);
	boost::regex doubleNewlineRegex("\r?\n\r?\n", boost::regex::perl);
	boost::regex requestLineRegex("^\\s*(GET|POST)\\s*(\\w+)\\s*HTTP/(\\d+\\.\\d+)\r?\n$");
	boost::regex headerContinuation("\r?\n\\s+");
	boost::regex spacesRegex("[ \t]+");
	boost::regex headerLineRegex("^([-\\w]+)\\s*:\\s*(.*\\S)\\s*$");
}

SerifXMLServer::SerifXMLServer(const char* parameter_file, int port)
	: _ioService(), _acceptor(_ioService) 
{
	// Initialize serif stuff..
	ParamReader::readParamFile(parameter_file);
	_documentDriver = _new DocumentDriver();

	_nextConnection.reset(_new SerifXMLServer::Connection(this));

	// Set the acceptor up to listen on the given port.
	tcp::endpoint endpoint(tcp::v4(), port);
	_acceptor.open(endpoint.protocol());
	_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	_acceptor.bind(endpoint);
	_acceptor.listen();

	// Tell the acceptor to call this->handle_accept() when a connection is made.
	_acceptor.async_accept(_nextConnection->socket,
		boost::bind(&SerifXMLServer::handle_accept, this, boost::asio::placeholders::error));

	std::cerr << "Serif XML Server listening on port " << port << std::endl;
}

SerifXMLServer::~SerifXMLServer() {
	delete _documentDriver;
}


void SerifXMLServer::run()
{
	_ioService.run(); 
}

void SerifXMLServer::stop()
{
	// Tell the io service to call this->handle_stop().  (We shouldn't call it ourselves,
	// because we're not in the same thread as the (blocking) server::run().)
	_ioService.post(boost::bind(&SerifXMLServer::handle_stop, this));
}

void SerifXMLServer::handle_accept(const boost::system::error_code& e)
{
	std::cerr << "Accepting new connection (" << _activeConnections.size()
		<< " connections already active)" << std::endl;
	if (e) {
		std::cerr << "  Error while accepting connection." << std::endl;
		return; // Ignore errors for now.
	}
	_nextConnection->start();
	_activeConnections.insert(_nextConnection);

	_nextConnection.reset(_new Connection(this));
	_acceptor.async_accept(_nextConnection->socket,
		boost::bind(&SerifXMLServer::handle_accept, this, boost::asio::placeholders::error));

}

void SerifXMLServer::handle_stop()
{
	std::cerr << "Shutting down the server..." << std::endl;
	_acceptor.close();
	BOOST_FOREACH(Connection_ptr c, _activeConnections) {
		c->stop();
	}
}

void SerifXMLServer::Connection::start()
{
	socket.async_read_some(boost::asio::buffer(_buffer),
		boost::bind(&SerifXMLServer::Connection::handleRead, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void SerifXMLServer::Connection::stop()
{
	socket.close();
	_server->_activeConnections.erase(shared_from_this());
}

void SerifXMLServer::Connection::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
{
	if (e) {
		if (e == boost::asio::error::operation_aborted)
			stop();
		else if (e == boost::asio::error::eof)
			stop();
		else if (e) 
			std::cerr << "Unknown error during handleRead: " << e << std::endl;
			stop();
		return;
	}

	// Read the new data.
	_request.append(_buffer.data(), bytes_transferred);

	// Check if we've got a complete request line; if so, process it.
	if (_state == READ_REQUEST_LINE ) {
		boost::smatch match;
		if (boost::regex_search(_request, match, newlineRegex)) {
			_end_of_request_line = match.position() + match.length();
			_state = READ_HEADERS;
			if (!handleRequestLine())
				return;
		}
	}

	// Check if we've got a complete set of headers; if so, process them.
	if (_state == READ_HEADERS) {
		boost::smatch match;
		if (boost::regex_search(_request, match, doubleNewlineRegex)) {
			_end_of_header_lines = match.position() + match.length();
			_state = READ_CONTENTS;
			if (!handleHeaders())
				return;
		}
	}

	if (_state == READ_CONTENTS) {
		if ((_request.size() - _end_of_header_lines) >= _content_length) {
			_state = WORKING;
			if (!handleContents())
				return;
		}
	}

	if ((_state == READ_REQUEST_LINE) || 
		(_state == READ_HEADERS) || 
		(_state = READ_CONTENTS)) {
		socket.async_read_some(boost::asio::buffer(_buffer),
			boost::bind(&SerifXMLServer::Connection::handleRead, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
}

bool SerifXMLServer::Connection::handleRequestLine() {
	std::string request_line(_request.begin(), _request.begin()+_end_of_request_line);
	boost::smatch match;

	// Extract information from the request line.  Should we check the HTTP version?
	if (!(boost::regex_match(request_line, match, requestLineRegex)))
		return reportError(400, "Bad HTTP request line");
	_request_method = match.str(1);
	_request_uri = match.str(2);

	// Handle GET requests.
	if (_request_method == std::string("GET")) {
		return reportError(404, "Use POST instead of GET.");
	}

	// Handle POST requests.
	else if (_request_method == std::string("POST")) {
		if (_request_uri == std::string("SerifXMLRequest"))
			return true; // ok.
		else if (_request_uri == std::string("sgm2apf"))
			return true; // ok.
		else
			return reportError(404, "Unknown URI", _request_uri);
	} 
	
	// Anything else is an error.  (And we should never get here, because
	// the regexp limits group 1 to be either GET or POST anyway.)
	else
		return reportError(400, "Bad HTTP request line: expected GET or POST.");
}

bool SerifXMLServer::Connection::handleHeaders() {
	std::vector<std::string> lines;
	std::string headers(_request.begin()+_end_of_request_line, 
		_request.begin()+_end_of_header_lines);

	// Normalize the headers
	headers = boost::regex_replace(headers, newlineRegex, "\n");
	headers = boost::regex_replace(headers, headerContinuation, " ");
	headers = boost::regex_replace(headers, spacesRegex, " ");

	if (headers.size() == 0)
		return reportError(400, "Content-Length header is required");

	std::string content_type;
	_content_length = 0;
	_content_encoding = "";

	// handle each header line.
	boost::split(lines, headers, boost::is_any_of("\n"));
	for (size_t i=0; i<lines.size(); ++i) {
		if (lines[i].size() == 0) 
			continue;
		boost::smatch match;
		if (boost::regex_search(lines[i], match, headerLineRegex)) {
			std::string field = match.str(1);
			std::string value = match.str(2);
			if (boost::iequals(field, "Allow")) 
				continue; // should never get this in a POST request anyway.
			else if (boost::iequals(field, "Authorization"))
				continue; // ignore credentials
			else if (boost::iequals(field, "Content-Encoding"))
				_content_encoding = value;
			else if (boost::iequals(field, "Content-Length")) {
				try {
					int content_length = boost::lexical_cast<int>(value);
					if (content_length > 0)
						_content_length = static_cast<size_t>(content_length);
					else if (content_length == 0)
						return reportError(400, "Expected non-zero content length"); 
					else
						return reportError(400, "Content-length must be positive");
				} catch(boost::bad_lexical_cast) {
					return reportError(400, "Content-length must be an int", value);
				}
			}
			else if (boost::iequals(field, "Content-Type"))
				content_type = value;
			else if (boost::iequals(field, "Date"))
				continue; // we don't care.
			else if (boost::iequals(field, "Expires"))
				continue; // we don't care.
			else if (boost::iequals(field, "From"))
				continue; // we don't care (though we could log this).
			else if (boost::iequals(field, "If-Modified-Since"))
				continue; // no caching currently enabled.
			else if (boost::iequals(field, "Last-Modified"))
				continue; // no caching currently enabled.
			else if (boost::iequals(field, "Location"))
				continue; // we don't care
			else if (boost::iequals(field, "Pragma"))
				continue; // we ignore all pragmas.
			else if (boost::iequals(field, "Referer"))
				continue; // we don't care
			else if (boost::iequals(field, "User-Agent"))
				continue; // we don't care
			else {
				return reportError(400, "Unsupported HTTP header", field);
			}
		} else {
			return reportError(400, "Bad header line", lines[i]);
		}
	}
	if (!(content_type.empty() || boost::iequals(content_type, "application/xml")))
		return reportError(400, "Unsupported Content-Type", content_type);
	// For now, we don't support any content encoding.
	if (!(_content_encoding.empty()))
		return reportError(400, "Unsupported Content-Encoding", _content_encoding);
	if (_content_length == 0) {
		if ((_request_uri == std::string("SerifXMLRequest")) ||
			(_request_uri == std::string("sgm2apf"))) {
			return reportError(400, "Expected content");
		}
	}
	return true; // success
}

bool SerifXMLServer::Connection::reportError(size_t error_code, 
											 const std::string &explanation,
											 const std::string &extra) 
{
	std::stringstream err;
	err << explanation << ": \"" << extra << "\"";
	return reportError(error_code, err.str());
}

bool SerifXMLServer::Connection::reportError(size_t error_code, 
											 const std::string &explanation) 
{
	std::string message;
	if (error_code == 400) message = "BAD REQUEST";
	else if (error_code == 404) message = "NOT FOUND";
	else if (error_code == 500) message = "SERVER ERROR";
	else message = "ERROR";

	std::stringstream out;
	out << "HTTP/1.0 " << error_code << " " << message << "\r\n\r\n";
	out << "<html>\r\n";
	out << "<head><title>Serif XML Server: " << message << "</title></head>\r\n";
	out << "<body>\r\n";
	out << "<h1>Serif XML Server: " << error_code << " " << message << "</h1>\r\n";
	out << "<p>" << explanation << "</p>" << "\r\n";
	out << "</body>\r\n";
	out << "</html>\r\n";
	_response = out.str();
	sendResponse();
	return false;
}

bool SerifXMLServer::Connection::sendResponse(DOMDocument *xmldoc)
{
	using namespace SerifXML;
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);
	DOMWriter *writer = impl->createDOMWriter();
	MemBufFormatTarget *target = new MemBufFormatTarget();
	writer->setEncoding(X_UTF8);
	writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
	writer->writeNode(target, *xmldoc);
	std::stringstream out;
	out << "HTTP/1.0 200 OK\r\n";
	out << "Content-length: " << target->getLen() << "\r\n";
	out << "\r\n"; // End of headers
	out << target->getRawBuffer();
	_response = out.str();
	sendResponse();
	delete writer;
	delete target;
	return true;
}

void SerifXMLServer::Connection::sendResponse() {
	_state = SEND_RESPONSE;

	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(boost::asio::buffer(_response));
	boost::asio::async_write(socket, buffers,
		boost::bind(&SerifXMLServer::Connection::handleWrite, shared_from_this(),
		boost::asio::placeholders::error));
}

void SerifXMLServer::Connection::handleWrite(const boost::system::error_code& e)
{
	if (e == boost::asio::error::operation_aborted) {
		stop();
	}
	else if (e && (e != boost::asio::error::eof)) {
		std::cerr << "Unknown error during handleWrite: " << e << std::endl;
		stop();
	}
	else {
		// Shut down the socket -- we're done!
	    boost::system::error_code ignored_ec;
	    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		std::cerr << "Finished processing connection" << std::endl;
	}
}

class SerifXMLErrorHandler: public ErrorHandler, public DOMErrorHandler {
public:
	std::vector<std::string> errors;
	void warning(const SAXParseException& exc) { saveError(exc); }
	void error(const SAXParseException& exc) { saveError(exc); }
	void fatalError(const SAXParseException& exc) { saveError(exc); }
	void resetErrors() { errors.clear(); }
	bool handleError(const DOMError& exc) { 
		char* message = XMLString::transcode(exc.getMessage());
		errors.push_back(message);
		XMLString::release(&message);
		return false;
	}
	void saveError(const SAXParseException& exc) {
		char* message = XMLString::transcode(exc.getMessage());
		std::stringstream err;
		err << "<p><b>Line " << exc.getLineNumber() << ", column " << exc.getColumnNumber()
			<< ":</b> " << HTTPConnection::encodeCharEntities(message) << "</p>\r\n";
		errors.push_back(err.str());
		XMLString::release(&message);
	}
};

bool SerifXMLServer::Connection::handleContents() {
	if (_request_uri == std::string("SerifXMLRequest")) {
		return handleXMLContents();
	} else if (_request_uri == std::string("sgm2apf")) {
		return reportError(404, "The sgm2apf command is not supported yet");
	} else {
		return reportError(404, "Unknown URI", _request_uri);
	}
}

bool SerifXMLServer::Connection::handleXMLContents() {
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);
	XercesDOMParser parser;
	SerifXMLErrorHandler errorHandler;
	parser.setErrorHandler(&errorHandler);

	const XMLByte* request_start = (const XMLByte*)(&(*_request.begin())) + _end_of_header_lines;
	MemBufInputSource src(request_start, _content_length, "SerifXMLRequest");
	try {
		parser.parse(src);
	} catch (...) {
		return reportError(400, "Unknown error while processing XML");
	}
	if (!errorHandler.errors.empty()) {
		std::stringstream err;
		err << "Error(s) while processing XML input:\r\n";
		BOOST_FOREACH(std::string message, errorHandler.errors)
			err << message;
		return reportError(400, err.str());
	}

	if (_request_uri == std::string("SerifXMLRequest")) {
		return handleSerifXMLRequest(parser.getDocument());
	} else {
		return reportError(404, "Unknown URI", _request_uri);
	}
}

bool SerifXMLServer::Connection::handleSerifXMLRequest(DOMDocument *xmldoc) {
	DOMElement* root = xmldoc->getDocumentElement();
	if (XMLString::compareIString(root->getTagName(), X("SerifXMLRequest"))!=0)
		return reportError(400, "Expected a <SerifXMLRequest> element");

	std::vector<DOMElement*> commands;
	DOMNodeList *children = root->getChildNodes();
	for (size_t i=0; i<children->getLength(); ++i) {
		DOMElement *elt = dynamic_cast<DOMElement*>(children->item(i));
		if (elt) commands.push_back(elt);
	}

	if (commands.size() == 0)
		return reportError(400, "Expected at least one command");
	if (commands.size() > 1)
		return reportError(400, "Multiple commands not currently supported");
	BOOST_FOREACH(DOMElement* command, commands) {
		if (XMLString::compareIString(command->getTagName(), X("ProcessDocument"))==0) {
			if (!handleProcessDocumentCommand(xmldoc, command))
				return false;
		} else {
			return reportError(400, "Unknown command");// todo: say what the command was
		}
	}
	return true;
}

Stage SerifXMLServer::Connection::getStageAttribute(DOMElement *elt, const XMLCh *attr, Stage default_value) {
	if (!elt->hasAttribute(attr))
		return default_value;
	try {
		return Stage(transcodeToStdString(elt->getAttribute(attr)).c_str());
	} 
	catch (const UnexpectedInputException &exc) {
		return reportError(400, exc.getMessage());
	}
}

bool SerifXMLServer::Connection::handleProcessDocumentCommand(DOMDocument *xmldoc, DOMElement *cmd) {
	if (!checkLanguageAttribute(cmd))
		return false;

	Stage startStage = getStageAttribute(cmd, X_start_stage, Stage("START"));
	Stage endStage = getStageAttribute(cmd, X_end_stage, Stage("output"));

	std::vector<DOMElement*> childElts;
	DOMNodeList *children = cmd->getChildNodes();
	for (size_t i=0; i<children->getLength(); ++i) {
		DOMElement *elt = dynamic_cast<DOMElement*>(children->item(i));
		if (elt) childElts.push_back(elt);
	}
	if (childElts.size() != 1)
		return reportError(400, "Expected exactly one document");
	if (XMLString::compareIString(childElts[0]->getTagName(), X("Document"))!=0)
		return reportError(400, "Expected a <Document> element");

	XMLDocument document(xmldoc, childElts[0]);
	DocTheory* docTheory = 0;
	try {
		docTheory = document.getDocTheory();

		std::wstring results;
		const wchar_t* docname = docTheory->getDocument()->getName().to_string();

		SessionProgram sessionProgram;
		sessionProgram.setStageRange(startStage, endStage);

		try {
			APF4ResultCollector resultCollector(APF4ResultCollector::APF2008);
			_server->_documentDriver->beginBatch(&sessionProgram, &resultCollector);
			_server->_documentDriver->runOnDocTheory(docTheory, docname, &results);
			_server->_documentDriver->endBatch();
		} catch (...) {
			// Make sure we've cleaned up after ourselves, then re-throw the
			// exception.
			if (_server->_documentDriver->inBatch())
				_server->_documentDriver->endBatch(); 
			throw;
		}

		// This will do for now -- later, try to preserve the input.
		XMLDocument finished(docTheory);
		sendResponse(finished.getXercesXMLDoc());
		return true;
	}
	catch (const UnexpectedInputException &exc) {
		return reportError(400, exc.getMessage());
	}
	catch (const InternalInconsistencyException &exc) {
		return reportError(500, exc.getMessage());
	}
	catch (const UnrecoverableException &exc) {
		return reportError(500, exc.getMessage());
	}
}

// Check to make sure the language attribute matches our build.
bool SerifXMLServer::Connection::checkLanguageAttribute(DOMElement *cmd) {
	using namespace SerifXML;
	if (!cmd->hasAttribute(X_language))
		return reportError(400, "The \"language\" attribute is required");
	const XMLCh* language = cmd->getAttribute(X_language);
	if (SerifVersion::isEnglish()) {
//	#if defined(ENGLISH_LANGUAGE)
		if (XMLString::compareIString(language, X_English) != 0)
			return reportError(500, "This server only supports language=\"English\"");
	} else if (SerifVersion::isArabic()) {
//	#elif defined(ARABIC_LANGUAGE)
		if (XMLString::compareIString(language, X_Arabic) != 0)
			return reportError(500, "This server only supports language=\"Arabic\"");
	} else if (SerifVersion::isChinese()) {
//	#elif defined(CHINESE_LANGUAGE)
		if (XMLString::compareIString(language, X_Chinese) != 0)
			return reportError(500, "This server only supports language=\"Chinese\"");
//	} else if (SerifVersion::isFarsi()) {
////    #elif defined(FARSI_LANGUAGE)
//        if (XMLString::compareIString(language, X_Farsi) != 0)
//          return reportError(500, "This server only supports language=\"Farsi\"");
	}
//	#else
//		#error "Add a case for your langauge here!" 
//	#endif
	return true;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << argv[0] << "should be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}
	std::cerr << "Serif XML Server starting up" << std::endl;
	SerifXMLServer(argv[1], 8080).run();
	std::cerr << "Serif XML Server shutting down" << std::endl;
    return 0;
}
