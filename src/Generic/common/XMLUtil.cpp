// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Decompressor.h"
#include "Generic/state/XMLStrings.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/util/BinInputStream.hpp>
#include <sstream>
#include <fstream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/scoped_array.hpp>

using namespace xercesc;
using namespace SerifXML;

// Private (local) helper classes
namespace {
	// Error handler used when parsing XML documents.
	class SerifXMLErrorHandler: public ErrorHandler, public DOMErrorHandler {
	public:
		struct Error {
			Error(const XMLCh* message, XMLFileLoc lineno=0, XMLFileLoc column=0):
				message(transcodeToStdString(message)), lineno(lineno), column(column) {}
			XMLFileLoc lineno;
			XMLFileLoc column;
			std::string message;
		};
		std::vector<Error> errors;

		void warning(const SAXParseException& exc) { saveError(exc); }
		void error(const SAXParseException& exc) { saveError(exc); }
		void fatalError(const SAXParseException& exc) { saveError(exc); }
		void resetErrors() { errors.clear(); }
		bool handleError(const DOMError& exc) { 
			errors.push_back(Error(exc.getMessage()));
			return false;
		}
		void saveError(const SAXParseException& exc) {
			errors.push_back(Error(exc.getMessage(), exc.getLineNumber(), exc.getColumnNumber()));
		}
		bool hasErrors() { return !errors.empty(); }
		std::string getErrorDescription() {
			std::stringstream err;
			err << "Error(s) while parsing XML:\r\n";
			BOOST_FOREACH(Error &error, errors) {
				if (error.lineno || error.column)
					err << "Line " << error.lineno << ", column " << error.column << ":\r\n";
				err << "  " << error.message << "\r\n";
			}
			return err.str();
		}
	};

	// binary input stream that reads from an istream.
	class IStreamBinInputStream : public BinInputStream
	{
		std::istream &_in;
	public:
		IStreamBinInputStream(std::istream & in): _in(in) { }
		XMLFilePos curPos(void) const { return static_cast<XMLFilePos>(_in.tellg()); }
		XMLSize_t readBytes(XMLByte * const buf, const XMLSize_t max)
		{ _in.read((char*)buf, max); return static_cast<XMLSize_t>(_in.gcount()); }
        const XMLCh* getContentType(void) const { return NULL; }
	};
	class IStreamInputSource : public InputSource
	{
		std::istream &_in;
	public:
		IStreamInputSource(std::istream & in): InputSource("istream"), _in(in) {}
		BinInputStream * makeStream(void) const
		{ return new IStreamBinInputStream(_in); }
	};

	// The xerces parser requires that its input be a byte stream.  So
	// if we want to process a wide stream containing xml, then we need
	// to convert it back to a utf8 byte stream on the fly.  That's
	// what this class does.
	class WIStreamBinInputStream : public BinInputStream
	{
		static const int BUFFER_SIZE=1024;
		std::wistream &_in;
		wchar_t _wbuf[BUFFER_SIZE];
		XMLByte _overflow[BUFFER_SIZE*3];
		XMLByte *_overflow_start;
		XMLSize_t _overflow_len;
	public:
		WIStreamBinInputStream(std::wistream & in): _in(in), _overflow_start(_overflow), _overflow_len(0) { }
		XMLFilePos curPos(void) const { return static_cast<XMLFilePos>(_in.tellg()); }

		XMLSize_t readBytes(XMLByte * const buffer, const XMLSize_t max) {
			XMLByte *buf = buffer;
			XMLSize_t bytes_read = 0;
			// If we have bytes left over from a previous read, then use those.
			if (_overflow_len) {
				if (_overflow_len <= max) {
					std::copy(_overflow_start, _overflow_start+_overflow_len, buf);
					bytes_read = _overflow_len;
					_overflow_start = _overflow;
					_overflow_len = 0;
				} else {
					std::copy(_overflow_start, _overflow_start+max, buf);
					bytes_read = max;
					_overflow_start += max;
					_overflow_len -= max;
				}
			}
			// Otherwise, read some new characters, and convert them to bytes.
			else {
				// Read into the wide buffer
				_in.read(_wbuf, BUFFER_SIZE);
				wchar_t *wbuf_end = _wbuf + _in.gcount();
				// Convert each character back to utf-8.
				XMLByte *buf_end = buf+max;
				for (wchar_t *src = _wbuf; src != wbuf_end; ++src) {
					if ((buf_end-buf) > 3) {
						size_t bytes_in_char = UnicodeUtil::convertUTF8Char(*src, buf);
						buf += bytes_in_char;
					} else {
						size_t bytes_in_char = UnicodeUtil::convertUTF8Char(*src, _overflow+_overflow_len);
						_overflow_len += bytes_in_char;
					}
				}
				bytes_read = static_cast<XMLSize_t>(buf-buffer);
			}
			return bytes_read;
		}
        const XMLCh* getContentType(void) const { return NULL; }
	};

