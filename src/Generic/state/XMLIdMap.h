// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
//
// XMLIDMap: Document-level identifier map for XMLSerialization.

#ifndef XML_ID_MAP_H
#define XML_ID_MAP_H

#include <string>
#include <vector>
#include <map>
#include "Generic/state/XMLStrings.h"
class Theory;

namespace SerifXML {

/** The document-level identifier map for XMLSerialization, which keeps 
  * track of identifiers for theory objects.  Each theory object that can 
  * be pointed to is assigned an XMLCh* identifier, which is also linked 
  * to the theory object's DOMElement.
  *
  * The IdMap can be told to generate either simple or verbose identifiers.  
  * Simple identifiers are just numbers.  Verbose identifiers have the form: 
  * "<parentid>.<prefix>-<num>", where <parentid> is the id of the parent 
  * object; <prefix> is a prefix indicating the type of the object (eg "sent" 
  * for sentence); and <num> is a number.
  */
class XMLIdMap {
private:
	// String identifiers: these are used in the XML.  Note that if we're 
	// reading in XML from the user, these might be anything.
	std::map<SerifXML::xstring, const Theory*> _id2theory;
	std::map<const Theory*, SerifXML::xstring> _theory2id;

	/** Map used to generate new XML identifiers.  It maps from 
	  * each prefix to the next unused id number. */
	std::map<SerifXML::xstring, size_t> _nextXMLId;

	/** If true, then number identifier starting at 1.  Otherwise,
	  * number identifiers starting at zero. */
	const bool _start_identifiers_at_one;

public:
	XMLIdMap();
	~XMLIdMap();

	/** Return true if the given item already has an identifier. */
	bool hasId(const Theory* theory) const;
	bool hasId(const SerifXML::xstring &id) const;

	/** Return the identifier for the given item.  Throws an exception
	  * if the specified item has not been assigned an identifier yet. */
	SerifXML::xstring getId(const Theory* theory) const;

	/** Return the item with the specified identifier.  Throws an exception
	  * if the specified identifier is not found.   */
	const Theory* getTheory(const SerifXML::xstring &id) const;

	/** Assign the specified identifier to the given item.  If the item already
	  * has an identifier, throw an exception. */
	void registerId(const XMLCh* idString, const Theory *theory);

	/** Assign the specified identifier to the given item.  If the item already
	  * has an identifier, then this overrides it. */
	void overrideId(const XMLCh* idString, const Theory *theory);

	/** Generate and return an identifier for the given item.  If the parent is
	  * specified, and verbose ids are enabled, then the new identifier will
	  * include the parent's identifier as a prefix. */
	SerifXML::xstring generateId(const Theory* theory, const XMLCh* parentId, bool verbose);

	/** Returns the size.  Mostly useful for debugging. **/
	int size() const;

private:
	/** Choose an identifier for the given item. */
	SerifXML::xstring chooseId(const Theory *theory, const XMLCh* parentId, bool verbose);
};

} // namespace SerifXML

#endif
