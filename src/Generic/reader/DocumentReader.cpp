// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/reader/DocumentReader.h"
#include "Generic/reader/MTDocumentReader.h"
#include "Generic/reader/INQADocumentReader.h"
#include "Generic/reader/DTRADocumentReader.h"
#include "Generic/icews/ICEWSDocumentReader.h"
#include "Generic/reader/VIKTRSDocumentReader.h"
#include "Generic/reader/xx_DocumentReader.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "Generic/common/UnicodeUtil.h"
#include <boost/scoped_ptr.hpp>


namespace { 
	template<typename T> struct DocumentReaderFactoryFor: public DocumentReader::Factory { 
		virtual DocumentReader *build() { return _new T(); }
	};
}

DocumentReader::FactoryMap &DocumentReader::_factoryMap() {
	static FactoryMap factoryMap;
	if (factoryMap.empty()) {
		// register default reader types.
		factoryMap["segments"] = boost::shared_ptr<Factory>(_new DocumentReaderFactoryFor<MTDocumentReader>());
		factoryMap["inqa"] = boost::shared_ptr<Factory>(_new DocumentReaderFactoryFor<INQADocumentReader>());
		factoryMap["sgm"] = boost::shared_ptr<Factory>(_new DocumentReaderFactoryFor<DefaultDocumentReader>());
		factoryMap["dtra"] = boost::shared_ptr<Factory>(_new DocumentReaderFactoryFor<DTRADocumentReader>());
		factoryMap["icews_xmltext"] = boost::shared_ptr<Factory>(_new DocumentReaderFactoryFor<ICEWSDocumentReader>());
		factoryMap["viktrs"] = boost::shared_ptr<Factory>(_new DocumentReaderFactoryFor<VIKTRSDocumentReader>());
		//Not yet implemented - nward, 2007.06.20
		//factoryMap["factiva"] = DocumentReaderFactoryFor<FactivaDocumentReader>();
	}
	return factoryMap;
}


boost::shared_ptr<DocumentReader::Factory> &DocumentReader::_factory(const char* source_format) {
	// case-normalize
	std::string source_format_str(source_format);
	boost::algorithm::to_lower(source_format_str);
	FactoryMap &factoryMap = _factoryMap();
	FactoryMap::iterator it = factoryMap.find(source_format_str);
	if (it == factoryMap.end()) {
		std::ostringstream err;
		err << "Unknown source_format: " << source_format << "  Expected one of:";
		BOOST_FOREACH(FactoryMap::value_type pair, factoryMap) {
			err << " " << pair.first;
		}
		throw UnexpectedInputException("DocumentReader::build", err.str().c_str());
	} else {
		return (*it).second;
	}
}

bool DocumentReader::hasFactory(const char* source_format) {
	std::string source_format_str(source_format);
	boost::algorithm::to_lower(source_format_str);
	FactoryMap &factoryMap = _factoryMap();
	return (factoryMap.find(source_format_str) != factoryMap.end());
}

DocumentReader *DocumentReader::build(const char* source_format) { 
	return _factory(source_format)->build(); 
}

void DocumentReader::setFactory(const char* source_format, boost::shared_ptr<Factory> factory) {
	std::string source_format_str(source_format);
	boost::algorithm::to_lower(source_format_str);
	_factoryMap()[source_format_str] = factory; 
}

Document* DocumentReader::readDocumentFromFile(const wchar_t* filename) {
	if (prefersByteStream()) {
		std::ifstream strm(UnicodeUtil::toUTF8StdString(filename).c_str());
		return readDocumentFromByteStream(strm, filename);
	} else {
		boost::scoped_ptr<UTF8InputStream> strm_scoped_ptr(UTF8InputStream::build(filename));
		UTF8InputStream& strm(*strm_scoped_ptr);
		return readDocument(strm, filename);
	}
}

Document* DocumentReader::readDocumentFromWString(const std::wstring& contents, const wchar_t *filename) {
	if (prefersByteStream()) {
		std::stringstream strm(UnicodeUtil::toUTF8StdString(contents));
		return readDocumentFromByteStream(strm, filename);
	} else {
		std::wstringstream strm(contents);
		return readDocument(strm, filename);
	}
}
