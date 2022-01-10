// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// XMLSerializedPatternFeatureSetVector: serialization & deserialization of SERIF's PatternFeatureSets

#ifndef XML_SERIALIZED_PATTERN_FEATURE_SET_VECTOR_H
#define XML_SERIALIZED_PATTERN_FEATURE_SET_VECTOR_H

#include <string>
#include <fstream>
#include <vector>
#include <xercesc/util/XercesDefs.hpp>
#include "Generic/state/XMLIdMap.h"
#include "Generic/patterns/features/PatternFeatureSet.h"

// Forward declarations.
XERCES_CPP_NAMESPACE_BEGIN
	class DOMDocument;
	class DOMElement;
XERCES_CPP_NAMESPACE_END

namespace SerifXML {

/** An XML-serialized version of a vector of PatternFeatureSets (which holds
  * the results of pattern matching). XMLSerializedPatternFeatureSetVector 
  * can be used for serializing and deserializing. 
  */

class XMLSerializedPatternFeatureSetVector {
public:
	
	~XMLSerializedPatternFeatureSetVector();
	XMLSerializedPatternFeatureSetVector(const std::vector<PatternFeatureSet_ptr>& snippetFeatureSets, const XMLIdMap *idMap);

	xercesc::DOMDocument* adoptXercesXMLDoc();

	/** Load a serialized pattern set vector from disk.  If the file 
	  * does not contain well-formed XML, then throw an 
	  * UnexpectedInputException.  The contents of the XML document 
	  * are *not* read, nor converted to an object, util 
	  * generatePatternSetVector() is called. */
	XMLSerializedPatternFeatureSetVector(const char* filename, const XMLIdMap *idMap);
	XMLSerializedPatternFeatureSetVector(const wchar_t* filename, const XMLIdMap *idMap);

	/** Create a serialized pattern set vector based on an existing 
	  * xercesc::DOMDocument.  The XMLSerializedPatternSetVector takes 
	  * ownership of its argument.  The vector of PatternSets is not 
	  * actually deserialized when this constructor is called -- that 
	  * happens during the call to generatePatternSetVector(). The 
	  * idMap is the IdMap from the corresponding 
	  * XMLSerializedDocTheory */
	XMLSerializedPatternFeatureSetVector(xercesc::DOMDocument *domDocument, const XMLIdMap *idMap);

	/** Generate a new vector of PatternFeatureSets from the XML
	  * Document. */
	std::vector<PatternFeatureSet_ptr> generatePatternFeatureSetVector();

	void load(const char* filename);

private:
	xercesc::DOMDocument *_xercesDOMDocument;
	const XMLIdMap *_idMap;
};

} // namespace SerifXML


#endif