	class WIStreamInputSource : public InputSource
	{
		std::wistream &_in;
	public:
		WIStreamInputSource(std::wistream & in): InputSource("istream"), _in(in) {}
		BinInputStream * makeStream(void) const
		{ return new WIStreamBinInputStream(_in); }
	};

	xercesc::DOMDocument* loadXercesDOMFromInputSource(InputSource &src) {
		DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);
		XercesDOMParser parser;
		SerifXMLErrorHandler errorHandler;
		parser.setErrorHandler(&errorHandler);

		// Optionally validate the XML as it is read
		std::string schemaPath = ParamReader::getParam("serifxml_schema");
		if (schemaPath.length() > 0) {
#if XERCES_VERSION_MAJOR == 2
			throw UnexpectedInputException("XMLUtil::loadXercesDOM",
				"Xerces 2 does not support SerifXML schema validation");
#elif XERCES_VERSION_MAJOR == 3
			if (NULL == parser.loadGrammar(schemaPath.c_str(), Grammar::SchemaGrammarType)) {
				throw UnexpectedInputException("XMLUtil::loadXercesDOM",
					"Could not process SerifXML schema");
			}
			parser.setDoNamespaces(true);
			parser.setDoSchema(true);
			parser.setValidationConstraintFatal(true);
			parser.setValidationSchemaFullChecking(true);
#endif
		}

		try {
			parser.parse(src);
		} catch (...) {
			throw UnexpectedInputException("XMLUtil::loadXercesDOM",
				"Unknown error while processing XML");
		}
		if (errorHandler.hasErrors()) {
			throw UnexpectedInputException("XMLUtil::loadXercesDOM",
				errorHandler.getErrorDescription().c_str());
		}
		return parser.adoptDocument();
	}

	template<typename T>
	void escapeLinefeedForXMLFormatTarget(T *target, const XMLByte* const toWrite,
										  const XMLSize_t count, XMLFormatter* const formatter) 
	{
		size_t num_lf = std::count(toWrite, toWrite+count, '\r');
		if (num_lf) {
			XMLByte *transformed = _new XMLByte[count+4*num_lf];
			const XMLByte *src = toWrite;
			XMLByte *dst = transformed;
			for (unsigned int n=0; n<count; ++n, ++src) {
				if ((*src) == '\r') {
					*dst++ = '&';
					*dst++ = '#';
					*dst++ = 'x';
					*dst++ = 'D';
					*dst++ = ';';
				} else {
					*dst++ = *src;
				}
			}
			target->writeCharsRaw(transformed, count+4*num_lf, formatter);
			delete transformed;
		} else {
			target->writeCharsRaw(toWrite, count, formatter);
		}
	}

	class LocalFileFormatTargetWithLinefeedEscape: public LocalFileFormatTarget {
	public:
		LocalFileFormatTargetWithLinefeedEscape(const char* filename): LocalFileFormatTarget(filename) {}
		void writeChars(const XMLByte* const toWrite, const XMLSize_t count, XMLFormatter* const formatter) {
			escapeLinefeedForXMLFormatTarget(this, toWrite, count, formatter); }
		void writeCharsRaw(const XMLByte* const toWrite, const XMLSize_t count, XMLFormatter* const formatter) {
			LocalFileFormatTarget::writeChars(toWrite, count, formatter); }
	};

	class MemBufFormatTargetWithLinefeedEscape: public MemBufFormatTarget {
	public:
		void writeChars(const XMLByte* const toWrite, const XMLSize_t count, XMLFormatter* const formatter) {
			escapeLinefeedForXMLFormatTarget(this, toWrite, count, formatter); }
		void writeCharsRaw(const XMLByte* const toWrite, const XMLSize_t count, XMLFormatter* const formatter) {
			MemBufFormatTarget::writeChars(toWrite, count, formatter); }
	};

}

xercesc::DOMDocument* XMLUtil::loadXercesDOMFromFilename(const char* filename) {
	if (Decompressor::canDecompress(filename)) {
		size_t size;
		boost::scoped_array<unsigned char> mem(
				Decompressor::decompressIntoMemory(filename, size));
		try {
			return loadXercesDOMFromString((const char*)mem.get(), size);
		} catch (UnexpectedInputException &exc) {
			std::ostringstream prefix;
			prefix << "In " << filename << ": ";
			exc.prependToMessage(prefix.str().c_str());
			throw;
		}
	} else {
		std::ifstream stream(filename);
		if ((!stream.is_open()) || (stream.fail()) ){
			std::stringstream errmsg;
			errmsg << "Failed to open '" << filename << "'";
			throw UnexpectedInputException( "XMLUtil::loadXercesDOMFromFilename()", errmsg.str().c_str() );
		}
		try {
			return loadXercesDOMFromStream(stream);
		} catch (UnexpectedInputException &exc) {
			std::ostringstream prefix;
			prefix << "In " << filename << ": ";
			exc.prependToMessage(prefix.str().c_str());
			throw;
		}
	}
}

