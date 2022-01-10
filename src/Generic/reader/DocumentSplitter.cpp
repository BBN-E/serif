// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/reader/DocumentSplitter.h"
#include "Generic/reader/DefaultDocumentSplitter.h"
#include "Generic/reader/RegionDocumentSplitter.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace { 
	template<typename T> struct DocumentSplitterFactoryFor: public DocumentSplitter::Factory { 
		virtual DocumentSplitter *build() { return _new T(); }
	};
}

DocumentSplitter::FactoryMap& DocumentSplitter::_factoryMap() {
	static FactoryMap factoryMap;
	if (factoryMap.empty()) {
		// Register splitter types.
		factoryMap["none"] = boost::shared_ptr<Factory>(_new DocumentSplitterFactoryFor<DefaultDocumentSplitter>());
		factoryMap["region"] = boost::shared_ptr<Factory>(_new DocumentSplitterFactoryFor<RegionDocumentSplitter>());
	}
	return factoryMap;
}


boost::shared_ptr<DocumentSplitter::Factory>& DocumentSplitter::_factory(const char* split_strategy) {
	// case-normalize
	std::string split_strategy_str(split_strategy);
	boost::algorithm::to_lower(split_strategy_str);
	FactoryMap& factoryMap = _factoryMap();
	FactoryMap::iterator it = factoryMap.find(split_strategy_str);
	if (it == factoryMap.end()) {
		std::ostringstream err;
		err << "Unknown source_format: " << split_strategy << "  Expected one of:";
		BOOST_FOREACH(FactoryMap::value_type pair, factoryMap) {
			err << " " << pair.first;
		}
		throw UnexpectedInputException("DocumentSplitter::build", err.str().c_str());
	} else {
		return (*it).second;
	}
}

bool DocumentSplitter::hasFactory(const char* split_strategy) {
	std::string split_strategy_str(split_strategy);
	boost::algorithm::to_lower(split_strategy_str);
	FactoryMap& factoryMap = _factoryMap();
	return (factoryMap.find(split_strategy_str) != factoryMap.end());
}

DocumentSplitter *DocumentSplitter::build() { 
	return _factory(ParamReader::getParam("document_split_strategy", "none").c_str())->build();
}

DocumentSplitter *DocumentSplitter::build(const char* split_strategy) { 
	return _factory(split_strategy)->build(); 
}

void DocumentSplitter::setFactory(const char* split_strategy, boost::shared_ptr<Factory> factory) {
	std::string source_format_str(split_strategy);
	boost::algorithm::to_lower(source_format_str);
	_factoryMap()[source_format_str] = factory; 
}
