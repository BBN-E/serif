// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/reader/DocumentZoner.h"
#include "Generic/reader/SectionHeadersDocumentZoner.h"
#include "Generic/reader/ProseDocumentZoner.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace { 
	template<typename T> struct DocumentZonerFactoryFor: public DocumentZoner::Factory { 
		virtual DocumentZoner *build() { return _new T(); }
	};
}

DocumentZoner::FactoryMap& DocumentZoner::_factoryMap() {
	static FactoryMap factoryMap;
	if (factoryMap.empty()) {
		// Register zoner types. There is no "none" type because we just don't run zoning in that case.
		factoryMap["section-headers"] = boost::shared_ptr<Factory>(_new DocumentZonerFactoryFor<SectionHeadersDocumentZoner>());
		factoryMap["prose"] = boost::shared_ptr<Factory>(_new DocumentZonerFactoryFor<ProseDocumentZoner>());
	}
	return factoryMap;
}


boost::shared_ptr<DocumentZoner::Factory>& DocumentZoner::_factory(const char* zoning_method) {
	// case-normalize
	std::string zoning_method_str(zoning_method);
	boost::algorithm::to_lower(zoning_method_str);
	FactoryMap& factoryMap = _factoryMap();
	FactoryMap::iterator it = factoryMap.find(zoning_method_str);
	if (it == factoryMap.end()) {
		std::ostringstream err;
		err << "Unknown zoning method: " << zoning_method << "  Expected one of:";
		BOOST_FOREACH(FactoryMap::value_type pair, factoryMap) {
			err << " " << pair.first;
		}
		throw UnexpectedInputException("DocumentZoner::build", err.str().c_str());
	} else {
		return (*it).second;
	}
}

bool DocumentZoner::hasFactory(const char* zoning_method) {
	std::string zoning_method_str(zoning_method);
	boost::algorithm::to_lower(zoning_method_str);
	FactoryMap& factoryMap = _factoryMap();
	return (factoryMap.find(zoning_method_str) != factoryMap.end());
}

DocumentZoner *DocumentZoner::build(const char* zoning_method) { 
	return dynamic_cast<DocumentZoner*>(_factory(zoning_method)->build());
}

void DocumentZoner::setFactory(const char* zoning_method, boost::shared_ptr<Factory> factory) {
	std::string zoning_method_str(zoning_method);
	boost::algorithm::to_lower(zoning_method_str);
	_factoryMap()[zoning_method_str] = factory; 
}
