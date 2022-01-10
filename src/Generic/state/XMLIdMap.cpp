// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
//
// XMLIDMap: Document-level identifier map for XMLSerialization.

#include "Generic/common/leak_detection.h"
#include "Generic/state/XMLIdMap.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/Theory.h"

namespace SerifXML {

XMLIdMap::XMLIdMap(): _start_identifiers_at_one(true) {
	// Register the null pointer:
	_theory2id[0] = X_NULL;
	_id2theory[X_NULL] = 0;
}

XMLIdMap::~XMLIdMap() {}

bool XMLIdMap::hasId(const Theory* theory) const {
	return ((theory==0) || (_theory2id.find(theory) != _theory2id.end()));
}

bool XMLIdMap::hasId(const xstring &id) const {
	return (_id2theory.find(id) != _id2theory.end());
}

xstring XMLIdMap::getId(const Theory* theory) const {
	if (!hasId(theory))
		throw InternalInconsistencyException(
			"SerifXMLDocument::IdMap::getId", "no id assigned!");
	return _theory2id.find(theory)->second;
}

const Theory* XMLIdMap::getTheory(const xstring &id) const {
	if (_id2theory.find(id) == _id2theory.end()) {
		std::wstringstream message;
		message << L"id (" << transcodeToStdWString(id.c_str()) << L") not found!";
		throw UnexpectedInputException(
			"SerifXMLDocument::IdMap::getItem", message);
	}
	return _id2theory.find(id)->second;
}

void XMLIdMap::registerId(const XMLCh* idString, const Theory *theory) {
	xstring &id_dst = _theory2id[theory];
	const Theory* &theory_dst = _id2theory[idString];
	if (!id_dst.empty()) {
		std::wstringstream wss;
		wss << "theory '" << theory->XMLIdentifierPrefix() << "' (" << (size_t) theory << ") already assigned! (id = " << idString << ")";
		throw UnexpectedInputException("SerifXMLDocument::IdMap::registerXMLId", wss);
	}
	if (theory_dst!=0) {
		std::wstringstream wss;
		wss << "id " << idString << " already assigned! (theory = '" << theory->XMLIdentifierPrefix() << "')";
		throw UnexpectedInputException("SerifXMLDocument::IdMap::registerXMLId", wss);
	}
	id_dst = idString;
	theory_dst = theory;
	// [XX] If the identifier came from an XML input string, then we really
	// need to check here whether the _nextXMLId variable needs to be 
	// updated!
}

void XMLIdMap::overrideId(const XMLCh* idString, const Theory *theory) {
	// Erase the mapping from the idString to its old Theory
	const Theory* oldTheory = _id2theory[idString];
	if (oldTheory != 0) _theory2id.erase(oldTheory);

	// Add the new mapping.
	_theory2id[theory] = idString;
	_id2theory[idString] = theory;
}

xstring XMLIdMap::generateId(const Theory* theory, const XMLCh* parentId, bool verbose_id) {
	xstring id = chooseId(theory, parentId, verbose_id);
	registerId(id.c_str(), theory); 
	return id;
}

int XMLIdMap::size() const {
	return static_cast<int>(_id2theory.size());
}

xstring XMLIdMap::chooseId(const Theory *theory, const XMLCh* parentId, bool verbose_id) {
	xstring id;
	if (parentId != 0) {
		id += parentId; 
		id += X_DOT;
	}
	if (verbose_id) {
		const wchar_t* type_prefix = theory->XMLIdentifierPrefix();
		if (type_prefix != 0) {
			id += transcodeToXString(type_prefix);
			id += X_DASH;
		}
	}
	if (id.empty()) {
		id += X_IDENTIFIER_PREFIX; // XML identifiers can't be bare numbers
	}
	size_t idNum = _nextXMLId[id]++;
	if (_start_identifiers_at_one && idNum==0) { 
		idNum = _nextXMLId[id]++; 
	}
	id += transcodeToXString(boost::lexical_cast<std::wstring>(idNum).c_str());
	return id;
}

} // namespace SerifXML
