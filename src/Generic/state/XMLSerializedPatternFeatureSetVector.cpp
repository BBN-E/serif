// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/XMLSerializedPatternFeatureSetVector.h"
#include "Generic/state/XMLIdMap.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLElement.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/XMLUtil.h"

#include <xercesc/dom/DOM.hpp>
//#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
//#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <boost/foreach.hpp>

using namespace xercesc;

namespace SerifXML {

XMLSerializedPatternFeatureSetVector::~XMLSerializedPatternFeatureSetVector() {
	if (_xercesDOMDocument)
		_xercesDOMDocument->release();
	_xercesDOMDocument = 0;
}

xercesc::DOMDocument* XMLSerializedPatternFeatureSetVector::adoptXercesXMLDoc() {
	DOMDocument* return_value = _xercesDOMDocument;
	_xercesDOMDocument = 0;
	return return_value;
}

XMLSerializedPatternFeatureSetVector::XMLSerializedPatternFeatureSetVector
(const std::vector<PatternFeatureSet_ptr>& snippetFeatureSets, const XMLIdMap* idMap)
: _idMap(idMap)
{
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);
	_xercesDOMDocument = impl->createDocument(0, X_SerifXML, 0);

	XMLElement rootElem(_xercesDOMDocument->getDocumentElement());
	BOOST_FOREACH(PatternFeatureSet_ptr pfs, snippetFeatureSets) {
		XMLElement pfsElement = rootElem.addChild(X_PatternFeatureSet);
		pfs->saveXML(pfsElement, idMap);
	}
}

XMLSerializedPatternFeatureSetVector::XMLSerializedPatternFeatureSetVector(const wchar_t* filename, const XMLIdMap *idMap)
: _xercesDOMDocument(0), _idMap(idMap)
{
	load(OutputUtil::convertToUTF8BitString(filename).c_str());
}

XMLSerializedPatternFeatureSetVector::XMLSerializedPatternFeatureSetVector(const char* filename, const XMLIdMap *idMap)
: _xercesDOMDocument(0), _idMap(idMap)
{
	load(filename);
}

XMLSerializedPatternFeatureSetVector::XMLSerializedPatternFeatureSetVector(xercesc::DOMDocument *domDocument, const XMLIdMap *idMap)
: _xercesDOMDocument(domDocument), _idMap(idMap)
{ }

void XMLSerializedPatternFeatureSetVector::load(const char* filename) {
	if (_xercesDOMDocument != 0)
		throw InternalInconsistencyException("XMLSerializedPatternFeatureSetVector::load",
			"load should not be called twice with the same XMLSerializedPatternFeatureSetVector object");
	_xercesDOMDocument = XMLUtil::loadXercesDOMFromFilename(filename);
}

std::vector<PatternFeatureSet_ptr> XMLSerializedPatternFeatureSetVector::generatePatternFeatureSetVector() {
	std::vector<PatternFeatureSet_ptr> results;
	
	XMLElement rootElem(_xercesDOMDocument->getDocumentElement());
	if (!rootElem.hasTag(X_SerifXML))
		throw UnexpectedInputException("XMLSerializedPatternFeatureSetVector::generatePatternFeatureSetVector()",
					"Expected SerifXML toplevel element");
	
	std::vector<XMLElement> snippetFeatureSetElements = rootElem.getChildElements();
	BOOST_FOREACH(XMLElement sfsElement, snippetFeatureSetElements) {
		if (!sfsElement.hasTag(X_PatternFeatureSet))
			throw UnexpectedInputException("XMLSerializedPatternFeatureSetVector::generatePatternFeatureSetVector()",
					"Expected PatternFeatureSets under SerifXML");
	
		PatternFeatureSet_ptr pfs = boost::make_shared<PatternFeatureSet>(sfsElement, _idMap);
		results.push_back(pfs);
	}

	return results;
}


} // namespace SerifXML