xercesc::DOMDocument* XMLUtil::loadXercesDOMFromStream(std::istream &stream) {
	IStreamInputSource src(stream);
	return loadXercesDOMFromInputSource(src);
}

xercesc::DOMDocument* XMLUtil::loadXercesDOMFromStream(std::wistream &stream) {
	WIStreamInputSource src(stream);
	return loadXercesDOMFromInputSource(src);
}

xercesc::DOMDocument* XMLUtil::loadXercesDOMFromString(const char* xml_string, size_t length) {
	if (!length)
		length = strlen(xml_string);
	MemBufInputSource src((const XMLByte*)(xml_string), length, "SerifXMLRequest");
	return loadXercesDOMFromInputSource(src);
}

xercesc::DOMDocument* XMLUtil::loadXercesDOMFromFilename(const wchar_t* filename) {
	std::string filename_bytes = UnicodeUtil::toUTF8StdString(filename);
	return loadXercesDOMFromFilename(filename_bytes.c_str());
}

xercesc::DOMDocument* XMLUtil::loadXercesDOMFromString(const wchar_t* xml_string) {
	std::string xml_string_bytes = UnicodeUtil::toUTF8StdString(xml_string);
	return loadXercesDOMFromString(xml_string_bytes.c_str(), xml_string_bytes.length());
}

void XMLUtil::saveXercesDOMToFilename(const xercesc::DOMNode* xml_doc, const wchar_t* filename) {
	saveXercesDOMToFilename(xml_doc, UnicodeUtil::toUTF8StdString(filename).c_str()); 
}

void XMLUtil::saveXercesDOMToTarget(const xercesc::DOMNode* xml_doc, XMLFormatTarget* target) {
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);
#if XERCES_VERSION_MAJOR == 2
	DOMWriter *writer = impl->createDOMWriter();
	writer->setEncoding(X_UTF8);
    if (writer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
        writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    if (writer->canSetFeature(XMLUni::fgDOMWRTBOM, false))
        writer->setFeature(XMLUni::fgDOMWRTBOM, false);
	writer->writeNode(target, *xml_doc);
	delete writer;
#elif XERCES_VERSION_MAJOR == 3
    DOMLSSerializer *serializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    DOMLSOutput *output = ((DOMImplementationLS*)impl)->createLSOutput();
    output->setEncoding(X_UTF8);
    output->setByteStream(target);
    DOMConfiguration* config = serializer->getDomConfig();
    if (config->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
        config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    if (config->canSetParameter(XMLUni::fgDOMWRTBOM, false))
        config->setParameter(XMLUni::fgDOMWRTBOM, false);
    serializer->write(xml_doc, output);
	delete output;
    delete serializer;
#endif
}

void XMLUtil::saveXercesDOMToFilename(const xercesc::DOMNode* xml_doc, const char* filename) {
	XMLFormatTarget *target = new LocalFileFormatTargetWithLinefeedEscape(filename);
    saveXercesDOMToTarget(xml_doc, target);
	delete target;

	// Optionally validate on write
	if (ParamReader::getParam("serifxml_schema").length() > 0) {
		loadXercesDOMFromFilename(filename);
	}
}

void XMLUtil::saveXercesDOMToStream(const xercesc::DOMNode* xml_doc, std::ostream &out) {
	MemBufFormatTarget *target = new MemBufFormatTarget();
    saveXercesDOMToTarget(xml_doc, target);
	out << ((char*)(target->getRawBuffer()));
	delete target;
}

void XMLUtil::saveXercesDOMToString(const xercesc::DOMNode* xml_doc, std::string& result) {
	MemBufFormatTarget *target = new MemBufFormatTarget();
    saveXercesDOMToTarget(xml_doc, target);
	result.assign((char*)(target->getRawBuffer()), target->getLen());
	delete target;

	// Optionally validate on write
	if (ParamReader::getParam("serifxml_schema").length() > 0) {
		loadXercesDOMFromString(result.c_str(), result.length());
	}
}

void XMLUtil::saveXercesDOMToStream(const xercesc::DOMNode* xml_doc, std::wostream &out) {
	std::string byte_string;
	saveXercesDOMToString(xml_doc, byte_string);
	std::wstring char_string = UnicodeUtil::toUTF16StdString(byte_string);
	out << char_string;
}

void XMLUtil::saveXercesDOMToString(const xercesc::DOMNode* xml_doc, std::wstring& result) {
	std::string byte_string;
	saveXercesDOMToString(xml_doc, byte_string);
	result = UnicodeUtil::toUTF16StdString(byte_string);
}
